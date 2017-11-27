/*
 * jxf22.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define DEBUG

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <sensor-common.h>
#include <apical-isp/apical_math.h>
#include <linux/proc_fs.h>
#include <soc/gpio.h>

#define JXF22_CHIP_ID_H	(0x0f)
#define JXF22_CHIP_ID_L	(0x22)
#define JXF22_REG_END		0xff
#define JXF22_REG_DELAY	0xfe
#define JXF22_SUPPORT_SCLK (81000000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_LOW_10BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

static int data_interface = TX_SENSOR_DATA_INTERFACE_DVP;
module_param(data_interface, int, S_IRUGO);
MODULE_PARM_DESC(data_interface, "Sensor Date interface");

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
	unsigned int value;
	unsigned int gain;
};

struct again_lut jxf22_again_lut[] = {
	{0x0,  0 },
	{0x1,  5731 },
	{0x2,  11136},
	{0x3,  16248},
	{0x4,  21097},
	{0x5,  25710},
	{0x6,  30109},
	{0x7,  34312},
	{0x8,  38336},
	{0x9,  42195},
	{0xa,  45904},
	{0xb,  49472},
	{0xc,  52910},
	{0xd,  56228},
	{0xe,  59433},
	{0xf,  62534},
	{0x10,  65536},
	{0x11,	71267},
	{0x12,	76672},
	{0x13,	81784},
	{0x14,	86633},
	{0x15,	91246},
	{0x16,	95645},
	{0x17,	99848},
	{0x18,  103872},
	{0x19,	107731},
	{0x1a,	111440},
	{0x1b,	115008},
	{0x1c,	118446},
	{0x1d,	121764},
	{0x1e,	124969},
	{0x1f,	128070},
	{0x20,	131072},
	{0x21,	136803},
	{0x22,	142208},
	{0x23,	147320},
	{0x24,	152169},
	{0x25,	156782},
	{0x26,	161181},
	{0x27,	165384},
	{0x28,	169408},
	{0x29,	173267},
	{0x2a,	176976},
	{0x2b,	180544},
	{0x2c,	183982},
	{0x2d,	187300},
	{0x2e,	190505},
	{0x2f,	193606},
	{0x30,	196608},
	{0x31,	202339},
	{0x32,	207744},
	{0x33,	212856},
	{0x34,	217705},
	{0x35,	222318},
	{0x36,	226717},
	{0x37,	230920},
	{0x38,	234944},
	{0x39,	238803},
	{0x3a,	242512},
	{0x3b,	246080},
	{0x3c,	249518},
	{0x3d,	252836},
	{0x3e,	256041},
	{0x3f,	259142},
	{0x40,	262144},
	{0x41,	267875},
	{0x42,	273280},
	{0x43,	278392},
	{0x44,	283241},
	{0x45,	287854},
	{0x46,	292253},
	{0x47,	296456},
	{0x48,	300480},
	{0x49,	304339},
	{0x4a,	308048},
	{0x4b,	311616},
	{0x4c,	315054},
	{0x4d,	318372},
	{0x4e,	321577},
	{0x4f,	324678},
};

struct tx_isp_sensor_attribute jxf22_attr;

unsigned int jxf22_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = jxf22_again_lut;

	while(lut->gain <= jxf22_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = 0;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == jxf22_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int jxf22_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute jxf22_attr={
	.name = "jxf22",
	.chip_id = 0xf22,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
	.cbus_device = 0x40,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 262144,
	.max_dgain = 0,
	.min_integration_time = 2,
	.min_integration_time_native = 2,
	.max_integration_time_native = 1350 - 4,
	.integration_time_limit = 1350 - 4,
	.total_width = 2400,
	.total_height = 1350,
	.max_integration_time = 1350 - 4,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 1,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = jxf22_alloc_again,
	.sensor_ctrl.alloc_dgain = jxf22_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};


static struct regval_list jxf22_init_regs_1920_1080_30fps_mipi[] = {

{JXF22_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list jxf22_init_regs_1920_1080_30fps_dvp[] = {
	{0x12, 0x40},
	{0x0E, 0x11},
	{0x0F, 0x00},
	{0x10, 0x36},
	{0x11, 0x80},
	{0x5F, 0x01},
	{0x60, 0x0A},
	{0x19, 0x20},
	{0x48, 0x05},
	{0x20, 0xB0},//HTS L
	{0x21, 0x04},//HTS H
	{0x22, 0x46},//VTS L
	{0x23, 0x05},//VTS H
	{0x24, 0xC0},
	{0x25, 0x38},
	{0x26, 0x43},
	{0x27, 0xD0},
	{0x28, 0x18},
	{0x29, 0x01},
	{0x2A, 0xC0},
	{0x2B, 0x21},
	{0x2C, 0x04},
	{0x2D, 0x01},
	{0x2E, 0x15},
	{0x2F, 0x44},
	{0x41, 0xCC},
	{0x42, 0x03},
	{0x39, 0x90},
	{0x1D, 0xFF},
	{0x1E, 0x9F},
	{0x6C, 0x90},
	{0x30, 0x8C},
	{0x31, 0x0C},
	{0x32, 0xF0},
	{0x33, 0x0C},
	{0x34, 0x1F},
	{0x35, 0xE3},
	{0x36, 0x0E},
	{0x37, 0x34},
	{0x38, 0x13},
	{0x3A, 0x08},
	{0x3B, 0x30},
	{0x3C, 0xC0},
	{0x3D, 0x00},
	{0x3E, 0x00},
	{0x3F, 0x00},
	{0x40, 0x00},
	{0x6F, 0x03},
	{0x0D, 0x14},//Driver Capability
	{0x56, 0x32},
	{0x5A, 0x20},
	{0x5B, 0xB3},
	{0x5C, 0xF7},
	{0x5D, 0xF0},
	{0x62, 0x80},
	{0x63, 0x82},
	{0x64, 0x00},
	{0x67, 0x75},
	{0x68, 0x04},
	{0x6A, 0x4D},
	{0x8F, 0x18},
	{0x91, 0x04},
	{0x0C, 0x00},
	{0x59, 0x97},
	{0x4A, 0x05},
	{0x49, 0x10},
	{0x50, 0x02},
	{0x47, 0x22},
	{0x7E, 0xCD},
	{0x13, 0x81},
	{0x12, 0x00},
	{0x93, 0x5C},
	{0x45, 0x89},
	/* sleep 500 */
	{JXF22_REG_DELAY, 250},
	{JXF22_REG_DELAY, 250},
	{0x45, 0x09},
	{0x1F, 0x01},
	{JXF22_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the jxf22_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting jxf22_win_sizes[] = {
	/* 1280*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= jxf22_init_regs_1920_1080_30fps_dvp,
	}
};

static enum v4l2_mbus_pixelcode jxf22_mbus_code[] = {
	V4L2_MBUS_FMT_SBGGR10_1X10,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list jxf22_stream_on_dvp[] = {

	{JXF22_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list jxf22_stream_off_dvp[] = {
	{JXF22_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list jxf22_stream_on_mipi[] = {

	{JXF22_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list jxf22_stream_off_mipi[] = {
	{JXF22_REG_END, 0x00},	/* END MARKER */
};

int jxf22_read(struct v4l2_subdev *sd, unsigned char reg,
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

int jxf22_write(struct v4l2_subdev *sd, unsigned char reg,
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

static int jxf22_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != JXF22_REG_END) {
		if (vals->reg_num == JXF22_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = jxf22_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}
static int jxf22_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != JXF22_REG_END) {
		if (vals->reg_num == JXF22_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = jxf22_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int jxf22_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int jxf22_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;

	ret = jxf22_read(sd, 0x0a, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != JXF22_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = jxf22_read(sd, 0x0b, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;

	if (v != JXF22_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int jxf22_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	unsigned int expo = value;
	ret = jxf22_write(sd,  0x01, (unsigned char)(expo & 0xff));
	ret += jxf22_write(sd, 0x02, (unsigned char)((expo >> 8) & 0xff));
	if (ret < 0)
		return ret;
	return 0;

}

static int jxf22_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	ret += jxf22_write(sd, 0x00, (unsigned char)(value & 0x7f));
	if (ret < 0)
		return ret;
	return 0;
}

static int jxf22_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int jxf22_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int jxf22_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &jxf22_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = jxf22_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int jxf22_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
			ret = jxf22_write_array(sd, jxf22_stream_on_dvp);
		} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
			ret = jxf22_write_array(sd, jxf22_stream_on_mipi);

		}else{
			printk("Don't support this Sensor Data interface\n");
		}
		pr_debug("jxf22 stream on\n");

	}
	else {
		if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
			ret = jxf22_write_array(sd, jxf22_stream_off_dvp);
		} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
			ret = jxf22_write_array(sd, jxf22_stream_off_mipi);

		}else{
			printk("Don't support this Sensor Data interface\n");
		}
		pr_debug("jxf22 stream off\n");
	}
	return ret;
}

