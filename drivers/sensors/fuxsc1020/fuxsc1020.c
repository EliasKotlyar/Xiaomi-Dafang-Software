/*
 * fuxsc1020.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <sensor-common.h>
#include <apical-isp/apical_math.h>

#include <soc/gpio.h>

#define FUXSC1020_REG_END		0xff
#define FUXSC1020_REG_DELAY		0xfe

#define FUXSC1020_SUPPORT_PCLK (74*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 25
#define SENSOR_OUTPUT_MIN_FPS 5

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_LOW_10BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

struct tx_isp_sensor_attribute fuxsc1020_attr;

unsigned int fuxsc1020_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	return 0;
}

unsigned int fuxsc1020_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return 0;
}
struct tx_isp_sensor_attribute fuxsc1020_attr={
	.name = "fuxsc1020",
	.chip_id = 0xa042,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
	.cbus_device = 0x30,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 0xff << (TX_ISP_GAIN_FIXED_POINT - 4),
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 745,
	.integration_time_limit = 745,
	.total_width = 1980,
	.total_height = 750,
	.max_integration_time = 745,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 1,
	.dgain_apply_delay = 1,
	.sensor_ctrl.alloc_again = fuxsc1020_alloc_again,
	.sensor_ctrl.alloc_dgain = fuxsc1020_alloc_dgain,
	.one_line_expr_in_us = 44,
	//void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list fuxsc1020_init_regs_1280_720_25fps[] = {
	{FUXSC1020_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the fuxsc1020_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting fuxsc1020_win_sizes[] = {
	/* 1280*800 */
	{
		.width		= 1280,
		.height		= 720,
		.fps		= 25 << 16 | 1, /* 12.5 fps */
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_1X16,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= fuxsc1020_init_regs_1280_720_25fps,
	}
};

/*
 * the part of driver was fixed.
 */

static struct regval_list fuxsc1020_stream_on[] = {
	{FUXSC1020_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list fuxsc1020_stream_off[] = {
	{FUXSC1020_REG_END, 0x00},	/* END MARKER */
};

int fuxsc1020_read(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value)
{
	return 0;
}

int fuxsc1020_write(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	return 0;
}

static int fuxsc1020_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	return 0;
}

static int fuxsc1020_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int fuxsc1020_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	return 0;
}

static int fuxsc1020_set_integration_time(struct v4l2_subdev *sd, int value)
{
	return 0;
}
static int fuxsc1020_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}
static int fuxsc1020_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int fuxsc1020_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int fuxsc1020_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &fuxsc1020_win_sizes[0];

	if(!enable)
		return ISP_SUCCESS;

	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int fuxsc1020_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = fuxsc1020_write_array(sd, fuxsc1020_stream_on);
		printk("fuxsc1020 stream on\n");
	}
	else {
		ret = fuxsc1020_write_array(sd, fuxsc1020_stream_off);
		printk("fuxsc1020 stream off\n");
	}
	return ret;
}

static int fuxsc1020_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int fuxsc1020_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}


static int fuxsc1020_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	return 0;
}

static int fuxsc1020_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &fuxsc1020_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &fuxsc1020_win_sizes[0];
	}
	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		if(sensor->priv != wsize){
			ret = fuxsc1020_write_array(sd, wsize->regs);
			if(!ret)
				sensor->priv = wsize;
		}
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}
static int fuxsc1020_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"fuxsc1020_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			mdelay(10);
			gpio_direction_output(reset_gpio, 0);
			mdelay(10);
			gpio_direction_output(reset_gpio, 1);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if (pwdn_gpio != -1) {
		ret = gpio_request(pwdn_gpio, "fuxsc1020_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			mdelay(50);
			gpio_direction_output(pwdn_gpio, 0);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = fuxsc1020_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an fuxsc1020 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "fuxsc1020 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int fuxsc1020_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long fuxsc1020_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = fuxsc1020_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = fuxsc1020_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = fuxsc1020_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = fuxsc1020_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = fuxsc1020_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		//	ret = fuxsc1020_write_array(sd, fuxsc1020_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		//	ret = fuxsc1020_write_array(sd, fuxsc1020_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = fuxsc1020_set_fps(sensor, ctrl->value);
			break;
		default:
			printk("do not support ctrl->cmd ====%d\n",ctrl->cmd);
			break;;
	}
	return 0;
}
static long fuxsc1020_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = fuxsc1020_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int fuxsc1020_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = fuxsc1020_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int fuxsc1020_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	fuxsc1020_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops fuxsc1020_core_ops = {
	.g_chip_ident = fuxsc1020_g_chip_ident,
	.reset = fuxsc1020_reset,
	.init = fuxsc1020_init,
	.s_power = fuxsc1020_s_power,
	.ioctl = fuxsc1020_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = fuxsc1020_g_register,
	.s_register = fuxsc1020_s_register,
#endif
};

static const struct v4l2_subdev_video_ops fuxsc1020_video_ops = {
	.s_stream = fuxsc1020_s_stream,
	.s_parm = fuxsc1020_s_parm,
	.g_parm = fuxsc1020_g_parm,
};

static const struct v4l2_subdev_ops fuxsc1020_ops = {
	.core = &fuxsc1020_core_ops,
	.video = &fuxsc1020_video_ops,
};

static int fuxsc1020_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &fuxsc1020_win_sizes[0];
	int ret = -1;

	sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
	if(!sensor){
		printk("Failed to allocate sensor subdev.\n");
		return -ENOMEM;
	}
	memset(sensor, 0 ,sizeof(*sensor));
	/* request mclk of sensor */
	sensor->mclk = clk_get(NULL, "cgu_cim");
	if (IS_ERR(sensor->mclk)) {
		printk("Cannot get sensor input clock cgu_cim\n");
		goto err_get_mclk;
	}
	clk_set_rate(sensor->mclk, 37125000);
	clk_enable(sensor->mclk);

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	fuxsc1020_attr.dvp.gpio = sensor_gpio_func;
	 /*
		convert sensor-gain into isp-gain,
	 */
	fuxsc1020_attr.max_again = 	log2_fixed_to_fixed(fuxsc1020_attr.max_again, TX_ISP_GAIN_FIXED_POINT, LOG2_GAIN_SHIFT);
	fuxsc1020_attr.max_dgain = fuxsc1020_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &fuxsc1020_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &fuxsc1020_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int fuxsc1020_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct tx_isp_sensor *sensor = v4l2_get_subdev_hostdata(sd);

	if(reset_gpio != -1)
		gpio_free(reset_gpio);
	if(pwdn_gpio != -1)
		gpio_free(pwdn_gpio);

	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);

	v4l2_device_unregister_subdev(sd);
	kfree(sensor);
	return 0;
}

static const struct i2c_device_id fuxsc1020_id[] = {
	{ "fuxsc1020", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fuxsc1020_id);

static struct i2c_driver fuxsc1020_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "fuxsc1020",
	},
	.probe		= fuxsc1020_probe,
	.remove		= fuxsc1020_remove,
	.id_table	= fuxsc1020_id,
};

static __init int init_fuxsc1020(void)
{
	return i2c_add_driver(&fuxsc1020_driver);
}

static __exit void exit_fuxsc1020(void)
{
	i2c_del_driver(&fuxsc1020_driver);
}

module_init(init_fuxsc1020);
module_exit(exit_fuxsc1020);

MODULE_DESCRIPTION("A low-level driver for OmniVision fuxsc1020 sensors");
MODULE_LICENSE("GPL");
