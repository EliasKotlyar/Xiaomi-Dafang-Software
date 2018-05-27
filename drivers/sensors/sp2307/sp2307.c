/*
 * sp2307.c
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

#define SP2307_CHIP_ID_H	(0x23)
#define SP2307_CHIP_ID_L	(0x08)
#define SP2307_REG_END		0xff
#define SP2307_REG_DELAY	0x00
#define SP2307_PAGE_REG	    0xfd

#define SP2307_SUPPORT_SCLK_FPS_30 (42000000)
#define SP2307_SUPPORT_SCLK_FPS_15 (60000000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = GPIO_PA(19);
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_LOW_10BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

static int sensor_max_fps = TX_SENSOR_MAX_FPS_25;
module_param(sensor_max_fps, int, S_IRUGO);
MODULE_PARM_DESC(sensor_max_fps, "Sensor Max Fps set interface");

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
struct again_lut sp2307_again_lut[] = {
  {0x10,	0},
  {0x11,	5731},
  {0x12,	11136},
  {0x13,	16248},
  {0x14,	21097},
  {0x15,	25710},
  {0x16,	30109},
  {0x17,	34312},
  {0x18,	38336},
  {0x19,	42195},
  {0x1a,	45904},
  {0x1b,	49472},
  {0x1c,	52910},
  {0x1d,	56228},
  {0x1e,	59433},
  {0x1f,	62534},
  {0x20,	65536},
  {0x21,	68445},
  {0x22,	71267},
  {0x23,	74008},
  {0x24,	76672},
  {0x25,	79262},
  {0x26,	81784},
  {0x27,	84240},
  {0x28,	86633},
  {0x29,	88968},
  {0x2a,	91246},
  {0x2b,	93471},
  {0x2c,	95645},
  {0x2d,	97770},
  {0x2e,	99848},
  {0x2f,	101881},
  {0x30,	103872},
  {0x31,	105821},
  {0x32,	107731},
  {0x33,	109604},
  {0x34,	111440},
  {0x35,	113241},
  {0x36,	115008},
  {0x37,	116743},
  {0x38,	118446},
  {0x39,	120120},
  {0x3a,	121764},
  {0x3b,	123380},
  {0x3c,	124969},
  {0x3d,	126532},
  {0x3e,	128070},
  {0x3f,	129583},
  {0x40,	131072},
  {0x41,	132537},
  {0x42,	133981},
  {0x43,	135403},
  {0x44,	136803},
  {0x45,	138184},
  {0x46,	139544},
  {0x47,	140885},
  {0x48,	142208},
  {0x49,	143512},
  {0x4a,	144798},
  {0x4b,	146067},
  {0x4c,	147320},
  {0x4d,	148556},
  {0x4e,	149776},
  {0x4f,	150980},
  {0x50,	152169},
  {0x51,	153344},
  {0x52,	154504},
  {0x53,	155650},
  {0x54,	156782},
  {0x55,	157901},
  {0x56,	159007},
  {0x57,	160100},
  {0x58,	161181},
  {0x59,	162249},
  {0x5a,	163306},
  {0x5b,	164350},
  {0x5c,	165384},
  {0x5d,	166406},
  {0x5e,	167417},
  {0x5f,	168418},
  {0x60,	169408},
  {0x61,	170387},
  {0x62,	171357},
  {0x63,	172317},
  {0x64,	173267},
  {0x65,	174208},
  {0x66,	175140},
  {0x67,	176062},
  {0x68,	176976},
  {0x69,	177880},
  {0x6a,	178777},
  {0x6b,	179664},
  {0x6c,	180544},
  {0x6d,	181415},
  {0x6e,	182279},
  {0x6f,	183134},
  {0x70,	183982},
  {0x71,	184823},
  {0x72,	185656},
  {0x73,	186482},
  {0x74,	187300},
  {0x75,	188112},
  {0x76,	188916},
  {0x77,	189714},
  {0x78,	190505},
  {0x79,	191290},
  {0x7a,	192068},
  {0x7b,	192840},
  {0x7c,	193606},
  {0x7d,	194365},
  {0x7e,	195119},
  {0x7f,	195866},
  {0x80,	196608},
  {0x81,	197343},
  {0x82,	198073},
  {0x83,	198798},
  {0x84,	199517},
  {0x85,	200230},
  {0x86,	200939},
  {0x87,	201642},
  {0x88,	202339},
  {0x89,	203032},
  {0x8a,	203720},
  {0x8b,	204402},
  {0x8c,	205080},
  {0x8d,	205753},
  {0x8e,	206421},
  {0x8f,	207085},
  {0x90,	207744},
  {0x91,	208398},
  {0x92,	209048},
  {0x93,	209693},
  {0x94,	210334},
  {0x95,	210971},
  {0x96,	211603},
  {0x97,	212232},
  {0x98,	212856},
  {0x99,	213476},
  {0x9a,	214092},
  {0x9b,	214704},
  {0x9c,	215312},
  {0x9d,	215916},
  {0x9e,	216516},
  {0x9f,	217113},
  {0xa0,	217705},
  {0xa1,	218294},
  {0xa2,	218880},
  {0xa3,	219462},
  {0xa4,	220040},
  {0xa5,	220615},
  {0xa6,	221186},
  {0xa7,	221754},
  {0xa8,	222318},
  {0xa9,	222880},
  {0xaa,	223437},
  {0xab,	223992},
  {0xac,	224543},
  {0xad,	225091},
  {0xae,	225636},
  {0xaf,	226178},
  {0xb0,	226717},
  {0xb1,	227253},
  {0xb2,	227785},
  {0xb3,	228315},
  {0xb4,	228842},
  {0xb5,	229365},
  {0xb6,	229886},
  {0xb7,	230404},
  {0xb8,	230920},
  {0xb9,	231432},
  {0xba,	231942},
  {0xbb,	232449},
  {0xbc,	232953},
  {0xbd,	233455},
  {0xbe,	233954},
  {0xbf,	234450},
  {0xc0,	234944},
  {0xc1,	235435},
  {0xc2,	235923},
  {0xc3,	236410},
  {0xc4,	236893},
  {0xc5,	237374},
  {0xc6,	237853},
  {0xc7,	238329},
  {0xc8,	238803},
  {0xc9,	239275},
  {0xca,	239744},
  {0xcb,	240211},
  {0xcc,	240676},
  {0xcd,	241138},
  {0xce,	241598},
  {0xcf,	242056},
  {0xd0,	242512},
  {0xd1,	242965},
  {0xd2,	243416},
  {0xd3,	243865},
  {0xd4,	244313},
  {0xd5,	244757},
  {0xd6,	245200},
  {0xd7,	245641},
  {0xd8,	246080},
  {0xd9,	246517},
  {0xda,	246951},
  {0xdb,	247384},
  {0xdc,	247815},
  {0xdd,	248243},
  {0xde,	248670},
  {0xdf,	249095},
  {0xe0,	249518},
  {0xe1,	249939},
  {0xe2,	250359},
  {0xe3,	250776},
  {0xe4,	251192},
  {0xe5,	251606},
  {0xe6,	252018},
  {0xe7,	252428},
  {0xe8,	252836},
  {0xe9,	253243},
  {0xea,	253648},
  {0xeb,	254051},
  {0xec,	254452},
  {0xed,	254852},
  {0xee,	255250},
  {0xef,	255647},
  {0xf0,	256041},
  {0xf1,	256435},
  {0xf2,	256826},
  {0xf3,	257216},
  {0xf4,	257604},
  {0xf5,	257991},
  {0xf6,	258376},
  {0xf7,	258760},
  {0xf8,	259142},

};

struct tx_isp_sensor_attribute sp2307_attr;

unsigned int sp2307_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = sp2307_again_lut;

	while(lut->gain <= sp2307_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = lut->value;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == sp2307_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int sp2307_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute sp2307_attr={
	.name = "sp2307",
	.chip_id = 0x2308,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
	.cbus_device = 0x3c,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
		.polar = {
			.hsync_polar = 0,
			.vsync_polar = 2,
		},
	},
	.max_again = 259142,
	.max_dgain = 0,
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 0x551-1,
	.integration_time_limit = 0x551-1,
	.total_width = 0x4d2,
	.total_height = 0x551,
	.max_integration_time = 0x551-1,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = sp2307_alloc_again,
	.sensor_ctrl.alloc_dgain = sp2307_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list sp2307_init_regs_1920_1080_25fps_dvp[] = {

	{0xfd, 0x00},
	{0x1b, 0x00},
	{0x1d, 0xaa},
	{0x2f, 0x10},
	{0x30, 0x15},
	{0x33, 0x81},
	{0xfd, 0x01},
	{0x03, 0x01},
	{0x04, 0x54},
	{0x06, 0xf4},
	{0x09, 0x00},
	{0x0a, 0xab},
	{0x24, 0x10},
	{0x39, 0x08},
	{0x01, 0x01},
	{0x11, 0x60},
	{0x16, 0xe8},
	{0x19, 0xa1},
	{0x1e, 0x09},
	{0x33, 0x60},
	{0x12, 0x05},
	{0x13, 0xe0},
	{0x1a, 0xfb},
	{0x20, 0x23},
	{0x25, 0xb7},
	{0x26, 0x09},
	{0x29, 0x01},
	{0x2a, 0x8d},
	{0x2c, 0x60},
	{0x2e, 0x02},
	{0x44, 0x05},
	{0x45, 0x7e},
	{0x55, 0x12},
	{0x57, 0x20},
	{0x59, 0x97},
	{0x5a, 0xff},
	{0x5b, 0x01},
	{0x5c, 0xf0},
	{0x66, 0x33},
	{0x68, 0x32},
	{0x71, 0x3a},
	{0x73, 0x35},
	{0x7c, 0xbb},
	{0x8a, 0x66},
	{0x8b, 0x66},
	{0xc0, 0xd0},
	{0xc1, 0xd0},
	{0xc2, 0xd0},
	{0xc3, 0xd0},
	{0xc4, 0x3c},
	{0xc5, 0x3c},
	{0xc6, 0x3c},
	{0xc7, 0x3c},
	{0xfb, 0x5b},
	{0xf0, 0x00},
	{0xf1, 0x00},
	{0xf2, 0x00},
	{0xf3, 0x00},
	{0xf5, 0x80},
	{0xa1, 0x04},
	{0x96, 0x1b},
	{0xfd, 0x02},
	{0x34, 0xff},
	{0x60, 0xff},
	{0xfd, 0x02},
	{0xa3, 0x38},
	{0xa7, 0xc0},
	{0xc0, 0x40},
	{0xc1, 0x40},
	{0xc2, 0x40},
	{0xc3, 0x40},
	{0xc5, 0x40},
	{0xc6, 0x40},
	{0xc7, 0x40},
	{0xc8, 0x40},
	{0xca, 0x40},
	{0xcb, 0x40},
	{0xcc, 0x40},
	{0xcd, 0x40},
	{0xcf, 0x40},
	{0xd0, 0x40},
	{0xd1, 0x40},
	{0xd2, 0x40},
	{0xfd, 0x01},

	{SP2307_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sp2307_init_regs_1920_1080_15fps_dvp[] = {

	{SP2307_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the sp2307_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sp2307_win_sizes[] = {
	/* 1920*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= sp2307_init_regs_1920_1080_25fps_dvp,
	}
};

static enum v4l2_mbus_pixelcode sp2307_mbus_code[] = {
	V4L2_MBUS_FMT_SBGGR10_1X10,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list sp2307_stream_on[] = {

	{SP2307_REG_END, 0x00},
};

static struct regval_list sp2307_stream_off[] = {

	{SP2307_REG_END, 0x00},
};

int sp2307_read(struct v4l2_subdev *sd, unsigned char reg,
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

static int sp2307_write(struct v4l2_subdev *sd, unsigned char reg,
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

static int sp2307_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SP2307_REG_END) {
		if (vals->reg_num == SP2307_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = sp2307_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			if (vals->reg_num == SP2307_PAGE_REG){
				val &= 0xf8;
				val |= (vals->value & 0x07);
				ret = sp2307_write(sd, vals->reg_num, val);
				ret = sp2307_read(sd, vals->reg_num, &val);
			}
		}
		vals++;
	}
	return 0;
}
static int sp2307_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SP2307_REG_END) {
		if (vals->reg_num == SP2307_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = sp2307_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int sp2307_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sp2307_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = sp2307_read(sd, 0x02, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SP2307_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = sp2307_read(sd, 0x03, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SP2307_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;

	return 0;
}

static int sp2307_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret = sp2307_write(sd, 0xfd, 0x01);
	ret += sp2307_write(sd, 0x04, (unsigned char)(value & 0xff));
	ret += sp2307_write(sd, 0x03, (unsigned char)((value & 0xff00) >> 8));
	ret += sp2307_write(sd, 0x01, 0x01);
	if (ret < 0) {
		printk("sp2307_write error  %d\n" ,__LINE__);
		return ret;
	}
	return 0;

}

static int sp2307_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret = sp2307_write(sd, 0xfd, 0x01);
	ret += sp2307_write(sd, 0x24, (unsigned char)value);
	ret += sp2307_write(sd, 0x01, 0x01);
	if (ret < 0){
		printk("sp2307_write error  %d\n" ,__LINE__ );
		return ret;
	}
	return 0;
}

static int sp2307_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sp2307_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sp2307_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &sp2307_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;

	switch (sensor_max_fps) {
	case TX_SENSOR_MAX_FPS_25:
		wsize->regs = sp2307_init_regs_1920_1080_25fps_dvp;
		wsize->fps = 25 << 16 | 1;
		break;
	case TX_SENSOR_MAX_FPS_15:
		wsize->regs = sp2307_init_regs_1920_1080_15fps_dvp;
		wsize->fps = 15 << 16 | 1;
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
	ret = sp2307_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	/*ret = sp2307_read_array(sd, wsize->regs);
	if (ret)
		return ret;*/
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int sp2307_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sp2307_write_array(sd, sp2307_stream_on);
		pr_debug("sp2307 stream on\n");
	}
	else {
		ret = sp2307_write_array(sd, sp2307_stream_off);
		pr_debug("sp2307 stream off\n");
	}
	return ret;
}

