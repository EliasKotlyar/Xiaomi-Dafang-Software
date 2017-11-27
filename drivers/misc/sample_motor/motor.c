/*
 * motor.c - Ingenic motor driver
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co.,Ltd
 *       http://www.ingenic.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/pwm.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/mfd/core.h>
#include <linux/mempolicy.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/mfd/jz_tcu.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <soc/irq.h>
#include <soc/base.h>
#include <soc/extal.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

#include <mach/platform.h>

#include "motor.h"

DEF_MOTOR(0);
DEF_MOTOR(1);

static unsigned char step_8[8] = {
	0x08,
	0x0c,
	0x04,
	0x06,
	0x02,
	0x03,
	0x01,
	0x09
};

static int motor_step(struct motor_info *info)
{
	int i = info->id;

	if (info->pdata[i]->motor_st1_gpio)
		gpio_direction_output(info->pdata[i]->motor_st1_gpio, step_8[info->run_step] & 0x8);
	if (info->pdata[i]->motor_st2_gpio)
		gpio_direction_output(info->pdata[i]->motor_st2_gpio, step_8[info->run_step] & 0x4);
	if (info->pdata[i]->motor_st3_gpio)
		gpio_direction_output(info->pdata[i]->motor_st3_gpio, step_8[info->run_step] & 0x2);
	if (info->pdata[i]->motor_st4_gpio)
		gpio_direction_output(info->pdata[i]->motor_st4_gpio, step_8[info->run_step] & 0x1);

	return 0;
}

static irqreturn_t jz_timer_interrupt(int irq, void *dev_id)
{
	struct motor_info *info = (struct motor_info *)dev_id;

	switch(info->direction) {
		case MOTOR_DIRECTIONAL_LEFT:
			if(info->cur_steps[info->direction] != info->set_steps[info->direction]) {
				info->run_step = info->cur_steps[info->direction] % sizeof(step_8);
				info->id = 0;
				motor_step(info);
				info->cur_steps[info->direction]++;
			}
			break;
		case MOTOR_DIRECTIONAL_RIGHT:
			if(info->cur_steps[info->direction] != info->set_steps[info->direction]) {
				info->run_step = (sizeof(step_8) - 1) - info->cur_steps[info->direction] % sizeof(step_8);
				info->id = 0;
				motor_step(info);
				info->cur_steps[info->direction]++;
			}
			break;
		case MOTOR_DIRECTIONAL_UP:
			if(info->cur_steps[info->direction] != info->set_steps[info->direction]) {
				info->run_step = info->cur_steps[info->direction] % sizeof(step_8);
				info->id = 1;
				motor_step(info);
				info->cur_steps[info->direction]++;
			}
			break;
		case MOTOR_DIRECTIONAL_DOWN:
			if(info->cur_steps[info->direction] != info->set_steps[info->direction]) {
				info->run_step = (sizeof(step_8) - 1) - info->cur_steps[info->direction] % sizeof(step_8);
				info->id = 1;
				motor_step(info);
				info->cur_steps[info->direction]++;
			}
			break;
		default:
			dev_err(info->dev, "unsupport cmd : %d\n", info->direction);
			break;
	}

	return IRQ_HANDLED;
}

static int motor_move(struct motor_info *info, int direction, int steps)
{
	info->direction = direction;
	info->cur_steps[direction] = 0;
	info->set_steps[direction] = steps;

	return 0;
}

static int motor_speed(struct motor_info *info, int speed)
{
	if ((speed < info->pdata[0]->min_speed) || (speed > info->pdata[0]->max_speed)) {
		dev_err(info->dev, "speed(%d) set error\n", speed);
		return -1;
	}

	info->speed = speed;
	jz_tcu_set_period(info->tcu, (24000000 / info->speed));

	return 0;
}

static int motor_attr_init(struct motor_info *info)
{
	/* TODO to be realized */
