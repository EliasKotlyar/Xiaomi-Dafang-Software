/*
 * sc2235.c
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

#define SC2235_CHIP_ID_H	(0x22)
#define SC2235_CHIP_ID_L	(0x35)
#define SC2235_REG_END		0xffff
#define SC2235_REG_DELAY	0xfffe
#define SC2235_SUPPORT_PCLK_FPS_30 (81000*1000)
#define SC2235_SUPPORT_PCLK_FPS_15 (33750*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
#define DRIVE_CAPABILITY_1

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_12BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

static int sensor_max_fps = TX_SENSOR_MAX_FPS_25;
module_param(sensor_max_fps, int, S_IRUGO);
MODULE_PARM_DESC(sensor_max_fps, "Sensor Max Fps set interface");

struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
	unsigned int value;
	unsigned int gain;
};

struct again_lut sc2235_again_lut[] = {
	{0x10, 0},
	{0x11, 5731},
	{0x12, 11136},
	{0x13, 16248},
	{0x14, 21097},
	{0x15, 25710},
	{0x16, 30109},
	{0x17, 34312},
	{0x18, 38336},
	{0x19, 42195},
	{0x1a, 45904},
	{0x1b, 49472},
	{0x1c, 52910},
	{0x1d, 56228},
	{0x1e, 59433},
	{0x1f, 62534},
	{0x110,	65536},
	{0x111,	71267},
	{0x112,	76672},
	{0x113,	81784},
	{0x114,	86633},
	{0x115,	91246},
	{0x116,	95645},
	{0x117,	99848},
	{0x118, 103872},
	{0x119,	107731},
	{0x11a,	111440},
	{0x11b,	115008},
	{0x11c,	118446},
	{0x11d,	121764},
	{0x11e,	124969},
	{0x11f,	128070},
	{0x310, 131072},
	{0x311,	136803},
	{0x312,	142208},
	{0x313,	147320},
	{0x314,	152169},
	{0x315,	156782},
	{0x316,	161181},
	{0x317,	165384},
	{0x318,	169408},
	{0x319,	173267},
	{0x31a,	176976},
	{0x31b,	180544},
	{0x31c,	183982},
	{0x31d,	187300},
	{0x31e,	190505},
	{0x31f,	193606},
	{0x710, 196608},
	{0x711,	202339},
	{0x712,	207744},
	{0x713,	212856},
	{0x714,	217705},
	{0x715,	222318},
	{0x716,	226717},
	{0x717,	230920},
	{0x718,	234944},
	{0x719,	238803},
	{0x71a,	242512},
	{0x71b,	246080},
	{0x71c,	249518},
	{0x71d,	252836},
	{0x71e,	256041},
};

struct tx_isp_sensor_attribute sc2235_attr;

unsigned int sc2235_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = sc2235_again_lut;
	while(lut->gain <= sc2235_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = lut[0].value;
			return lut[0].gain;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == sc2235_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int sc2235_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute sc2235_attr={
	.name = "sc2235",
	.chip_id = 0x2235,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
	.cbus_device = 0x30,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 256041,
	.max_dgain = 0,
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 0x5a0 - 4,
	.integration_time_limit = 0x5a0 - 4,
	.total_width = 0x8ca,
	.total_height = 0x5a0,
	.max_integration_time = 0x5a0 - 4,
	.one_line_expr_in_us = 28,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 2,
	.sensor_ctrl.alloc_again = sc2235_alloc_again,
	.sensor_ctrl.alloc_dgain = sc2235_alloc_dgain,
};


static struct regval_list sc2235_init_regs_1920_1080_25fps[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x3621, 0x28},
	{0x3309, 0x60},
	{0x331f, 0x4d},
	{0x3321, 0x4f},
	{0x33b5, 0x10},
	{0x3303, 0x20},
	{0x331e, 0x0d},
	{0x3320, 0x0f},
	{0x3622, 0x02},
	{0x3633, 0x42},
	{0x3634, 0x42},
	{0x3306, 0x66},
	{0x330b, 0xd1},
	{0x3301, 0x0e},
	{0x320c, 0x08},
	{0x320d, 0x98},
	{0x3364, 0x05},// [2] 1: write at sampling ending
	{0x363c, 0x28}, //bypass nvdd
	{0x363b, 0x0a}, //HVDD
	{0x3635, 0xa0}, //TXVDD
	{0x4500, 0x59},
	{0x3d08, 0x01},/*pclk polarity*/
	{0x3908, 0x11},
	{0x363c, 0x08},
	{0x3e03, 0x03},
	{0x3e01, 0x46},
	{0x3381, 0x0a},
	{0x3348, 0x09},
	{0x3349, 0x50},
	{0x334a, 0x02},
	{0x334b, 0x60},
	{0x3380, 0x04},
	{0x3340, 0x06},
	{0x3341, 0x50},
	{0x3342, 0x02},
	{0x3343, 0x60},
	{0x3632, 0x88}, //anti sm
	{0x3309, 0xa0},
	{0x331f, 0x8d},
	{0x3321, 0x8f},
	{0x335e, 0x01},  //ana dithering
	{0x335f, 0x03},
	{0x337c, 0x04},
	{0x337d, 0x06},
	{0x33a0, 0x05},
	{0x3301, 0x05},
	{0x3670, 0x08} , //[3]:3633 logic ctrl  real value in 3682
	{0x367e, 0x07},  //gain0
	{0x367f, 0x0f},  //gain1
	{0x3677, 0x2f},  //<gain0
	{0x3678, 0x22},  //gain0 - gain1
	{0x3679, 0x43},  //>gain1
	{0x337f, 0x03},
	{0x3369, 0x00},
	{0x336a, 0x00},
	{0x336b, 0x00},
	{0x3367, 0x08},
	{0x330e, 0x30},
	{0x3366, 0x7c}, // div_rst gap
	{0x3635, 0xc1},
	{0x363b, 0x09},
	{0x363c, 0x07},
	{0x391e, 0x00},
	{0x3637, 0x14}, //fullwell 7K
	{0x3306, 0x54},
	{0x330b, 0xd8},
	{0x366e, 0x08},  // ofs auto en [3]
	{0x366f, 0x2f},
	{0x3630, 0x48},
	{0x3622, 0x06},
	{0x3638, 0x1f},
	{0x3625, 0x02},
	{0x3636, 0x24},
	{0x3348, 0x08},
	{0x3e03, 0x0b},
	{0x3342, 0x03},
	{0x3343, 0xa0},
	{0x334a, 0x03},
	{0x334b, 0xa0},
	{0x3343, 0xb0},
	{0x334b, 0xb0},
	{0x3802, 0x01},
	{0x3235, 0x04},
	{0x3236, 0x63}, // vts-2
	{0x3343, 0xd0},
	{0x334b, 0xd0},
	{0x3348, 0x07},
	{0x3349, 0x80},
	{0x391b, 0x4d},
	{0x3342, 0x04},
	{0x3343, 0x20},
	{0x334a, 0x04},
	{0x334b, 0x20},
	{0x3222, 0x29},
	{0x3901, 0x02},
	{0x3f00, 0x07},  // bit[2] = 1
	{0x3f04, 0x08},
	{0x3f05, 0x74},  // hts - 0x24
	{0x330b, 0xc8},
	{0x3306, 0x4a},
	{0x330b, 0xca},
	{0x3639, 0x09},
	{0x5780, 0xff},
	{0x5781, 0x04},
	{0x5785, 0x18},
	{0x3039, 0x35},
	{0x303a, 0x2e},
	{0x3034, 0x05},
	{0x3035, 0x2a},
	{0x320c, 0x08},
	{0x320d, 0xca},
	{0x320e, 0x05},
	{0x320f, 0xa0},
	{0x3f04, 0x08},
	{0x3f05, 0xa6}, // hts - 0x24
	{0x3235, 0x04},
	{0x3236, 0xae}, // vts-2
	{0x3313, 0x05},
	{0x3678, 0x42},
	{0x3670, 0x00},
	{0x3633, 0x42},
	{0x3802, 0x00},
	{SC2235_REG_END, 0x00},	/* END MARKER */
};
static struct regval_list sc2235_init_regs_1920_1080_15fps[] = {
};
/*
 * the order of the sc2235_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sc2235_win_sizes[] = {
	/* 1920*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= sc2235_init_regs_1920_1080_25fps,
	}
};

static enum v4l2_mbus_pixelcode sc2235_mbus_code[] = {
	V4L2_MBUS_FMT_SBGGR10_1X10,
	V4L2_MBUS_FMT_SBGGR12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list sc2235_stream_on[] = {
	{0x0100, 0x01},
	{SC2235_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc2235_stream_off[] = {
	{0x0100, 0x00},
	{SC2235_REG_END, 0x00},	/* END MARKER */
};

