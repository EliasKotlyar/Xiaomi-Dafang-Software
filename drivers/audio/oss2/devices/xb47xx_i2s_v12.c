/**
 * xb_snd_i2s.c
 *
 * jbbi <jbbi@ingenic.cn>
 *
 * 24 APR 2012
 *
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <linux/switch.h>
#include <linux/dma-mapping.h>
#include <linux/soundcard.h>
#include <linux/wait.h>
#include <mach/jzdma.h>
#include <mach/jzsnd.h>
#include <soc/irq.h>
#include <soc/base.h>
#include "xb47xx_i2s_v12.h"
#include "codecs/jz_t10_codec.h"
#include "ex_codecs/es8374_codec.h"
#include "ex_codecs/codec_i2c_dev.h"

/* to be modify */
void volatile __iomem *volatile i2s_iomem;

/* head phone detect */
static LIST_HEAD(codecs_head);
struct snd_switch_data switch_data;

static struct i2s_device * g_i2s_dev;
extern unsigned int snd_debug_level;
extern int codec_type;

/* for interface/dsp and interface/mixer */
static int i2s_set_private_data(struct snd_dev_data *ddata, struct i2s_device * i2s_dev)
{
	ddata->priv_data = (void *)i2s_dev;
	return 0;
}
static struct i2s_device * i2s_get_private_data(struct snd_dev_data *ddata)
{
	return (struct i2s_device *)ddata->priv_data;
}

static struct snd_dev_data *i2s_get_ddata(struct platform_device * pdev)
{
	return pdev->dev.platform_data;
}
/* for hp detect */
static int i2s_set_switch_data(struct snd_switch_data *switch_data, struct i2s_device *i2s_dev)
{

	switch_data->priv_data = (void *)i2s_dev;
	return 0;
}
static struct i2s_device *i2s_get_switch_data(struct snd_switch_data *switch_data)
{
	return (struct i2s_device *)switch_data->priv_data;
}

/*##################################################################*\
 | dump
\*##################################################################*/

static void dump_i2s_reg(struct i2s_device *i2s_dev)
{
	int i;
	unsigned long reg_addr[] = {
		AICFR,AICCR,I2SCR,AICSR,I2SSR,I2SDIV
	};

	for (i = 0;i < 6; i++) {
		printk("##### aic reg0x%x,=0x%x.\n",
			(unsigned int)reg_addr[i],i2s_read_reg(i2s_dev, reg_addr[i]));
	}
}

/*##################################################################*\
 |* suspand func
\*##################################################################*/
static int i2s_suspend(struct platform_device *, pm_message_t state);
static int i2s_resume(struct platform_device *);
static void i2s_shutdown(struct platform_device *);

/*##################################################################*\
  |* codec control
\*##################################################################*/
/**
 * register the codec
 **/
static int codec_ctrl(struct codec_info *cur_codec, unsigned int cmd, unsigned long arg)
{
	return cur_codec->codec_ctl_2(cur_codec, cmd, arg);
}
static struct codec_info * gcodec_dev = NULL;
int i2s_register_codec_2(struct codec_info * codec_dev)
{
	if(!codec_dev) {
		snd_error_print("codec_dev register failed\n");
		return -EINVAL;
	}
	list_add_tail(&codec_dev->list, &codecs_head);
	gcodec_dev = codec_dev;

	return 0;
}

int i2s_release_codec_2(struct codec_info * codec_dev)
{
	if(!codec_dev) {
		snd_error_print("codec_dev register failed\n");
		return -EINVAL;
	}
	list_del(&codec_dev->list);
	return 0;
}


static void i2s_match_codec(struct i2s_device *i2s_dev, char *name)
{
	struct codec_info *codec_info;
	struct list_head  *list,*tmp;

	list_for_each_safe(list,tmp,&codecs_head) {
		codec_info = container_of(list,struct codec_info,list);
		if (!strcmp(codec_info->name,name)) {
			codec_info->codec_parent = i2s_dev;
			codec_info->dsp_endpoints = i2s_dev->i2s_endpoints;
			i2s_dev->cur_codec = codec_info;
			return;
		}
	}
}

/*##################################################################*\
|* filter opt
\*##################################################################*/
static void i2s_set_filter(struct i2s_device *i2s_dev, int mode , uint32_t channels)
{
	struct dsp_pipe *dp = NULL;
	struct codec_info * cur_codec = i2s_dev->cur_codec;

	if (mode & CODEC_RMODE)
		dp = cur_codec->dsp_endpoints->in_endpoint;
	else
		return;

	switch(cur_codec->record_format) {
		case AFMT_U8:
			if (channels == 1) {
				dp->filter = convert_8bits_stereo2mono_signed2unsigned;
				snd_debug_print("dp->filter convert_8bits_stereo2mono_signed2unsigned .\n");
			}
			else {
				//dp->filter = convert_8bits_signed2unsigned;
				dp->filter = NULL; //hardware convert
				snd_debug_print("dp->filter convert_8bits_signed2unsigned.\n");
			}
			break;
		case AFMT_S8:
			if (channels == 1) {
				dp->filter = convert_8bits_stereo2mono;
				snd_debug_print("dp->filter convert_8bits_stereo2mono\n");
			}
			else {
				dp->filter = NULL;
				snd_debug_print("dp->filter null\n");
			}
			break;
		case AFMT_U16_LE:
		case AFMT_S16_LE:
			if (channels == 1) {
#if (defined(CONFIG_SOC_T10) || defined(CONFIG_SOC_T20) || defined(CONFIG_SOC_T30))
				dp->filter = convert_16bits_stereo2mono_inno;
#else
				dp->filter = convert_16bits_stereo2mono;
#endif
				snd_debug_print("dp->filter convert_16bits_stereo2mono\n");
			}
			else {
				dp->filter = NULL;
				snd_debug_print("dp->filter null\n");
			}
			break;
		default :
			dp->filter = NULL;
			snd_error_print("AUDIO DEVICE :filter set error.\n");
	}
}