static int sp2307_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sp2307_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sp2307_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	unsigned int sclk = 0;
	unsigned int hts = 0;
	unsigned int vts = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	unsigned int max_fps = 0; //the format is 24.8
	int ret = 0;

	switch (sensor_max_fps) {
	case TX_SENSOR_MAX_FPS_25:
		sclk = SP2307_SUPPORT_SCLK_FPS_30;
		max_fps = SENSOR_OUTPUT_MAX_FPS;
		break;
	case TX_SENSOR_MAX_FPS_15:
		sclk = SP2307_SUPPORT_SCLK_FPS_15;
		max_fps = TX_SENSOR_MAX_FPS_15;
		break;
	default:
		printk("Now we do not support this framerate!!!\n");
	}

	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}

	ret = sp2307_write(sd, 0xfd, 0x01);
	if(ret < 0)
		return ret;
	ret = sp2307_read(sd, 0x8c, &tmp);
	hts = tmp;
	ret += sp2307_read(sd, 0x8d, &tmp);
	if (0 != ret) {
		printk("err: sp2307_write err\n");
		return ret;
	}
	hts = (hts << 8) + tmp;
	vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret = 0;
	ret += sp2307_write(sd, 0xfd, 0x01);
	ret += sp2307_write(sd, 0x0d, 0x10);//frame_exp_seperate_en
	ret += sp2307_write(sd, 0x0e, (vts >> 8) & 0xff);
	ret += sp2307_write(sd, 0x0f, vts & 0xff);
	ret += sp2307_write(sd, 0x01, 0x01);
	if (0 != ret) {
		printk("err: sp2307_write err\n");
		return ret;
	}
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts -1;
	sensor->video.attr->integration_time_limit = vts - 1;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 1;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return ret;
}

