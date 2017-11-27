/*
 * ov2710.c
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

#define OV2710_CHIP_ID_H	(0x27)
#define OV2710_CHIP_ID_L	(0x10)
#define OV2710_REG_END		0xffff
#define OV2710_REG_DELAY	0xfffe
#define OV2710_SUPPORT_SCLK (80150400)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
#define DRIVE_CAPABILITY_2

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
struct again_lut ov2710_again_lut[] = {
	{0x0, 0},
	{0x1, 5731},
	{0x2, 11136},
	{0x3, 16248},
	{0x4, 21097},
	{0x5, 25710},
	{0x6, 30109},
	{0x7, 34312},
	{0x8, 38336},
	{0x9, 42195},
	{0xa, 45904},
	{0xb, 49472},
	{0xc, 52910},
	{0xd, 56228},
	{0xe, 59433},
	{0xf, 62534},
	{0x10, 65536},
	{0x11, 71267},
	{0x12, 76672},
	{0x13, 81784},
	{0x14, 86633},
	{0x15, 91246},
	{0x16, 95645},
	{0x17, 99848},
	{0x18, 103872},
	{0x19, 107731},
	{0x1a, 111440},
	{0x1b, 115008},
	{0x1c, 118446},
	{0x1d, 121764},
	{0x1e, 124969},
	{0x1f, 128070},
	{0x30, 131072},
	{0x31, 136803},
	{0x32, 142208},
	{0x33, 147320},
	{0x34, 152169},
	{0x35, 156782},
	{0x36, 161181},
	{0x37, 165384},
	{0x38, 169408},
	{0x39, 173267},
	{0x3a, 176976},
	{0x3b, 180544},
	{0x3c, 183982},
	{0x3d, 187300},
	{0x3e, 190505},
	{0x3f, 193606},
	{0x70, 196608},
	{0x71, 202339},
	{0x72, 207744},
	{0x73, 212856},
	{0x74, 217705},
	{0x75, 222318},
	{0x76, 226717},
	{0x77, 230920},
	{0x78, 234944},
	{0x79, 238803},
	{0x7a, 242512},
	{0x7b, 246080},
	{0x7c, 249518},
	{0x7d, 252836},
	{0x7e, 256041},
	{0x7f, 259142},
	{0xf0, 262144},
	{0xf1, 267875},
	{0xf2, 273280},
	{0xf3, 278392},
	{0xf4, 283241},
	{0xf5, 287854},
	{0xf6, 292253},
	{0xf7, 296456},
	{0xf8, 300480},
	{0xf9, 304339},
	{0xfa, 308048},
	{0xfb, 311616},
	{0xfc, 315054},
	{0xfd, 318372},
	{0xfe, 321577},
	{0xff, 324678},
	{0x1f0, 327680},
	{0x1f1, 333411},
	{0x1f2, 338816},
	{0x1f3, 343928},
	{0x1f4, 348777},
	{0x1f5, 353390},
	{0x1f6, 357789},
	{0x1f7, 361992},
	{0x1f8, 366016},
	{0x1f9, 369875},
	{0x1fa, 373584},
	{0x1fb, 377152},
	{0x1fc, 380590},
	{0x1fd, 383908},
	{0x1fe, 387113},
	{0x1ff, 390214},
};

struct tx_isp_sensor_attribute ov2710_attr;

unsigned int ov2710_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = ov2710_again_lut;

	while(lut->gain <= ov2710_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = 0;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == ov2710_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;

}

unsigned int ov2710_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_mipi_bus ov2710_mipi={
	.clk = 800,
	.lans = 1,
};
struct tx_isp_dvp_bus ov2710_dvp={
	.mode = SENSOR_DVP_HREF_MODE,
	.blanking = {
		.vblanking = 0,
		.hblanking = 0,
	},
};

struct tx_isp_sensor_attribute ov2710_attr={
	.name = "ov2710",
	.chip_id = 0x2710,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
	.cbus_device = 0x36,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 390214,
	.max_dgain = 0,
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 0x52c-4,
	.integration_time_limit = 0x52c-4,
	.total_width = 0x974,
	.total_height = 0x52c,
	.max_integration_time = 0x52c-4,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 1,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = ov2710_alloc_again,
	.sensor_ctrl.alloc_dgain = ov2710_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};


static struct regval_list ov2710_init_regs_1920_1080_30fps_mipi[] = {

#if 0 //5fps 400Mbps
{0x3103, 0x93},
{0x3008, 0x82},
{0x3017, 0x7f},
{0x3018, 0xfc},
{0x3706, 0x61},
{0x3712, 0x0c},
{0x3630, 0x6d},
{0x3801, 0xb4},
{0x3621, 0x04},
{0x3604, 0x60},
{0x3603, 0xa7},
{0x3631, 0x26},
{0x3600, 0x04},
{0x3620, 0x37},
{0x3623, 0x00},
{0x3702, 0x9e},
{0x3703, 0x5c},
{0x3704, 0x40},
{0x370d, 0x0f},
{0x3713, 0x9f},
{0x3714, 0x4c},
{0x3710, 0x9e},
{0x3801, 0xc4},
{0x3605, 0x05},
{0x3606, 0x3f},
{0x302d, 0x90},
{0x370b, 0x40},
{0x3716, 0x31},
{0x3707, 0x52},
{0x380d, 0x74},
{0x5181, 0x20},
{0x518f, 0x00},
{0x4301, 0xff},
{0x4303, 0x00},
{0x3a00, 0x78},
{0x300f, 0x88},
{0x3011, 0x28},
{0x3a1a, 0x06},
{0x3a18, 0x00},
{0x3a19, 0x7a},
{0x3a13, 0x54},
{0x382e, 0x0f},
{0x381a, 0x1a},
{0x401d, 0x02},
{0x5688, 0x03},
{0x5684, 0x07},
{0x5685, 0xa0},
{0x5686, 0x04},
{0x5687, 0x43},
{0x3011, 0x05},
{0x300f, 0x8a},
{0x3017, 0x00},
{0x3018, 0x00},
{0x300e, 0x04},
{0x4801, 0x0f},
{0x300f, 0xc3},
{0x3a0f, 0x40},
{0x3a10, 0x38},
{0x3a1b, 0x48},
{0x3a1e, 0x30},
{0x3a11, 0x90},
{0x3a1f, 0x10},
{0x380c, 0x09},
{0x380d, 0x74},
{0x380e, 0x0c},
{0x380f, 0xf0},
#endif

#if 0 //15fps 800Mbps
{0x3103, 0x93},
{0x3008, 0x82},
{0x3017, 0x7f},
{0x3018, 0xfc},
{0x3706, 0x61},
{0x3712, 0x0c},
{0x3630, 0x6d},
{0x3801, 0xb4},
{0x3621, 0x04},
{0x3604, 0x60},
{0x3603, 0xa7},
{0x3631, 0x26},
{0x3600, 0x04},
{0x3620, 0x37},
{0x3623, 0x00},
{0x3702, 0x9e},
{0x3703, 0x5c},
{0x3704, 0x40},
{0x370d, 0x0f},
{0x3713, 0x9f},
{0x3714, 0x4c},
{0x3710, 0x9e},
{0x3801, 0xc4},
{0x3605, 0x05},
{0x3606, 0x3f},
{0x302d, 0x90},
{0x370b, 0x40},
{0x3716, 0x31},
{0x3707, 0x52},
{0x380d, 0x74},
{0x5181, 0x20},
{0x518f, 0x00},
{0x4301, 0xff},
{0x4303, 0x00},
{0x3a00, 0x78},
{0x300f, 0x88},
{0x3011, 0x28},
{0x3a1a, 0x06},
{0x3a18, 0x00},
{0x3a19, 0x7a},
{0x3a13, 0x54},
{0x382e, 0x0f},
{0x381a, 0x1a},
{0x401d, 0x02},
{0x5688, 0x03},
{0x5684, 0x07},
{0x5685, 0xa0},
{0x5686, 0x04},
{0x5687, 0x43},
{0x3011, 0x05},
{0x300f, 0x8a},
{0x3017, 0x00},
{0x3018, 0x00},
{0x300e, 0x04},
{0x4801, 0x0f},
{0x300f, 0xc3},
{0x3a0f, 0x40},
{0x3a10, 0x38},
{0x3a1b, 0x48},
{0x3a1e, 0x30},
{0x3a11, 0x90},
{0x3a1f, 0x10},
{0x380c, 0x09},
{0x380d, 0x74},
{0x380e, 0x04},
{0x380f, 0x50},
#endif

#if 1 //30fps 800Mbps
{0x3103, 0x93},
{0x3008, 0x82},
{0x3008, 0x42},
{0x3017, 0x7f},
{0x3018, 0xfc},
{0x3706, 0x61},
{0x3712, 0x0c},
{0x3630, 0x6d},
{0x3801, 0xb4},
{0x3621, 0x04},
{0x3604, 0x60},
{0x3603, 0xa7},
{0x3631, 0x26},
{0x3600, 0x04},
{0x3620, 0x37},
{0x3623, 0x00},
{0x3702, 0x9e},
{0x3703, 0x5c},
{0x3704, 0x40},
{0x370d, 0x0f},
{0x3713, 0x9f},
{0x3714, 0x4c},
{0x3710, 0x9e},
{0x3801, 0xc4},
{0x3605, 0x05},
{0x3606, 0x3f},
{0x302d, 0x90},
{0x370b, 0x40},
{0x3716, 0x31},
{0x3707, 0x52},
{0x380d, 0x74},
{0x5181, 0x20},
{0x518f, 0x00},
{0x4301, 0xff},
{0x4303, 0x00},
{0x3a00, 0x00},
{0x300f, 0x88},
{0x3011, 0x28},
{0x3a1a, 0x06},
{0x3a18, 0x00},
{0x3a19, 0x7a},
{0x3a13, 0x54},
{0x382e, 0x0f},
{0x381a, 0x1a},
{0x401d, 0x02},
{0x5688, 0x03},
{0x5684, 0x07},
{0x5685, 0xa0},
{0x5686, 0x04},
{0x5687, 0x43},
{0x3011, 0x0a},
{0x300f, 0x8a},
{0x3017, 0x00},
{0x3018, 0x00},
{0x300e, 0x04},
{0x300f, 0xc3},
{0x4801, 0x0f},
{0x3a0f, 0x40},
{0x3a10, 0x38},
{0x3a1b, 0x48},
{0x3a1e, 0x30},
{0x3a11, 0x90},
{0x3a1f, 0x10},
{0x3503, 0x03},
{0x3501, 0x2e},
{0x3502, 0x00},
{0x350b, 0x10},
{0x5001, 0x4e},
{0x3406, 0x01},
{0x3400, 0x04},
{0x3401, 0x00},
{0x3402, 0x04},
{0x3403, 0x00},
{0x3404, 0x04},
{0x3405, 0x00},
{0x4000, 0x05},
{0x302c, 0x00},
{0x5000, 0x5f},
{0x3008, 0x02},
{0x380c, 0x09},
{0x380d, 0x74},
{0x380e, 0x05},
{0x380f, 0x2c},
#endif

{0x3103, 0x93},
{0x4800, 0x20},
{0x3000, 0x02},
{0x3008, 0x02},

/* {0x503d, 0x80}, //[7] [5:4] color bar */

