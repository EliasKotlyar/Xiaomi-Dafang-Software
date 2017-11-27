/*
 * sound/soc/ingenic/icodec/inno_s40.c
 * ALSA SoC Audio driver -- ingenic internal codec (inno_s40) driver

 * Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	nily <xianghui.shen@ingenic.com>
 *
 * Note: inno_s40 is an internal codec for jz SOC
 *	 used for x1000 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/div64.h>
#include <sound/soc-dai.h>
#include <soc/irq.h>
#include "inno-s40.h"

static int inno_s40_debug = 0;
module_param(inno_s40_debug, int, 0644);
#define DEBUG_MSG(msg...)			\
	do {					\
		if (inno_s40_debug)		\
			printk("ICDC: " msg);	\
	} while(0)

static u8 inno_s40_reg_defcache[TS_CODEC_MAX_REG_NUM] = {
/* reg 0x0 ... 0x7 */
		0x03,0x00,0x50,0x0e,0x50,0x0e,0x00,0x00,
/* reg 0x8 ... 0xf */
		0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xff,
/* reg 0x10 ... 0x20 */
		0xe0,0x00,[TS_CODEC_RES_12 ... TS_CODEC_RES_20] = 0x00,
/* reg 0x21 ... 0x27 */
		0x00,0x00,0x0c,0x00,0x00,0x00,0x40,
/* reg 0x28 ... 0x3f */
		0x1e,[TS_CODEC_RES_29 ... TS_CODEC_RES_3f] = 0x00,
/* reg 0x40 ... 0x47 */
		0x00,0x46,0x41,0x2c,0x00,0x26,0x40,0x36,
/* reg 0x48 ... 0x4c */
		0x20,0x38,0x00,0x00,0x0c,
};

static int inno_s40_volatile(struct snd_soc_codec *codec, unsigned int reg)
{
	return 1;
}

static int inno_s40_writable(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg > TS_CODEC_MAX_REG_NUM)
		return 0;

	switch (reg) {
	case TS_CODEC_RES_01:
	case TS_CODEC_RES_06:
	case TS_CODEC_RES_08:
	case TS_CODEC_RES_09:
	case TS_CODEC_RES_0b:
	case TS_CODEC_RES_0c:
	case TS_CODEC_RES_0d:
	case TS_CODEC_RES_12 ... TS_CODEC_RES_20:
	case TS_CODEC_RES_29 ... TS_CODEC_RES_3f:
	case TS_CODEC_RES_4a:
	case TS_CODEC_RES_4b:
	case TS_CODEC_CCAGVR_4c:
		return 0;
	default:
		return 1;
	}
}

static int inno_s40_readable(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg > TS_CODEC_MAX_REG_NUM)
		return 0;

	switch (reg) {
	case TS_CODEC_RES_01:
	case TS_CODEC_RES_06:
	case TS_CODEC_RES_08:
	case TS_CODEC_RES_09:
	case TS_CODEC_RES_0b:
	case TS_CODEC_RES_0c:
	case TS_CODEC_RES_0d:
	case TS_CODEC_RES_12 ... TS_CODEC_RES_20:
	case TS_CODEC_RES_29 ... TS_CODEC_RES_3f:
	case TS_CODEC_RES_4a:
	case TS_CODEC_RES_4b:
		return 0;
	default:
		return 1;
	}
}

static void dump_registers_hazard(struct inno_s40 *inno_s40)
{
	int reg = 0;
	dev_info(inno_s40->dev, "-------------------register:");
	for ( ; reg < TS_CODEC_MAX_REG_NUM; reg++) {
		if (reg % 8 == 0)
			printk("\n");
		if (inno_s40_readable(inno_s40->codec, reg))
			printk(" 0x%02x:0x%02x,", reg, inno_s40_hw_read(inno_s40->mapped_base, reg));
	}
	printk("\n");
	dev_info(inno_s40->dev, "----------------------------\n");
	return;
}