#if 0
	info->move_is_min = 0;
	info->move_is_max = 0;

	//motor_move(info, MOTOR_DIRECTIONAL_LEFT, 0, info->speed);
	//motor_move(info, MOTOR_DIRECTIONAL_RIGHT, 0, info->speed);
	info->current_steps[0] = info->total_steps[0] / 2;
	info->total_steps[0] = 4096;//motor_move(info, MOTOR_DIRECTIONAL_MAX, 0, info->speed);
#endif
	return 0;
}

static irqreturn_t motor_min_gpio_interrupt(int irq, void *dev_id)
{
	/* TODO to be realized */
#if 0
	int i;
	struct motor_info *info = (struct motor_info *)dev_id;

	for(i = 0; i < 2; i++) {
		if (irq == info->pdata[i]->motor_min_gpio) {
			break;
		}
	}

	info->move_is_min = 1;
	info->move_is_max = 0;
	info->status = MOTOR_MOVE_STOP;
	info->current_steps[i] = 0;
#endif
	return IRQ_HANDLED;
}

static irqreturn_t motor_max_gpio_interrupt(int irq, void *dev_id)
{
	/* TODO to be realized */
#if 0
	int i;
	struct motor_info *info = (struct motor_info *)dev_id;

	for(i = 0; i < 2; i++) {
		if (irq == info->pdata[i]->motor_max_gpio) {
			break;
		}
	}

	info->move_is_min = 0;
	info->move_is_max = 1;
	info->status = MOTOR_MOVE_STOP;
	info->current_steps[i] = info->total_steps[i];
#endif
	return IRQ_HANDLED;
}

static int motor_move_thread(void *data)
{
	struct motor_info *info = (struct motor_info *)data;

	motor_attr_init(info);

	down(&info->semaphore);

	return 0;
}

