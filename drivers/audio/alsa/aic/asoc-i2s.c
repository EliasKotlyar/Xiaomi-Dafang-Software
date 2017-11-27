/*
 *  sound/soc/ingenic/asoc-i2s.c
 *  ALSA Soc Audio Layer -- ingenic i2s (part of aic controller) driver
 *
 *  Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	niky <xianghui.shen@ingenic.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/slab.h>
#include "asoc-aic.h"

static int jz_i2s_debug = 0;
module_param(jz_i2s_debug, int, 0644);
#define I2S_DEBUG_MSG(msg...)			\
	do {					\
		if (jz_i2s_debug)		\
			printk("I2S: " msg);	\
	} while(0)

struct jz_i2s {
	struct device *aic;
#define I2S_WRITE 0x1
#define I2S_READ  0x2
#define I2S_INCODEC (0x1 <<4)
#define I2S_EXCODEC (0x2 <<4)
#define I2S_SLAVE (0x1 << 8)
#define I2S_MASTER (0x2 << 8)
	int i2s_mode;
	struct jz_pcm_dma_params tx_dma_data;
	struct jz_pcm_dma_params rx_dma_data;
	struct clk	*i2s_enable;
};

#define I2S_RFIFO_DEPTH 32
#define I2S_TFIFO_DEPTH 64
#define JZ_I2S_FORMATS (SNDRV_PCM_FMTBIT_S8 |  SNDRV_PCM_FMTBIT_S16_LE |	\
		SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE |	\
		SNDRV_PCM_FMTBIT_S24_LE)
#define JZ_I2S_RATE (SNDRV_PCM_RATE_8000_192000&(~SNDRV_PCM_RATE_64000))

static void dump_registers(struct device *aic)
{
	struct jz_aic *jz_aic = dev_get_drvdata(aic);

	pr_info("AIC_FR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICFR),jz_aic_read_reg(aic, AICFR));
	pr_info("AIC_CR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICCR),jz_aic_read_reg(aic, AICCR));
	pr_info("AIC_I2SCR\t%p : 0x%08x\n",(jz_aic->vaddr_base+I2SCR),jz_aic_read_reg(aic, I2SCR));
	pr_info("AIC_SR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICSR),jz_aic_read_reg(aic, AICSR));
	pr_info("AIC_I2SSR\t%p : 0x%08x\n",(jz_aic->vaddr_base+I2SSR),jz_aic_read_reg(aic, I2SSR));
	pr_info("AIC_I2SDIV\t%p : 0x%08x\n",(jz_aic->vaddr_base+I2SDIV),jz_aic_read_reg(aic, I2SDIV));
	pr_info("AIC_DR\t%p : 0x%08x\n",(jz_aic->vaddr_base+AICDR),jz_aic_read_reg(aic, AICDR));
	pr_info("AIC_I2SCDR\t 0x%08x\n",*(volatile unsigned int*)0xb0000060);
	pr_info("AICSR\t 0x%08x\n",*(volatile unsigned int*)0xb0020014);
	return;
}

static int jz_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;

	I2S_DEBUG_MSG("enter %s dai fmt %x\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	default:
	case SND_SOC_DAIFMT_I2S:		/*i2s format*/
		__i2s_select_i2s_fmt(aic);
		break;
	case SND_SOC_DAIFMT_MSB:
		__i2s_select_msb_fmt(aic);	/*msb format*/
		break;
	}
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	default:
	case SND_SOC_DAIFMT_CBM_CFM:	/*sync and bit clk (codec master : i2s slave)*/
		__i2s_bclk_input(aic);
		__i2s_sync_input(aic);
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		__i2s_bclk_output(aic);
		__i2s_sync_output(aic);
		break;
	}

	return 0;
}

static int jz_set_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;

	I2S_DEBUG_MSG("enter %s clk_id %d req %d clk dir %d\n", __func__,
			clk_id, freq, dir);

	if (clk_id == JZ_I2S_INNER_CODEC) {
		__aic_select_internal_codec_master(aic);
	} else
		__aic_select_external_codec(aic);

	aic_set_rate(aic, freq);

	if (dir  == SND_SOC_CLOCK_OUT){
		__i2s_select_sysclk_output(aic);
 		__i2s_enable_sysclk_output(aic);
	}
	else{
		__i2s_select_sysclk_input(aic);
		__i2s_disable_sysclk_output(aic);
	}
	return 0;
}