static int inno_s40_write(struct snd_soc_codec *codec, unsigned int reg,
			unsigned int value)
{
	struct inno_s40 *inno_s40 = snd_soc_codec_get_drvdata(codec);
	int val = value;
	BUG_ON(reg > TS_CODEC_MAX_REG_NUM);
	dev_dbg(inno_s40->dev,"%s reg = %x value = %x \n",__func__,reg,val);
	if (inno_s40_writable(codec, reg)) {
		inno_s40_hw_write(inno_s40->mapped_base, reg, val);
	}
	return 0;
}

static unsigned int inno_s40_read(struct snd_soc_codec *codec, unsigned int reg)
{

	struct inno_s40 *inno_s40 = snd_soc_codec_get_drvdata(codec);
	BUG_ON(reg > TS_CODEC_MAX_REG_NUM);

	if (inno_s40_readable(codec, reg))
		return inno_s40_hw_read(inno_s40->mapped_base, reg);

	return 0;
}

static int inno_s40_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level) {
	DEBUG_MSG("%s enter set level %d\n", __func__, level);

	codec->dapm.bias_level = level;
	return 0;
}

static int inno_s40_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	int playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	int bit_width_sel = 0;
	int speed_sel = 0;
	int fcr_reg = playback ? TS_CODEC_CDCR2_05 : TS_CODEC_CACR_02;
	unsigned int sample_attr[] = {8000, 12000, 16000, 24000, 36000, 44100, 48000, 96000};
	unsigned int rate_fs[8] = {7, 6, 5, 4, 3, 2, 1, 0};
	int speed = params_rate(params);
	int bit_width = params_format(params);
	DEBUG_MSG("%s enter  set bus width %d , sample rate %d\n",
			__func__, bit_width, speed);
	/* bit width */
	switch (bit_width) {
	case SNDRV_PCM_FORMAT_S20_3LE:
		bit_width_sel = 1;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		bit_width_sel = 2;
		break;
	default:
	case SNDRV_PCM_FORMAT_S16_LE:
		bit_width_sel = 0;
		break;
	}

	/*sample rate*/
	for (speed_sel = 0; speed_sel < ARRAY_SIZE(sample_attr); speed_sel++) {
		if ( speed <= sample_attr[speed_sel])
			break;
	}
	if(speed_sel == ARRAY_SIZE(sample_attr))
		speed_sel = 0;

	snd_soc_update_bits(codec, fcr_reg, CODEC_ADC_DAC_VALID_LENGTH_MASK,
			(bit_width_sel << CODEC_ADC_DAC_VALID_LENGTH_SHIFT));
	snd_soc_update_bits(codec, TS_CODEC_CSRR_44, CODEC_SAMPLE_RATE_MASK,
			(rate_fs[speed_sel] << CODEC_SAMPLE_RATE_SHIFT));
	return 0;
}

static int inno_s40_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, CODEC_MIC_MUTE_MASK, (mute ? 0 : 1)<< CODEC_MIC_MUTE_SHIFT);
	snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, CODEC_ALC_MUTE_SHIFT,(mute ? 0 : 1)<< CODEC_ALC_MUTE_SHIFT);
	snd_soc_update_bits(codec, TS_CODEC_CHR_26, CODEC_HPOUT_MUTE_MASK, (mute ? 0 : 1)<< CODEC_HPOUT_MUTE_SHIFT);

	return 0;
}

static int inno_s40_trigger(struct snd_pcm_substream * stream, int cmd,
		struct snd_soc_dai *dai)
{
#ifdef DEBUG
	struct snd_soc_codec *codec = dai->codec;
	struct inno_s40 *inno_s40 = snd_soc_codec_get_drvdata(codec);
	int i = 0;
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/*dump_registers_hazard(inno_s40);*/
		for(i = 0; i < TS_CODEC_MAX_REG_NUM; i ++)
			printk("reg = 0x%04x value = 0x%08x\n", i<<2, inno_s40_hw_read(inno_s40->mapped_base, i));
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/*dump_registers_hazard(inno_s40);*/
		for(i = 0; i < TS_CODEC_MAX_REG_NUM; i ++)
			printk("reg = 0x%04x value = 0x%08x\n", i<<2, inno_s40_hw_read(inno_s40->mapped_base, i));
		break;
	}