static int motor_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int motor_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long motor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *dev = filp->private_data;
	struct motor_info *info = container_of(dev, struct motor_info, mdev);

	switch (cmd) {
		case MOTOR_STOP:
			jz_tcu_disable_counter(info->tcu);
			info->status = MOTOR_MOVE_STOP;
			break;
		case MOTOR_RESET:
			motor_attr_init(info);
			break;
		case MOTOR_MOVE:
			jz_tcu_enable_counter(info->tcu);
			{
				struct motor_move_st move;
				if (copy_from_user(&move, (void __user *)arg,
							sizeof(struct motor_move_st))) {
					dev_err(info->dev, "[%s][%d] copy from user error\n",
							__func__, __LINE__);
					return -EFAULT;
				}
				motor_move(info, move.motor_directional, move.motor_move_steps);
			}
			break;
		case MOTOR_GET_STATUS:
			{
				struct motor_status_st status;
				status.move_is_min = info->move_is_min;
				status.move_is_max = info->move_is_max;
				status.directional_attr = info->pdata[0]->directional_attr;
				status.min_speed = info->pdata[0]->min_speed;
				status.max_speed = info->pdata[0]->max_speed;
				status.cur_speed = info->speed;

				if (copy_to_user((void __user *)arg, &status,
							sizeof(struct motor_status_st))) {
					dev_err(info->dev, "[%s][%d] copy to user error\n",
							__func__, __LINE__);
					return -EFAULT;
				}
			}
			break;
		case MOTOR_SPEED:
			{
				int speed;

				if (copy_from_user(&speed, (void __user *)arg, sizeof(int))) {
					dev_err(info->dev, "[%s][%d] copy to user error\n", __func__, __LINE__);
					return -EFAULT;
				}

				motor_speed(info, speed);
			}
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static struct file_operations motor_fops = {
	.open = motor_open,
	.release = motor_release,
	.unlocked_ioctl = motor_ioctl,
};

static int motor_probe(struct platform_device *pdev)
{
	int i, j, ret = 0;
	struct motor_info *info;

	info = devm_kzalloc(&pdev->dev, sizeof(struct motor_info), GFP_KERNEL);
	if (!info) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "kzalloc motor_info memery error\n");
		goto error_devm_kzalloc;
	}

	info->cell = mfd_get_cell(pdev);
	if (!info->cell) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get mfd cell for jz_adc_aux!\n");
		goto error_devm_kzalloc;
	}

	info->pdata[0] = &jz_motor0_pdata;
	info->pdata[1] = &jz_motor1_pdata;

	info->speed = MOTOR_INIT_SPEED;

	info->dev = &pdev->dev;

	info->tcu = (struct jz_tcu_chn *)info->cell->platform_data;

	info->tcu->irq_type = FULL_IRQ_MODE;
	info->tcu->clk_src = TCU_CLKSRC_EXT;

	jz_tcu_config_chn(info->tcu);

	jz_tcu_set_period(info->tcu, (24000000 / info->speed));

	jz_tcu_start_counter(info->tcu);

	init_completion(&info->time_completion);

	sema_init(&info->semaphore, 0);

	platform_set_drvdata(pdev, info);

	for(i = 0; i < 2; i++) {
		if (info->pdata[i]->motor_min_gpio != -1) {
			gpio_request(info->pdata[i]->motor_min_gpio, "motor_min_gpio");
			ret = request_irq(gpio_to_irq(info->pdata[i]->motor_min_gpio),
					motor_min_gpio_interrupt,
					(info->pdata[i]->motor_gpio_level ?
					 IRQF_TRIGGER_RISING :
					 IRQF_TRIGGER_FALLING)
					| IRQF_DISABLED ,
					"motor_min_gpio", info);
			if (ret) {
				dev_err(&pdev->dev, "request motor_min_gpio error\n");
				goto error_min_gpio;
			}
		}
		if (info->pdata[i]->motor_max_gpio != -1) {
			gpio_request(info->pdata[i]->motor_max_gpio, "motor_max_gpio");
			ret = request_irq(gpio_to_irq(info->pdata[i]->motor_max_gpio),
					motor_max_gpio_interrupt,
					(info->pdata[i]->motor_gpio_level ?
					 IRQF_TRIGGER_RISING :
					 IRQF_TRIGGER_FALLING)
					| IRQF_DISABLED ,
					"motor_max_gpio", info);
			if (ret) {
				dev_err(&pdev->dev, "request motor_max_gpio error\n");
				goto error_max_gpio;
			}
		}

		if (info->pdata[i]->motor_st1_gpio != -1) {
			gpio_request(info->pdata[i]->motor_st1_gpio, "motor_st1_gpio");
		}
		if (info->pdata[i]->motor_st2_gpio != -1) {
			gpio_request(info->pdata[i]->motor_st2_gpio, "motor_st2_gpio");
		}
		if (info->pdata[i]->motor_st3_gpio != -1) {
			gpio_request(info->pdata[i]->motor_st3_gpio, "motor_st3_gpio");
		}
		if (info->pdata[i]->motor_st4_gpio != -1) {
			gpio_request(info->pdata[i]->motor_st4_gpio, "motor_st4_gpio");
		}
	}

	info->motor_thread = kthread_run(motor_move_thread, info, "motor_thread");
	if (info->motor_thread == ERR_PTR(-ENOMEM)) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to run motor thread!\n");
		goto error_kthread_run;
	}

	info->run_step_irq = platform_get_irq(pdev,0);
	if (info->run_step_irq < 0) {
		ret = info->run_step_irq;
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n", ret);
		goto error_kthread_run;
	}

	ret = request_irq(info->run_step_irq, jz_timer_interrupt, 0,
				"jz_timer_interrupt", info);
	if (ret) {
		dev_err(&pdev->dev, "Failed to run request_irq() !\n");
		goto error_request_irq;
	}

	info->mdev.minor = MISC_DYNAMIC_MINOR;
	info->mdev.name = "motor";
	info->mdev.fops = &motor_fops;
	ret = misc_register(&info->mdev);
	if (ret < 0) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "misc_register failed\n");
		goto error_misc_register;
	}

	return 0;

error_misc_register:
	free_irq(info->run_step_irq, info);
