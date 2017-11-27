/*
 *  asoc-aic.c
 *  ALSA Soc Audio Layer -- ingenic aic device driver
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
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include "asoc-aic.h"

static const char *aic_no_mode = "no mode";
static const char *aic_i2s_mode = "i2s mode";
static const char *aic_ac97_mode = "ac97 mode";

const char* aic_work_mode_str(enum aic_mode mode)
{
	switch (mode) {
	default:
	case AIC_NO_MODE:
		return aic_no_mode;
	case AIC_I2S_MODE:
		return aic_i2s_mode;
	case AIC_AC97_MODE:
		return aic_ac97_mode;
	}
}
EXPORT_SYMBOL_GPL(aic_work_mode_str);

enum aic_mode aic_set_work_mode(struct device *aic,
		enum aic_mode module_mode, bool enable)
{
	struct jz_aic *jz_aic = dev_get_drvdata(aic);
	enum aic_mode working_mode;

	spin_lock(&jz_aic->mode_lock);
	if  (module_mode != AIC_AC97_MODE &&
			module_mode != AIC_I2S_MODE)
		goto out;

	if (enable && jz_aic->aic_working_mode == AIC_NO_MODE) {
		jz_aic->aic_working_mode = module_mode;
	} else if (!enable && jz_aic->aic_working_mode == module_mode) {
		jz_aic->aic_working_mode = AIC_NO_MODE;
	}
out:
	working_mode = jz_aic->aic_working_mode;
	spin_unlock(&jz_aic->mode_lock);
	return working_mode;
}
EXPORT_SYMBOL_GPL(aic_set_work_mode);

int aic_set_rate(struct device *aic, unsigned long freq)
{
	struct jz_aic *jz_aic = dev_get_drvdata(aic);
	int ret;
	if (jz_aic->clk_rate != freq) {
		ret = clk_set_rate(jz_aic->clk, freq);
		if (!ret)
			jz_aic->clk_rate = clk_get_rate(jz_aic->clk);
	}
	return jz_aic->clk_rate;
}
EXPORT_SYMBOL_GPL(aic_set_rate);

static int jzaic_add_subdevs(struct jz_aic *jz_aic)
{
	int ret = 0;
	jz_aic->subdev_pdata.dma_base = (dma_addr_t)jz_aic->res_start;

#define AIC_REGISTER_SUBDEV(name) \
do { \
	jz_aic->psubdev_##name = platform_device_register_data(jz_aic->dev, \
			"jz-asoc-aic-"#name,	\
			PLATFORM_DEVID_NONE,	\
			(void *)&jz_aic->subdev_pdata,	\
			sizeof(struct jz_aic_subdev_pdata));	\
	if (IS_ERR(jz_aic->psubdev_##name)) {	\
		ret = PTR_ERR(jz_aic->psubdev_##name);	\
		dev_err(jz_aic->dev, "add %s device errno %ld\n", "jz-asoc-aic-"#name,	\
				PTR_ERR(jz_aic->psubdev_##name));	\
		jz_aic->psubdev_##name	= NULL;\
	}	\
} while (0)

	AIC_REGISTER_SUBDEV(i2s);
	/*AIC_REGISTER_SUBDEV(spdif);*/
	/*AIC_REGISTER_SUBDEV(ac97);*/
	return ret;
}

static void jzaic_del_subdevs(struct jz_aic *jz_aic)
{
	/*platform_device_unregister(jz_aic->psubdev_ac97);*/
	jz_aic->psubdev_ac97 = NULL;
	/*platform_device_unregister(jz_aic->psubdev_spdif);*/
	jz_aic->psubdev_spdif = NULL;
	platform_device_unregister(jz_aic->psubdev_i2s);
	jz_aic->psubdev_i2s = NULL;
	return;
}

static irqreturn_t jz_aic_irq_thread(int irq, void *dev_id)
{
	struct jz_aic *jz_aic = (struct jz_aic *)dev_id;

	if ((jz_aic->mask & 0x8) && __aic_test_ror(jz_aic->dev)) {
		jz_aic->ror++;
		dev_printk(KERN_DEBUG, jz_aic->dev,
				"recieve fifo [overrun] interrupt time [%d]\n",
				jz_aic->ror);
	}

	if ((jz_aic->mask & 0x4) && __aic_test_tur(jz_aic->dev)) {
		jz_aic->tur++;
		dev_printk(KERN_DEBUG, jz_aic->dev,
				"transmit fifo [underrun] interrupt time [%d]\n",
				jz_aic->tur);
	}

	if ((jz_aic->mask & 0x2) && __aic_test_rfs(jz_aic->dev)) {
		dev_printk(KERN_DEBUG, jz_aic->dev,
				"[recieve] fifo at or above threshold interrupt time\n");
	}

	if ((jz_aic->mask & 0x1) && __aic_test_tfs(jz_aic->dev)) {
		dev_printk(KERN_DEBUG, jz_aic->dev,
				"[transmit] fifo at or blow threshold interrupt time\n");
	}

	/*sleep, avoid frequently interrupt*/
	msleep(200);
	__aic_clear_all_irq_flag(jz_aic->dev);
	__aic_set_irq_enmask(jz_aic->dev, jz_aic->mask);
	return IRQ_HANDLED;
}

