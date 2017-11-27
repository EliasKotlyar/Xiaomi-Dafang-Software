/*
 * sound/soc/ingenic/icodec/inno_s40.h
 * ALSA SoC Audio driver -- ingenic internal codec (inno_s40) driver

 * Copyright 2016 Ingenic Semiconductor Co.,Ltd
 *	niky <xianghui.shen@ingenic.com>
 *
 * Note: inno_s40 is an internal codec for jz SOC
 *	 used for t10 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __INNO_S40_H__
#define __INNO_S40_H__

#include <linux/spinlock.h>
#include <sound/soc.h>
#include "../aic/asoc-aic.h"

struct inno_s40 {
	struct device		*dev;		/*aic device used to access register*/
	struct snd_soc_codec	*codec;
	spinlock_t       io_lock;		/*codec hw io lock,
							  Note codec cannot opt in irq context*/
	void * __iomem mapped_base;				/*vir addr*/
	resource_size_t mapped_resstart;				/*resource phy addr*/
	resource_size_t mapped_ressize;				/*resource phy addr size*/

	int dac_user_mute;				/*dac user mute state*/
	/*aohp power on anti pop event*/
	volatile int aohp_in_pwsq;				/*aohp in power up/down seq*/
	int hpl_wished_gain;				/*keep original hpl/r gain register value*/
	int hpr_wished_gain;
	int linl_wished_gain;				/*keep original hpl/r gain register value*/
	int linr_wished_gain;
	/*codec irq*/
	int irqno;
	int irqflags;
	int codec_imr;
	struct work_struct irq_work;
	/*headphone detect*/
	struct snd_soc_jack *jack;
	int report_mask;
};

/*
 * Note: inno_s40 codec just only support detect headphone jack
 * detected_type: detect event treat as detected_type
 *	 example: SND_JACK_HEADSET detect event treat as SND_JACK_HEADSET
 */
int inno_s40_hp_detect(struct snd_soc_codec *codec,
		struct snd_soc_jack *jack, int detected_type);

/*  inno_s40 internal register space */
enum {
	/* 0x00 */ 	TS_CODEC_CGR_00		= 0x00,
	 			TS_CODEC_RES_01,
	/* 0x08 */	TS_CODEC_CACR_02,
	/* 0x0c */	TS_CODEC_CMCR_03,
	/* 0x10 */	TS_CODEC_CDCR1_04,
	/* 0x14 */	TS_CODEC_CDCR2_05,
			 	TS_CODEC_RES_06,
	/* 0x1c */	TS_CODEC_CADR_07,
				TS_CODEC_RES_08,
				TS_CODEC_RES_09,
	/* 0x28 */	TS_CODEC_CGAINR_0a,
				TS_CODEC_RES_0b,
				TS_CODEC_RES_0c,
				TS_CODEC_RES_0d,
	/* 0x38 */	TS_CODEC_CDPR_0e,
	/* 0x3c */	TS_CODEC_CDDPR2_0f,
	/* 0x40 */	TS_CODEC_CDDPR1_10,
	/* 0x44 */	TS_CODEC_CDDPR0_11,
				TS_CODEC_RES_12		= 0x12,
				TS_CODEC_RES_20		= 0x20,
	/* 0x84 */ 	TS_CODEC_CAACR_21,
	/* 0x88 */ 	TS_CODEC_CMICCR_22,
	/* 0x8c */ 	TS_CODEC_CACR2_23,
	/* 0x90 */ 	TS_CODEC_CAMPCR_24,
	/* 0x94 */ 	TS_CODEC_CAR_25,
	/* 0x98 */ 	TS_CODEC_CHR_26,
	/* 0x9c */ 	TS_CODEC_CHCR_27,
	/* 0xa0 */ 	TS_CODEC_CCR_28,
				TS_CODEC_RES_29		= 0x29,
				TS_CODEC_RES_3f		= 0x3f,
	/* 0x100 */	TS_CODEC_CMR_40,
	/* 0x104 */	TS_CODEC_CTR_41,
	/* 0x108 */	TS_CODEC_CAGCCR_42,
	/* 0x10c */	TS_CODEC_CPGR_43,
	/* 0x110 */	TS_CODEC_CSRR_44,
	/* 0x114 */	TS_CODEC_CALMR_45,
	/* 0x118 */	TS_CODEC_CAHMR_46,
	/* 0x11c */	TS_CODEC_CALMINR_47,
	/* 0x120 */	TS_CODEC_CAHMINR_48,
	/* 0x124 */	TS_CODEC_CAFG_49,
			   	TS_CODEC_RES_4a,
			   	TS_CODEC_RES_4b,
	/* 0x130 */	TS_CODEC_CCAGVR_4c,
			   	TS_CODEC_MAX_REG_NUM
};