/*##################################################################*\
|* dev_ioctl
\*##################################################################*/
static int i2s_set_fmt(struct i2s_device *i2s_dev, unsigned long *format,int mode)
{
	int ret = 0;
	int data_width = 0;
	struct dsp_pipe *dp = NULL;
	struct codec_info * cur_codec = i2s_dev->cur_codec;
    /*
	 * The value of format reference to soundcard.
	 * AFMT_MU_LAW      0x00000001
	 * AFMT_A_LAW       0x00000002
	 * AFMT_IMA_ADPCM   0x00000004
	 * AFMT_U8			0x00000008
	 * AFMT_S16_LE      0x00000010
	 * AFMT_S16_BE      0x00000020
	 * AFMT_S8			0x00000040
	 */
	snd_debug_print("format = %ld\n",*format);
	switch (*format) {
	case AFMT_U8:
		data_width = 8;
		if (mode & CODEC_WMODE) {
			__i2s_set_oss_sample_size(i2s_dev, 0);
			__i2s_disable_byteswap(i2s_dev);
		}
		if (mode & CODEC_RMODE) {
			__i2s_set_iss_sample_size(i2s_dev, 0);
		}
		__i2s_enable_signadj(i2s_dev);
		break;
	case AFMT_S8:
		data_width = 8;
		if (mode & CODEC_WMODE) {
			__i2s_set_oss_sample_size(i2s_dev, 0);
			__i2s_disable_byteswap(i2s_dev);
		}
		if (mode & CODEC_RMODE)
			__i2s_set_iss_sample_size(i2s_dev, 0);
		__i2s_disable_signadj(i2s_dev);
		break;
	case AFMT_S16_LE:
		data_width = 16;
		if (mode & CODEC_WMODE) {
			__i2s_set_oss_sample_size(i2s_dev, 1);
			__i2s_disable_byteswap(i2s_dev);
		}
		if (mode & CODEC_RMODE)
			__i2s_set_iss_sample_size(i2s_dev, 1);
		__i2s_disable_signadj(i2s_dev);
		break;
        case AFMT_U16_LE:
                data_width = 16;
                if (mode & CODEC_WMODE) {
                        __i2s_set_oss_sample_size(i2s_dev, 1);
                        __i2s_disable_byteswap(i2s_dev);
                }
                if (mode & CODEC_RMODE)
                        __i2s_set_iss_sample_size(i2s_dev, 1);
                __i2s_enable_signadj(i2s_dev);
                break;
	case AFMT_S16_BE:
		data_width = 16;
		if (mode & CODEC_WMODE) {
			__i2s_set_oss_sample_size(i2s_dev, 1);
			__i2s_enable_byteswap(i2s_dev);
		}
		if (mode == CODEC_RMODE) {
			__i2s_set_iss_sample_size(i2s_dev, 1);
			*format = AFMT_S16_LE;
		}
		__i2s_disable_signadj(i2s_dev);
		break;
	default :
		snd_error_print("I2S: there is unknown format 0x%x.\n",(unsigned int)*format);
		return -EINVAL;
	}
	if (!cur_codec)
		return -ENODEV;

	if (mode & CODEC_WMODE) {
		dp = cur_codec->dsp_endpoints->out_endpoint;
		ret = codec_ctrl(cur_codec, CODEC_SET_REPLAY_DATA_WIDTH, data_width);
		if(ret < 0) {
			snd_error_print("JZ I2S: CODEC ioctl error, command: CODEC_SET_REPLAY_FORMAT");
			return ret;
		}
		if(data_width == 8)
			dp->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		else if(data_width == 16)
			dp->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		else
			dp->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

		if (cur_codec->replay_format != *format) {
			cur_codec->replay_format = *format;
			ret |= NEED_RECONF_TRIGGER;
			ret |= NEED_RECONF_DMA;
		}
	}

	if (mode & CODEC_RMODE) {
		dp = cur_codec->dsp_endpoints->in_endpoint;
		ret = codec_ctrl(cur_codec, CODEC_SET_RECORD_DATA_WIDTH, data_width);
		if(ret != 0) {
			snd_error_print("JZ I2S: CODEC ioctl error, command: CODEC_SET_RECORD_FORMAT");
			return ret;
		}

                if(data_width == 8)
                        dp->dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
                else if(data_width == 16)
                        dp->dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
                else
                        dp->dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

		if (cur_codec->record_format != *format) {
			cur_codec->record_format = *format;
			ret |= NEED_RECONF_TRIGGER | NEED_RECONF_FILTER;
			ret |= NEED_RECONF_DMA;
		}
	}

	return ret;
}

static int i2s_set_channel(struct i2s_device * i2s_dev, int* channel,int mode)
{
	int ret = 0;
	struct codec_info * cur_codec = i2s_dev->cur_codec;
	if (!cur_codec)
		return -ENODEV;
	snd_debug_print("channel = %d\n",*channel);
	if (mode & CODEC_WMODE) {
			ret = codec_ctrl(cur_codec, CODEC_SET_REPLAY_CHANNEL,(unsigned long)channel);
		if (ret < 0) {
			snd_debug_print("channel = %d\n", *channel);
			cur_codec->replay_codec_channel = *channel;
			return ret;
		}
		if (*channel ==2 || *channel == 4||
			*channel ==6 || *channel == 8) {
			__i2s_out_channel_select(i2s_dev, *channel - 1);
			__i2s_disable_mono2stereo(i2s_dev);
		} else if (*channel == 1) {
			__i2s_out_channel_select(i2s_dev, *channel - 1);	//FIXME
			__i2s_enable_mono2stereo(i2s_dev);
		} else
			return -EINVAL;
		if (cur_codec->replay_codec_channel != *channel) {
			cur_codec->replay_codec_channel = *channel;
			ret |= NEED_RECONF_FILTER;
		}
	}
	if (mode & CODEC_RMODE) {
			ret = codec_ctrl(cur_codec, CODEC_SET_RECORD_CHANNEL,(unsigned long)channel);
		if (ret < 0)
			return ret;
		ret = 0;
		if (cur_codec->record_codec_channel != *channel) {
			cur_codec->record_codec_channel = *channel;
			ret |= NEED_RECONF_FILTER;
		}
	}
	return ret;
}

/***************************************************************\
 *  Use codec slave mode clock rate list
 *  We do not hope change EPLL,so we use 270.67M (fix) epllclk
 *  for minimum error
 *  270.67M ---	M:203 N:9 OD:1
 *	 rate	 i2sdr	 cguclk		 i2sdv.div	samplerate/error
 *	|192000	|1		|135.335M	|10			|+0.12%
 *	|96000	|3		|67.6675M	|10			|+0.12%
 *	|48000	|7		|33.83375M	|10			|-0.11%
 *	|44100	|7		|33.83375M	|11			|-0.10%
 *	|32000	|11		|22.555833M	|10			|+0.12%
 *	|24000	|15		|16.916875M	|10			|+0.12%
 *	|22050	|15		|16.916875M	|11			|-0.12%
 *	|16000	|23		|11.277916M	|10			|+0.12%
 *	|12000  |31		|8.458437M	|10			|+0.12%
 *	|11025	|31		|8.458437M	|11			|-0.10%
 *	|8000	|47		|5.523877M	|10			|+0.12%
 *	HDMI:
 *	sysclk 11.2896M (theoretical)
 *	i2sdr  23
 *	cguclk 11.277916M (practical)
 *	error  -0.10%
 *  If using internal codec, EPLL should be 204MHz to divide 12MHz SYSCLK
 *  If using external codec and AIC support BITCLK, EPLL should be 270MHz to divide exact sample rate
\***************************************************************/
static unsigned long calculate_cgu_aic_rate(struct i2s_device * i2s_dev, unsigned long *rate)
{
	int i;
	unsigned long mrate[12] = {
		8000, 11025, 12000, 16000,22050,24000,
		32000,44100, 48000, 88200,96000,192000,
	};

	unsigned long mcguclk[12] = {
		8192000, 11333338, 12288000, 8192000, 11333338, 12288000,
		8192000, 11333338, 12288000, 11333338,12288000, 25500000,
	};
	for (i=0; i<9; i++) {
		if (*rate <= mrate[i]) {
			*rate = mrate[i];
			break;
		}
	}

	if (i >= 9) {
		*rate = 44100; /*unsupport rate use default*/
		return mcguclk[6];
	}

	return mcguclk[i];
}