static int jxf22_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int jxf22_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int jxf22_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	int ret = 0;
	unsigned int sclk = 0;
	unsigned int hts = 0;
	unsigned int vts = 0;
	unsigned char val = 0;
	unsigned int newformat = 0; //the format is 24.8

	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}
	sclk = JXF22_SUPPORT_SCLK;

	val = 0;
	ret += jxf22_read(sd, 0x21, &val);
	hts = val<<8;
	val = 0;
	ret += jxf22_read(sd, 0x20, &val);
	hts |= val;
	hts *= 2;
	if (0 != ret) {
		printk("err: jxf22 read err\n");
		return ret;
	}

	vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

	ret += jxf22_write(sd, 0x22, vts & 0xff);
	ret += jxf22_write(sd, 0x23, (vts >> 8) & 0xff);

	if (0 != ret) {
		printk("err: jxf22_write err\n");
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
}

static int jxf22_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &jxf22_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &jxf22_win_sizes[0];
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
static int jxf22_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"jxf22_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(5);
			gpio_direction_output(reset_gpio, 0);
			msleep(15);
			gpio_direction_output(reset_gpio, 1);
			msleep(5);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"jxf22_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = jxf22_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an jxf22 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "jxf22 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int jxf22_s_power(struct v4l2_subdev *sd, int on)
{

	return 0;
}
static long jxf22_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = jxf22_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = jxf22_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = jxf22_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = jxf22_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = jxf22_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
				ret = jxf22_write_array(sd, jxf22_stream_off_dvp);
			} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = jxf22_write_array(sd, jxf22_stream_off_mipi);

			}else{
				printk("Don't support this Sensor Data interface\n");
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
				ret = jxf22_write_array(sd, jxf22_stream_on_dvp);
			} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = jxf22_write_array(sd, jxf22_stream_on_mipi);

			}else{
				printk("Don't support this Sensor Data interface\n");
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = jxf22_set_fps(sensor, ctrl->value);
			break;
		default:
			pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
			break;;
	}
	return 0;
}
static long jxf22_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = jxf22_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int jxf22_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = jxf22_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int jxf22_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	jxf22_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops jxf22_core_ops = {
	.g_chip_ident = jxf22_g_chip_ident,
	.reset = jxf22_reset,
	.init = jxf22_init,
	.s_power = jxf22_s_power,
	.ioctl = jxf22_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = jxf22_g_register,
	.s_register = jxf22_s_register,
#endif
};

