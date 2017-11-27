/*
 * imx225.c
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

#define IMX225_CHIP_ID_H	(0x10)
#define IMX225_CHIP_ID_L	(0x01)

#define IMX225_REG_END		0xffff
#define IMX225_REG_DELAY	0xfffe

#define IMX225_SUPPORT_PCLK (37125*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_12BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */

unsigned int imx225_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	unsigned int gain = (isp_gain * 20) >> LOG2_GAIN_SHIFT;
	unsigned int remainder = (isp_gain * 20) % (1<<LOG2_GAIN_SHIFT);
	unsigned int threshold = 1 << LOG2_GAIN_SHIFT;
	if(gain * 3 > 0x2d0)
		*sensor_again = 0x2d0;
	else{
		if(remainder < (threshold / 4)){
			remainder = 0;
		}else if(remainder < (threshold / 2)){
			remainder = 1;
		}else if(remainder < (threshold * 5 / 2)){
			remainder = 2;
		}else
			remainder = 3;
		*sensor_again = gain * 3 + remainder;
	}
//	printk("isp_gain = 0x%08x  0x%08x again = 0x%08x\n",isp_gain, gain, *sensor_again);
	return isp_gain;
}

unsigned int imx225_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	*sensor_dgain = 0;
	return 0;
}

struct tx_isp_sensor_attribute imx225_attr = {
	.name = "imx225",
	.chip_id = 0x1001,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
	.cbus_device = 0x1a,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_SONY_MODE,
		.blanking = {
			.vblanking = 17,
			.hblanking = 15,
		},
	},
	.max_again = 786420,
//	.max_again = ((0x2d0 / 3) * (1 << LOG2_GAIN_SHIFT))/20,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 0x528-3,
	.integration_time_limit = 0x528-4,
	.total_width = 0x519,
	.total_height = 0x3d1,
	.max_integration_time = 0x44c-4,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = imx225_alloc_again,
	.sensor_ctrl.alloc_dgain = imx225_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};


