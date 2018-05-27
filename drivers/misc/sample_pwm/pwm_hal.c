/*
 * pwm api code;
 *
 * Copyright (c) 2015 Ingenic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/mfd/jz_tcu.h>

#if defined(CONFIG_SOC_T30)
#define PWM_NUM		NR_TCU_CHNS
#else /* other soc type */
#define PWM_NUM		4
#endif
#define PWM_CONFIG	0x001
#define PWM_CONFIG_DUTY	0x002
#define PWM_ENABLE	0x010
#define PWM_DISABLE 0x100

struct platform_device pwm_device = {
	.name = "pwm-jz",
	.id   = -1,
};

struct pwm_lookup jz_pwm_lookup[] = {
#ifdef CONFIG_PWM0
	PWM_LOOKUP("tcu_chn0.0", 1, "pwm-jz", "pwm-jz.0"),
#endif
#ifdef CONFIG_PWM1
	PWM_LOOKUP("tcu_chn1.1", 1, "pwm-jz", "pwm-jz.1"),
#endif
#ifdef CONFIG_PWM2
	PWM_LOOKUP("tcu_chn2.2", 1, "pwm-jz", "pwm-jz.2"),
#endif
#ifdef CONFIG_PWM3
	PWM_LOOKUP("tcu_chn3.3", 1, "pwm-jz", "pwm-jz.3"),
#endif
#ifdef CONFIG_PWM4
	PWM_LOOKUP("tcu_chn4.4", 1, "pwm-jz", "pwm-jz.4"),
#endif
#ifdef CONFIG_PWM5
	PWM_LOOKUP("tcu_chn5.5", 1, "pwm-jz", "pwm-jz.5"),
#endif
#ifdef CONFIG_PWM6
	PWM_LOOKUP("tcu_chn6.6", 1, "pwm-jz", "pwm-jz.6"),
#endif
#ifdef CONFIG_PWM7
	PWM_LOOKUP("tcu_chn7.7", 1, "pwm-jz", "pwm-jz.7"),
#endif
};

struct pwm_ioctl_t {
	int index;
	int duty;
	int period;
	int polarity;
};

struct pwm_device_t {
	int duty;
	int period;
	int polarity;
	struct pwm_device *pwm_device;
};

struct pwm_jz_t {
	struct device *dev;
	struct miscdevice mdev;
	struct pwm_device_t *pwm_device_t[PWM_NUM];
};

static int pwm_jz_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int pwm_jz_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long pwm_jz_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int id, ret = 0;
	struct pwm_ioctl_t pwm_ioctl;
	struct miscdevice *dev = filp->private_data;
	struct pwm_jz_t *gpwm = container_of(dev, struct pwm_jz_t, mdev);

	switch(cmd) {
		case PWM_CONFIG:
			ret = copy_from_user(&pwm_ioctl, (void __user *)arg, sizeof(pwm_ioctl));
			if(ret){
				dev_err(gpwm->dev, "ioctl error(%d) !\n", __LINE__);
				ret = -1;
				break;
			}

			id = pwm_ioctl.index;
			if((id >= PWM_NUM) || (id < 0)) {
				dev_err(gpwm->dev, "ioctl error(%d) !\n", __LINE__);
				ret = -1;
				break;
			}

			if((pwm_ioctl.period > 1000000000) || (pwm_ioctl.period < 200)) {
				dev_err(gpwm->dev, "period error !\n");
				ret = -1;
				break;
			}

			if((pwm_ioctl.duty > pwm_ioctl.period) || (pwm_ioctl.duty < 0)) {
				dev_err(gpwm->dev, "duty error !\n");
				ret = -1;
				break;
			}

			if((pwm_ioctl.polarity > 1) || (pwm_ioctl.polarity < 0)) {
				dev_err(gpwm->dev, "polarity error !\n");
				ret = -1;
				break;
			}

			gpwm->pwm_device_t[id]->period = pwm_ioctl.period;
			gpwm->pwm_device_t[id]->duty = pwm_ioctl.duty;
			gpwm->pwm_device_t[id]->polarity = pwm_ioctl.polarity;

			break;
		case PWM_CONFIG_DUTY:
			ret = copy_from_user(&pwm_ioctl, (void __user *)arg, sizeof(pwm_ioctl));
			if(ret){
				dev_err(gpwm->dev, "ioctl error(line %d) !\n", __LINE__);
				ret = -1;
				break;
			}
			if((pwm_ioctl.duty > pwm_ioctl.period) || (pwm_ioctl.duty < 0)) {
				dev_err(gpwm->dev, "duty error(line %d) !\n",__LINE__);
				ret = -1;
				break;
			}
			id = pwm_ioctl.index;
			gpwm->pwm_device_t[id]->duty = pwm_ioctl.duty;
			pwm_config(gpwm->pwm_device_t[id]->pwm_device, gpwm->pwm_device_t[id]->duty, gpwm->pwm_device_t[id]->period);

			break;
		case PWM_ENABLE:
			id = (int)arg;

			if((id >= PWM_NUM) || (id < 0)) {
				dev_err(gpwm->dev, "ioctl error(%d) !\n", __LINE__);
				ret = -1;
				break;
			}

			if((gpwm->pwm_device_t[id]->pwm_device == NULL) || (IS_ERR(gpwm->pwm_device_t[id]->pwm_device))) {
				dev_err(gpwm->dev, "pwm could not work !\n");
				ret = -1;
				break;
			}

			if((gpwm->pwm_device_t[id]->period == -1) || (gpwm->pwm_device_t[id]->duty == -1) || (gpwm->pwm_device_t[id]->polarity == -1)) {
				dev_err(gpwm->dev, "the parameter of pwm could not init !\n");
				ret = -1;
				break;
			}

			if(gpwm->pwm_device_t[id]->polarity == 0)
				pwm_set_polarity(gpwm->pwm_device_t[id]->pwm_device, PWM_POLARITY_INVERSED);
			else
				pwm_set_polarity(gpwm->pwm_device_t[id]->pwm_device, PWM_POLARITY_NORMAL);

			pwm_enable(gpwm->pwm_device_t[id]->pwm_device);

			pwm_config(gpwm->pwm_device_t[id]->pwm_device, gpwm->pwm_device_t[id]->duty, gpwm->pwm_device_t[id]->period);
			break;
		case PWM_DISABLE:
			id = (int)arg;

			if((id >= PWM_NUM) || (id < 0)) {
				dev_err(gpwm->dev, "ioctl error(%d) !\n", __LINE__);
				ret = -1;
				break;
			}

			if((gpwm->pwm_device_t[id]->pwm_device == NULL) || (IS_ERR(gpwm->pwm_device_t[id]->pwm_device))) {
				dev_err(gpwm->dev, "pwm could not work !\n");
				ret = -1;
				break;
			}

			if((gpwm->pwm_device_t[id]->period == -1) || (gpwm->pwm_device_t[id]->duty == -1) || (gpwm->pwm_device_t[id]->polarity == -1)) {
				dev_err(gpwm->dev, "the parameter of pwm could not init !\n");
				ret = -1;
				break;
			}

			pwm_disable(gpwm->pwm_device_t[id]->pwm_device);

			break;
		default:
			dev_err(gpwm->dev, "unsupport cmd !\n");
			break;
	}

	return ret;
}

