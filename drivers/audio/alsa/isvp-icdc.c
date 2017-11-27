/*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: niky <xianghui.shen@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/gpio.h>
#include <mach/jzsnd.h>
#include "icodec/inno-s40.h"
#include "aic/asoc-aic.h"
#include "dma/asoc-dma.h"

static int spk_gpio = GPIO_PB(31);
module_param(spk_gpio, int, S_IRUGO);
MODULE_PARM_DESC(spk_gpio, "Speaker GPIO NUM");

static int spk_gpio_level = 1;
module_param(spk_gpio_level, int, S_IRUGO);
MODULE_PARM_DESC(spk_gpio_level, "Speaker GPIO active level");

static struct snd_codec_data codec_platform_data = {
	.gpio_spk_en = {
		.gpio = GPIO_PB(0),
		.active_level = 0,
	},
};

static int isvp_spk_power(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (codec_platform_data.gpio_spk_en.gpio != -1) {
			gpio_direction_output(codec_platform_data.gpio_spk_en.gpio, codec_platform_data.gpio_spk_en.active_level);
			printk("gpio speaker enable %d\n", gpio_get_value(codec_platform_data.gpio_spk_en.gpio));
		} else
			printk("set speaker enable failed. please check codec_platform_data\n");
	} else {
		if (codec_platform_data.gpio_spk_en.gpio != -1) {
			gpio_direction_output(codec_platform_data.gpio_spk_en.gpio, !(codec_platform_data.gpio_spk_en.active_level));
			printk("gpio speaker disable %d\n", gpio_get_value(codec_platform_data.gpio_spk_en.gpio));
		} else
			printk("set speaker disable failed. please check codec_platform_data\n");
	}
	return 0;
}

void isvp_spk_sdown(struct snd_pcm_substream *sps){

	if (codec_platform_data.gpio_spk_en.gpio != -1) {
		gpio_direction_output(codec_platform_data.gpio_spk_en.gpio, !(codec_platform_data.gpio_spk_en.active_level));
	}

	return;
}

int isvp_spk_sup(struct snd_pcm_substream *sps){
	if (codec_platform_data.gpio_spk_en.gpio != -1) {
		gpio_direction_output(codec_platform_data.gpio_spk_en.gpio, codec_platform_data.gpio_spk_en.active_level);
	}
	return 0;
}

int isvp_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int pll_out = 0;
	int ret;

	/*FIXME snd_soc_dai_set_pll*/
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM);
	if (ret)
		return ret;

	pll_out = params_rate(params) * 256;
	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, pll_out, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;
	return 0;
};

int isvp_i2s_hw_free(struct snd_pcm_substream *substream)
{
	/*notify release pll*/
	return 0;
};


static struct snd_soc_ops isvp_i2s_ops = {
	.startup = isvp_spk_sup,
	.shutdown = isvp_spk_sdown,
	.hw_params = isvp_i2s_hw_params,
	.hw_free = isvp_i2s_hw_free,

};
static const struct snd_soc_dapm_widget isvp_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", isvp_spk_power),
	SND_SOC_DAPM_MIC("Mic Buildin", NULL),
};

/* isvp machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* ext speaker connected to DO_LO_PWM  */
	{"Speaker", NULL, "DO_LO_PWM"},

	/* mic is connected to AIP/N1 */
	{"MICBIAS", NULL, "Mic Buildin"},

};

static int isvp_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err = 0;
	if (codec_platform_data.gpio_spk_en.gpio != -1) {
		err = devm_gpio_request(card->dev, codec_platform_data.gpio_spk_en.gpio, "Speaker_en");
		if (err)
			return err;
	} else {
		pr_err("codec_platform_data gpio_spk_en is NULL\n");
		return err;
	}
	err = snd_soc_dapm_new_controls(dapm, isvp_dapm_widgets,
			ARRAY_SIZE(isvp_dapm_widgets));
	if (err){
		printk("isvp dai add controls err!!\n");
		return err;
	}
	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err){
		printk("add isvp dai routes err!!\n");
		return err;
	}

	snd_soc_dapm_enable_pin(dapm, "Speaker");
	snd_soc_dapm_enable_pin(dapm, "Mic Buildin");

	snd_soc_dapm_sync(dapm);
	return err;
}

static struct snd_soc_dai_link isvp_dais[] = {
	[0] = {
		.name = "isvp ICDC",
		.stream_name = "isvp ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = isvp_dlv_dai_link_init,
		.codec_dai_name = "jz-codec-hifi",
		.codec_name = "jz-codec",
		.ops = &isvp_i2s_ops,
	},
#if 0
	[1] = {
		.name = "isvp PCMBT",
		.stream_name = "isvp PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
#endif
};

static struct snd_soc_card isvp = {
	.name = "isvp",
	.owner = THIS_MODULE,
	.dai_link = isvp_dais,
	.num_links = ARRAY_SIZE(isvp_dais),
};

static int snd_isvp_probe(struct platform_device *pdev)
{
	int ret = 0;

	if(pdev && pdev->dev.parent){
		device_unlock(pdev->dev.parent);
	}
	jz_asoc_aic_init();
	jz_i2s_init();
	jz_pcm_init();
	inno_s40_modinit();

	if(pdev && pdev->dev.parent){
		device_lock(pdev->dev.parent);
	}

	codec_platform_data.gpio_spk_en.gpio = spk_gpio;
	codec_platform_data.gpio_spk_en.active_level = spk_gpio_level;
	isvp.dev = &pdev->dev;
	ret = snd_soc_register_card(&isvp);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
	return ret;
}

static int snd_isvp_remove(struct platform_device *pdev)
{
	jz_asoc_aic_exit();
	jz_i2s_exit();
	jz_pcm_exit();

	inno_s40_exit();
	snd_soc_unregister_card(&isvp);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_isvp_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-alsa",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_isvp_probe,
	.remove = snd_isvp_remove,
};
module_platform_driver(snd_isvp_driver);

MODULE_AUTHOR("niky<xianghui.shen@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC isvp Snd Card");
MODULE_LICENSE("GPL");