#endif
	return 0;
}

#define CODEC_PCM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
			 SNDRV_PCM_FMTBIT_S20_3LE |SNDRV_PCM_FMTBIT_S24_LE)

#define CODEC_SAMPLE_RATE (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 \
			SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000)

static int inno_s40_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	int i = 0;
	struct snd_soc_codec *codec = dai->codec;

	/* codec precharge */
	snd_soc_update_bits(codec, TS_CODEC_CHCR_27, (0x3 << 6), (1<<6));
	msleep(10);
#ifdef CONFIG_SOC_T20
	snd_soc_update_bits(codec, TS_CODEC_CCR_28, (1<<7), (1<<7));
	msleep(10);
	for (i = 0; i < 6; i++) {
		snd_soc_update_bits(codec, TS_CODEC_CCR_28, 0x3f, 0x3f >> (6 -i));
		usleep_range(20000, 22000);
		/*msleep(20);*/
	}
	snd_soc_update_bits(codec, TS_CODEC_CCR_28, 0x3f, 0x3f);
	msleep(20);
#else	/* CONFIG_SOC_T10 */
	snd_soc_update_bits(codec, TS_CODEC_CCR_28, (1<<7), (0<<7));
	msleep(10);
	for (i = 0; i < 6; i++) {
		snd_soc_update_bits(codec, TS_CODEC_CCR_28, 0x3f, 0x3f >> i);
		usleep_range(20000, 22000);
		/*msleep(20);*/
	}
	snd_soc_update_bits(codec, TS_CODEC_CCR_28, 0x3f, 0x0);
	msleep(20);
#endif
	return 0;
}


static void inno_s40_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	/*power off codec*/
	/*close record*/
	snd_soc_update_bits(codec, TS_CODEC_CAMPCR_24, (0x7<<5), 0x0);
	/*close replay*/
	snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0xf, 0x0);
	return;
}


int inno_s40_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_codec *codec = dai->codec;

	if(stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_update_bits(codec, TS_CODEC_CHR_26, CODEC_HPOUT_MUTE_MASK, (mute ? 0 : 1)<< CODEC_HPOUT_MUTE_SHIFT);
	} else if(stream == SNDRV_PCM_STREAM_CAPTURE) {
		snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, CODEC_MIC_MUTE_MASK, (mute ? 0 : 1)<< CODEC_MIC_MUTE_SHIFT);
		snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, CODEC_ALC_MUTE_SHIFT,(mute ? 0 : 1)<< CODEC_ALC_MUTE_SHIFT);
	}
	return 0;
}


static struct snd_soc_dai_ops inno_s40_dai_ops = {
	.hw_params	= inno_s40_hw_params,
	.mute_stream = inno_s40_mute_stream,
//	.digital_mute	= inno_s40_digital_mute,
	.trigger = inno_s40_trigger,
	.shutdown	= inno_s40_shutdown,
	.startup	= inno_s40_startup,
};

static struct snd_soc_dai_driver  inno_s40_codec_dai = {
	.name = "jz-codec-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100
						| SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100
						| SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops = &inno_s40_dai_ops,
};

/* unit: 0.01dB */
static const DECLARE_TLV_DB_SCALE(dac_tlv, -3900, 150, 0);
static const DECLARE_TLV_DB_SCALE(adc_tlv, -1800, 150, 0);