error_request_irq:
	up(&info->semaphore);
	kthread_stop(info->motor_thread);
error_kthread_run:
	i--;
	if (info->pdata[i]->motor_st1_gpio != -1)
		gpio_free(info->pdata[i]->motor_st1_gpio);

	if (info->pdata[i]->motor_st2_gpio != -1)
		gpio_free(info->pdata[i]->motor_st2_gpio);

	if (info->pdata[i]->motor_st3_gpio != -1)
		gpio_free(info->pdata[i]->motor_st3_gpio);

	if (info->pdata[i]->motor_st4_gpio != -1)
		gpio_free(info->pdata[i]->motor_st4_gpio);
error_max_gpio:
	if (info->pdata[i]->motor_max_gpio != -1) {
		gpio_free(info->pdata[i]->motor_max_gpio);
		free_irq(info->pdata[i]->motor_max_gpio, info);
	}
error_min_gpio:
	if (info->pdata[i]->motor_min_gpio != -1) {
		gpio_free(info->pdata[i]->motor_min_gpio);
		free_irq(info->pdata[i]->motor_min_gpio, info);
	}

	for(j = 0; j < i; i++) {
		if (info->pdata[j]->motor_min_gpio != -1) {
			gpio_free(info->pdata[j]->motor_min_gpio);
			free_irq(info->pdata[j]->motor_min_gpio, info);
		}

		if (info->pdata[j]->motor_max_gpio != -1) {
			gpio_free(info->pdata[j]->motor_max_gpio);
			free_irq(info->pdata[j]->motor_max_gpio, info);
		}

		if (info->pdata[j]->motor_st1_gpio != -1)
			gpio_free(info->pdata[j]->motor_st1_gpio);

		if (info->pdata[j]->motor_st2_gpio != -1)
			gpio_free(info->pdata[j]->motor_st2_gpio);

		if (info->pdata[j]->motor_st3_gpio != -1)
			gpio_free(info->pdata[j]->motor_st3_gpio);

		if (info->pdata[j]->motor_st4_gpio != -1)
			gpio_free(info->pdata[j]->motor_st4_gpio);
	}
error_devm_kzalloc:
	return ret;
}

static int motor_remove(struct platform_device *pdev)
{
	int i;
	struct motor_info *info = platform_get_drvdata(pdev);

	misc_deregister(&info->mdev);

	free_irq(info->run_step_irq, info);

	up(&info->semaphore);

	kthread_stop(info->motor_thread);

	for(i = 0; i < 2; i++) {
		if (info->pdata[i]->motor_min_gpio != -1) {
			gpio_free(info->pdata[i]->motor_min_gpio);
			free_irq(info->pdata[i]->motor_min_gpio, info);
		}

		if (info->pdata[i]->motor_max_gpio != -1) {
			gpio_free(info->pdata[i]->motor_max_gpio);
			free_irq(info->pdata[i]->motor_max_gpio, info);
		}

		if (info->pdata[i]->motor_st1_gpio != -1)
			gpio_free(info->pdata[i]->motor_st1_gpio);

		if (info->pdata[i]->motor_st2_gpio != -1)
			gpio_free(info->pdata[i]->motor_st2_gpio);

		if (info->pdata[i]->motor_st3_gpio != -1)
			gpio_free(info->pdata[i]->motor_st3_gpio);

		if (info->pdata[i]->motor_st4_gpio != -1)
			gpio_free(info->pdata[i]->motor_st4_gpio);
	}

	return 0;
}

static struct platform_driver motor_driver = {
	.probe = motor_probe,
	.remove = motor_remove,
	.driver = {
		.name	= "tcu_chn2",
		.owner	= THIS_MODULE,
	}
};

static int __init motor_init(void)
{
	return platform_driver_register(&motor_driver);
}

static void __exit motor_exit(void)
{
	platform_driver_unregister(&motor_driver);
}

module_init(motor_init);
module_exit(motor_exit);

MODULE_LICENSE("GPL");

