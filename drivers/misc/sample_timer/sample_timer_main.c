/*
 * samme_timer.c - jz_tcu mfd cell driver.
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/mfd/core.h>
#include <linux/mfd/jz_tcu.h>

#define TIMER_SRC TCU_CLKSRC_EXT
#define TIMER_SRC_FREQ (CONFIG_EXTAL_CLOCK * 1000000)
#define TIMER_DIV (1 << (2 * TCU_PRESCALE_64))
#define TIMER_FREQ (TIMER_SRC_FREQ / TIMER_DIV)
#define TIMER_MAX_PERIOD_MS (0xffff * 1000 / TIMER_FREQ)

/* If more than 2 devices, their mdev.name name cannot be the same. */
#define MISC_DEV_NAME "sample_timer"

struct sample_timer_data {
	struct device *dev;
	const struct mfd_cell *cell;
	struct miscdevice mdev;

	int irq;
	struct jz_tcu_chn *tcu_chn;
};

static irqreturn_t sample_timer_interrupt(int irq, void *devid)
{
	struct sample_timer_data *timer_data = devid;

	/* echo 8 > /proc/sys/kernel/printk to see this print. */
	dev_dbg(timer_data->dev, "t=%llu\n", sched_clock());

	return IRQ_HANDLED;
}

static ssize_t sample_timer_period_ms_store(struct device *dev,
				   struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct sample_timer_data *timer_data = dev_get_drvdata(dev);
	int period_ms;

	if (buf == NULL)
		return 0;

	sscanf(buf, "%d", &period_ms);
	if (period_ms > TIMER_MAX_PERIOD_MS) {
		dev_err(timer_data->dev, "Timer max period is %d\n", TIMER_MAX_PERIOD_MS);
		return -1;
	}

	jz_tcu_set_period(timer_data->tcu_chn, (u16)(period_ms * TIMER_FREQ / 1000));

	return count;
}

static ssize_t sample_timer_enable_store(struct device *dev,
				   struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct sample_timer_data *timer_data = dev_get_drvdata(dev);

	if (buf == NULL)
		return 0;

	if (strncmp(buf, "1", 1) == 0) {
		jz_tcu_enable_counter(timer_data->tcu_chn); /* Enable TCU Channel */
		enable_irq(timer_data->irq); /* Enable Channel IRQ */
		jz_tcu_set_count(timer_data->tcu_chn, 0); /* Clear Count */
		jz_tcu_start_counter(timer_data->tcu_chn); /* start Counter */
		dev_info(timer_data->dev, "timer enable\n");

	} else if (strncmp(buf, "0", 1) == 0) {
		jz_tcu_stop_counter(timer_data->tcu_chn); /* Stop Counter */
		disable_irq(timer_data->irq);/* Disable Channel IRQ */
		jz_tcu_disable_counter(timer_data->tcu_chn); /* Disable TCU Channel */
		dev_info(timer_data->dev, "timer disable\n");

	} else {
		dev_err(timer_data->dev, "Invalid option\n");
		return -1;
	}

	return count;
}

static ssize_t sample_timer_count_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct sample_timer_data *timer_data = dev_get_drvdata(dev);
	u16 cnt_now = jz_tcu_get_count(timer_data->tcu_chn);

	return sprintf(buf, "%u\n", cnt_now);
}

static DEVICE_ATTR(period_ms, S_IWUSR, NULL, sample_timer_period_ms_store);
static DEVICE_ATTR(enable, S_IWUSR, NULL, sample_timer_enable_store);
static DEVICE_ATTR(count, S_IRUSR, sample_timer_count_show, NULL);

static struct attribute *sample_timer_attributes[] = {
	&dev_attr_period_ms.attr,
	&dev_attr_enable.attr,
	&dev_attr_count.attr,
	NULL
};

static const struct attribute_group sample_timer_attr_group = {
	.attrs = sample_timer_attributes,
};

static int sample_timer_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct sample_timer_data *timer_data;

	timer_data = kzalloc(sizeof(struct sample_timer_data), GFP_KERNEL);
	if (!timer_data) {
		dev_err(&pdev->dev, "Failed to allocate driver structure\n");
		return -ENOMEM;
	}

	timer_data->cell = mfd_get_cell(pdev);
	if (!timer_data->cell) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get mfd cell\n");
		goto err_free;
	}

	timer_data->irq = platform_get_irq(pdev, 0);
	if (timer_data->irq < 0) {
		ret = timer_data->irq;
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n", ret);
		goto err_free;
	}

	ret = request_irq(timer_data->irq, sample_timer_interrupt, 0, pdev->name, timer_data);
	if (ret) {
		dev_err(timer_data->dev, "Failed to request irq %d\n", ret);
		goto err_free;
	}
	disable_irq(timer_data->irq);

	timer_data->tcu_chn = (struct jz_tcu_chn *)pdev->dev.platform_data;

	timer_data->dev = &pdev->dev;
	timer_data->mdev.minor = MISC_DYNAMIC_MINOR;
	timer_data->mdev.name = MISC_DEV_NAME;
	timer_data->mdev.fops = NULL;

	ret = misc_register(&timer_data->mdev);
	if (ret < 0) {
		dev_err(timer_data->dev, "misc_register failed\n");
		goto err_irq;
	}

	/* Timer configuration */
	timer_data->tcu_chn->irq_type = FULL_IRQ_MODE;
	timer_data->tcu_chn->clk_src = TCU_CLKSRC_EXT;
	timer_data->tcu_chn->tcu_mode = TCU_MODE_1;
	timer_data->tcu_chn->prescale = TCU_PRESCALE_64;

	jz_tcu_config_chn(timer_data->tcu_chn);

	platform_set_drvdata(pdev, timer_data);

	ret = sysfs_create_group(&pdev->dev.kobj, &sample_timer_attr_group);
	if (ret < 0)
		goto err_sysfs_create;

	return 0;

err_sysfs_create:
	misc_deregister(&timer_data->mdev);
err_irq:
	free_irq(timer_data->irq, timer_data);
err_free:
	kfree(timer_data);
	return ret;
}

static int sample_timer_remove(struct platform_device *pdev)
{
	struct sample_timer_data *timer_data = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &sample_timer_attr_group);
	misc_deregister(&timer_data->mdev);
	free_irq(timer_data->irq, timer_data);
	kfree(timer_data);

	return 0;
}

static struct platform_driver sample_timer_driver = {
	.probe	= sample_timer_probe,
	.remove	= sample_timer_remove,
	.driver = {
		.name	= "tcu_chn0", /* Match TCU Channel name */
		.owner	= THIS_MODULE,
	},
};

static int __init sample_timer_init(void)
{
	platform_driver_register(&sample_timer_driver);

	return 0;
}

static void __exit sample_timer_exit(void)
{
	platform_driver_unregister(&sample_timer_driver);
}

module_init(sample_timer_init);
module_exit(sample_timer_exit);

MODULE_LICENSE("GPL");