static int jz_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;

	I2S_DEBUG_MSG("enter %s div_id %d div %d\n", __func__, div_id , div);

	/*BIT CLK fix 64FS*/
	/*SYS_CLK is 256, 384, 512, 768*/
	if (div != 256 && div != 384 &&
			div != 512 && div != 768)
		return -EINVAL;

	__i2s_set_dv(aic, (div/64));
	__i2s_set_idv(aic, (div/64));
	return 0;
}


static int jz_i2s_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	enum aic_mode work_mode = AIC_I2S_MODE;

	I2S_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	work_mode = aic_set_work_mode(aic, work_mode, true);
	if (work_mode != AIC_I2S_MODE) {
		dev_warn(jz_i2s->aic, "Aic now is working on %s mode, open i2s mode failed\n",
				aic_work_mode_str(work_mode));
		return -EPERM;
	}

	if (!jz_i2s->i2s_mode) {
		__aic_select_i2s(aic);
		__i2s_play_lastsample(aic);
		__i2s_set_transmit_trigger(aic, I2S_TFIFO_DEPTH/4);
		__i2s_set_receive_trigger(aic, (I2S_RFIFO_DEPTH/4 - 1));
		__aic_enable(aic);
	}

	if (substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK) {
		__i2s_send_rfirst(aic);
		__i2s_disable_transmit_dma(aic);
		__i2s_disable_replay(aic);
		__aic_clear_tur(aic);
		jz_i2s->i2s_mode |= I2S_WRITE;
	} else {
		__i2s_send_rfirst(aic);
		__i2s_disable_receive_dma(aic);
		__i2s_disable_record(aic);
		__aic_clear_ror(aic);
		jz_i2s->i2s_mode |= I2S_READ;
	}

	return 0;
}

static int jz_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	int channels = params_channels(params);
	int fmt_width = snd_pcm_format_width(params_format(params));
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	enum dma_slave_buswidth buswidth;
	int trigger;

	I2S_DEBUG_MSG("enter %s, substream = %s\n", __func__,
		      (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
	if (!((1 << params_format(params)) & JZ_I2S_FORMATS) ||
			channels > 2) {
		dev_err(dai->dev, "hw params not inval channel %d params %x\n",
				channels, params_format(params));
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* channel */
		__i2s_channel(aic, channels);

		/* format */
		if (fmt_width == 8)
			buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
		else if (fmt_width == 16)
			buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
		else
			buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;

		jz_i2s->tx_dma_data.buswidth = buswidth;
		jz_i2s->tx_dma_data.max_burst = (I2S_TFIFO_DEPTH * buswidth)/2;
		trigger = I2S_TFIFO_DEPTH - (jz_i2s->tx_dma_data.max_burst/(int)buswidth);
		__i2s_set_oss(aic, fmt_width);
		if (trigger >= I2S_TFIFO_DEPTH)
			__i2s_set_transmit_trigger(aic, ((I2S_TFIFO_DEPTH >> 1) - 2));
		else
			__i2s_set_transmit_trigger(aic, trigger >> 1);
		snd_soc_dai_set_dma_data(dai, substream, (void *)&jz_i2s->tx_dma_data);
	} else {
		/* format */
		if (fmt_width == 8)
			buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
		else if (fmt_width == 16)
			buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
		else
			buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;

		jz_i2s->rx_dma_data.buswidth = buswidth;
		jz_i2s->rx_dma_data.max_burst = (I2S_RFIFO_DEPTH * buswidth)/2;
		trigger = jz_i2s->tx_dma_data.max_burst/(int)buswidth;
		__i2s_set_iss(aic, fmt_width);
		if (I2S_RFIFO_DEPTH <= trigger)
			__i2s_set_receive_trigger(aic, (((trigger +1) >> 1) - 1));
		else
			__i2s_set_receive_trigger(aic, 8);
		snd_soc_dai_set_dma_data(dai, substream, (void *)&jz_i2s->rx_dma_data);
	}

	return 0;
}

static void jz_i2s_start_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	int timeout = 150000;
	struct jz_aic *jz_aic = dev_get_drvdata(aic);
	I2S_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		int i = 4;
		dev_dbg(dai->dev, "codec fifo level0 %x\n", jz_aic_read_reg(aic, AICSR));
		for (i= 0; i < 16 ; i++) {
			__aic_write_txfifo(aic, 0x0);
			__aic_write_txfifo(aic, 0x0);
		}
		__aic_clear_tur(aic);
		dev_dbg(dai->dev, "codec fifo level1 %x\n", jz_aic_read_reg(aic, AICSR));
		__i2s_enable_replay(aic);
		while((!__aic_test_tur(aic)) && (timeout--)){
			if(timeout == 0){
				printk("%s %d: wait tansmit fifo under run error\n",__func__,__LINE__);
				return;
			}
		}
		__i2s_enable_transmit_dma(aic);
		__aic_clear_tur(aic);
		if (jz_i2s_debug) __aic_en_tur_int(aic);
	} else {
		__aic_flush_rxfifo(aic);
		mdelay(1);

		__i2s_enable_receive_dma(aic);
	pr_info("AIC_CR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICCR),jz_aic_read_reg(aic, AICCR));
		__i2s_enable_record(aic);
		/*clk_enable(jz_i2s->i2s_enable);*/
		if (jz_i2s_debug) __aic_en_ror_int(aic);
	}
	return;
}