static struct regval_list imx225_init_regs_1280_960_25fps[] = {
#if 0
	/* 720P is ok */
	{0x3002,0x01}, //add
	{0x3005,0x01}, //add
	{0x3007,0x10},
//	{0x3009,0x02},
	{0x3009,0x01}, // modify
	{0x320c,0xcf},
	{0x300f,0x00},
	{0x310f,0x0f},
	{0x3110,0x0e},
	{0x3111,0xe7},
	{0x3012,0x2c},
	{0x3112,0x9c},
	{0x3013,0x01},
	{0x3113,0x83},
	{0x3014,0x00}, // gain value [7:0]
	{0x3015,0x00}, // gain value [9:8]
	{0x3114,0x10},
	{0x3115,0x42},
	{0x3016,0x09},
	{0x3018,0x28}, // VTS value [7:0]
	{0x3019,0x05}, // VTS value [15:8]
	{0x301a,0x00}, // VTS value [16]
	{0x301b,0x94}, // HTS value [7:0]
	{0x301c,0x11}, // HTS value [13:8]
	{0x301d,0xc2},
	{0x3020,0x00}, // SHS1 value; Integration time = 1 frame period - (SHS1 + 1) * (1H period)
	{0x3021,0x00},
	{0x3022,0x00},
	{0x3128,0x1e},
	{0x3049,0x0a}, // Vsync Hsync output
	{0x324c,0x40},
	{0x324d,0x03},
	{0x305d,0x00},
	{0x305f,0x00},
	{0x3261,0xe0},
	{0x3262,0x02},
	{0x326e,0x2f},
	{0x326f,0x30},
	{0x3070,0x02},
	{0x3270,0x03},
	{0x3071,0x01},
	{0x3298,0x00},
	{0x329a,0x12},
	{0x329b,0xf1},
	{0x329c,0x0c},
	{0x309e,0x22},
	{0x30a5,0xfb},
	{0x30a6,0x02},
	{0x30b3,0xff},
	{0x30b4,0x01},
	{0x30b5,0x42},
	{0x30b8,0x10},
	{0x30c2,0x01},
	{0x31ed,0x38},
	{0x3054,0x67}, //add
	{0x3000,0x30}, //STANDBY = 0
#else
	/* 960p ok */
	{0x3002,0x01}, //add
	{0x3005,0x01}, //add
	{0x3006,0x00},
	{0x3007,0x00},
	{0x3009,0x01}, // modify
	{0x320c,0xcf},
	{0x300f,0x00},
	{0x310f,0x0f},
	{0x3110,0x0e},
	{0x3111,0xe7},
	{0x3012,0x2c},
	{0x3112,0x9c},
	{0x3013,0x01},
	{0x3113,0x83},
	{0x3014,0x00}, // gain value [7:0]
	{0x3015,0x00}, // gain value [9:8]
	{0x3114,0x10},
	{0x3115,0x42},
	{0x3016,0x09},
	{0x3018,0x28}, // VTS value [7:0]
	{0x3019,0x05}, // VTS value [15:8]
	{0x301a,0x00}, // VTS value [16]
	{0x301b,0x94}, // HTS value [7:0]
	{0x301c,0x11}, // HTS value [13:8]
	{0x301d,0xc2},
	{0x3020,0x00}, // SHS1 value; Integration time = 1 frame period - (SHS1 + 1) * (1H period)
	{0x3021,0x00},
	{0x3022,0x00},
	{0x3128,0x1e},
	{0x3049,0x0a}, // Vsync Hsync output
	{0x324c,0x40},
	{0x324d,0x03},
	{0x305c,0x20}, // input clk;When the clk is 37.125M or 74.25M, it is 0x20; then 27M or 54M, it's 0x2c
	{0x305d,0x00}, // input clk;When the clk is 54M or 74.25M, it is 0x10; then 27M or 37.125M, it's 0x00
	{0x305e,0x20}, // input clk;When the clk is 37.125M or 74.25M, it is 0x20; then 27M or 54M, it's 0x2c
	{0x305f,0x00}, // input clk;When the clk is 54M or 74.25M, it is 0x10; then 27M or 37.125M, it's 0x00
	{0x3261,0xe0},
	{0x3262,0x02},
	{0x326e,0x2f},
	{0x326f,0x30},
	{0x3070,0x02},
	{0x3270,0x03},
	{0x3071,0x01},
	{0x3298,0x00},
	{0x329a,0x12},
	{0x329b,0xf1},
	{0x329c,0x0c},
	{0x309e,0x22},
	{0x30a5,0xfb},
	{0x30a6,0x02},
	{0x30b3,0xff},
	{0x30b4,0x01},
	{0x30b5,0x42},
	{0x30b8,0x10},
	{0x30c2,0x01},
	{0x31ed,0x38},
	{0x3054,0x67}, //add
	{0x3000,0x30}, //STANDBY = 0
#endif
	{IMX225_REG_DELAY, 0x14},	/* END MARKER */
	{IMX225_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the imx225_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting imx225_win_sizes[] = {
	/* 1280*960 */
	{
		.width		= 1280,
		.height		= 960,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG12_1X12,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= imx225_init_regs_1280_960_25fps,
	}
};

static enum v4l2_mbus_pixelcode imx225_mbus_code[] = {
	V4L2_MBUS_FMT_SGRBG10_1X10,
	V4L2_MBUS_FMT_SGRBG12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list imx225_stream_on[] = {
	{0x3002,0x00},
	{IMX225_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list imx225_stream_off[] = {
	{0x3000,0x01},
	{0x3002,0x01},
	{IMX225_REG_END, 0x00},	/* END MARKER */
};

int imx225_read(struct v4l2_subdev *sd, uint16_t reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	uint8_t buf[2] = {(reg>>8)&0xff, reg&0xff};
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= buf,
		},
		[1] = {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= value,
		}
	};
	int ret;
	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}

int imx225_write(struct v4l2_subdev *sd, uint16_t reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	uint8_t buf[3] = {(reg>>8)&0xff, reg&0xff, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 3,
		.buf	= buf,
	};
	int ret;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int imx225_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != IMX225_REG_END) {
		if (vals->reg_num == IMX225_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = imx225_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			printk("{0x%0x, 0x%02x}\n",vals->reg_num, val);
		}
		vals++;
	}
	return 0;
}
static int imx225_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != IMX225_REG_END) {
		if (vals->reg_num == IMX225_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = imx225_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int imx225_reset(struct v4l2_subdev *sd, u32 val)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int imx225_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = imx225_read(sd, 0x3004, &v);
	printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != IMX225_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = imx225_read(sd, 0x300e, &v);
	printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != IMX225_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int imx225_set_integration_time(struct v4l2_subdev *sd, int int_time)
{
	int ret = 0;
	char value = 0;
	unsigned short shs = 0;
	unsigned short vmax = 0;

	ret = imx225_read(sd, 0x3018, &value);
	vmax = value;
	ret = imx225_read(sd, 0x3019, &value);
	vmax |= value << 8;
	shs = vmax - int_time - 1;

	ret = imx225_write(sd, 0x3020, (unsigned char)(shs & 0xff));
	if (ret < 0)
		return ret;
	ret = imx225_write(sd, 0x3021, (unsigned char)((shs >> 8) & 0xff));
	if (ret < 0)
		return ret;
	return 0;

}

static int imx225_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	/* printk("sensor again write value 0x%x\n",value); */
	ret = imx225_write(sd, 0x3014, (unsigned char)(value & 0xff));
	if (ret < 0)
		return ret;
	ret = imx225_write(sd, 0x3015, (unsigned char)((value >> 8) & 0xff));
	if (ret < 0)
		return ret;
	return 0;
}

static int imx225_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int imx225_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int imx225_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &imx225_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = imx225_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int imx225_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	if (enable) {
		ret = imx225_write_array(sd, imx225_stream_on);
		printk("imx225 stream on\n");
	}
	else {
		ret = imx225_write_array(sd, imx225_stream_off);
		printk("imx225 stream off\n");
	}
//	imx225_read_array(sd, imx225_init_regs_1280_960_25fps);
	return ret;
}

static int imx225_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int imx225_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int imx225_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	return 0;
#if 0
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	int ret = 0;
	unsigned int pclk = 0;
	unsigned int hts = 0;
	unsigned int vts = 0;
	unsigned char val = 0;
	unsigned int newformat = 0; //the format is 24.8

	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}
	pclk = IMX225_SUPPORT_PCLK;

	val = 0;
	ret += imx225_read(sd, 0x380c, &val);
	hts = val<<8;
	val = 0;
	ret += imx225_read(sd, 0x380d, &val);
	hts |= val;
	if (0 != ret) {
		printk("err: imx225 read err\n");
		return ret;
	}
	vts = (pclk << 4) / (hts * (newformat >> 4));
	ret += imx225_write(sd, 0x380f, vts&0xff);
	ret += imx225_write(sd, 0x380e, (vts>>8)&0xff);
	if (0 != ret) {
		printk("err: imx225_write err\n");
		return ret;
	}
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 4;
	sensor->video.attr->integration_time_limit = vts - 4;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 4;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return ret;