static int i2s_set_rate(struct i2s_device * i2s_dev, unsigned long *rate,int mode)
{
	int ret = 0;
	unsigned long cgu_aic_clk = 0;
	struct codec_info *cur_codec = i2s_dev->cur_codec;

	if (!cur_codec)
		return -ENODEV;

	if (!strcmp(cur_codec->name,"hdmi")){
		printk("HDMI can't be support!\n");
		return -EINVAL;
	}

	snd_debug_print("rate = %ld\n",*rate);
	clk_set_rate(i2s_dev->i2s_clk, (*rate) * 256);
	clk_enable(i2s_dev->i2s_clk);
	if (mode & CODEC_WMODE) {
		if (cur_codec->codec_mode == CODEC_SLAVE) {
			/*************************************************************\
			|* WARING:when use codec slave mode ,                        *|
			|* EPLL must be output 270.67M clk to support all sample rate*|
			|* SYSCLK not standard over sample rate clock ,so it would   *|
			|* not be output to external codec                           *|
			\*************************************************************/
			cgu_aic_clk = calculate_cgu_aic_rate(i2s_dev, rate);
			__i2s_stop_bitclk(i2s_dev);
			if (cur_codec->codec_clk != cgu_aic_clk) {
				cur_codec->codec_clk = cgu_aic_clk;
				if (i2s_dev->i2s_clk == NULL)
					return -1;
				clk_set_rate(i2s_dev->i2s_clk, cur_codec->codec_clk);
				if (clk_get_rate(i2s_dev->i2s_clk) > cur_codec->codec_clk) {
					snd_error_print("external codec set rate fail.\n");
				}
			}
			*rate = __i2s_set_sample_rate(i2s_dev, cur_codec->codec_clk,*rate);
			__i2s_start_bitclk(i2s_dev);
		}
		ret = codec_ctrl(cur_codec, CODEC_SET_REPLAY_RATE,(unsigned long)rate);
		cur_codec->replay_rate = *rate;
	}
	if (mode & CODEC_RMODE) {
		if (cur_codec->codec_mode == CODEC_SLAVE) {
			cgu_aic_clk = calculate_cgu_aic_rate(i2s_dev, rate);
			if (strcmp(cur_codec->name,"hdmi"))
				return 0;
			__i2s_stop_ibitclk(i2s_dev);
			if (cur_codec->codec_clk != cgu_aic_clk) {
				cur_codec->codec_clk = cgu_aic_clk;
				if (i2s_dev->i2s_clk == NULL)
					return -1;
				clk_set_rate(i2s_dev->i2s_clk, cur_codec->codec_clk);
				if (clk_get_rate(i2s_dev->i2s_clk) > cur_codec->codec_clk) {
					snd_error_print("external codec set rate fail.\n");
				}
			}
			*rate = __i2s_set_isample_rate(i2s_dev, cur_codec->codec_clk,*rate);
			__i2s_start_ibitclk(i2s_dev);
		}
		ret = codec_ctrl(cur_codec, CODEC_SET_RECORD_RATE,(unsigned long)rate);
		cur_codec->record_rate = *rate;
	}
	return ret;
}
#define I2S_TX_FIFO_DEPTH		64
#define I2S_RX_FIFO_DEPTH		32

static int get_burst_length(struct i2s_device *i2s_dev, unsigned long val)
{
	/* burst bytes for 1,2,4,8,16,32,64 bytes */
	int ord;

	ord = ffs(val) - 1;
	if (ord < 0)
		ord = 0;
	else if (ord > 6)
		ord = 6;

	/* if tsz == 8, set it to 4 */
	return (ord == 3 ? 4 : 1 << ord)*8;
}

static void i2s_set_trigger(struct i2s_device * i2s_dev, int mode)
{
	int data_width = 0;
	struct dsp_pipe *dp = NULL;
	int burst_length = 0;
	struct codec_info *cur_codec = i2s_dev->cur_codec;
	if (!cur_codec)
		return;

	if (mode & CODEC_WMODE) {
		switch(cur_codec->replay_format) {
		case AFMT_S8:
		case AFMT_U8:
			data_width = 8;
			break;
		default:
		case AFMT_S16_BE:
		case AFMT_U16_BE:
		case AFMT_S16_LE:
		case AFMT_U16_LE:
			data_width = 16;
			break;
		}
		dp = cur_codec->dsp_endpoints->out_endpoint;
		burst_length = get_burst_length(i2s_dev, (int)dp->paddr|(int)dp->fragment_size|
				dp->dma_config.src_maxburst|dp->dma_config.dst_maxburst);
		if (I2S_TX_FIFO_DEPTH - burst_length/data_width >= 60){
			__i2s_set_transmit_trigger(i2s_dev, 30);
		}else{
			/*__i2s_set_transmit_trigger(i2s_dev, (I2S_TX_FIFO_DEPTH - burst_length/data_width) >> 1);*/
			__i2s_set_transmit_trigger(i2s_dev, 16);
			snd_debug_print("$$ trigger = %d\n", (I2S_TX_FIFO_DEPTH - burst_length/data_width) >> 1);
		}
	}
	if (mode &CODEC_RMODE) {
		switch(cur_codec->record_format) {
		case AFMT_S8:
		case AFMT_U8:
			data_width = 8;
			break;
		default :
		case AFMT_U16_LE:
		case AFMT_S16_LE:
			data_width = 16;
			break;
		}
		dp = cur_codec->dsp_endpoints->in_endpoint;
		burst_length = get_burst_length(i2s_dev, (int)dp->paddr|(int)dp->fragment_size|
				dp->dma_config.src_maxburst|dp->dma_config.dst_maxburst);
		if (I2S_RX_FIFO_DEPTH <= burst_length/data_width)
			__i2s_set_receive_trigger(i2s_dev, ((burst_length/data_width +1) >> 1) - 1);
		else
			__i2s_set_receive_trigger(i2s_dev, 8);
	}

	return;
}