static const struct snd_kcontrol_new inno_s40_snd_controls[] = {
	/* Volume controls */
	SOC_SINGLE_TLV("Speaker Playback Volume", TS_CODEC_CHCR_27, 0, 31, 0, dac_tlv),
	SOC_SINGLE_TLV("Mic Mixer Volume", TS_CODEC_CACR2_23, 0, 31, 0, adc_tlv),

	SOC_SINGLE("Mic Capture Volume", TS_CODEC_CMICCR_22, 5, 1, 0),

	/* HPOUT private controls */
	SOC_SINGLE("Speaker Playback mute", TS_CODEC_CHR_26, 5, 1, 1),

	/* mic private controls */
	SOC_SINGLE("Mic Capture mute", TS_CODEC_CMICCR_22, 4, 1, 1),

	/* ALC private controls */
	SOC_SINGLE("Automatic Level Control mute", TS_CODEC_CMICCR_22, 0, 1, 1),
};

static int inno_s40_adc_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) {

	struct snd_soc_codec *codec = w->codec;
	if(event == SND_SOC_DAPM_POST_PMU){
		/* ADC valid data length */
		snd_soc_update_bits(codec, TS_CODEC_CACR_02, (0x3 << 5), 0);
		/* Choose ADC I2S interface mode */
		snd_soc_update_bits(codec, TS_CODEC_CACR_02, (0x3 << 3), 2 << 3);
		/* Choose ADC chn */
		snd_soc_update_bits(codec, TS_CODEC_CACR_02, (0x1 << 0), 0x01);
		msleep(5);
		/* Choose ADC I2S Master Mode */
		snd_soc_update_bits(codec, TS_CODEC_CMCR_03, 0xff, 0x3e);
		msleep(5);
		/* set sample rate */
		snd_soc_update_bits(codec, TS_CODEC_CSRR_44, 0x7, 0x7);
		/* set record volume */
		snd_soc_update_bits(codec, TS_CODEC_CACR2_23, 0x1f, 0xc);
		/* MIC mode: 1: Signal-ended input , 0: Full Diff input */
		snd_soc_update_bits(codec, TS_CODEC_CACR2_23, (0x1 << 5), (0x1 << 5));
		msleep(5);
		/* enable current source of ADC module, enable mic bias */
		snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x1 << 7, 0x1<<7);
		snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x1 << 6, 0x1<<6);
		/* set MIC Bias is 1.6 * vcc */
		snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x7 << 0, 6 << 0);
		msleep(5);
		/* enable ref voltage for ADC */
		snd_soc_update_bits(codec, TS_CODEC_CAMPCR_24, 0x1<<7, 0x1<<7);
		msleep(5);
		/* enable BST mode */
		snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, 0x1<<6, 0x1<<6);
		/* enable ALC mode */
		snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, 0x1<<0, 0x1<<0);
		msleep(5);
		/* enable ADC clk and ADC amplifier */
		snd_soc_update_bits(codec, TS_CODEC_CAMPCR_24, 0x1<<6, 0x01<<6);
		snd_soc_update_bits(codec, TS_CODEC_CAMPCR_24, 0x01<<5, 0x01<<5);
		msleep(5);
		/* make ALC in work state */
		snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, 0x01<<1, 0x01<<1);
		/* make BST in work state */
		snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, 0x01<<4, 0x01<<4);
		msleep(5);
		/* enable zero-crossing detection */
		snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x01<<5, 0x1<<5);
		msleep(5);
		/* record ALC gain 6dB */
		snd_soc_update_bits(codec, TS_CODEC_CACR2_23, 0x1f<<0, 0x13<<0);
		msleep(5);
		/* record fix alg gain 20dB */
		snd_soc_update_bits(codec, TS_CODEC_CMICCR_22, 0x1<<5, 0x1<<5);
		msleep(5);
	}
	if(event == SND_SOC_DAPM_PRE_PMD){
		/* disable ALC mode */
		/*snd_soc_update_bits(codec, TS_CODEC_CAACR_49, 0x01<<6, 0x0<<6);*/
		/*msleep(5);*/
		/* disable zero-crossing detection */
		snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x01<<5, 0x0<<5);
		msleep(5);
		/* disable current source of ADC module, enable mic bias */
		snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x1 << 6, 0x0<<6);
		snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x1 << 7, 0x0<<7);
	}
	return 0;
}