#endif
}

static int imx225_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &imx225_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &imx225_win_sizes[0];
	}

	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}
static int imx225_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"imx225_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 0);
			mdelay(5);
			gpio_direction_output(reset_gpio, 1);
			mdelay(5);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"imx225_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			mdelay(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = imx225_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an imx225 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "imx225 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int imx225_s_power(struct v4l2_subdev *sd, int on)
{
	printk("--%s:%d\n", __func__, __LINE__);
	return 0;
}
static long imx225_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = imx225_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = imx225_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = imx225_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = imx225_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = imx225_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			ret = imx225_write_array(sd, imx225_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			ret = imx225_write_array(sd, imx225_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = imx225_set_fps(sensor, ctrl->value);
			break;
		default:
			printk("do not support ctrl->cmd ====%d\n",ctrl->cmd);
			break;;
	}
	return 0;
}
static long imx225_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = imx225_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int imx225_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = imx225_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 1;
	return ret;
}

static int imx225_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	imx225_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops imx225_core_ops = {
	.g_chip_ident = imx225_g_chip_ident,
	.reset = imx225_reset,
	.init = imx225_init,
	.s_power = imx225_s_power,
	.ioctl = imx225_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = imx225_g_register,
	.s_register = imx225_s_register,
#endif
};

static const struct v4l2_subdev_video_ops imx225_video_ops = {
	.s_stream = imx225_s_stream,
	.s_parm = imx225_s_parm,
	.g_parm = imx225_g_parm,
};

static const struct v4l2_subdev_ops imx225_ops = {
	.core = &imx225_core_ops,
	.video = &imx225_video_ops,
};

static int imx225_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &imx225_win_sizes[0];
	enum v4l2_mbus_pixelcode mbus;
	int i = 0;
	int ret;

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

	imx225_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = imx225_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = imx225_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(imx225_win_sizes); i++)
		imx225_win_sizes[i].mbus_code = mbus;

	/*
	 *(volatile unsigned int*)(0xb000007c) = 0x20000017;
	 while(*(volatile unsigned int*)(0xb000007c) & (1 << 28));
	 */

	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &imx225_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &imx225_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	printk("@@@@@@@sssssssssssssssssprobe ok ------->imx225\n");
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int imx225_remove(struct i2c_client *client)
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

static const struct i2c_device_id imx225_id[] = {
	{ "imx225", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx225_id);

static struct i2c_driver imx225_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "imx225",
	},
	.probe		= imx225_probe,
	.remove		= imx225_remove,
	.id_table	= imx225_id,
};

static __init int init_imx225(void)
{
	return i2c_add_driver(&imx225_driver);
}

static __exit void exit_imx225(void)
{
	i2c_del_driver(&imx225_driver);
}

module_init(init_imx225);
module_exit(exit_imx225);

MODULE_DESCRIPTION("A low-level driver for OmniVision imx225 sensors");
MODULE_LICENSE("GPL");
