/*
 * ov7740.c
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

#define OV7740_CHIP_ID_H	(0x77)
#define OV7740_CHIP_ID_L	(0x42)

#define OV7740_REG_END		0xff
#define OV7740_REG_DELAY	0xfe

#define TX_ISP_SUPPORT_MCLK (24*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 60
#define SENSOR_OUTPUT_MIN_FPS 5
#define OV7740_SUPPORT_PCLK (12*1000*1000)
#define DRIVE_CAPABILITY_1

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

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
	unsigned char value;
	unsigned int gain;
};
struct again_lut ov7740_again_lut[] = {
	{0x0, 0},
	{0x1, 5731},
	{0x2, 11136},
	{0x3, 16247},
	{0x4, 21097},
	{0x5, 25710},
	{0x6, 30108},
	{0x7, 34311},
	{0x8, 38335},
	{0x9, 42195},
	{0xa, 47747},
	{0xb, 51316},
	{0xc, 54754},
	{0xd, 58072},
	{0xe, 61277},
	{0xf, 64378},
	{0x10, 65535},
	{0x11, 71266},
	{0x12, 76671},
	{0x13, 81782},
	{0x14, 86632},
	{0x15, 91245},
	{0x16, 95643},
	{0x17, 99846},
	{0x18, 103870},
	{0x19, 107730},
	{0x1a, 111438},
	{0x1b, 115006},
	{0x1c, 118445},
	{0x1d, 121762},
	{0x1e, 124968},
	{0x1f, 128068},
	{0x30, 131070},
	{0x31, 136801},
	{0x32, 142206},
	{0x33, 147317},
	{0x34, 152167},
	{0x35, 156780},
	{0x36, 161178},
	{0x37, 165381},
	{0x38, 169405},
	{0x39, 173265},
	{0x3a, 176973},
	{0x3b, 180541},
	{0x3c, 183980},
	{0x3d, 187297},
	{0x3e, 190503},
	{0x3f, 193603},
	{0x70, 196605},
	{0x71, 202336},
	{0x72, 207741},
	{0x73, 212852},
	{0x74, 217702},
	{0x75, 222315},
	{0x76, 226713},
	{0x77, 230916},
	{0x78, 234940},
	{0x79, 238800},
	{0x7a, 242508},
	{0x7b, 246076},
	{0x7c, 249515},
	{0x7d, 252832},
	{0x7e, 256038},
	{0x7f, 259138},
	{0xf0, 262140},
	{0xf1, 267871},
	{0xf2, 273276},
	{0xf3, 278387},
	{0xf4, 283237},
	{0xf5, 287850},
	{0xf6, 292248},
	{0xf7, 296451},
	{0xf8, 300475},
	{0xf9, 304335},
	{0xfa, 308043},
	{0xfb, 311611},
	{0xfc, 315050},
	{0xfd, 318367},
	{0xfe, 321573},
	{0xff, 324673},
};

struct tx_isp_sensor_attribute ov7740_attr;

unsigned int ov7740_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = ov7740_again_lut;

	while(lut->gain <= ov7740_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = 0;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == ov7740_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}
unsigned int ov7740_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}
struct tx_isp_sensor_attribute ov7740_attr={
	.name = "ov7740",
	.chip_id = 0x7742,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
	.cbus_device = 0x21,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 260986,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 250,
	.integration_time_limit = 250,
	.total_width = 791,
	.total_height = 252,
	.max_integration_time = 250,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 1,
	.sensor_ctrl.alloc_again = ov7740_alloc_again,
	.sensor_ctrl.alloc_dgain = ov7740_alloc_dgain,
	//void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list ov7740_init_regs_320_240_60fps[] = {

	{0x12, 0x80},
	{0x11, 0x01},
	{0x12, 0x41},
	{0xd5, 0x10},
	{0x0c, 0x12},
	{0x0d, 0xd2},
	{0x17, 0x25},
	{0x18, 0xa0},
	{0x19, 0x01},
	{0x1a, 0x78},
	{0x1b, 0x81},
	{0x1e, 0x31},
	{0x29, 0x17},
	{0x2b, 0xfc},
	{0x2c, 0x00},
	{0x31, 0x50},
	{0x32, 0x78},
	{0x33, 0x44},
	{0x36, 0x2f},
	{0x04, 0x40},
	{0x27, 0x80},
	{0x3a, 0x06},
	{0x3d, 0x0f},
	{0x3e, 0x80},
	{0x3f, 0x40},
	{0x40, 0x7f},
	{0x41, 0x6a},
	{0x42, 0x29},
	{0x44, 0x33},
	{0x45, 0x41},
	{0x47, 0x02},
	{0x49, 0x64},
	{0x4a, 0xa1},
	{0x4b, 0x40},
	{0x4c, 0x1a},
	{0x4d, 0x50},
	{0x4e, 0x13},
	{0x64, 0x00},
	{0x67, 0x88},
	{0x68, 0x0a},
	{0x14, 0x38},
	{0x24, 0x3c},
	{0x25, 0x30},
	{0x26, 0x72},
	{0x50, 0x97},
	{0x51, 0x7e},
	{0x52, 0x00},
	{0x53, 0x40},
	{0x20, 0x00},
	{0x21, 0x23},
	{0x38, 0x14},
	{0xe9, 0x00},
	{0x56, 0x55},
	{0x57, 0xff},
	{0x58, 0xff},
	{0x59, 0xff},
	{0x5f, 0x04},
	{0xec, 0x01},
	{0x13, 0xd0},
	{0x80, 0x77},
	{0x81, 0x3f},
	{0x82, 0x3f},
	{0x83, 0x01},
	{0x38, 0x11},
	{0x84, 0x70},
	{0x85, 0x00},
	{0x86, 0x03},
	{0x87, 0x01},
	{0x88, 0x05},
	{0x89, 0x34},
	{0x8d, 0x30},
	{0x8f, 0x85},
	{0x93, 0x30},
	{0x95, 0x85},
	{0x99, 0x30},
	{0x9b, 0x85},
	{0xce, 0x78},
	{0xcf, 0x6e},
	{0xd0, 0x0a},
	{0xd1, 0x0c},
	{0xd2, 0x84},
	{0xd3, 0x90},
	{0xd4, 0x1e},
	{0x5a, 0x24},
	{0x5b, 0x1f},
	{0x5c, 0x88},
	{0x5d, 0x60},
	{0xac, 0x6e},
	{0xbe, 0xff},
	{0xbf, 0x00},

	{OV7740_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the ov7740_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ov7740_win_sizes[] = {
	/* 320*240 */
	{
		.width		= 320,
		.height		= 240,
		.fps		= 60 << 16 | 1, /* 12.5 fps */
		.mbus_code	= V4L2_MBUS_FMT_SGRBG10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov7740_init_regs_320_240_60fps,
	}
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ov7740_stream_on[] = {

	{OV7740_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov7740_stream_off[] = {
	/* Sensor enter LP11*/

	{OV7740_REG_END, 0x00},	/* END MARKER */
};

int ov7740_read(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= &reg,
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

int ov7740_write(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 2,
		.buf	= buf,
	};
	int ret;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int ov7740_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV7740_REG_END) {
		if (vals->reg_num == OV7740_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov7740_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			vals->value = val;
		}
		vals++;
	}
	return 0;
}
static int ov7740_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV7740_REG_END) {
		if (vals->reg_num == OV7740_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov7740_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int ov7740_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int ov7740_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = ov7740_read(sd, 0x0a, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != OV7740_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = ov7740_read(sd, 0x0b, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != OV7740_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int ov7740_set_integration_time(struct v4l2_subdev *sd, int value)
{
	unsigned int expo = value;
	int ret = 0;

	ov7740_write(sd, 0x10, (unsigned char)(expo & 0xff));
	ov7740_write(sd, 0x0f, (unsigned char)((expo >> 8) & 0xff));
	if (ret < 0)
		return ret;

	return 0;
}
static int ov7740_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	ov7740_write(sd, 0x00, (unsigned char)(value & 0xff));
	ov7740_write(sd, 0x15, (unsigned char)((value >> 8)& 0x03));
	return 0;
}
static int ov7740_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[7] if gain > 2X set 0; if gain > 4X set 1 */
	return 0;
}

static int ov7740_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
#if 0
	int ret = 0;
	int black = 0;
	unsigned char h,l;
	unsigned char reg = 0xff;
	unsigned int * v = (unsigned int *)(value);
	ret = ov7740_read(sd, 0x48, &h);
	if (ret < 0)
		return ret;
	switch(*v){
		case SENSOR_R_BLACK_LEVEL:
			black = (h & 0x3) << 8;
			reg = 0x44;
			break;
		case SENSOR_GR_BLACK_LEVEL:
			black = (h & (0x3 << 2)) << 8;
			reg = 0x45;
			break;
		case SENSOR_GB_BLACK_LEVEL:
			black = (h & (0x3 << 4)) << 8;
			reg = 0x46;
			break;
		case SENSOR_B_BLACK_LEVEL:
			black = (h & (0x3 << 6)) << 8;
			reg = 0x47;
			break;
		default:
			return -1;
	}
	ret = ov7740_read(sd, reg, &l);
	if (ret < 0)
		return ret;
	*v = (black | l);
#endif
	return 0;
}

static int ov7740_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &ov7740_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = ov7740_write_array(sd, wsize->regs);
	if (ret < 0)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int ov7740_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	if (enable) {
		ret = ov7740_write_array(sd, ov7740_stream_on);
		printk("ov7740 stream on\n");
	}
	else {
		ret = ov7740_write_array(sd, ov7740_stream_off);
		printk("ov7740 stream off\n");
	}
	return ret;
}