static int i2s_enable(struct i2s_device * i2s_dev, int mode)
{
	/*unsigned long rate = 0;*/
	/*unsigned long format = AFMT_U8;*/
	/*int channel = 0;*/
	/*struct dsp_pipe *dp_other = NULL;*/
	struct codec_info *cur_codec = i2s_dev->cur_codec;
	if (!cur_codec)
			return -ENODEV;
#if 0
	if (mode & CODEC_WMODE) {
		dp_other = cur_codec->dsp_endpoints->in_endpoint;
		rate = dp_other->living_attr.rate;
		format = dp_other->living_attr.format;
		channel = dp_other->living_attr.channel;
		i2s_set_fmt(i2s_dev, &format,mode);
		i2s_set_channel(i2s_dev, &channel,mode);
		i2s_set_rate(i2s_dev, &rate,mode);
	}
	if (mode & CODEC_RMODE) {
		dp_other = cur_codec->dsp_endpoints->out_endpoint;
		rate = dp_other->living_attr.rate;
		format = dp_other->living_attr.format;
		channel = dp_other->living_attr.channel;
		i2s_set_fmt(i2s_dev, &format,mode);
		i2s_set_channel(i2s_dev, &channel,mode);
		i2s_set_rate(i2s_dev, &rate,mode);
		i2s_set_filter(i2s_dev, mode,channel);
	}
#endif
	i2s_set_trigger(i2s_dev, mode);

	__i2s_select_i2s(i2s_dev);
	__i2s_enable(i2s_dev);

	codec_ctrl(cur_codec, CODEC_ANTI_POP,mode);

	return 0;
}

void i2s_replay_zero_for_flush_codec(struct i2s_device *i2s_dev)
{
	__i2s_write_tfifo(i2s_dev, 0);	//avoid pop
	__i2s_write_tfifo(i2s_dev, 0);
	__i2s_write_tfifo(i2s_dev, 0);
	__i2s_write_tfifo(i2s_dev, 0);
	__i2s_enable_replay(i2s_dev);
	msleep(2);
	__i2s_disable_replay(i2s_dev);
}


static int i2s_disable_channel(struct i2s_device *i2s_dev, int mode)
{
	struct codec_info * cur_codec = i2s_dev->cur_codec;
	if (cur_codec) {
		codec_ctrl(cur_codec, CODEC_TURN_OFF,mode);
	}

	if (mode & CODEC_WMODE) {
		i2s_replay_zero_for_flush_codec(i2s_dev);
	}
	if (mode & CODEC_RMODE) {
		__i2s_disable_record(i2s_dev);
	}
	return 0;
}


static int i2s_dma_enable(struct i2s_device * i2s_dev, int mode)		//CHECK
{
	int val;
	struct codec_info * cur_codec = i2s_dev->cur_codec;
	if (!cur_codec)
			return -ENODEV;
	if (mode & CODEC_WMODE) {
		__i2s_flush_tfifo(i2s_dev);
		codec_ctrl(cur_codec, CODEC_DAC_MUTE,0);
		__i2s_enable_transmit_dma(i2s_dev);
		__i2s_enable_replay(i2s_dev);
	}
	if (mode & CODEC_RMODE) {
		__i2s_flush_rfifo(i2s_dev);
		codec_ctrl(cur_codec, CODEC_ADC_MUTE,0);
		/* read the first sample and ignore it */
		val = __i2s_read_rfifo(i2s_dev);
		__i2s_enable_receive_dma(i2s_dev);
		__i2s_enable_record(i2s_dev);
	}

	return 0;
}

static int i2s_dma_disable(struct i2s_device *i2s_dev, int mode)		//CHECK seq dma and func
{
	struct codec_info *cur_codec = i2s_dev->cur_codec;
	if (!cur_codec)
			return -ENODEV;
	if (mode & CODEC_WMODE) {
		__i2s_disable_transmit_dma(i2s_dev);
		__i2s_disable_replay(i2s_dev);
	}
	if (mode & CODEC_RMODE) {
		__i2s_disable_record(i2s_dev);
		__i2s_disable_receive_dma(i2s_dev);
	}
	return 0;
}

static int i2s_get_fmt_cap(struct i2s_device *i2s_dev, unsigned long *fmt_cap,int mode)
{
	unsigned long i2s_fmt_cap = 0;
	struct codec_info * cur_codec = i2s_dev->cur_codec;
	if (!cur_codec)
			return -ENODEV;
	if (mode & CODEC_WMODE) {
		i2s_fmt_cap |= AFMT_S16_LE|AFMT_S16_BE|AFMT_U16_LE|AFMT_U16_BE|AFMT_S8|AFMT_U8;
		codec_ctrl(cur_codec, CODEC_GET_REPLAY_FMT_CAP, *fmt_cap);

	}
	if (mode & CODEC_RMODE) {
		i2s_fmt_cap |= AFMT_U16_LE|AFMT_S16_LE|AFMT_S8|AFMT_U8;
		codec_ctrl(cur_codec, CODEC_GET_RECORD_FMT_CAP, *fmt_cap);
	}

	if (*fmt_cap == 0)
		*fmt_cap = i2s_fmt_cap;
	else
		*fmt_cap &= i2s_fmt_cap;

	return 0;
}


static int i2s_get_fmt(struct i2s_device *i2s_dev, unsigned long *fmt, int mode)
{
	struct codec_info * cur_codec = i2s_dev->cur_codec;
	if (!cur_codec)
			return -ENODEV;

	if (mode & CODEC_WMODE)
		*fmt = cur_codec->replay_format;
	if (mode & CODEC_RMODE)
		*fmt = cur_codec->record_format;

	return 0;
}

static void i2s_dma_need_reconfig(struct i2s_device *i2s_dev, int mode)
{
	struct dsp_pipe	*dp = NULL;

	struct codec_info * cur_codec = i2s_dev->cur_codec;
	if (!cur_codec)
			return;
	if (mode & CODEC_WMODE) {
		dp = cur_codec->dsp_endpoints->out_endpoint;
	}
	if (mode & CODEC_RMODE) {
		dp = cur_codec->dsp_endpoints->in_endpoint;
	}
	return;
}