static int sp2307_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &sp2307_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &sp2307_win_sizes[0];
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
static int sp2307_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"sp2307_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(10);
			gpio_direction_output(pwdn_gpio, 0);
			msleep(10);
			gpio_direction_output(pwdn_gpio, 1);
			msleep(10);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"sp2307_reset");
		if(!ret){
		gpio_direction_output(reset_gpio, 1);
			msleep(10);
			gpio_direction_output(reset_gpio, 0);
			msleep(10);
			gpio_direction_output(reset_gpio, 1);
			msleep(15);

		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}

	ret = sp2307_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sp2307 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "sp2307 chip found @ 0x%02x (%s)\n",
		client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sp2307_s_power(struct v4l2_subdev *sd, int on)
{

	return 0;
}
static long sp2307_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = sp2307_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = sp2307_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = sp2307_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = sp2307_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = sp2307_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
				ret = sp2307_write_array(sd, sp2307_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
				ret = sp2307_write_array(sd, sp2307_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = sp2307_set_fps(sensor, ctrl->value);
			break;
		default:
			pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
			break;
	}
	return 0;
}
static long sp2307_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = sp2307_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sp2307_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = sp2307_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int sp2307_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	sp2307_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops sp2307_core_ops = {
	.g_chip_ident = sp2307_g_chip_ident,
	.reset = sp2307_reset,
	.init = sp2307_init,
	.s_power = sp2307_s_power,
	.ioctl = sp2307_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sp2307_g_register,
	.s_register = sp2307_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sp2307_video_ops = {
	.s_stream = sp2307_s_stream,
	.s_parm = sp2307_s_parm,
	.g_parm = sp2307_g_parm,
};

static const struct v4l2_subdev_ops sp2307_ops = {
	.core = &sp2307_core_ops,
	.video = &sp2307_video_ops,
};

static int sp2307_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &sp2307_win_sizes[0];
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

#if 0
	sp2307_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = sp2307_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = sp2307_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(sp2307_win_sizes); i++)
		sp2307_win_sizes[i].mbus_code = mbus;