static int ov7740_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov7740_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov7740_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = OV7740_SUPPORT_PCLK;
	unsigned short hts;
	unsigned short vts = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;
	ret += ov7740_read(sd, 0x2a, &tmp);
	hts = tmp;
	ret += ov7740_read(sd, 0x29, &tmp);
	if(ret < 0)
		return -1;
	hts = (hts << 8) + tmp;
	/*vts = (pclk << 4) / (hts * (newformat >> 4));*/
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret = ov7740_write(sd, 0x2b, (unsigned char)(vts & 0xff));
	ret += ov7740_write(sd, 0x2c, (unsigned char)(vts >> 8));
	if(ret < 0)
		return -1;
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 2;
	sensor->video.attr->integration_time_limit = vts - 2;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 2;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return 0;

}

static int ov7740_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &ov7740_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &ov7740_win_sizes[0];
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
		if(sensor->priv != wsize){
			ret = ov7740_write_array(sd, wsize->regs);
			if(!ret)
				sensor->priv = wsize;
		}
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}
static int ov7740_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"ov7740_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(20);
			gpio_direction_output(reset_gpio, 0);
			msleep(20);
			gpio_direction_output(reset_gpio, 1);
			msleep(1);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"ov7740_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
			msleep(10);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = ov7740_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an ov7740 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "ov7740 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ov7740_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long ov7740_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = ov7740_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = ov7740_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = ov7740_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = ov7740_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = ov7740_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			ret = ov7740_write_array(sd, ov7740_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			ret = ov7740_write_array(sd, ov7740_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = ov7740_set_fps(sensor, ctrl->value);
			break;
		default:
			break;
	}
	return ret;
}
static long ov7740_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = ov7740_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return ret;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov7740_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov7740_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov7740_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov7740_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops ov7740_core_ops = {
	.g_chip_ident = ov7740_g_chip_ident,
	.reset = ov7740_reset,
	.init = ov7740_init,
	.s_power = ov7740_s_power,
	.ioctl = ov7740_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov7740_g_register,
	.s_register = ov7740_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov7740_video_ops = {
	.s_stream = ov7740_s_stream,
	.s_parm = ov7740_s_parm,
	.g_parm = ov7740_g_parm,
};

static const struct v4l2_subdev_ops ov7740_ops = {
	.core = &ov7740_core_ops,
	.video = &ov7740_video_ops,
};

static int ov7740_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &ov7740_win_sizes[0];
	int ret;

	sensor = (struct tx_isp_sensor *)kzalloc(sizeof(*sensor), GFP_KERNEL);
	if(!sensor){
		printk("Failed to allocate sensor subdev.\n");
		return -ENOMEM;
	}
	/* request mclk of sensor */
	sensor->mclk = clk_get(NULL, "cgu_cim");
	if (IS_ERR(sensor->mclk)) {
		printk("Cannot get sensor input clock cgu_cim\n");
		goto err_get_mclk;
	}
	clk_set_rate(sensor->mclk, 24000000);
	clk_enable(sensor->mclk);

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	ov7740_attr.dvp.gpio = sensor_gpio_func;
	 /*
		convert sensor-gain into isp-gain,
	 */
	ov7740_attr.max_again = 260986;
	ov7740_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &ov7740_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &ov7740_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("probe ok ------->ov7740\n");
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int ov7740_remove(struct i2c_client *client)
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

static const struct i2c_device_id ov7740_id[] = {
	{ "ov7740", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov7740_id);

static struct i2c_driver ov7740_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ov7740",
	},
	.probe		= ov7740_probe,
	.remove		= ov7740_remove,
	.id_table	= ov7740_id,
};

static __init int init_ov7740(void)
{
	return i2c_add_driver(&ov7740_driver);
}

static __exit void exit_ov7740(void)
{
	i2c_del_driver(&ov7740_driver);
}

module_init(init_ov7740);
module_exit(exit_ov7740);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov7740 sensors");
MODULE_LICENSE("GPL");