static int i2s_set_device(struct i2s_device * i2s_dev, unsigned long device)
{
	unsigned long tmp_rate = 44100;
  	int ret = 0;
	struct codec_info * cur_codec = i2s_dev->cur_codec;

	if (!cur_codec)
		return -1;

	if (*(enum snd_device_t *)device == SND_DEVICE_LOOP_TEST) {
		codec_ctrl(cur_codec, CODEC_ADC_MUTE,0);
		codec_ctrl(cur_codec, CODEC_DAC_MUTE,0);
		i2s_set_rate(i2s_dev, &tmp_rate, CODEC_RWMODE);
	}

	snd_debug_print("i2s clk rate is %ld\n", clk_get_rate(i2s_dev->i2s_clk));
	/*route operatiom*/
	ret = codec_ctrl(cur_codec, CODEC_SET_DEVICE, device);

	return ret;
}

/********************************************************\
 * dev_ioctl
\********************************************************/

static long do_ioctl_work(struct i2s_device *i2s_dev, unsigned int cmd, unsigned long arg);


static long i2s_ioctl_2(struct snd_dev_data *ddata, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct i2s_device *i2s_dev = i2s_get_private_data(ddata);
	struct codec_info *cur_codec = i2s_dev->cur_codec;
	//printk("[i2s debug]%s:%d, ddata:%p, cmd:%d, arg:%x\n", __func__, __LINE__, ddata, cmd, arg? *(unsigned int *)arg : arg);
	switch(cmd) {
		case SND_DSP_GET_REPLAY_RATE:
			if (cur_codec && cur_codec->replay_rate)
				*(unsigned long *)arg = cur_codec->replay_rate;
			ret = 0;
			break;
		case SND_DSP_GET_RECORD_RATE:
			if (cur_codec && cur_codec->record_rate)
				*(unsigned long *)arg = cur_codec->record_rate;
			ret = 0;
			break;

		case SND_DSP_GET_REPLAY_CHANNELS:
			if (cur_codec && cur_codec->replay_codec_channel)
				*(unsigned long *)arg = cur_codec->replay_codec_channel;
			ret = 0;
			break;

		case SND_DSP_GET_RECORD_CHANNELS:
			if (cur_codec && cur_codec->record_codec_channel)
				*(unsigned long *)arg = cur_codec->record_codec_channel;
			ret = 0;
			break;

		case SND_DSP_GET_REPLAY_FMT_CAP:
			/* return the support replay formats */
			ret = i2s_get_fmt_cap(i2s_dev, (unsigned long *)arg,CODEC_WMODE);
			break;

		case SND_DSP_GET_REPLAY_FMT:
			/* get current replay format */
			i2s_get_fmt(i2s_dev, (unsigned long *)arg,CODEC_WMODE);
			break;
		case SND_DSP_ENABLE_DMA_RX:
			ret = i2s_dma_enable(i2s_dev, CODEC_RMODE);
			break;

		case SND_DSP_DISABLE_DMA_RX:
			ret = i2s_dma_disable(i2s_dev, CODEC_RMODE);
			break;

		case SND_DSP_ENABLE_DMA_TX:
			ret = i2s_dma_enable(i2s_dev, CODEC_WMODE);
			break;

		case SND_DSP_DISABLE_DMA_TX:
			ret = i2s_dma_disable(i2s_dev, CODEC_WMODE);
			break;
		case SND_DSP_FLUSH_SYNC:
			/*flush_work(&i2s_dev->i2s_work);*/
			break;
		default:
			ret = do_ioctl_work(i2s_dev, cmd, arg);
			break;


	}
	return ret;
}

#define JZSOUND_DEFAULT_ALIGN_FRAGMENTSIZE(X) (1000/(X))
static inline int get_record_fragmentsize(struct codec_info *cur_codec)
{
	int data_bytes = 1;
	int fragment = 0;
	if(!cur_codec)
		return -EPERM;
	switch (cur_codec->record_format) {
	case AFMT_U8:
	case AFMT_S8:
		data_bytes = 1;
		break;
	case AFMT_S16_LE:
    case AFMT_U16_LE:
	case AFMT_S16_BE:
		data_bytes = 2;
		break;
	default :
		return -EINVAL;
	}
#if (defined(CONFIG_SOC_T10) || defined(CONFIG_SOC_T20) || defined(CONFIG_SOC_T30))
	fragment = data_bytes * cur_codec->record_codec_channel * cur_codec->record_rate / JZSOUND_DEFAULT_ALIGN_FRAGMENTSIZE(10) * 2; //10ms
#else
	fragment = data_bytes * cur_codec->record_codec_channel * cur_codec->record_rate / JZSOUND_DEFAULT_ALIGN_FRAGMENTSIZE(10); //10ms
#endif
	return fragment;
}

static inline int get_replay_fragmentsize(struct codec_info * cur_codec)
{
	int data_bytes = 1;
	int fragment = 0;
	if(!cur_codec)
		return -EPERM;
	switch (cur_codec->replay_format) {
	case AFMT_U8:
	case AFMT_S8:
		data_bytes = 1;
		break;
	case AFMT_S16_LE:
    case AFMT_U16_LE:
	case AFMT_S16_BE:
		data_bytes = 2;
		break;
	default :
		return -EINVAL;
	}

	fragment = data_bytes * cur_codec->replay_codec_channel * cur_codec->replay_rate / JZSOUND_DEFAULT_ALIGN_FRAGMENTSIZE(10); //10ms
	return fragment;
}