static int inno_s40_dac_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) {

	struct snd_soc_codec *codec = w->codec;
	if(event == SND_SOC_DAPM_POST_PMU){
		/* Choose ADC I2S interface mode */
		snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x3<<6, 0x3<<6);
		msleep(5);
		/* enable current source */
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<6, 0x1<<6);
		/* enable reference voltage */
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<5, 0x1<<5);
		msleep(5);
		/* enable POP sound control */
		snd_soc_update_bits(codec, TS_CODEC_CHCR_27, 0x3<<6, 0x2<<6);
		msleep(5);
		/* enable DAC */
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<3, 0x1<<3);
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<2, 0x1<<2);
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<1, 0x1<<1);
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<0, 0x1<<0);
		msleep(5);
		/* enable HP OUT */
		snd_soc_update_bits(codec, TS_CODEC_CHR_26, 0x1<<7, 0x1<<7);
		snd_soc_update_bits(codec, TS_CODEC_CHR_26, 0x1<<6, 0x1<<6);
		snd_soc_update_bits(codec, TS_CODEC_CHR_26, 0x1<<5, 0x1<<5);
		msleep(5);

		/* set sample rate */
		snd_soc_update_bits(codec, TS_CODEC_CSRR_44, 0x7<<0, 0x7<<0);
		msleep(5);

		/* set replay volume */
		snd_soc_update_bits(codec, TS_CODEC_CHCR_27, 0x1f<<0, 0x18<<0);
	}

	if(event == SND_SOC_DAPM_PRE_PMD){
		/* set replay volume */
		snd_soc_update_bits(codec, TS_CODEC_CHCR_27, 0x1f<<0, 0x10<<0);
		/* set HPOUT mute mode*/
		snd_soc_update_bits(codec, TS_CODEC_CHR_26, 0x1<<5, 0x0<<5);
		msleep(5);
		/* disable HPOUT */
		snd_soc_update_bits(codec, TS_CODEC_CHR_26, 0x1<<6, 0x0<<6);
		snd_soc_update_bits(codec, TS_CODEC_CHR_26, 0x1<<7, 0x0<<7);
		/* disable DAC */
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<1, 0x0<<1);
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<2, 0x0<<2);
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<3, 0x0<<3);
		msleep(5);

		/* HPOUT POP control signal  */
		snd_soc_update_bits(codec, TS_CODEC_CHCR_27, 0x3<<6, 0x1<<6);

		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<5, 0x0<<5);
		msleep(5);
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<6, 0x0<<5);
		snd_soc_update_bits(codec, TS_CODEC_CAR_25, 0x1<<0, 0x0<<0);
	}
	return 0;
}

static const struct snd_soc_dapm_widget inno_s40_dapm_widgets[] = {
/* ADC */
	SND_SOC_DAPM_ADC_E("ADC", "Capture", SND_SOC_NOPM, 0, 0, inno_s40_adc_event,
			SND_SOC_DAPM_PRE_PMD|SND_SOC_DAPM_POST_PMU),

	SND_SOC_DAPM_MICBIAS("MICBIAS", TS_CODEC_CMICCR_22, 6, 0),

	/*SND_SOC_DAPM_ADC("ADC", "Capture" , TS_CODEC_CMICCR_22, 7, 0),*/
	/*SND_SOC_DAPM_ADC("ADC zore cross", "Capture" , TS_CODEC_CMICCR_22, 5, 0),*/

	/*SND_SOC_DAPM_PGA("AMIC", SCODA_REG_CR_MIC1, 4, 1, NULL, 0),*/
	/*SND_SOC_DAPM_PGA("DMIC", SCODA_REG_CR_DMIC, 7, 0, NULL, 0),*/

/* DAC */
	SND_SOC_DAPM_DAC_E("DAC", "Playback", SND_SOC_NOPM, 0, 0, inno_s40_dac_event,
			SND_SOC_DAPM_PRE_PMD|SND_SOC_DAPM_POST_PMU),

	/*SND_SOC_DAPM_PGA("DAC_MERCURY", SCODA_REG_CR_DAC, 4, 1, NULL, 0),*/

/* PINS */
	SND_SOC_DAPM_INPUT("AIP"),
	SND_SOC_DAPM_INPUT("AIN"),
	SND_SOC_DAPM_OUTPUT("DO_LO_PWM"),
};