{OV2710_REG_END, 0x00},	/* END MARKER */
};


static struct regval_list ov2710_init_regs_1920_1080_30fps_dvp[] = {
#if 0
	{0x302c,}, //drive capability [7:6]
	{0x3212,}, //Group Access
	{0x503d,}, //[7] [5:4]
	{0x3503,}, //[1]agc manual [0]aec manual
#endif

	{0x3103, 0x93},
	{0x3008, 0x82},
	{0x3017, 0x7f},
	{0x3018, 0xfc},
	{0x3706, 0x61},
	{0x3712, 0x0c},
	{0x3630, 0x6d},
	{0x3801, 0xb4},
	{0x3621, 0x04},
	{0x3604, 0x60},
	{0x3603, 0xa7},
	{0x3631, 0x26},
	{0x3600, 0x04},
	{0x3620, 0x37},
	{0x3623, 0x00},
	{0x3702, 0x9e},
	{0x3703, 0x5c},
	{0x3704, 0x40},
	{0x370d, 0x0f},
	{0x3713, 0x9f},
	{0x3714, 0x4c},
	{0x3710, 0x9e},
	{0x3801, 0xc4},
	{0x3605, 0x05},
	{0x3606, 0x3f},
	{0x302d, 0x90},
	{0x370b, 0x40},
	{0x3716, 0x31},
	{0x3707, 0x52},
	{0x380d, 0x74},
	{0x5181, 0x20},
	{0x518f, 0x00},
	{0x4301, 0xff},
	{0x4303, 0x00},
	{0x3a00, 0x78},
	{0x300f, 0x88},
	{0x3011, 0x28},
	{0x3a1a, 0x06},
	{0x3a18, 0x00},
	{0x3a19, 0x7a},
	{0x3a13, 0x54},
	{0x382e, 0x0f},
	{0x381a, 0x1a},
	{0x401d, 0x02},
	{0x5688, 0x03},
	{0x5684, 0x07},
	{0x5685, 0xa0},
	{0x5686, 0x04},
	{0x5687, 0x43},
	{0x3a0f, 0x40},
	{0x3a10, 0x38},
	{0x3a1b, 0x48},
	{0x3a1e, 0x30},
	{0x3a11, 0x90},
	{0x3a1f, 0x10},