static void jz_i2s_stop_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	int timeout = 150000;
	I2S_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (jz_i2s_debug) __aic_dis_tur_int(aic);
		if (__i2s_transmit_dma_is_enable(aic)) {
			__i2s_disable_transmit_dma(aic);
			__aic_clear_tur(aic);
			/*hrtime mode: stop will be happen in any where, make sure there is
			 *	no data transfer on ahb bus before stop dma
			 */
			while((!__aic_test_tur(aic)) && (timeout--)){
				if(timeout == 0){
					printk("%s %d: wait tansmit fifo under run error\n",__func__,__LINE__);
					return;
				}
			}
		}
		__i2s_disable_replay(aic);
		__aic_clear_tur(aic);
	} else {
		if (jz_i2s_debug) __aic_dis_ror_int(aic);
		if (__i2s_receive_dma_is_enable(aic)) {
			__i2s_disable_receive_dma(aic);
			__aic_clear_ror(aic);
			while(!__aic_test_ror(aic) && timeout--){
				if(timeout == 0){
					printk("%s %d: wait tansmit fifo over run error\n",__func__,__LINE__);
					return;
				}
			}
		}
		/*__i2s_disable_record(dai->dev);*/
		/*__i2s_disable_receive_dma(dai->dev);*/
		__i2s_disable_record(aic);
		__i2s_disable_receive_dma(aic);



		/*clk_disable(jz_i2s->i2s_enable);*/
		__aic_clear_ror(aic);
	}
	return;
}

static int jz_i2s_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	I2S_DEBUG_MSG("enter %s, substream = %s cmd = %d\n",
		      __func__,
		      (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture",
		      cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
		if (atomic_read(&prtd->stopped_pending))
			return -EPIPE;
#endif
		jz_i2s_start_substream(substream, dai);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
		if (atomic_read(&prtd->stopped_pending))
			return 0;
#endif
		jz_i2s_stop_substream(substream, dai);
		break;
	}
	dump_registers(jz_i2s->aic);
	return 0;
}

static void jz_i2s_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai) {
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	enum aic_mode work_mode = AIC_I2S_MODE;

	I2S_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
	work_mode = aic_set_work_mode(jz_i2s->aic, work_mode, false);
	BUG_ON((work_mode != AIC_NO_MODE));


	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		jz_i2s->i2s_mode &= ~I2S_WRITE;
	else
		jz_i2s->i2s_mode &= ~I2S_READ;

	if (!jz_i2s->i2s_mode)
		__aic_disable(aic);
	return;
}

static int jz_i2s_probe(struct snd_soc_dai *dai)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dai->dev);
	struct device *aic = jz_i2s->aic;
	/*struct jz_aic *jz_aic = dev_get_drvdata(aic);*/
	I2S_DEBUG_MSG("enter %s\n", __func__);

	__i2s_select_sysclk_output(aic);
	__aic_select_internal_codec_master(aic);
	__aic_select_i2s(aic);
	__aic_enable(aic);
	return 0;
}