static long do_ioctl_work(struct i2s_device *i2s_dev, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	struct codec_info * cur_codec = i2s_dev->cur_codec;
	switch (cmd) {
	case SND_DSP_ENABLE_REPLAY:
		/* enable i2s record */
		/* set i2s default record format, channels, rate */
		/* set default replay route */
		ret = i2s_enable(i2s_dev, CODEC_WMODE);
		break;

	case SND_DSP_DISABLE_REPLAY:
		/* disable i2s replay */
		ret = i2s_disable_channel(i2s_dev, CODEC_WMODE);
		break;

	case SND_DSP_ENABLE_RECORD:
		/* enable i2s record */
		/* set i2s default record format, channels, rate */
		/* set default record route */
		ret = i2s_enable(i2s_dev, CODEC_RMODE);
		break;

	case SND_DSP_DISABLE_RECORD:
		/* disable i2s record */
		ret = i2s_disable_channel(i2s_dev, CODEC_RMODE);
		break;

	case SND_DSP_SET_REPLAY_RATE:
		ret = i2s_set_rate(i2s_dev, (unsigned long *)arg,CODEC_WMODE);
		break;

	case SND_DSP_SET_RECORD_RATE:
		ret = i2s_set_rate(i2s_dev, (unsigned long *)arg,CODEC_RMODE);
		break;

	case SND_DSP_SET_REPLAY_CHANNELS:
		/* set replay channels */
		ret = i2s_set_channel(i2s_dev, (int *)arg,CODEC_WMODE);
		if (ret < 0)
			break;
		//if (ret & NEED_RECONF_FILTER)
		//	i2s_set_filter(i2s_dev, CODEC_WMODE,cur_codec->replay_codec_channel);
		ret = 0;
		break;

	case SND_DSP_SET_RECORD_CHANNELS:
		/* set record channels */
		ret = i2s_set_channel(i2s_dev, (int*)arg,CODEC_RMODE);
		if (ret < 0)
			break;
		/*if (ret & NEED_RECONF_FILTER)*/
			/*i2s_set_filter(i2s_dev, CODEC_RMODE,cur_codec->record_codec_channel);*/
		ret = 0;
		break;

	case SND_DSP_SET_REPLAY_FMT:
		/* set replay format */
		ret = i2s_set_fmt(i2s_dev, (unsigned long *)arg,CODEC_WMODE);
		if (ret < 0)
			break;
		/* if need reconfig the trigger, reconfig it */
		if (ret & NEED_RECONF_TRIGGER)
			i2s_set_trigger(i2s_dev, CODEC_WMODE);
		/* if need reconfig the dma_slave.max_tsz, reconfig it and
		   set the dp->need_reconfig_dma as true */
		if (ret & NEED_RECONF_DMA)
			i2s_dma_need_reconfig(i2s_dev, CODEC_WMODE);
		ret = 0;
		break;

	case SND_DSP_SET_RECORD_FMT:
		/* set record format */
		ret = i2s_set_fmt(i2s_dev, (unsigned long *)arg,CODEC_RMODE);
		if (ret < 0)
			break;
		/* if need reconfig the trigger, reconfig it */
		if (ret & NEED_RECONF_TRIGGER)
			i2s_set_trigger(i2s_dev, CODEC_RMODE);
		/* if need reconfig the dma_slave.max_tsz, reconfig it and
		   set the dp->need_reconfig_dma as true */
		if (ret & NEED_RECONF_DMA)
			i2s_dma_need_reconfig(i2s_dev, CODEC_RMODE);
		/* if need reconfig the filter, reconfig it */
		if (ret & NEED_RECONF_FILTER)
			i2s_set_filter(i2s_dev, CODEC_RMODE,cur_codec->record_codec_channel);
		ret = 0;
		break;

	case SND_DSP_GET_REPLAY_FRAGMENTSIZE:
	{	int size = 0;
		/* now, the size of fragment is 10ms data */
		if (!cur_codec)
			return -ENODEV;
		size = get_replay_fragmentsize(cur_codec);
		if(size >= 0){
			*(unsigned int*)arg = size;
		}else{
			*(unsigned int*)arg = 0;
			ret = -EPERM;
		}
	}
		break;
	case SND_DSP_GET_RECORD_FRAGMENTSIZE:
	{	int size = 0;
		/* now, the size of fragment is 10ms data */
		if (!cur_codec)
			return -ENODEV;
		size = get_record_fragmentsize(cur_codec);
		if(size >= 0){
			*(unsigned int*)arg = size;
		}else{
			*(unsigned int*)arg = 0;
			ret = -EPERM;
		}
		break;
	}
	case SND_MIXER_DUMP_REG:
		dump_i2s_reg(i2s_dev);
		if (cur_codec)
			ret = codec_ctrl(cur_codec, CODEC_DUMP_REG,0);
		break;
	case SND_MIXER_DUMP_GPIO:
		if (cur_codec)
			ret = codec_ctrl(cur_codec, CODEC_DUMP_GPIO,0);
		break;

	case SND_DSP_SET_STANDBY:
		if (cur_codec)
			ret = codec_ctrl(cur_codec, CODEC_SET_STANDBY,arg);
		break;

	case SND_DSP_SET_DEVICE:
		ret = i2s_set_device(i2s_dev, arg);
		break;
	case SND_DSP_SET_RECORD_VOL:
		if (cur_codec)
			ret = codec_ctrl(cur_codec, CODEC_SET_RECORD_VOLUME, arg);
		break;
	case SND_DSP_SET_REPLAY_VOL:
		if (cur_codec)
			ret = codec_ctrl(cur_codec, CODEC_SET_REPLAY_VOLUME, arg);
		break;
	case SND_DSP_SET_MIC_VOL:
		if (cur_codec)
			ret = codec_ctrl(cur_codec, CODEC_SET_MIC_VOLUME, arg);
		break;
	case SND_DSP_CLR_ROUTE:
		if (cur_codec)
			ret = codec_ctrl(cur_codec, CODEC_CLR_ROUTE,arg);
		break;
	case SND_DSP_DEBUG:
		if (cur_codec)
			ret = codec_ctrl(cur_codec, CODEC_DEBUG,arg);
		break;
	case SND_DSP_RESUME_PROCEDURE:
		if (cur_codec)
			codec_ctrl(cur_codec, CODEC_RESUME,0);
		break;
	default:
		snd_error_print("SOUND_ERROR: unknown command!");
		ret = -EINVAL;
	}

	return ret;
}

/*##################################################################*\
|* functions
\*##################################################################*/

static irqreturn_t i2s_irq_handler(int irq, void *dev_id)
{
	irqreturn_t ret = IRQ_HANDLED;

	return ret;
}

static int i2s_init_pipe_2(struct dsp_pipe *dp, enum dma_data_direction direction, unsigned long iobase)
{
	if(dp == NULL) {
		snd_error_print("dp is null !! \n");
		return -EINVAL;
	}
	dp->dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	dp->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	dp->dma_type = JZDMA_REQ_I2S0;
	dp->fragment_size = 0;
	dp->fragment_cnt = 0;
	dp->living_attr.rate = DEFAULT_REPLAY_SAMPLERATE;
	dp->living_attr.format = 16;
	dp->living_attr.channel = DEFAULT_REPLAY_CHANNEL;;

	if (direction == DMA_TO_DEVICE) {
		dp->dma_config.direction = DMA_MEM_TO_DEV;
		dp->dma_config.src_maxburst = 64;
		dp->dma_config.dst_maxburst = 64;
		dp->dma_config.dst_addr = iobase + AICDR;
		dp->dma_config.src_addr = 0;
	} else if (direction == DMA_FROM_DEVICE) {
		dp->dma_config.direction = DMA_DEV_TO_MEM;
		dp->dma_config.src_maxburst = 32;
		dp->dma_config.dst_maxburst = 32;
		dp->dma_config.src_addr = iobase + AICDR;
		dp->dma_config.dst_addr = 0;
	} else
		return -1;

	return 0;
}