static struct file_operations pwm_jz_fops = {
	.owner		= THIS_MODULE,
	.open		= pwm_jz_open,
	.release	= pwm_jz_release,
	.unlocked_ioctl	= pwm_jz_ioctl,
};

static int jz_pwm_probe(struct platform_device *pdev)
{
	int i, ret;
	struct pwm_jz_t *gpwm;
	char pd_name[10];

	gpwm = devm_kzalloc(&pdev->dev, sizeof(struct pwm_jz_t), GFP_KERNEL);
	if(gpwm == NULL) {
		dev_err(&pdev->dev, "devm_kzalloc gpwm error !\n");
		return -ENOMEM;
	}

	for(i = 0; i < PWM_NUM; i++) {
		if (i == 0){
#ifndef CONFIG_PWM0
			continue;
#endif
		} else if (i == 1){
#ifndef CONFIG_PWM1
			continue;
#endif
		} else if (i == 2){
#ifndef CONFIG_PWM2
			continue;
#endif
		} else if (i == 3){
#ifndef CONFIG_PWM3
			continue;
#endif
		} else if (i == 4){
#ifndef CONFIG_PWM4
			continue;
#endif
		} else if (i == 5){
#ifndef CONFIG_PWM5
			continue;
#endif
		} else if (i == 6){
#ifndef CONFIG_PWM6
			continue;
#endif
		} else if (i == 7){
#ifndef CONFIG_PWM7
			continue;
#endif
		}

		gpwm->pwm_device_t[i] = devm_kzalloc(&pdev->dev, (sizeof(struct pwm_device_t)), GFP_KERNEL);
		if(gpwm->pwm_device_t[i] == NULL) {
			dev_err(&pdev->dev, "devm_kzalloc pwm_device_t error !\n");
			return -ENOMEM;
		}

		sprintf(pd_name, "pwm-jz.%d", i);
		gpwm->pwm_device_t[i]->pwm_device = devm_pwm_get(&pdev->dev, pd_name);
		if (IS_ERR(gpwm->pwm_device_t[i]->pwm_device)) {
			dev_err(&pdev->dev, "devm_pwm_get error !");
			return -ENOMEM;
		}

		gpwm->pwm_device_t[i]->duty = -1;
		gpwm->pwm_device_t[i]->period = -1;
		gpwm->pwm_device_t[i]->polarity = -1;
	}

	gpwm->mdev.minor = MISC_DYNAMIC_MINOR;
	gpwm->mdev.name = "pwm";
	gpwm->mdev.fops = &pwm_jz_fops;
	ret = misc_register(&gpwm->mdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "misc_register failed !\n");
		return ret;
	}

	gpwm->dev = &pdev->dev;

	platform_set_drvdata(pdev, gpwm);

	dev_info(&pdev->dev, "%s register ok !\n", __func__);

	return 0;
}

static int jz_pwm_remove(struct platform_device *pdev)
{
	struct pwm_jz_t *gpwm = platform_get_drvdata(pdev);
	int i = 0;
	if(gpwm == NULL)
		return 0;
	misc_deregister(&gpwm->mdev);

	for(i = 0; i < PWM_NUM; i++) {
		if(gpwm->pwm_device_t[i]->pwm_device){
			devm_pwm_put(&pdev->dev, gpwm->pwm_device_t[i]->pwm_device);
			devm_kfree(&pdev->dev, gpwm->pwm_device_t[i]);
		}
	}
	devm_kfree(&pdev->dev, gpwm);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver jz_pwm_driver = {
	.driver = {
		.name = "pwm-jz",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};


static int __init pwm_init(void)
{
	int jz_pwm_lookup_size = ARRAY_SIZE(jz_pwm_lookup);

	platform_device_register(&pwm_device);
	pwm_add_table(jz_pwm_lookup, jz_pwm_lookup_size);

	return platform_driver_register(&jz_pwm_driver);
}

static void __exit pwm_exit(void)
{

	platform_driver_unregister(&jz_pwm_driver);

}

module_init(pwm_init);
module_exit(pwm_exit);



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("JZ PWM Driver");