static const struct snd_soc_dapm_route intercon[] = {

	{ "MICBIAS",  NULL,  "AIP" },
	{ "MICBIAS",  NULL,  "AIN" },
	/*input*/
	{ "ADC",  NULL,  "MICBIAS" },

	/*output*/
	{ "DO_LO_PWM", NULL, "DAC" },
};

#ifdef CONFIG_PM
static int inno_s40_suspend(struct snd_soc_codec *codec)
{
	struct inno_s40 *inno_s40 = snd_soc_codec_get_drvdata(codec);

	inno_s40_set_bias_level(codec, SND_SOC_BIAS_OFF);

	/* disable ADC clk */
	snd_soc_update_bits(codec, TS_CODEC_CAMPCR_24, 0x1<<6, 0x0<<6);
	/* disable DAC clk */
	snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x1<<2, 0x1<<2);
	return 0;
}

static int inno_s40_resume(struct snd_soc_codec *codec)
{
	struct inno_s40 *inno_s40 = snd_soc_codec_get_drvdata(codec);

	/* enable ADC clk */
	snd_soc_update_bits(codec, TS_CODEC_CAMPCR_24, 0x1<<6, 0x1<<6);

	/* enable DAC clk */
	snd_soc_update_bits(codec, TS_CODEC_CAACR_21, 0x1<<2, 0x0<<2);
	inno_s40_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	return 0;
}
#endif

static int inno_s40_probe(struct snd_soc_codec *codec)
{
	struct inno_s40 *inno_s40 = snd_soc_codec_get_drvdata(codec);

	dev_info(codec->dev, "codec inno-s40 probe enter\n");

	/* reset codec */
	snd_soc_update_bits(codec, TS_CODEC_CGR_00, 0x3, 0);
	msleep(10);
	snd_soc_update_bits(codec, TS_CODEC_CGR_00, 0x3, 0x3);
	msleep(10);

	/* Choose DAC I2S Master Mode */
	snd_soc_update_bits(codec, TS_CODEC_CMCR_03, 0xff, 0x3e);
	snd_soc_update_bits(codec, TS_CODEC_CDCR1_04, 0xff, 0x10);//DAC I2S interface is I2S Mode.
	snd_soc_update_bits(codec, TS_CODEC_CDCR2_05, 0xff, 0xe);

	inno_s40->codec = codec;
	return 0;
}

static int inno_s40_remove(struct snd_soc_codec *codec)
{
	/*struct inno_s40 *inno_s40 = snd_soc_codec_get_drvdata(codec);*/
	dev_info(codec->dev, "codec inno_s40 remove enter\n");
	inno_s40_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_inno_s40_codec = {
	.probe = 	inno_s40_probe,
	.remove = 	inno_s40_remove,
#ifdef CONFIG_PM
	.suspend =	inno_s40_suspend,
	.resume =	inno_s40_resume,
#endif

	.read = 	inno_s40_read,
	.write = 	inno_s40_write,
	.volatile_register = inno_s40_volatile,
	.readable_register = inno_s40_readable,
	.writable_register = inno_s40_writable,
	/*.reg_cache_default = inno_s40_reg_defcache,*/
	.reg_word_size = sizeof(u8),
	.reg_cache_step = 1,
	.reg_cache_size = TS_CODEC_MAX_REG_NUM,
	/*.set_bias_level = inno_s40_set_bias_level,*/

	.controls = 	inno_s40_snd_controls,
	.num_controls = ARRAY_SIZE(inno_s40_snd_controls),
	.dapm_widgets = inno_s40_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(inno_s40_dapm_widgets),
	.dapm_routes = intercon,
	.num_dapm_routes = ARRAY_SIZE(intercon),
};

/*Just for debug*/
static ssize_t inno_s40_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct inno_s40 *inno_s40 = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = inno_s40->codec;
	if (!codec) {
		dev_info(dev, "inno_s40 is not probe, can not use %s function\n", __func__);
		return 0;
	}
	mutex_lock(&codec->mutex);
	dump_registers_hazard(inno_s40);
	mutex_unlock(&codec->mutex);
	return 0;
}