static int i2s_global_init(struct platform_device *pdev, struct snd_switch_data *switch_data)
{
	int ret = 0;
	struct dsp_pipe *i2s_pipe_out = NULL;
	struct dsp_pipe *i2s_pipe_in = NULL;
	struct i2s_device * i2s_dev;

	snd_debug_print("i2s global init\n");

	i2s_dev = (struct i2s_device *)kzalloc(sizeof(struct i2s_device), GFP_KERNEL);
	if(!i2s_dev) {
		dev_err(&pdev->dev, "failed to alloc i2s dev\n");
		return -ENOMEM;
	}
	memset(i2s_dev, 0, sizeof(*i2s_dev));

	i2s_dev->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(i2s_dev->res == NULL) {
		snd_error_print("i2s resource get failed\n");
		goto err_get_resource;
	}

	if(!request_mem_region(i2s_dev->res->start, resource_size(i2s_dev->res), pdev->name)) {
		snd_error_print("request mem region failed\n");
		goto err_req_mem_region;
	}

	i2s_dev->i2s_iomem = ioremap(i2s_dev->res->start, resource_size(i2s_dev->res));
	if(!i2s_dev->i2s_iomem) {
		snd_error_print("ioremap failed!\n");
		goto err_ioremap;
	}

	i2s_iomem = i2s_dev->i2s_iomem;/*modify later*/


	i2s_pipe_out = kmalloc(sizeof(struct dsp_pipe), GFP_KERNEL);
	if(!i2s_pipe_out){
		ret = -ENOMEM;
		snd_error_print("Failed to malloc pipe_out!\n");
		goto failed_alloc_out;
	}
	memset(i2s_pipe_out, 0, sizeof(struct dsp_pipe));
	ret = i2s_init_pipe_2(i2s_pipe_out, DMA_TO_DEVICE, i2s_dev->res->start);
	if(ret < 0) {
		snd_error_print("init write pipe failed!\n");
		goto err_init_pipeout;
	}

	i2s_pipe_in = kmalloc(sizeof(struct dsp_pipe), GFP_KERNEL);
	if(!i2s_pipe_in){
		ret = -ENOMEM;
		snd_error_print("Failed to malloc pipe_in!\n");
		goto failed_alloc_in;
	}
	memset(i2s_pipe_in, 0, sizeof(struct dsp_pipe));
	ret = i2s_init_pipe_2(i2s_pipe_in, DMA_FROM_DEVICE, i2s_dev->res->start);
	if(ret < 0) {
		snd_error_print("init read pipe failed!\n");
		goto err_init_pipein;
	}

	i2s_dev->i2s_endpoints = kmalloc(sizeof(struct dsp_endpoints), GFP_KERNEL);
	if(!i2s_dev->i2s_endpoints) {
		snd_debug_print("unable to malloc dsp endpoints!\n");
		goto err_init_endpoints;
	}
	memset(i2s_dev->i2s_endpoints, 0, sizeof(struct dsp_endpoints));
	i2s_dev->i2s_endpoints->out_endpoint = i2s_pipe_out;
	i2s_dev->i2s_endpoints->in_endpoint = i2s_pipe_in;

	i2s_dev->i2s_clk = clk_get(&pdev->dev, "cgu_i2s");
	if(IS_ERR(i2s_dev->i2s_clk)) {
		dev_err(&pdev->dev, "cgu i2s clk get failed!\n");
		goto err_get_i2s_clk;
	}
	i2s_dev->aic_clk = clk_get(&pdev->dev, "aic");
	if(IS_ERR(i2s_dev->aic_clk)) {
		dev_err(&pdev->dev, "aic clk get failed!\n");
		goto err_get_aic_clk;
	}

	spin_lock_init(&i2s_dev->i2s_irq_lock);
	spin_lock_init(&i2s_dev->i2s_lock);

#if (defined(CONFIG_JZ_INTERNAL_CODEC_V12) && defined(CONFIG_JZ_EXTERNAL_CODEC_V12))
	if(!codec_type)
		i2s_match_codec(i2s_dev, "internal_codec");
	else
		i2s_match_codec(i2s_dev, "i2s_external_codec");
#elif(defined(CONFIG_JZ_INTERNAL_CODEC_V12))
		i2s_match_codec(i2s_dev, "internal_codec");
#elif(defined(CONFIG_JZ_EXTERNAL_CODEC_V12))
		i2s_match_codec(i2s_dev, "i2s_external_codec");
#else
	//dev_info("WARNING: unless one codec must be select for i2s.\n");
#endif


	if(!i2s_dev->cur_codec) {
		snd_debug_print("err: no codec matched!\n");
		goto err_codec_found;
	}
	i2s_dev->i2s_irq = platform_get_irq(pdev, 0);
	ret = request_irq(i2s_dev->i2s_irq, i2s_irq_handler, IRQF_DISABLED, "i2s_irq", i2s_dev);
	if(ret < 0) {
		snd_debug_print("request i2s irq error!\n");
		goto err_irq;
	}

	g_i2s_dev = i2s_dev;

	i2s_set_switch_data(switch_data, i2s_dev);


	clk_enable(i2s_dev->aic_clk);
	__i2s_disable(i2s_dev);
	schedule_timeout(5);
	__i2s_disable(i2s_dev);
	__i2s_stop_bitclk(i2s_dev);
	__i2s_stop_ibitclk(i2s_dev);
	/*select i2s trans*/
	__aic_select_i2s(i2s_dev);
	__i2s_select_i2s(i2s_dev);

#if defined(CONFIG_JZ_EXTERNAL_CODEC_V12)
	__i2s_external_codec(i2s_dev);
#endif

	if(i2s_dev->cur_codec->codec_mode == CODEC_MASTER) {
#if defined(CONFIG_JZ_INTERNAL_CODEC_V12)
		if (!codec_type)
			__i2s_internal_codec_master(i2s_dev);
#endif
		__i2s_slave_clkset(i2s_dev);
		/*sysclk output*/
		__i2s_enable_sysclk_output(i2s_dev);

	} else if(i2s_dev->cur_codec->codec_mode == CODEC_SLAVE) {
#if defined(CONFIG_JZ_INTERNAL_CODEC_V12)
		if (!codec_type)
			__i2s_internal_codec_slave(i2s_dev);
#endif
		__i2s_master_clkset(i2s_dev);
		__i2s_disable_sysclk_output(i2s_dev);
	}

	clk_set_rate(i2s_dev->i2s_clk, 8000*256);
	snd_debug_print("i2s clk rate is %ld\n", clk_get_rate(i2s_dev->i2s_clk));
	clk_enable(i2s_dev->i2s_clk);

	__i2s_start_bitclk(i2s_dev);
	__i2s_start_ibitclk(i2s_dev);

	__i2s_disable_receive_dma(i2s_dev);
	__i2s_disable_transmit_dma(i2s_dev);
	__i2s_disable_record(i2s_dev);
	__i2s_disable_replay(i2s_dev);
	__i2s_disable_loopback(i2s_dev);
	__i2s_clear_ror(i2s_dev);
	__i2s_clear_tur(i2s_dev);
	__i2s_set_receive_trigger(i2s_dev, 3);
	__i2s_set_transmit_trigger(i2s_dev, 4);
	__i2s_disable_overrun_intr(i2s_dev);
	__i2s_disable_underrun_intr(i2s_dev);
	__i2s_disable_transmit_intr(i2s_dev);
	__i2s_disable_receive_intr(i2s_dev);
	__i2s_send_rfirst(i2s_dev);

	/* play zero or last sample when underflow */
	__i2s_play_lastsample(i2s_dev);
	__i2s_enable(i2s_dev);

	snd_debug_print("i2s init success.\n");
	codec_ctrl(i2s_dev->cur_codec, CODEC_INIT,0);

	return 0;

err_irq:

err_codec_found:

err_get_aic_clk:
	clk_put(i2s_dev->i2s_clk);
err_get_i2s_clk:
	kfree(i2s_dev->i2s_endpoints);
err_init_endpoints:
err_init_pipein:
	kfree(i2s_pipe_in);
failed_alloc_in:
err_init_pipeout:
	kfree(i2s_pipe_out);
failed_alloc_out:
	iounmap(i2s_dev->i2s_iomem);
err_ioremap:

err_req_mem_region:
err_get_resource:
	kfree(i2s_dev);
	return ret;
}