static const struct v4l2_subdev_video_ops jxf22_video_ops = {
	.s_stream = jxf22_s_stream,
	.s_parm = jxf22_s_parm,
	.g_parm = jxf22_g_parm,
};

static const struct v4l2_subdev_ops jxf22_ops = {
	.core = &jxf22_core_ops,
	.video = &jxf22_video_ops,
};

static int jxf22_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &jxf22_win_sizes[0];
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
	clk_set_rate(sensor->mclk, 24000000);
	clk_enable(sensor->mclk);

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	jxf22_attr.dbus_type = data_interface;
	if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
		wsize->regs = jxf22_init_regs_1920_1080_30fps_dvp;
	} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
		wsize->regs = jxf22_init_regs_1920_1080_30fps_mipi;
	} else{
		printk("Don't support this Sensor Data Output Interface.\n");
		goto err_set_sensor_data_interface;
	}
#if 0
	jxf22_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = jxf22_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = jxf22_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(jxf22_win_sizes); i++)
		jxf22_win_sizes[i].mbus_code = mbus;

#endif
	 /*
		convert sensor-gain into isp-gain,
	 */
	jxf22_attr.max_again = 262144;
	jxf22_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &jxf22_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &jxf22_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("probe ok ------->jxf22\n");
	return 0;
err_set_sensor_data_interface:
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int jxf22_remove(struct i2c_client *client)
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

static const struct i2c_device_id jxf22_id[] = {
	{ "jxf22", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, jxf22_id);

static struct i2c_driver jxf22_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "jxf22",
	},
	.probe		= jxf22_probe,
	.remove		= jxf22_remove,
	.id_table	= jxf22_id,
};

static __init int init_jxf22(void)
{
	return i2c_add_driver(&jxf22_driver);
}

static __exit void exit_jxf22(void)
{
	i2c_del_driver(&jxf22_driver);
}

module_init(init_jxf22);
module_exit(exit_jxf22);

MODULE_DESCRIPTION("A low-level driver for OmniVision jxf22 sensors");
MODULE_LICENSE("GPL");