	/* add by Huddy */
	/* {0x503d, 0x80}, //[7] [5:4] color bar */
#ifdef DRIVE_CAPABILITY_1
	{0x302c,0x00},
#elif defined(DRIVE_CAPABILITY_2)
	{0x302c,0x40},
#elif defined(DRIVE_CAPABILITY_3)
	{0x302c,0x80},
#elif defined(DRIVE_CAPABILITY_4)
	{0x302c,0xc0},
#endif
	{0x3503, 0x07}, //Manual,AEC[0],AGC[1],VTS manual enable[2]
	{0x3500, 0x00}, //integration time
	{0x3501, 0x03},
	{0x3502, 0x03},
	{0x350a, 0x00}, //Gain
	{0x350b, 0x00},
	{0x4000, 0x01}, //BLC[0]
	{0x4006, 0x00}, //target
	{0x4007, 0x10}, //target
	{0x401d, 0x22}, //BLC frame control
	/* /\* {0x380c, 0x09}, *\/ //HTS */
	/* /\* {0x380d, 0x74}, *\/  */
	/* /\* {0x380e, 0x04}, *\/ //VTS */
	/* /\* {0x380f, 0x50}, *\/ */
	{0x5000, 0x00}, //lenc & dpc
	{0x5001, 0x00}, //awb
	/* {0x5002, 0x00}, //vap:not to change it, will be Broken Screen */
	/* {0x5003, 0x00}, //?? */
	/* {0x5004, 0x00}, //??:not to change it, will be Broken Screen */
	/* {0x5005, 0x00}, //awb bias */

#if 0 //1080p@10fps
	{0x380c, 0x09},
	{0x380d, 0x74},
	{0x380e, 0x04},
	{0x380f, 0x50},
	{0x3010, 0x20},
#endif
#if 0 //1080p@30fps
	{0x380c, 0x09},
	{0x380d, 0x74},
	{0x380e, 0x04},
	{0x380f, 0x50},
	{0x3010, 0x00},
#endif

#if 1 //1080p@25fps
	{0x380c, 0x09},
	{0x380d, 0x74},
	{0x380e, 0x05},
	{0x380f, 0x2c},
	{0x3010, 0x00},
#endif