static int i2s_global_exit(struct platform_device *pdev, struct snd_switch_data *switch_data)
{
	struct dsp_pipe *i2s_pipe_out = NULL;
	struct dsp_pipe *i2s_pipe_in = NULL;
	struct i2s_device * i2s_dev = g_i2s_dev;

	if(!i2s_dev)
		return 0;

	i2s_pipe_out = i2s_dev->i2s_endpoints->out_endpoint;
	i2s_pipe_in = i2s_dev->i2s_endpoints->in_endpoint;
	__i2s_disable(i2s_dev);

	clk_disable(i2s_dev->i2s_clk);

	clk_disable(i2s_dev->aic_clk);

	i2s_set_switch_data(switch_data, NULL);

	free_irq(i2s_dev->i2s_irq, i2s_dev);

	clk_put(i2s_dev->aic_clk);
	clk_put(i2s_dev->i2s_clk);

	kfree(i2s_pipe_in);
	kfree(i2s_pipe_out);
	kfree(i2s_dev->i2s_endpoints);

	iounmap(i2s_dev->i2s_iomem);
	release_mem_region(i2s_dev->res->start, resource_size(i2s_dev->res));

	kfree(i2s_dev);
	g_i2s_dev = NULL;

	return 0;
}

static int i2s_init(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_dev_data *tmp;

	tmp = i2s_get_ddata(pdev);

	if(!g_i2s_dev) {
#if (defined(CONFIG_JZ_INTERNAL_CODEC_V12)&& defined(CONFIG_JZ_EXTERNAL_CODEC_V12))
		if (!codec_type) {
			jz_codec_init();
		} else {
			codec_i2c_dev_init();
			jz_es8374_init();
		}
#elif (defined(CONFIG_JZ_INTERNAL_CODEC_V12))
		jz_codec_init();
#elif (defined(CONFIG_JZ_EXTERNAL_CODEC_V12))
		if (1 == codec_type) {
			codec_i2c_dev_init();
			jz_es8374_init();
		}
#endif
		ret = i2s_global_init(pdev, &switch_data);
		i2s_set_private_data(tmp, g_i2s_dev);
	}
	return ret;
}

static int i2s_exit(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_dev_data *tmp;

	tmp = i2s_get_ddata(pdev);

	i2s_global_exit(pdev, &switch_data);
	if (!codec_type) {
		jz_codec_exit();
	} else {
		jz_es8374_exit();
		codec_i2c_dev_exit();
	}

	i2s_set_private_data(tmp, NULL);

	return ret;
}

static void i2s_shutdown(struct platform_device *pdev)
{
	/* close i2s and current codec */
	struct snd_dev_data *tmp_ddata = i2s_get_ddata(pdev);
	struct i2s_device *i2s_dev = i2s_get_private_data(tmp_ddata);
	struct codec_info *cur_codec = i2s_dev->cur_codec;

	if (cur_codec) {
		codec_ctrl(cur_codec, CODEC_TURN_OFF,CODEC_RWMODE);
		codec_ctrl(cur_codec, CODEC_SHUTDOWN,0);
	}
	__i2s_disable(i2s_dev);

	return;
}

static int i2s_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_dev_data *tmp_ddata = i2s_get_ddata(pdev);
	struct i2s_device *i2s_dev = i2s_get_private_data(tmp_ddata);
	struct codec_info *cur_codec = i2s_dev->cur_codec;

	if (cur_codec)
		codec_ctrl(cur_codec, CODEC_SUSPEND,0);
	__i2s_disable(i2s_dev);
	if(clk_is_enabled(i2s_dev->aic_clk)) {
		clk_disable(i2s_dev->aic_clk);
	}
	/*flush_work(&i2s_dev->i2s_work);*/
	return 0;
}

static int i2s_resume(struct platform_device *pdev)
{
	struct snd_dev_data *tmp_ddata = i2s_get_ddata(pdev);
	struct i2s_device *i2s_dev = i2s_get_private_data(tmp_ddata);

	if(!clk_is_enabled(i2s_dev->aic_clk)) {
		clk_enable(i2s_dev->aic_clk);
	}
	__i2s_enable(i2s_dev);

#if 0
	i2s_dev->ioctl_cmd = SND_DSP_RESUME_PROCEDURE;
	i2s_dev->ioctl_arg = 0;
	queue_work(i2s_dev->i2s_work_queue_1, &i2s_dev->i2s_work);
#else
	do_ioctl_work(i2s_dev, SND_DSP_RESUME_PROCEDURE, 0);
#endif
	return 0;
}
struct dsp_endpoints * i2s_get_endpoints(struct snd_dev_data *ddata)
{
	struct i2s_device *i2s_dev = i2s_get_private_data(ddata);

	return i2s_dev->i2s_endpoints;
}

struct snd_dev_data i2s_data = {
	.dev_ioctl_2	= i2s_ioctl_2,
	.get_endpoints	= i2s_get_endpoints,
	.minor			= SND_DEV_DSP0,
	.init			= i2s_init,
	.exit			= i2s_exit,
	.shutdown		= i2s_shutdown,
	.suspend		= i2s_suspend,
	.resume			= i2s_resume,
};

