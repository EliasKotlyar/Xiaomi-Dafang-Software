#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/seq_file.h>
#include <linux/miscdevice.h>
#include <linux/input-polldev.h>
#include <linux/input.h>
#include <linux/gfp.h>
#include <linux/proc_fs.h>
#include "common_codec.h"

#include "../xb47xx_i2s_v12.h"
#include "es8374_codec.h"

static struct snd_codec_data es8374_codec_data = {
	.codec_sys_clk = 0,
	.codec_dmic_clk = 0,
	/* volume */
	.replay_volume_base = 0,
	.record_volume_base = 0,
	.record_digital_volume_base = 23,
	.replay_digital_volume_base = 0,
	/* default route */
	.replay_def_route = {.route = SND_ROUTE_REPLAY_DACRL_TO_ALL,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.record_def_route = {.route = SND_ROUTE_RECORD_MIC1_AN1,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	/* device <-> route map */
	.record_headset_mic_route = {.route = SND_ROUTE_RECORD_MIC1_SIN_AN2,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.record_buildin_mic_route = {.route = SND_ROUTE_RECORD_MIC1_AN1,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.replay_headset_route = {.route = SND_ROUTE_REPLAY_DACRL_TO_HPRL,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_DISABLE},
	.replay_speaker_route = {.route = SND_ROUTE_REPLAY_DACRL_TO_ALL,
					.gpio_hp_mute_stat = STATE_DISABLE,
					.gpio_spk_en_stat = STATE_ENABLE},
	.replay_headset_and_speaker_route = {.route = SND_ROUTE_REPLAY_DACRL_TO_ALL,
						.gpio_hp_mute_stat = STATE_DISABLE,
						.gpio_spk_en_stat = STATE_ENABLE},
	/* linein route */
	.record_linein_route = {.route = SND_ROUTE_RECORD_LINEIN1_AN2_SIN_TO_ADCL_AND_LINEIN2_AN3_SIN_TO_ADCR,
						.gpio_hp_mute_stat = STATE_DISABLE,
						.gpio_spk_en_stat = STATE_ENABLE,
	},

	.record_linein1_route = {.route = SND_ROUTE_RECORD_LINEIN1_DIFF_AN2,
						.gpio_hp_mute_stat = STATE_DISABLE,
						.gpio_spk_en_stat = STATE_ENABLE,
	},
	.record_linein2_route = {.route = SND_ROUTE_RECORD_LINEIN2_SIN_AN3,
						.gpio_hp_mute_stat = STATE_DISABLE,
						.gpio_spk_en_stat = STATE_ENABLE,
	},
};

static struct snd_codec_data *es8374_platform_data = &es8374_codec_data;
struct codec_operation *es8374_codec_ope = NULL;
extern int i2s_register_codec_2(struct codec_info * codec_dev);

struct es8374_data {
	struct i2c_client *client;
	struct snd_codec_data *pdata;
	struct mutex lock_rw;
};

struct es8374_data *es8374_save = NULL;

int es8374_i2c_read(struct es8374_data *es, unsigned char reg)
{
	unsigned char value;
	struct i2c_client *client = es->client;
	struct i2c_msg msg[2] = {
		[0] = {
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		},
		[1] = {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = &value,
		},
	};
	int err;
	err = i2c_transfer(client->adapter, msg, 2);
	if (err < 0) {
		printk("error:(%s,%d), Read msg error\n", __func__, __LINE__);
		return err;
	}
	return value;

}

int es8374_i2c_write(struct es8374_data *es, unsigned char reg, unsigned char value)
{
	struct i2c_client *client = es->client;
	unsigned char buf[2] = {reg, value};
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = 2,
		.buf = buf,
	};
	int err;
	err = i2c_transfer(client->adapter, &msg, 1);
	if (err < 0) {
		printk("error:(%s,%d), Write msg error.\n",__func__,__LINE__);
		return err;
	}

	return 0;
}

int es8374_reg_set(unsigned char reg, int start, int end, int val)
{
	int ret;
	int i = 0, mask = 0;
	unsigned char oldv = 0, new = 0;
	for(i = 0; i < (end-start + 1); i++) {
		mask += (1 << (start + i));
	}
	oldv = es8374_i2c_read(es8374_save, reg);
	new = oldv & (~mask);
	new |= val << start;
	ret = es8374_i2c_write(es8374_save, reg, new);
	if(ret < 0) {
#ifdef ES8374_DEBUG
		printk("fun:%s,ES8374 I2C Write error.\n",__func__);
#endif
	}
	if(new != es8374_i2c_read(es8374_save, reg)) {
#ifdef ES8374_DEBUG
		printk("es8374 write error, reg = 0x%x, start = %d, end = %d, val = 0x%x\n", reg, start, end, val);
#endif
	}

	return ret;
}

void es8374_dac_mute(int mute)
{
	if(mute) { //mute dac
		es8374_reg_set(0x36, 0, 7, 0x20);
	}
	else {    //unmute
		es8374_reg_set(0x36, 0, 7, 0x00);
	}
}

#ifdef ES8374_DEBUG
int dump_es8374_codec_regs(void)
{
	unsigned int i;
	unsigned char value;
	printk("es8374 codec register list as:.\n");
	if(!es8374_save) return 0;

	for(i = 0; i <= 0x6E; i++) {
		value = es8374_i2c_read(es8374_save, i);
		printk("addr:0x%02x---value:0x%02x\n", i, value);
	}

	return 0;
}
#endif

int es8374_codec_init(void)
{
		es8374_reg_set(0x00, 0, 7, 0x3F); //IC Rst start //ERROR:0000
		es8374_reg_set(0x00, 0, 7, 0x03); //IC Rst stop
		/*
		*	user can decide the valule of MCLKDIV2 according to the frequency of MCLK clock.
		*/
		es8374_reg_set(0x01, 0, 7, 0x7F); //IC clk on, MCLK DIV2 =0 IF MCLK is lower than 16MHZ

		es8374_reg_set(0x6F, 0, 7, 0xA0); //pll set:mode enable
		es8374_reg_set(0x72, 0, 7, 0x41); //pll set:mode set :ERROR:0000
		es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start
		/* PLL FOR 12MHZ/48KHZ */
		es8374_reg_set(0x0C, 0, 7, 0x08); //pll set:k
		es8374_reg_set(0x0D, 0, 7, 0x13); //pll set:k
		es8374_reg_set(0x0E, 0, 7, 0xE0); //pll set:k
		es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
		es8374_reg_set(0x0B, 0, 7, 0x08);//pll set:n

		/* PLL FOR 12MHZ/44.1KHZ */
		es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset off ,set stop

		es8374_reg_set(0x05, 0, 7, 0x11); //clk div =1
		es8374_reg_set(0x03, 0, 7, 0x20); //osr =32
		es8374_reg_set(0x06, 0, 7, 0x01); //LRCK div =0100H = 256D
		es8374_reg_set(0x07, 0, 7, 0x00);

		/*
		*	user can select master or slave mode for ES8374
		*/
		es8374_reg_set(0x0F, 0, 7, 0x84); //MASTER MODE, BCLK = MCLK/4
		//es8374_reg_set(0x0F, 0, 7, 0x04); //SLAVE MODE, BCLK = MCLK/4


		es8374_reg_set(0x10, 0, 7, 0x0C); //I2S-16BIT, ADC
		es8374_reg_set(0x11, 0, 7, 0x0C); //I2S-16BIT, DAC
		/*
		*	user can decide to use PLL clock or not.
		*/
		es8374_reg_set(0x02, 0, 7, 0x00);  //don't select PLL clock, use clock from MCLK pin.

		es8374_reg_set(0x24, 0, 7, 0x08); //adc set
		es8374_reg_set(0x36, 0, 7, 0x40); //dac set
		es8374_reg_set(0x12, 0, 7, 0x30); //timming set
		es8374_reg_set(0x13, 0, 7, 0x20); //timming set
		/*
		*	by default, select LIN1&RIN1 for ADC recording
		*/
		es8374_reg_set(0x21, 0, 7, 0x50); //adc set: SEL LIN1 CH+PGAGAIN=0DB
		es8374_reg_set(0x22, 0, 7, 0x55); //adc set: PGA GAIN=15DB//ERROR:0000
		es8374_reg_set(0x21, 0, 7, 0x14); //adc set: SEL LIN1 CH+PGAGAIN=18DB
		/*
		*	chip start
		*/
		es8374_reg_set(0x00, 0, 7, 0x80); // IC START
		msleep(50); //DELAY_MS
		es8374_reg_set(0x14, 0, 7, 0x8A); // IC START
		es8374_reg_set(0x15, 0, 7, 0x40); // IC START
		es8374_reg_set(0x1C, 0, 7, 0x90); // spk set
		es8374_reg_set(0x1D, 0, 7, 0x02); // spk set
		es8374_reg_set(0x1F, 0, 7, 0x00); // spk set
		es8374_reg_set(0x1E, 0, 7, 0xA0); // spk on
		es8374_reg_set(0x28, 0, 7, 0x00); // alc set
		es8374_reg_set(0x25, 0, 7, 0x00); // ADCVOLUME on
		es8374_reg_set(0x38, 0, 7, 0x00); // DACVOLUMEL on
		es8374_reg_set(0x37, 0, 7, 0x30); // dac set
		es8374_reg_set(0x6D, 0, 7, 0x60); //SEL:GPIO1=DMIC CLK OUT+SEL:GPIO2=PLL CLK OUT

		es8374_reg_set(0x71, 0, 7, 0x05);
		es8374_reg_set(0x73, 0, 7, 0x70);
		es8374_reg_set(0x36, 0, 7, 0x00); //dac set
		es8374_reg_set(0x37, 0, 7, 0x00); // dac set

		es8374_reg_set(0x10, 6, 7, 0x3); //I2S-16BIT, ADC, MUTE ADC SDP
		es8374_reg_set(0x11, 6, 6, 0x1); //I2S-16BIT, DAC, MUTE DAC SDP

		return 0;

}

static int es8374_codec_turn_off(int mode)
{

	int ret = 0;
	if (mode & CODEC_RMODE) {
		/*close record*/
		es8374_reg_set(0x10, 6, 7, 0x3); //I2S-16BIT, ADC, MUTE ADC SDP
		es8374_reg_set(0x25, 0, 7, 0xC0);
		es8374_reg_set(0x28, 0, 7, 0x1C);
		es8374_reg_set(0x21, 0, 7, 0xD4);
	}
	if (mode & CODEC_WMODE) {
		/*close replay*/
		es8374_dac_mute(1);
		es8374_reg_set(0x11, 6, 6, 0x1); //I2S-16BIT, DAC, MUTE DAC SDP
		es8374_reg_set(0x1D, 0, 7, 0x10);
		es8374_reg_set(0x1E, 0, 7, 0x40);
	}

	return ret;

}
static int es8374_set_sample_rate(unsigned int rate)
{

	int i = 0;

	unsigned int mrate[8] = {8000, 12000, 16000, 24000, 32000, 44100, 48000, 96000};
	unsigned int rate_fs[8] = {7, 6, 5, 4, 3, 2, 1, 0};
	for (i = 0; i < 8; i++) {
		if (rate <= mrate[i])
			break;
	}
	if (i == 8)
		i = 0;

	switch(rate_fs[i]) {
    	case  7: //8000
    		/*
    		*	set pll, 12MHZ->12.288MHZ
    		*/
				es8374_dac_mute(1);
				es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start

				es8374_reg_set(0x0C, 0, 7, 0x08); //pll set:k
				es8374_reg_set(0x0D, 0, 7, 0x13); //pll set:k
				es8374_reg_set(0x0E, 0, 7, 0xE0); //pll set:k
				es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
				es8374_reg_set(0x0B, 0, 7, 0x08);//pll set:n

				es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset off ,set stop

				es8374_reg_set(0x05, 0, 7, 0x11); //adc&dac clkdiv
				es8374_reg_set(0x02, 0, 7, 0x00);//
				es8374_reg_set(0x06, 0, 7, 0x06); //LRCK div =0600H = 1536D, lrck = 12.288M/1536 = 8K
				es8374_reg_set(0x06, 0, 7, 0x01); //add by sxzhang
				es8374_reg_set(0x07, 0, 7, 0x00); //add by sxzhang
				es8374_reg_set(0x0f, 0, 7, 0x9d);  //bclk = mclk/ 48 = 256K
				es8374_reg_set(0x0f, 0, 4, 0x04);
				es8374_dac_mute(0);
				break;
    	case 6: //12000
    		/*
    		*	set pll, 12MHZ->12.288MHZ
    		*/
    		es8374_dac_mute(1);
    		es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start

				es8374_reg_set(0x0C, 0, 7, 0x08); //pll set:k
				es8374_reg_set(0x0D, 0, 7, 0x13); //pll set:k
				es8374_reg_set(0x0E, 0, 7, 0xE0); //pll set:k
				es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
				es8374_reg_set(0x0B, 0, 7, 0x08); //pll set:n

				es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset on ,set start

				es8374_reg_set(0x05, 0, 7, 0x44);//adc&dac clkdiv
				es8374_reg_set(0x02, 0, 7, 0x08);//pll enable
				es8374_reg_set(0x06, 0, 7, 0x04); //LRCK div =0400H = 1024D, lrck = 12.288M/1024 = 12K
				es8374_reg_set(0x07, 0, 7, 0x00);
				es8374_reg_set(0x0f, 0, 7, 0x90); //bclk = mclk/ 16 = 768K
				es8374_dac_mute(0);
				break;
    	case 5: //16000
    		/*
    		*	set pll, 26MHZ->12.288MHZ
    		*/

				es8374_dac_mute(1);
				es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start

				es8374_reg_set(0x0C, 0, 7, 0x08); //pll set:k
				es8374_reg_set(0x0D, 0, 7, 0x13); //pll set:k
				es8374_reg_set(0x0E, 0, 7, 0xE0); //pll set:k
				es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
				es8374_reg_set(0x0B, 0, 7, 0x08); //pll set:n

				es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset on ,set start

				es8374_reg_set(0x05, 0, 7, 0x11);//adc&dac clkdiv
				es8374_reg_set(0x02, 0, 7, 0x00);
				es8374_reg_set(0x06, 0, 7, 0x01);
				es8374_reg_set(0x07, 0, 7, 0x00);

				es8374_reg_set(0x0f, 0, 7, 0x8c);
				es8374_reg_set(0x0f, 0, 4, 0x04);
				es8374_dac_mute(0);
				break;
    	case 4: //24000
    		/*
    		*	set pll, 12MHZ->12.288MHZ
    		*/
				es8374_dac_mute(1);
				es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start

				es8374_reg_set(0x0C, 0, 7, 0x08); //pll set:k
				es8374_reg_set(0x0D, 0, 7, 0x13); //pll set:k
				es8374_reg_set(0x0E, 0, 7, 0xE0); //pll set:k
				es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
				es8374_reg_set(0x0B, 0, 7, 0x08); //pll set:n

				es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset on ,set start

				es8374_reg_set(0x05, 0, 7, 0x11);//adc&dac clkdiv
				es8374_reg_set(0x02, 0, 7, 0x00);
				es8374_reg_set(0x06, 0, 7, 0x01);
				es8374_reg_set(0x07, 0, 7, 0x00);
				es8374_reg_set(0x0f, 0, 7, 0x84);
				es8374_dac_mute(0);
				break;
    	case 3: //32000
    		/*
    		*	set pll, 12MHZ->8.192MHZ
    		*/
				es8374_dac_mute(1);
				es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start

				es8374_reg_set(0x0C, 0, 7, 0x13); //pll set:k
				es8374_reg_set(0x0D, 0, 7, 0x68); //pll set:k
				es8374_reg_set(0x0E, 0, 7, 0xa5); //pll set:k
				es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
				es8374_reg_set(0x0B, 0, 7, 0x05); //pll set:n

				es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset on ,set start

				es8374_reg_set(0x05, 0, 7, 0x11);//adc&dac clkdiv
				es8374_reg_set(0x02, 0, 7, 0x08);//pll enable
				es8374_reg_set(0x06, 0, 7, 0x01); //LRCK div =0100H = 256D, lrck = 12.288M/256 = 32K
				es8374_reg_set(0x07, 0, 7, 0x00);
				es8374_reg_set(0x0f, 0, 7, 0x84);  //bclk = mclk/4 = 2048k
				es8374_dac_mute(0);
				break;
		case 2: //44100;
    		/*
    		*	set pll, 12MHZ->11.2896MHZ
    		*/
				es8374_dac_mute(1);
				es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start

				es8374_reg_set(0x0C, 0, 7, 0x16); //pll set:k
				es8374_reg_set(0x0D, 0, 7, 0x25); //pll set:k
				es8374_reg_set(0x0E, 0, 7, 0x6c); //pll set:k
				es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
				es8374_reg_set(0x0B, 0, 7, 0x07); //pll set:n

				es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset on ,set start

				es8374_reg_set(0x05, 0, 7, 0x11);//adc&dac clkdiv
				es8374_reg_set(0x02, 0, 7, 0x08);
				es8374_reg_set(0x06, 0, 7, 0x01); //LRCK div =0100H = 256D, lrck = 11.2896M/256 = 44.1K
				es8374_reg_set(0x07, 0, 7, 0x00);
				es8374_reg_set(0x0f, 0, 7, 0x84);  //bclk = mclk/4 = 2822.4k
				es8374_dac_mute(0);
				break;
     	case 1: //48000
				es8374_dac_mute(1);
				es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start

				es8374_reg_set(0x0C, 0, 7, 0x08); //pll set:k
				es8374_reg_set(0x0D, 0, 7, 0x13); //pll set:k
				es8374_reg_set(0x0E, 0, 7, 0xE0); //pll set:k
				es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
				es8374_reg_set(0x0B, 0, 7, 0x08); //pll set:n

				es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset on ,set start

				es8374_reg_set(0x05, 0, 7, 0x11);//adc&dac clkdiv
				es8374_reg_set(0x02, 0, 7, 0x00);//pll enable
				es8374_reg_set(0x06, 0, 7, 0x01); //LRCK div =0100H = 256D, lrck = 12.288M/256 = 48K
				es8374_reg_set(0x07, 0, 7, 0x00);
				es8374_reg_set(0x0f, 0, 7, 0x84);  //bclk = mclk/ 8 = 3072k
				es8374_dac_mute(0);
				break;
     	case 0: //96000
    		/*
    		*	set pll, 12MHZ->11.2896MHZ
    		*/
				es8374_dac_mute(1);
				es8374_reg_set(0x09, 0, 7, 0x01); //pll set:reset on ,set start

				es8374_reg_set(0x0C, 0, 7, 0x08); //pll set:k
				es8374_reg_set(0x0D, 0, 7, 0x13); //pll set:k
				es8374_reg_set(0x0E, 0, 7, 0xE0); //pll set:k
				es8374_reg_set(0x0A, 0, 7, 0x8A); //pll set:
				es8374_reg_set(0x0B, 0, 7, 0x08); //pll set:n

				es8374_reg_set(0x09, 0, 7, 0x41); //pll set:reset on ,set start

				es8374_reg_set(0x05, 0, 7, 0x11);//adc&dac clkdiv
				es8374_reg_set(0x02, 4, 5, 0x03);//double speed
				es8374_reg_set(0x02, 3, 3, 0x01);//pll enable
				es8374_reg_set(0x06, 0, 7, 0x00); //LRCK div =0080H = 128D, lrck = 12.288M/128 = 96K
				es8374_reg_set(0x07, 0, 7, 0x80);
				es8374_reg_set(0x0f, 0, 7, 0x82);  //bclk = mclk/ 2 = 6144k
				es8374_dac_mute(0);
				break;
    	default:
				printk("error:(%s,%d), error audio sample rate.\n",__func__,__LINE__);
				break;
	}
	return 0;
}


static int es8374_set_speaker(void)
{
	es8374_reg_set(0x11, 6, 6, 0x0); //I2S-16BIT, DAC, MUTE DAC SDP

	es8374_reg_set(0x1D, 0, 7, 0x02);
	es8374_reg_set(0x1E, 0, 7, 0xA0);

	es8374_dac_mute(0);
	return 0;
}

static int es8374_set_buildin_mic(void)
{
	es8374_reg_set(0x10, 6, 7, 0x3); //I2S-16BIT, ADC, MUTE ADC SDP

	es8374_reg_set(0x21, 0, 7, 0x24); //adc set: SEL LIN2&RIN2 for buildin mic Recording

	es8374_reg_set(0x26, 0, 7, 0x4E); // alc set
	es8374_reg_set(0x27, 0, 7, 0x08); // alc set
	es8374_reg_set(0x28, 0, 7, 0x70); // alc set
	es8374_reg_set(0x29, 0, 7, 0x00); // alc set
	es8374_reg_set(0x2b, 0, 7, 0x00); // alc set
	//eq filter
	es8374_reg_set(0x45, 0, 7, 0x03);
	es8374_reg_set(0x46, 0, 7, 0xF7);
	es8374_reg_set(0x47, 0, 7, 0xFD);
	es8374_reg_set(0x48, 0, 7, 0xFF);
	es8374_reg_set(0x49, 0, 7, 0x1F);
	es8374_reg_set(0x4A, 0, 7, 0xF7);
	es8374_reg_set(0x4B, 0, 7, 0xFD);
	es8374_reg_set(0x4C, 0, 7, 0xFF);
	es8374_reg_set(0x4D, 0, 7, 0x03);
	es8374_reg_set(0x4E, 0, 7, 0xF7);
	es8374_reg_set(0x4F, 0, 7, 0xFD);
	es8374_reg_set(0x50, 0, 7, 0xFF);
	es8374_reg_set(0x51, 0, 7, 0x1F);
	es8374_reg_set(0x52, 0, 7, 0xF7);
	es8374_reg_set(0x53, 0, 7, 0xFD);
	es8374_reg_set(0x54, 0, 7, 0xFF);
	es8374_reg_set(0x55, 0, 7, 0x1F);
	es8374_reg_set(0x56, 0, 7, 0xF7);
	es8374_reg_set(0x57, 0, 7, 0xFD);
	es8374_reg_set(0x58, 0, 7, 0xFF);
	es8374_reg_set(0x59, 0, 7, 0x03);
	es8374_reg_set(0x5A, 0, 7, 0xF7);
	es8374_reg_set(0x5B, 0, 7, 0xFD);
	es8374_reg_set(0x5C, 0, 7, 0xFF);
	es8374_reg_set(0x5D, 0, 7, 0x1F);
	es8374_reg_set(0x5E, 0, 7, 0xF7);
	es8374_reg_set(0x5F, 0, 7, 0xFD);
	es8374_reg_set(0x60, 0, 7, 0xFF);
	es8374_reg_set(0x61, 0, 7, 0x03);
	es8374_reg_set(0x62, 0, 7, 0xF7);
	es8374_reg_set(0x63, 0, 7, 0xFD);
	es8374_reg_set(0x64, 0, 7, 0xFF);
	es8374_reg_set(0x65, 0, 7, 0x1F);
	es8374_reg_set(0x66, 0, 7, 0xF7);
	es8374_reg_set(0x67, 0, 7, 0xFD);
	es8374_reg_set(0x68, 0, 7, 0xFF);
	es8374_reg_set(0x69, 0, 7, 0x1F);
	es8374_reg_set(0x6A, 0, 7, 0xF7);
	es8374_reg_set(0x6B, 0, 7, 0xFD);
	es8374_reg_set(0x6C, 0, 7, 0xFF);
	es8374_reg_set(0x2D, 0, 7, 0x85);

	es8374_reg_set(0x10, 6, 7, 0x0); //I2S-16BIT, ADC, un-MUTE ADC SDP

	return 0;
}

void es8374_codec_set_play_volume(int * vol)
{
	int volume = 0;

	if (*vol < 0) *vol = 0;
	if (*vol > 0x1f) *vol = 0x1f;

	volume = *vol;
	volume = 124 - volume * 4;
	/*printk("%s %d: codec set play volume = 0x%02x\n",__func__ , __LINE__,  volume);*/
	es8374_reg_set(0x38, 0, 7, volume);
}

static unsigned int cur_out_device = -1;
static int es8374_set_device(struct es8374_data *es, enum snd_device_t device)
{

	int ret = 0;
	int iserror = 0;

	switch (device) {
	case SND_DEVICE_SPEAKER:
		ret = es8374_set_speaker();
		printk("%s: set device: speaker...\n", __func__);
		break;

	case SND_DEVICE_BUILDIN_MIC:
		ret = es8374_set_buildin_mic();
		printk("%s: set device: MIC...\n", __func__);
		break;
	default:
		iserror = 1;
		printk("JZ CODEC: Unkown ioctl argument %d in SND_SET_DEVICE\n",device);
	}

	if (!iserror)
		cur_out_device = device;

	return ret;

}

void es8374_codec_set_record_volume(int * vol)
{
	int volume = 0;

	if (*vol < 0) *vol = 0;
	if (*vol > 0x1f) *vol = 0x1f;

	volume = *vol;
	volume = 124 - volume * 4;
	/*printk("%s %d: codec set play volume = 0x%02x\n",__func__ , __LINE__,  volume);*/
	es8374_reg_set(0x25, 0, 7, volume);

}


static int es8374_codec_ctl(struct codec_info *codec_dev, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
		case CODEC_INIT:
			ret = es8374_codec_init();
			break;

		case CODEC_TURN_OFF:
			printk("%s: set CODEC_TURN_OFF...\n", __func__);
			ret = es8374_codec_turn_off(arg);
			break;

		case CODEC_SHUTDOWN:
			printk("%s: set CODEC_SHUTDOWN...\n", __func__);
			break;

		case CODEC_RESET:
			break;

		case CODEC_SUSPEND:
			break;

		case CODEC_RESUME:
			break;

		case CODEC_ANTI_POP:
			break;

		case CODEC_SET_DEFROUTE:
			break;

		case CODEC_SET_DEVICE:
			printk(KERN_DEBUG"%s: set device...\n", __func__);
			ret = es8374_set_device(es8374_save, *(enum snd_device_t *)arg);
			break;

		case CODEC_SET_STANDBY:
			break;

		case CODEC_SET_REPLAY_RATE:
		case CODEC_SET_RECORD_RATE:
			printk(KERN_DEBUG"%s: set sample rate...\n", __func__);
			ret = es8374_set_sample_rate(*(enum snd_device_t *)arg);
			break;
		case CODEC_SET_MIC_VOLUME:
			break;
		case CODEC_SET_RECORD_VOLUME:
			es8374_codec_set_record_volume((int *)arg);
			break;
		case CODEC_SET_RECORD_CHANNEL:
			break;
		case CODEC_SET_REPLAY_VOLUME:
			es8374_codec_set_play_volume((int *)arg);
			break;
		case CODEC_SET_REPLAY_CHANNEL:
			printk(KERN_DEBUG"%s: set replay channel...\n", __func__);
			break;
		case CODEC_DAC_MUTE:
			break;
		case CODEC_ADC_MUTE:
			break;
		case CODEC_DUMP_REG:
#ifdef ES8374_DEBUG
			ret = dump_es8374_codec_regs();
#endif
			break;
		case CODEC_CLR_ROUTE:
			printk("%s: set CODEC_CLR_ROUTE...\n", __func__);
			break;
		case CODEC_DEBUG:
			break;
		default:
			break;
	}
#ifdef ES8374_DEBUG
	dump_es8374_codec_regs();
#endif

	return ret;

}
static const struct i2c_device_id es8374_id[] = {
	{"es8374", 0},
	{ }
};

static int es8374_codec_register(struct platform_device *pdev)
{
	if(es8374_codec_ope) return 0;

	es8374_codec_ope = (struct codec_operation*) kzalloc(sizeof(struct codec_operation), GFP_KERNEL);
	if(!es8374_codec_ope) {
		printk("fun:%s,alloc es8374 codec mem failed,\n",__func__);
		return -ENOMEM;
	}
	pdev->dev.platform_data = es8374_platform_data;
	platform_set_drvdata(pdev, &es8374_codec_ope);

	printk("%s, probe() successful!\n",__func__);
	return 0;
}

static int es8374_codec_release(void)
{
	return 0;
}

static int codec_match(struct device *dev, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	int ret = 0;

	if(!get_device(dev))
		return -ENODEV;
	if(!strncmp(pdev->name, "es8374-codec", sizeof("es8374-codec"))) {
		ret = es8374_codec_register(pdev);
	}

	put_device(dev);
	return ret;
}

#define T30_EXTERNAL_CODEC_CLOCK 12000000
int jz_es8374_init(void)
{
	int retval;
	struct codec_info *codec_dev;

	codec_dev = kzalloc(sizeof(struct codec_info), GFP_KERNEL);
	if(!codec_dev) {
		printk("error:(%s,%d),alloc codec device error\n",__func__,__LINE__);
		return -1;
	}

	sprintf(codec_dev->name, "i2s_external_codec");
	codec_dev->codec_ctl_2 = es8374_codec_ctl;
	codec_dev->codec_clk = T30_EXTERNAL_CODEC_CLOCK;
	codec_dev->codec_mode = CODEC_MASTER;

	i2s_register_codec_2(codec_dev);

	retval = bus_for_each_dev(&platform_bus_type, NULL, NULL, codec_match);
	if(retval) {
		 printk("%s[%d]: Failed to register codec driver!\n",__func__,__LINE__);
		 return retval;
	}

	if(es8374_codec_ope) {
		es8374_codec_ope->priv = (void*)codec_dev;
	}

	es8374_codec_init();
	return retval;
}

void jz_es8374_exit(void)
{
	struct codec_info *codec_dev;

	es8374_codec_release();
	codec_dev = es8374_codec_ope->priv;
	i2s_release_codec_2(codec_dev);

	kfree(codec_dev);
	kfree(es8374_codec_ope);
	es8374_codec_ope = NULL;
}

int es8374_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct es8374_data *es;

	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "client not i2c capable\n");
		ret = -ENODEV;
		goto err_dev;
	}

	es = kzalloc(sizeof(struct es8374_data), GFP_KERNEL);
	if (NULL == es) {
		dev_err(&client->dev, "failed to allocate memery for module data\n");
		ret = -ENOMEM;
		goto err_klloc;
	}

	es->client = client;
	i2c_set_clientdata(client, es);

	es8374_save = es;

	return 0;
err_klloc:
	kfree(es);
err_dev:
	return ret;
}

static int es8374_remove(struct i2c_client *client)
{
	if (es8374_save) {
		kfree(es8374_save);
		es8374_save = NULL;
	}
	return 0;
}


MODULE_DEVICE_TABLE(i2c, es8374_id);
static struct i2c_driver es8374_driver = {
	.driver = {
		.name = "es8374",
		.owner = THIS_MODULE,
	},

	.probe = es8374_probe,
	.remove = es8374_remove,
	.id_table = es8374_id,
};


int es8374_i2c_drv_init(void)
{
	i2c_add_driver(&es8374_driver);
	return 0;

}

void es8374_i2c_drv_exit(void)
{
	i2c_del_driver(&es8374_driver);
}