#endif

	switch (sensor_max_fps) {
	case TX_SENSOR_MAX_FPS_25:
		break;
	case TX_SENSOR_MAX_FPS_15:
		sp2307_attr.max_integration_time_native = 0x76b - 4;
		sp2307_attr.integration_time_limit = 0x76b - 4;
		sp2307_attr.total_width = 0x83a;
		sp2307_attr.total_height = 0x76b;
		sp2307_attr.max_integration_time = 0x76b - 4;
		break;
	default:
		printk("Now we do not support this framerate!!!\n");
	}
	sp2307_attr.max_again = 259142;
	sp2307_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &sp2307_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &sp2307_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("probe ok ------->sp2307\n");
	return 0;

err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int sp2307_remove(struct i2c_client *client)
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

static const struct i2c_device_id sp2307_id[] = {
	{ "sp2307", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sp2307_id);

static struct i2c_driver sp2307_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "sp2307",
	},
	.probe		= sp2307_probe,
	.remove		= sp2307_remove,
	.id_table	= sp2307_id,
};

static __init int init_sp2307(void)
{
	return i2c_add_driver(&sp2307_driver);
}

static __exit void exit_sp2307(void)
{
	i2c_del_driver(&sp2307_driver);
}

module_init(init_sp2307);
module_exit(exit_sp2307);

MODULE_DESCRIPTION("A low-level driver for Superpix sp2307 sensors");
MODULE_LICENSE("GPL");