int sc2235_read(struct v4l2_subdev *sd, uint16_t reg, unsigned char *value)
{
	int ret;
	unsigned char buf[2] = {reg >> 8, reg & 0xff};
	struct i2c_client *client = v4l2_get_subdevdata(sd);
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

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}

int sc2235_write(struct v4l2_subdev *sd, uint16_t reg, unsigned char value)
{
	int ret;;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	uint8_t buf[3] = {(reg>>8)&0xff, reg&0xff, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 3,
		.buf	= buf,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int sc2235_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SC2235_REG_END) {
		if (vals->reg_num == SC2235_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = sc2235_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int sc2235_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SC2235_REG_END) {
		if (vals->reg_num == SC2235_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = sc2235_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int sc2235_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sc2235_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	int ret;
	unsigned char v;

	ret = sc2235_read(sd, 0x3107, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC2235_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = sc2235_read(sd, 0x3108, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC2235_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int sc2235_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	ret = sc2235_write(sd, 0x3e00, (unsigned char)((value >> 12) & 0x0f));
	ret += sc2235_write(sd, 0x3e01, (unsigned char)((value >> 4) & 0xff));
	ret += sc2235_write(sd, 0x3e02, (unsigned char)((value & 0x0f) << 4));
	if (value < 0x50) {
		ret = sc2235_write(sd, 0x3314,0x12);
		if (ret < 0)
			return ret;
	}
	else if (value > 0xa0) {
		ret += sc2235_write(sd, 0x3314,0x02);
		if (ret < 0)
			return ret;
	}
	if (ret < 0)
		return ret;
	return 0;
}

static int sc2235_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret = sc2235_write(sd, 0x3e09, (unsigned char)(value & 0xff));
	ret += sc2235_write(sd, 0x3e08, (unsigned char)((value >> 8 << 2) | 0x03));
	if (ret < 0)
		return ret;

	if (value < 0x110) {
		sc2235_write(sd,0x3812,0x00);
		sc2235_write(sd,0x3301,0x0b);
		sc2235_write(sd,0x3631,0x84);
		sc2235_write(sd,0x366f,0x2f);
		sc2235_write(sd,0x5781,0x04);
		sc2235_write(sd,0x5785,0x18);
		sc2235_write(sd,0x3812,0x30);
	}
	else if (value>=0x110&&value<0x710){
		sc2235_write(sd,0x3812,0x00);
		sc2235_write(sd,0x3301,0x14);
		sc2235_write(sd,0x3631,0x88);
		sc2235_write(sd,0x366f,0x2f);
		sc2235_write(sd,0x5781,0x04);
		sc2235_write(sd,0x5785,0x18);
		sc2235_write(sd,0x3812,0x30);
	}
	else if(value>=0x710&&value<=0x71e){
		sc2235_write(sd,0x3812,0x00);
		sc2235_write(sd,0x3301,0x1c);
		sc2235_write(sd,0x3631,0x88);
		sc2235_write(sd,0x366f,0x2f);
		sc2235_write(sd,0x5781,0x02);
		sc2235_write(sd,0x5785,0x18);
		sc2235_write(sd,0x3812,0x30);
	}
	else{ //may be flick
		sc2235_write(sd,0x3812,0x00);
		sc2235_write(sd,0x3301,0xff);
		sc2235_write(sd,0x3631,0x88);
		sc2235_write(sd,0x366f,0x3c);
		sc2235_write(sd,0x5781,0x01);
		sc2235_write(sd,0x5785,0x18);
		sc2235_write(sd,0x3812,0x30);
	}

	return 0;
}

static int sc2235_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sc2235_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sc2235_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &sc2235_win_sizes[0];
	int ret = 0;

	if(!enable)
		return ISP_SUCCESS;

	switch (sensor_max_fps) {
	case TX_SENSOR_MAX_FPS_25:
		wsize->fps = 25 << 16 | 1;
		wsize->regs = sc2235_init_regs_1920_1080_25fps;
		break;
	case TX_SENSOR_MAX_FPS_15:
		wsize->fps = 15 << 16 | 1;
		wsize->regs = sc2235_init_regs_1920_1080_15fps;
		break;
	default:
		printk("Now we do not support this framerate!!!\n");
	}

	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;

	ret = sc2235_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int sc2235_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sc2235_write_array(sd, sc2235_stream_on);
		pr_debug("sc2235 stream on\n");
	}
	else {
		ret = sc2235_write_array(sd, sc2235_stream_off);
		pr_debug("sc2235 stream off\n");
	}
	return ret;
}

static int sc2235_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sc2235_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sc2235_set_fps(struct tx_isp_sensor *sensor, int fps)
{

	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	unsigned int pclk = 0;
	unsigned short hts;
	unsigned short vts = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	unsigned int max_fps = 0; //the format is 24.8
	int ret = 0;

	switch (sensor_max_fps) {
	case TX_SENSOR_MAX_FPS_25:
		pclk = SC2235_SUPPORT_PCLK_FPS_30;
		max_fps = SENSOR_OUTPUT_MAX_FPS;
		break;
	case TX_SENSOR_MAX_FPS_15:
		pclk = SC2235_SUPPORT_PCLK_FPS_15;
		max_fps = TX_SENSOR_MAX_FPS_15;
		break;
	default:
		printk("Now we do not support this framerate!!!\n");
	}

	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}
	ret = sc2235_read(sd, 0x320c, &tmp);
	hts = tmp;
	ret += sc2235_read(sd, 0x320d, &tmp);
	if(ret < 0)
		return -1;
	hts = ((hts << 8) + tmp);

	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	printk("Vts ====%x\n",vts);

	ret = sc2235_write(sd, 0x320f, (unsigned char)(vts & 0xff));
	ret += sc2235_write(sd, 0x320e, (unsigned char)(vts >> 8));
	if(ret < 0){
		printk("err: sc2235_write err\n");
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

static int sc2235_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &sc2235_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &sc2235_win_sizes[0];
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

static int sc2235_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"sc2235_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(10);
			gpio_direction_output(reset_gpio, 0);
			msleep(10);
			gpio_direction_output(reset_gpio, 1);
			msleep(1);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if (pwdn_gpio != -1) {
		ret = gpio_request(pwdn_gpio, "sc2235_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			msleep(50);
			gpio_direction_output(pwdn_gpio, 0);
			msleep(10);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = sc2235_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sc2235 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "sc2235 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sc2235_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static long sc2235_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = sc2235_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = sc2235_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = sc2235_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = sc2235_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = sc2235_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			ret = sc2235_write_array(sd, sc2235_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			ret = sc2235_write_array(sd, sc2235_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = sc2235_set_fps(sensor, ctrl->value);
			break;
		default:
			pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
			break;;
	}
	return 0;
}

static long sc2235_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = sc2235_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc2235_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = sc2235_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int sc2235_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	sc2235_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops sc2235_core_ops = {
	.g_chip_ident = sc2235_g_chip_ident,
	.reset = sc2235_reset,
	.init = sc2235_init,
	.s_power = sc2235_s_power,
	.ioctl = sc2235_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sc2235_g_register,
	.s_register = sc2235_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sc2235_video_ops = {
	.s_stream = sc2235_s_stream,
	.s_parm = sc2235_s_parm,
	.g_parm = sc2235_g_parm,
};

static const struct v4l2_subdev_ops sc2235_ops = {
	.core = &sc2235_core_ops,
	.video = &sc2235_video_ops,
};

static int sc2235_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &sc2235_win_sizes[0];
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

	switch (sensor_max_fps) {
		case TX_SENSOR_MAX_FPS_25:
			clk_set_rate(sensor->mclk, 24000000);
			break;
		case TX_SENSOR_MAX_FPS_15:
			clk_set_rate(sensor->mclk, 12000000);
			break;
		default:
			printk("Now we do not support this framerate!!!\n");
	}
	clk_enable(sensor->mclk);

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	sc2235_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = sc2235_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = sc2235_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(sc2235_win_sizes); i++)
		sc2235_win_sizes[i].mbus_code = mbus;

	/*
	  convert sensor-gain into isp-gain,
	*/
	switch (sensor_max_fps) {
		case TX_SENSOR_MAX_FPS_25:
			break;
		case TX_SENSOR_MAX_FPS_15:
			sc2235_attr.max_integration_time_native = 1121;
			sc2235_attr.integration_time_limit = 1121;
			sc2235_attr.total_width = 2000;
			sc2235_attr.total_height = 1125;
			sc2235_attr.max_integration_time = 1121;
			break;
		default:
			printk("Now we do not support this framerate!!!\n");
	}
	sc2235_attr.max_again = 256041;
	sc2235_attr.max_dgain = 0; //sc2235_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &sc2235_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &sc2235_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("@@@@@@@probe ok ------->sc2235\n");
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int sc2235_remove(struct i2c_client *client)
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

static const struct i2c_device_id sc2235_id[] = {
	{ "sc2235", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc2235_id);

static struct i2c_driver sc2235_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "sc2235",
	},
	.probe		= sc2235_probe,
	.remove		= sc2235_remove,
	.id_table	= sc2235_id,
};

static __init int init_sc2235(void)
{
	return i2c_add_driver(&sc2235_driver);
}

static __exit void exit_sc2235(void)
{
	i2c_del_driver(&sc2235_driver);
}

module_init(init_sc2235);
module_exit(exit_sc2235);

MODULE_DESCRIPTION("A low-level driver for SmartSens sc2235 sensors");
MODULE_LICENSE("GPL");