static ssize_t inno_s40_regs_store(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	struct inno_s40 *inno_s40 = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = inno_s40->codec;
	const char *start = buf;
	unsigned int reg, val;
	int ret_count = 0;

	if (!codec) {
		dev_info(dev, "inno_s40 is not probe, can not use %s function\n", __func__);
		return count;
	}

	while(!isxdigit(*start)) {
		start++;
		if (++ret_count >= count)
			return count;
	}
	reg = simple_strtoul(start, (char **)&start, 16);
	while(!isxdigit(*start)) {
		start++;
		if (++ret_count >= count)
			return count;
	}
	val = simple_strtoul(start, (char **)&start, 16);
	mutex_lock(&codec->mutex);
	inno_s40_write(codec, reg, val);
	mutex_unlock(&codec->mutex);
	return count;
}

static struct device_attribute inno_s40_sysfs_attrs =
	__ATTR(hw_regs, S_IRUGO|S_IWUGO, inno_s40_regs_show, inno_s40_regs_store);

static int inno_s40_platform_probe(struct platform_device *pdev)
{
	struct inno_s40 *inno_s40 = NULL;
	struct resource *res = NULL;
	int ret = 0;

	inno_s40 = (struct inno_s40*)devm_kzalloc(&pdev->dev,
			sizeof(struct inno_s40), GFP_KERNEL);
	if (!inno_s40)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Faild to get ioresource mem\n");
		return -ENOENT;
	}

	if (!devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "Failed to request mmio memory region\n");
		return -EBUSY;
	}
	inno_s40->mapped_resstart = res->start;
	inno_s40->mapped_ressize = resource_size(res);
	inno_s40->mapped_base = devm_ioremap_nocache(&pdev->dev,
			inno_s40->mapped_resstart,
			inno_s40->mapped_ressize);
	if (!inno_s40->mapped_base) {
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		return -ENOMEM;
	}

//	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
//	if (!res)
//		return -ENOENT;
	inno_s40->dev = &pdev->dev;
	inno_s40->dac_user_mute = 1;
	inno_s40->aohp_in_pwsq = 0;
	spin_lock_init(&inno_s40->io_lock);
	platform_set_drvdata(pdev, (void *)inno_s40);

	ret = snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_inno_s40_codec, &inno_s40_codec_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Faild to register codec\n");
		platform_set_drvdata(pdev, NULL);
		return ret;
	}

	ret = device_create_file(&pdev->dev, &inno_s40_sysfs_attrs);
	if (ret)
		dev_warn(&pdev->dev,"attribute %s create failed %x",
				attr_name(inno_s40_sysfs_attrs), ret);
	dev_info(&pdev->dev, "codec inno-s40 platfrom probe success\n");


	return 0;
}

static int inno_s40_platform_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "codec inno-s40 platform remove\n");
	snd_soc_unregister_codec(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver inno_s40_codec_driver = {
	.driver = {
		.name = "jz-codec",
		.owner = THIS_MODULE,
	},
	.probe = inno_s40_platform_probe,
	.remove = inno_s40_platform_remove,
};

int inno_s40_modinit(void)
{
	return platform_driver_register(&inno_s40_codec_driver);
}

void inno_s40_exit(void)
{
	platform_driver_unregister(&inno_s40_codec_driver);
}