static struct snd_soc_dai_ops jz_i2s_dai_ops = {
	.startup	= jz_i2s_startup,
	.trigger 	= jz_i2s_trigger,
	.hw_params 	= jz_i2s_hw_params,
	.shutdown	= jz_i2s_shutdown,
	.set_fmt	= jz_set_dai_fmt,
	.set_sysclk	= jz_set_sysclk,
	.set_clkdiv	= jz_set_clkdiv,
};

#define jz_i2s_suspend	NULL
#define jz_i2s_resume	NULL
static struct snd_soc_dai_driver jz_i2s_dai = {
		.probe   = jz_i2s_probe,
		.suspend = jz_i2s_suspend,
		.resume  = jz_i2s_resume,
		.playback = {
			.channels_min = 1,
			.channels_max = 1,
			.rates = JZ_I2S_RATE,
			.formats = JZ_I2S_FORMATS,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 1,
			.rates = JZ_I2S_RATE,
			.formats = JZ_I2S_FORMATS,
		},
		.ops = &jz_i2s_dai_ops,
};

static ssize_t jz_i2s_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct jz_i2s *jz_i2s = dev_get_drvdata(dev);
	dump_registers(jz_i2s->aic);
	return 0;
}

static struct device_attribute jz_i2s_sysfs_attrs[] = {
	__ATTR(i2s_regs, S_IRUGO, jz_i2s_regs_show, NULL),
};

static const struct snd_soc_component_driver jz_i2s_component = {
	.name		= "jz-i2s",
};

static int jz_i2s_platfrom_probe(struct platform_device *pdev)
{
	struct jz_aic_subdev_pdata *pdata = dev_get_platdata(&pdev->dev);
	struct jz_i2s *jz_i2s;
	int i = 0, ret;
	/*struct device *aic = pdev->dev.parent;*/
	jz_i2s = devm_kzalloc(&pdev->dev, sizeof(struct jz_i2s), GFP_KERNEL);
	if (!jz_i2s)
		return -ENOMEM;

	jz_i2s->aic = pdev->dev.parent;
	jz_i2s->i2s_mode = 0;
	jz_i2s->tx_dma_data.dma_addr = pdata->dma_base + AICDR;
	jz_i2s->rx_dma_data.dma_addr = pdata->dma_base + AICDR;
	platform_set_drvdata(pdev, (void *)jz_i2s);

	for (; i < ARRAY_SIZE(jz_i2s_sysfs_attrs); i++) {
		ret = device_create_file(&pdev->dev, &jz_i2s_sysfs_attrs[i]);
		if (ret)
			dev_warn(&pdev->dev,"attribute %s create failed %x",
					attr_name(jz_i2s_sysfs_attrs[i]), ret);
	}
#if 0
	jz_i2s->i2s_enable = clk_get(&pdev->dev, "aic-i2s");
	if (IS_ERR_OR_NULL(jz_i2s->i2s_enable)) {
		ret = PTR_ERR(jz_i2s->i2s_enable);
		jz_i2s->i2s_enable = NULL;
		dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
		return ret;
	}
#endif
	ret = snd_soc_register_component(&pdev->dev, &jz_i2s_component,
					 &jz_i2s_dai, 1);

	if (!ret)
		dev_info(&pdev->dev, "i2s platform probe success\n");
	return ret;
}

static int jz_i2s_platfom_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(jz_i2s_sysfs_attrs); i++)
		device_remove_file(&pdev->dev, &jz_i2s_sysfs_attrs[i]);
	platform_set_drvdata(pdev, NULL);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static struct platform_driver jz_i2s_plat_driver = {
	.probe  = jz_i2s_platfrom_probe,
	.remove = jz_i2s_platfom_remove,
	.driver = {
		.name = "jz-asoc-aic-i2s",
		.owner = THIS_MODULE,
	},
};

int jz_i2s_init(void)
{
	return platform_driver_register(&jz_i2s_plat_driver);
}

void jz_i2s_exit(void)
{
	platform_driver_unregister(&jz_i2s_plat_driver);
}