#define ADC_VALID_DATA_LEN_16BIT	0x0
#define ADC_VALID_DATA_LEN_20BIT	0x1
#define ADC_VALID_DATA_LEN_24BIT	0x2
#define ADC_VALID_DATA_LEN_32BIT	0x3

#define ADC_I2S_INTERFACE_RJ_MODE	0x0
#define ADC_I2S_INTERFACE_LJ_MODE	0x1
#define ADC_I2S_INTERFACE_I2S_MODE	0x2
#define ADC_I2S_INTERFACE_PCM_MODE	0x3

#define CHOOSE_ADC_CHN_STEREO		0x0
#define CHOOSE_ADC_CHN_MONO			0x1

#define SAMPLE_RATE_8K		0x7
#define SAMPLE_RATE_12K		0x6
#define SAMPLE_RATE_16K		0x5
#define SAMPLE_RATE_24K		0x4
#define SAMPLE_RATE_32K		0x3
#define SAMPLE_RATE_44_1K	0x2
#define SAMPLE_RATE_48K		0x1
#define SAMPLE_RATE_96K		0x0

/* ADC/DAC control register */
#define CODEC_ADC_DAC_I2S_LRC_SHIFT (7)
#define CODEC_ADC_DAC_I2S_LRC_MASK (0x1 << CODEC_ADC_DAC_I2S_LRC_SHIFT)
#define CODEC_ADC_DAC_VALID_LENGTH_SHIFT (5)
#define CODEC_ADC_DAC_VALID_LENGTH_MASK (0x3 << CODEC_ADC_DAC_VALID_LENGTH_SHIFT)
#define CODEC_ADC_DAC_I2S_MODE_SHIFT (3)
#define CODEC_ADC_DAC_I2S_MODE_MASK (0x3 << CODEC_ADC_DAC_I2S_MODE_SHIFT)
#define CODEC_ADC_DAC_I2S_TYPE_SHIFT (0)
#define CODEC_ADC_DAC_I2S_TYPE_MASK (0x1 << CODEC_ADC_DAC_I2S_TYPE_SHIFT)

/* CODEC mic control register */
#define CODEC_MIC_ENABLE_SHIFT (6)
#define CODEC_MIC_ENABLE_MASK (0x1 << CODEC_MIC_ENABLE_SHIFT)
#define CODEC_MIC_GAIN_SHIFT (5)
#define CODEC_MIC_GAIN_MASK (0x1 << CODEC_MIC_GAIN_SHIFT)
#define CODEC_MIC_MUTE_SHIFT (4)
#define CODEC_MIC_MUTE_MASK (0x1 << CODEC_MIC_MUTE_SHIFT)
#define CODEC_ALC_ENABLE_SHIFT (1)
#define CODEC_ALC_ENABLE_MASK (0x1 << CODEC_ALC_ENABLE_SHIFT)
#define CODEC_ALC_MUTE_SHIFT (0)
#define CODEC_ALC_MUTE_MASK (0x1 << CODEC_ALC_MUTE_SHIFT)

/* CODEC HPOUT register */
#define CODEC_HPOUT_ENABLE_SHIFT (7)
#define CODEC_HPOUT_ENABLE_MASK (0x1 << CODEC_HPOUT_ENABLE_SHIFT)
#define CODEC_HPOUT_INIT_SHIFT (6)
#define CODEC_HPOUT_INIT_MASK (0x1 << CODEC_HPOUT_INIT_SHIFT)
#define CODEC_HPOUT_MUTE_SHIFT (5)
#define CODEC_HPOUT_MUTE_MASK (0x1 << CODEC_HPOUT_MUTE_SHIFT)

/* CODEC sample rate register */
#define CODEC_SAMPLE_RATE_SHIFT (0)
#define CODEC_SAMPLE_RATE_MASK (0x7 << CODEC_SAMPLE_RATE_SHIFT)

static inline int inno_s40_hw_read(void __iomem * xreg, unsigned int offset)
{
	return readl(xreg + (offset << 2));
}

static inline void inno_s40_hw_write(void __iomem * xreg, unsigned int offset, int xval)
{
	writel(xval, xreg + (offset << 2));
}

int inno_s40_modinit(void);
void inno_s40_exit(void);
#endif	/* __INNO_S40_H__ */