static irqreturn_t jz_aic_irq_handler(int irq, void *dev_id)
{
	struct jz_aic *jz_aic = (struct jz_aic *)dev_id;

	jz_aic->mask = __aic_get_irq_enmask(jz_aic->dev);
	if (jz_aic->mask && (jz_aic->mask & __aic_get_irq_flag(jz_aic->dev))) {
		/*Disable all aic interrupt*/
		__aic_set_irq_enmask(jz_aic->dev, 0);
		return IRQ_WAKE_THREAD;
	}
	return IRQ_NONE;
}

#define CODEC_DEF_RATE (8000*256)

static int jz_aic_probe(struct platform_device *pdev)
{
	struct jz_aic *jz_aic;
	struct resource *res = NULL;
	int ret;

	jz_aic = devm_kzalloc(&pdev->dev, sizeof(struct jz_aic), GFP_KERNEL);
	if (!jz_aic)
		return -ENOMEM;
	jz_aic->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOENT;
	if (!devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res),
				pdev->name))
		return -EBUSY;
	jz_aic->res_start = res->start;
	jz_aic->res_size = resource_size(res);
	jz_aic->vaddr_base = devm_ioremap_nocache(&pdev->dev,
			jz_aic->res_start, jz_aic->res_size);
	if (!jz_aic->vaddr_base)
		return -ENOMEM;

	jz_aic->clk_gate = devm_clk_get(&pdev->dev, "aic");
	if (IS_ERR_OR_NULL(jz_aic->clk_gate)) {
		ret = PTR_ERR(jz_aic->clk_gate);
		dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
		return ret;
	}
	jz_aic->clk = devm_clk_get(&pdev->dev, "cgu_i2s");
	if (IS_ERR_OR_NULL(jz_aic->clk)) {
		ret = PTR_ERR(jz_aic->clk);
		dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
		return ret;
	}
	clk_set_rate(jz_aic->clk, CODEC_DEF_RATE);
	jz_aic->clk_rate = CODEC_DEF_RATE;

	clk_enable(jz_aic->clk_gate);
	clk_enable(jz_aic->clk);
	spin_lock_init(&jz_aic->mode_lock);
	jz_aic->irqno = -1;
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res) {
		jz_aic->irqno = res->start;
		jz_aic->irqflags = res->flags & IORESOURCE_IRQ_SHAREABLE ? IRQF_SHARED : 0;
		ret = devm_request_threaded_irq(&pdev->dev, jz_aic->irqno,
				jz_aic_irq_handler, jz_aic_irq_thread,
				jz_aic->irqflags, pdev->name, (void *)jz_aic);
		if (ret)
			jz_aic->irqno = -1;
	} else {
		dev_err(&pdev->dev, "Failed to get platform irq resource\n");
	}

	platform_set_drvdata(pdev, (void *)jz_aic);
	ret = jzaic_add_subdevs(jz_aic);
	if (ret) {
		platform_set_drvdata(pdev, NULL);
		return ret;
	}
	dev_info(&pdev->dev, "Aic core probe success\n");
	return 0;
}

static int jz_aic_remove(struct platform_device *pdev)
{
	struct jz_aic * jz_aic = platform_get_drvdata(pdev);
	if (!jz_aic) return 0;
	jzaic_del_subdevs(jz_aic);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM
int jz_aic_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct jz_aic * jz_aic = platform_get_drvdata(pdev);
	clk_disable(jz_aic->clk);
	clk_disable(jz_aic->clk_gate);
	return 0;
}

int jz_aic_resume(struct platform_device *pdev)
{
	struct jz_aic * jz_aic = platform_get_drvdata(pdev);
	clk_enable(jz_aic->clk_gate);
	clk_enable(jz_aic->clk);
	return 0;
}
#endif

struct platform_driver jz_asoc_aic_driver = {
	.probe  = jz_aic_probe,
	.remove = jz_aic_remove,
#ifdef CONFIG_PM
	.suspend = jz_aic_suspend,
	.resume = jz_aic_resume,
#endif
	.driver = {
		.name   = "jz-asoc-aic",
		.owner  = THIS_MODULE,
	},
};

int jz_asoc_aic_init(void)
{
	return platform_driver_register(&jz_asoc_aic_driver);
}

void jz_asoc_aic_exit(void)
{
	platform_driver_unregister(&jz_asoc_aic_driver);
}