	{0x4202, 0x0f},

	{OV2710_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the ov2710_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ov2710_win_sizes[] = {
	/* 1280*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov2710_init_regs_1920_1080_30fps_dvp,
	}
};

static enum v4l2_mbus_pixelcode ov2710_mbus_code[] = {
	V4L2_MBUS_FMT_SBGGR10_1X10,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ov2710_stream_on_dvp[] = {
	{0x4202, 0x00},
	{OV2710_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov2710_stream_off_dvp[] = {
	{0x4202, 0x0f},
	{OV2710_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov2710_stream_on_mipi[] = {

	{0x4800, 0x00},
	{0x3000, 0x00},
	{0x4202, 0x00},
	/* {0x0100, 0x01}, */
	{OV2710_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov2710_stream_off_mipi[] = {
	{0x4202, 0x0f},
	/*{0x0100, 0x00},*/
	{OV2710_REG_END, 0x00},	/* END MARKER */
};

int ov2710_read(struct v4l2_subdev *sd, uint16_t reg,
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

int ov2710_write(struct v4l2_subdev *sd, uint16_t reg,
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

static int ov2710_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV2710_REG_END) {
		if (vals->reg_num == OV2710_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov2710_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}
static int ov2710_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV2710_REG_END) {
		if (vals->reg_num == OV2710_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov2710_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int ov2710_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int ov2710_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = ov2710_read(sd, 0x300a, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != OV2710_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = ov2710_read(sd, 0x300b, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != OV2710_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int ov2710_set_integration_time(struct v4l2_subdev *sd, int value)
{

	int ret = 0;
	/* low 4 bits are fraction bits */
	unsigned int expo = value;

	ret += ov2710_write(sd, 0x3212, 0x01);
	ret = ov2710_write(sd, 0x3502, ((unsigned char)(expo & 0xf)) << 4);
	ret += ov2710_write(sd, 0x3501, (unsigned char)((expo >> 4) & 0xff));
	ret += ov2710_write(sd, 0x3500, (unsigned char)((expo >> 12) & 0xf));
	ret += ov2710_write(sd, 0x3212, 0x11);
	ret += ov2710_write(sd, 0x3212, 0xa1);
	if (ret < 0)
		return ret;
	return 0;

}

static int ov2710_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	ret = ov2710_write(sd, 0x3212, 0x02);
	ret += ov2710_write(sd, 0x350b, (unsigned char)(value & 0xff));
	ret += ov2710_write(sd, 0x350a, (unsigned char)((value>>8) & 0xff));
	ret += ov2710_write(sd, 0x3212, 0x12);
	ret += ov2710_write(sd, 0x3212, 0xa2);
	if (ret < 0)
		return ret;
	return 0;
}

static int ov2710_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ov2710_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ov2710_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &ov2710_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = ov2710_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int ov2710_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	int val = 0;
	unsigned char value = 0;

	if (enable) {
		if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
			ret = ov2710_write_array(sd, ov2710_stream_on_dvp);
		} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
			ret = ov2710_write_array(sd, ov2710_stream_on_mipi);

		}else{
			printk("Don't support this Sensor Data interface\n");
		}
		pr_debug("ov2710 stream on\n");

	}
	else {
		if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
			ret = ov2710_write_array(sd, ov2710_stream_off_dvp);
		} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
			ret = ov2710_write_array(sd, ov2710_stream_off_mipi);

		}else{
			printk("Don't support this Sensor Data interface\n");
		}
		pr_debug("ov2710 stream off\n");
	}
	return ret;
}

static int ov2710_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov2710_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov2710_set_fps(struct tx_isp_sensor *sensor, int fps)
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
	sclk = OV2710_SUPPORT_SCLK;

	val = 0;
	ret += ov2710_read(sd, 0x380c, &val);
	hts = val<<8;
	val = 0;
	ret += ov2710_read(sd, 0x380d, &val);
	hts |= val;
	if (0 != ret) {
		printk("err: ov2710 read err\n");
		return ret;
	}

	vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

	ret += ov2710_write(sd, 0x3212, 0x00);
	ret += ov2710_write(sd, 0x380f, vts & 0xff);
	ret += ov2710_write(sd, 0x380e, (vts >> 8) & 0xff);
	ret += ov2710_write(sd, 0x3212, 0x10);
	ret += ov2710_write(sd, 0x3212, 0xa0);
	/* ret += ov2710_write(sd, 0x3208, 0xa0); */
	if (0 != ret) {
		printk("err: ov2710_write err\n");
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

static int ov2710_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &ov2710_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &ov2710_win_sizes[0];
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
static int ov2710_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"ov2710_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(10);
			gpio_direction_output(reset_gpio, 0);
			msleep(10);
			gpio_direction_output(reset_gpio, 1);
			msleep(30);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"ov2710_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = ov2710_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an ov2710 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "ov2710 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ov2710_s_power(struct v4l2_subdev *sd, int on)
{

	return 0;
}
static long ov2710_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = ov2710_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = ov2710_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = ov2710_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = ov2710_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = ov2710_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
				ret = ov2710_write_array(sd, ov2710_stream_off_dvp);
			} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = ov2710_write_array(sd, ov2710_stream_off_mipi);

			}else{
				printk("Don't support this Sensor Data interface\n");
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
				ret = ov2710_write_array(sd, ov2710_stream_on_dvp);
			} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = ov2710_write_array(sd, ov2710_stream_on_mipi);

			}else{
				printk("Don't support this Sensor Data interface\n");
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = ov2710_set_fps(sensor, ctrl->value);
			break;
		default:
			pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
			break;
	}
	return 0;
}
static long ov2710_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = ov2710_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov2710_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov2710_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov2710_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov2710_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops ov2710_core_ops = {
	.g_chip_ident = ov2710_g_chip_ident,
	.reset = ov2710_reset,
	.init = ov2710_init,
	.s_power = ov2710_s_power,
	.ioctl = ov2710_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov2710_g_register,
	.s_register = ov2710_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov2710_video_ops = {
	.s_stream = ov2710_s_stream,
	.s_parm = ov2710_s_parm,
	.g_parm = ov2710_g_parm,
};

static const struct v4l2_subdev_ops ov2710_ops = {
	.core = &ov2710_core_ops,
	.video = &ov2710_video_ops,
};

static int ov2710_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &ov2710_win_sizes[0];
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

	ov2710_attr.dbus_type = data_interface;
	if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
		wsize->regs = ov2710_init_regs_1920_1080_30fps_dvp;
		memcpy((void*)(&(ov2710_attr.dvp)),(void*)(&ov2710_dvp),sizeof(ov2710_dvp));
	} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
		wsize->regs = ov2710_init_regs_1920_1080_30fps_mipi;
		memcpy((void*)(&(ov2710_attr.mipi)),(void*)(&ov2710_mipi),sizeof(ov2710_mipi));
	} else{
		printk("Don't support this Sensor Data Output Interface.\n");
		goto err_set_sensor_data_interface;
	}
#if 0
	ov2710_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = ov2710_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = ov2710_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(ov2710_win_sizes); i++)
		ov2710_win_sizes[i].mbus_code = mbus;

#endif
	 /*
		convert sensor-gain into isp-gain,
	 */
	ov2710_attr.max_again = 390214;
	ov2710_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &ov2710_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &ov2710_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("probe ok ------->ov2710\n");
	return 0;
err_set_sensor_data_interface:
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int ov2710_remove(struct i2c_client *client)
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

static const struct i2c_device_id ov2710_id[] = {
	{ "ov2710", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2710_id);

static struct i2c_driver ov2710_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ov2710",
	},
	.probe		= ov2710_probe,
	.remove		= ov2710_remove,
	.id_table	= ov2710_id,
};

static __init int init_ov2710(void)
{
	return i2c_add_driver(&ov2710_driver);
}

static __exit void exit_ov2710(void)
{
	i2c_del_driver(&ov2710_driver);
}

module_init(init_ov2710);
module_exit(exit_ov2710);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov2710 sensors");
MODULE_LICENSE("GPL");
