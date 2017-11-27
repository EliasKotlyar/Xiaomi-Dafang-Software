/*
 * ov9750.c
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

#define OV9750_CHIP_ID_H	(0x97)
#define OV9750_CHIP_ID_L	(0x50)

#define OV9750_REG_END		0xffff
#define OV9750_REG_DELAY	0xfffe

#define OV9750_SUPPORT_PCLK (48*1000*1000)
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

struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */

struct tx_isp_sensor_attribute ov9750_attr;

unsigned int ov9750_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	unsigned int gain_one = 0;
	unsigned int gain_one1 = 0;
	uint16_t regs = 0;
	unsigned int isp_gain1 = 0;
	uint32_t mask;
	/* low 7 bits are fraction bits, max again 15.5x */
	gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
	if (gain_one >= (uint32_t)(0x7c0<<(TX_ISP_GAIN_FIXED_POINT-7)))
		gain_one = (uint32_t)(0x7c0<<(TX_ISP_GAIN_FIXED_POINT-7));
	regs = gain_one>>(TX_ISP_GAIN_FIXED_POINT-7);
	mask = ~0;
	mask = mask >> (TX_ISP_GAIN_FIXED_POINT-7);
	mask = mask << (TX_ISP_GAIN_FIXED_POINT-7);
	gain_one1 = gain_one&mask;
	isp_gain1 = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
	*sensor_again = regs;
	/* printk("info:  isp_gain = 0x%08x, isp_gain1 = 0x%08x, gain_one = 0x%08x, gain_one1 = 0x%08x, sensor_again = 0x%08x\n", isp_gain, isp_gain1, gain_one, gain_one1, regs); */
	return isp_gain1;
}

unsigned int ov9750_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return 0;
}

struct tx_isp_sensor_attribute ov9750_attr={
	.name = "ov9750",
	.chip_id = 0x9750,
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
	.max_again = 0x3f446,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 0x3dc-4,
	.integration_time_limit = 0x3dc-4,
	.total_width = 0x797,
	.total_height = 0x3dc,
	.max_integration_time = 0x3dc-4,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = ov9750_alloc_again,
	.sensor_ctrl.alloc_dgain = ov9750_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};


static struct regval_list ov9750_init_regs_1280_960_25fps[] = {
#if 1
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x0300, 0x02},
	{0x0302, 0x50},
	{0x0303, 0x01},
	{0x0304, 0x01},
	{0x0305, 0x01},
	{0x0306, 0x01},
	{0x030a, 0x00},
	{0x030b, 0x00},
	{0x030d, 0x1e},
	{0x030e, 0x01},
	{0x030f, 0x04},
	{0x0312, 0x01},
	{0x031e, 0x04},
	{0x3000, 0x0f},
	{0x3001, 0xff},
	{0x3002, 0xe1},
	{0x3005, 0xf0},
	{0x3009, 0x00},
	{0x300f, 0x00},
#ifdef DRIVE_CAPABILITY_1
	{0x3011,0x00},
#elif defined(DRIVE_CAPABILITY_2)
	{0x3011,0x20},
#elif defined(DRIVE_CAPABILITY_3)
	{0x3011,0x40},
#elif defined(DRIVE_CAPABILITY_4)
	{0x3011,0x60},
#endif
	{0x3016, 0x00},
	{0x3018, 0x32},
	{0x301a, 0xf0},
	{0x301b, 0xf0},
	{0x301c, 0xf0},
	{0x301d, 0xf0},
	{0x301e, 0xf0},
	{0x3022, 0x21},
	{0x3031, 0x0a},
	{0x3032, 0x80},
	{0x303c, 0xff},
	{0x303e, 0xff},
	{0x3040, 0xf0},
	{0x3041, 0x00},
	{0x3042, 0xf0},
	{0x3104, 0x01},
	{0x3106, 0x15},
	{0x3107, 0x01},
	{0x3500, 0x00},
	{0x3501, 0x02},
	{0x3502, 0x00},
	{0x3503, 0x08},
	{0x3504, 0x03},
	{0x3505, 0x83},
	{0x3508, 0x00},
	{0x3509, 0x80},
	{0x3600, 0x65},
	{0x3601, 0x60},
	{0x3602, 0x32},
	{0x3610, 0xe8},
	{0x3611, 0x56},
	{0x3612, 0x48},
	{0x3613, 0x5a},
	{0x3614, 0x96},
	{0x3615, 0x79},
	{0x3617, 0x07},
	{0x3620, 0x84},
	{0x3621, 0x90},
	{0x3622, 0x00},
	{0x3623, 0x00},
	{0x3633, 0x10},
	{0x3634, 0x10},
	{0x3635, 0x14},
	{0x3636, 0x13},
	{0x3650, 0x00},
	{0x3652, 0xff},
	{0x3654, 0x20},
	{0x3653, 0x34},
	{0x3655, 0x20},
	{0x3656, 0xff},
	{0x3657, 0xc4},
	{0x365a, 0xff},
	{0x365b, 0xff},
	{0x365e, 0xff},
	{0x365f, 0x00},
	{0x3668, 0x00},
	{0x366a, 0x07},
	{0x366d, 0x00},
	{0x366e, 0x10},
	{0x3700, 0x14},
	{0x3701, 0x08},
	{0x3702, 0x1d},
	{0x3703, 0x10},
	{0x3704, 0x14},
	{0x3705, 0x00},
	{0x3706, 0x48},
	{0x3707, 0x04},
	{0x3708, 0x12},
	{0x3709, 0x24},
	{0x370a, 0x00},
	{0x370b, 0xfa},
	{0x370c, 0x04},
	{0x370d, 0x00},
	{0x370e, 0x00},
	{0x370f, 0x05},
	{0x3710, 0x18},
	{0x3711, 0x0e},
	{0x3714, 0x24},
	{0x3718, 0x13},
	{0x371a, 0x5e},
	{0x3720, 0x05},
	{0x3721, 0x05},
	{0x3726, 0x06},
	{0x3728, 0x05},
	{0x372a, 0x03},
	{0x372b, 0x53},
	{0x372c, 0x53},
	{0x372d, 0x53},
	{0x372e, 0x06},
	{0x372f, 0x10},
	{0x3730, 0x82},
	{0x3731, 0x06},
	{0x3732, 0x14},
	{0x3733, 0x10},
	{0x3736, 0x30},
	{0x373a, 0x02},
	{0x373b, 0x03},
	{0x373c, 0x05},
	{0x373e, 0x18},
	{0x3755, 0x00},
	{0x3758, 0x00},
	{0x375a, 0x06},
	{0x375b, 0x13},
	{0x375f, 0x14},
	{0x3772, 0x23},
	{0x3773, 0x05},
	{0x3774, 0x16},
	{0x3775, 0x12},
	{0x3776, 0x08},
	{0x377a, 0x06},
	{0x3788, 0x00},
	{0x3789, 0x04},
	{0x378a, 0x01},
	{0x378b, 0x60},
	{0x3799, 0x27},
	{0x37a0, 0x44},
	{0x37a1, 0x2d},
	{0x37a7, 0x44},
	{0x37a8, 0x38},
	{0x37aa, 0x44},
	{0x37ab, 0x24},
	{0x37b3, 0x33},
	{0x37b5, 0x36},
	{0x37c0, 0x42},
	{0x37c1, 0x42},
	{0x37c2, 0x04},
	{0x37c5, 0x00},
	{0x37c7, 0x30},
	{0x37c8, 0x00},
	{0x37d1, 0x13},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x05},
	{0x3805, 0x0f},
	{0x3806, 0x03},
	{0x3807, 0xcb},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x03},
	{0x380b, 0xc0},
	{0x380c, 0x07}, /* hts */
	{0x380d, 0x97},
	{0x380e, 0x03}, /* vts */
	{0x380f, 0xdc}, //0xdc
	{0x3810, 0x00},
	{0x3811, 0x08},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3816, 0x00},
	{0x3817, 0x00},
	{0x3818, 0x00},
	{0x3819, 0x00},
	{0x3820, 0x80},
	{0x3821, 0x40},
	{0x3826, 0x00},
	{0x3827, 0x08},
	{0x382a, 0x01},
	{0x382b, 0x01},
	{0x3836, 0x02},
	{0x3838, 0x10},
	{0x3861, 0x00},
	{0x3862, 0x00},
	{0x3863, 0x02},
	{0x3b00, 0x00},
	{0x3c00, 0x89},
	{0x3c01, 0xab},
	{0x3c02, 0x01},
	{0x3c03, 0x00},
	{0x3c04, 0x00},
	{0x3c05, 0x03},
	{0x3c06, 0x00},
	{0x3c07, 0x05},
	{0x3c0c, 0x00},
	{0x3c0d, 0x00},
	{0x3c0e, 0x00},
	{0x3c0f, 0x00},
	{0x3c40, 0x00},
	{0x3c41, 0xa3},
	{0x3c43, 0x7d},
	{0x3c56, 0x80},
	{0x3c80, 0x08},
	{0x3c82, 0x01},
	{0x3c83, 0x61},
	{0x3d85, 0x17},
	{0x3f08, 0x08},
	{0x3f0a, 0x00},
	{0x3f0b, 0x30},
	{0x4000, 0xcd},
	{0x4003, 0x40},
	{0x4009, 0x0d},
	{0x4010, 0xf0},
	{0x4011, 0x70},
	{0x4017, 0x10},
	{0x4040, 0x00},
	{0x4041, 0x00},
	{0x4303, 0x00},
	{0x4307, 0x30},
	{0x4500, 0x30},
	{0x4502, 0x40},
	{0x4503, 0x06},
	{0x4508, 0xaa},
	{0x450b, 0x00},
	{0x450c, 0x00},
	{0x4600, 0x00},
	{0x4601, 0x80},
	{0x4700, 0x04},
	{0x4704, 0x00},
	{0x4705, 0x04},
	{0x481f, 0x26},
	{0x4837, 0x14},
	{0x484a, 0x3f},
	{0x5000, 0x30},
	{0x5001, 0x01},
	{0x5002, 0x28},
	{0x5004, 0x0c},
	{0x5006, 0x0c},
	{0x5007, 0xe0},
	{0x5008, 0x01},
	{0x5009, 0xb0},
	{0x502a, 0x14},
	{0x5901, 0x00},
	{0x5a01, 0x00},
	{0x5a03, 0x00},
	{0x5a04, 0x0c},
	{0x5a05, 0xe0},
	{0x5a06, 0x09},
	{0x5a07, 0xb0},
	{0x5a08, 0x06},
	{0x5e00, 0x00},
	{0x5e10, 0xfc},
	{0x0100, 0x01},
#else

	{0x0103,0x01},
	{0x0100,0x00},
	{0x0300,0x02},
	{0x0302,0x50},
	{0x0303,0x01},
	{0x0304,0x01},
	{0x0305,0x01},
	{0x0306,0x01},
	{0x030a,0x00},
	{0x030b,0x00},
	{0x030d,0x1e},
	{0x030e,0x01},
	{0x030f,0x04},
	{0x0312,0x01},
	{0x031e,0x04},
	{0x3000,0x0f},
	{0x3001,0xff},
	{0x3002,0xe1},
	{0x3005,0xf0},
#ifdef DRIVE_CAPABILITY_1
	{0x3011,0x00},
#elif defined(DRIVE_CAPABILITY_2)
	{0x3011,0x20},
#elif defined(DRIVE_CAPABILITY_3)
	{0x3011,0x40},
#elif defined(DRIVE_CAPABILITY_4)
	{0x3011,0x60},
#endif
	{0x3016,0x00},
	{0x3018,0x32},
	{0x301a,0xf0},
	{0x301b,0xf0},
	{0x301c,0xf0},
	{0x301d,0xf0},
	{0x301e,0xf0},
	{0x3022,0x21},
	{0x3031,0x0a},
	{0x3032,0x80},
	{0x303c,0xff},
	{0x303e,0xff},
	{0x3040,0xf0},
	{0x3041,0x00},
	{0x3042,0xf0},
	{0x3104,0x01},
	{0x3106,0x15},
	{0x3107,0x01},
	{0x3500,0x00},
	{0x3501,0x3d},
	{0x3502,0x00},
	{0x3503,0x08},
	{0x3504,0x03},
	{0x3505,0x83},
	{0x3508,0x02},
	{0x3509,0x80},
	{0x3600,0x65},
	{0x3601,0x60},
	{0x3602,0x22},
	{0x3610,0xb8},
	{0x3612,0x18},
	{0x3613,0x3a},
	{0x3615,0x79},
	{0x3617,0x07},
	{0x3621,0x90},
	{0x3622,0x00},
	{0x3623,0x00},
	{0x3633,0x10},
	{0x3634,0x10},
	{0x3635,0x10},
	{0x3636,0x10},
	{0x3650,0x00},
	{0x3652,0xff},
	{0x3654,0x20},
	{0x3653,0x34},
	{0x3655,0x20},
	{0x3656,0xff},
	{0x3657,0xc4},
	{0x365a,0xff},
	{0x365b,0xff},
	{0x365e,0xff},
	{0x365f,0x00},
	{0x3668,0x00},
	{0x366a,0x07},
	{0x366d,0x00},
	{0x366e,0x10},
	{0x3702,0x1d},
	{0x3703,0x10},
	{0x3704,0x14},
	{0x3705,0x00},
	{0x3706,0x27},
	{0x3709,0x24},
	{0x370a,0x00},
	{0x370b,0x7d},
	{0x3714,0x24},
	{0x371a,0x5e},
	{0x3730,0x82},
	{0x3733,0x00},
	{0x373e,0x18},
	{0x3755,0x00},
	{0x3758,0x00},
	{0x375b,0x13},
	{0x3772,0x23},
	{0x3773,0x05},
	{0x3774,0x16},
	{0x3775,0x12},
	{0x3776,0x08},
	{0x37a8,0x38},
	{0x37b5,0x36},
	{0x37c2,0x04},
	{0x37c5,0x00},
	{0x37c7,0x01},
	{0x37c8,0x00},
	{0x37d1,0x13},
	{0x3800,0x00},
	{0x3801,0x00},
	{0x3802,0x00},
	{0x3803,0x04},
	{0x3804,0x05},
	{0x3805,0x0f},
	{0x3806,0x03},
	{0x3807,0xcb},
	{0x3808,0x05},
	{0x3809,0x00},
	{0x380a,0x03},
	{0x380b,0xc0},
	{0x380c,0x03},
	{0x380d,0x2a},
	{0x380e,0x04},
	{0x380f,0xa4},
	{0x3810,0x00},
	{0x3811,0x08},
	{0x3812,0x00},
	{0x3813,0x04},
	{0x3814,0x01},
	{0x3815,0x01},
	{0x3816,0x00},
	{0x3817,0x00},
	{0x3818,0x00},
	{0x3819,0x00},
	{0x3820,0x80},
	{0x3821,0x40},
	{0x3826,0x00},
	{0x3827,0x08},
	{0x382a,0x01},
	{0x382b,0x01},
	{0x3836,0x02},
	{0x3838,0x10},
	{0x3861,0x00},
	{0x3862,0x00},
	{0x3863,0x02},
	{0x3b00,0x00},
	{0x3c00,0x89},
	{0x3c01,0xab},
	{0x3c02,0x01},
	{0x3c03,0x00},
	{0x3c04,0x00},
	{0x3c05,0x03},
	{0x3c06,0x00},
	{0x3c07,0x05},
	{0x3c0c,0x00},
	{0x3c0d,0x00},
	{0x3c0e,0x00},
	{0x3c0f,0x00},
	{0x3c40,0x00},
	{0x3c41,0xa3},
	{0x3c43,0x7d},
	{0x3c56,0x80},
	{0x3c80,0x08},
	{0x3c82,0x01},
	{0x3c83,0x61},
	{0x3d85,0x17},
	{0x3f08,0x08},
	{0x3f0a,0x00},
	{0x3f0b,0x30},
	{0x4000,0xcd},
	{0x4003,0x40},
	{0x4009,0x0d},
	{0x4010,0xf0},
	{0x4011,0x70},
	{0x4017,0x10},
	{0x4040,0x00},
	{0x4041,0x00},
	{0x4303,0x00},
	{0x4307,0x30},
	{0x4500,0x30},
	{0x4502,0x40},
	{0x4503,0x06},
	{0x4508,0xaa},
	{0x450b,0x00},
	{0x450c,0x00},
	{0x4600,0x00},
	{0x4601,0x80},
	{0x4700,0x04},
	{0x4704,0x00},
	{0x4705,0x04},
	{0x4837,0x14},
	{0x484a,0x3f},
	{0x5000,0x10},
	{0x5001,0x01},
	{0x5002,0x28},
	{0x5004,0x0c},
	{0x5006,0x0c},
	{0x5007,0xe0},
	{0x5008,0x01},
	{0x5009,0xb0},
	{0x502a,0x18},
	{0x5901,0x00},
	{0x5a01,0x00},
	{0x5a03,0x00},
	{0x5a04,0x0c},
	{0x5a05,0xe0},
	{0x5a06,0x09},
	{0x5a07,0xb0},
	{0x5a08,0x06},
	{0x5e00,0x00},
	{0x5e10,0xfc},
	{0x3733,0x10},
	{0x3610,0xe8},
	{0x3611,0x5c},
	{0x3635,0x14},
	{0x3614,0x9d},
#endif

	{OV9750_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the ov9750_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ov9750_win_sizes[] = {
	/* 1280*960 */
	{
		.width		= 1280,
		.height		= 960,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR12_1X12,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov9750_init_regs_1280_960_25fps,
	}
};

static enum v4l2_mbus_pixelcode ov9750_mbus_code[] = {
	V4L2_MBUS_FMT_SBGGR10_1X10,
	V4L2_MBUS_FMT_SBGGR12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ov9750_stream_on[] = {
	{0x0100, 0x01},
	{OV9750_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov9750_stream_off[] = {
	{0x0100, 0x00},
	{OV9750_REG_END, 0x00},	/* END MARKER */
};

int ov9750_read(struct v4l2_subdev *sd, uint16_t reg,
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

int ov9750_write(struct v4l2_subdev *sd, uint16_t reg,
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

static int ov9750_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV9750_REG_END) {
		if (vals->reg_num == OV9750_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov9750_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}
static int ov9750_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV9750_REG_END) {
		if (vals->reg_num == OV9750_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov9750_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int ov9750_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int ov9750_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = ov9750_read(sd, 0x300b, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;
	if (v != OV9750_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = ov9750_read(sd, 0x300c, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;
	if (v != OV9750_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int ov9750_set_integration_time(struct v4l2_subdev *sd, int value)
{


	int ret = 0;
	/* low 4 bits are fraction bits */
	unsigned int expo = value<<4;
	ret = ov9750_write(sd, 0x3502, (unsigned char)(expo & 0xff));
	if (ret < 0)
		return ret;
	ret = ov9750_write(sd, 0x3501, (unsigned char)((expo >> 8) & 0xff));
	if (ret < 0)
		return ret;
	ret = ov9750_write(sd, 0x3500, (unsigned char)((expo >> 16) & 0xf));
	if (ret < 0)
		return ret;
	return 0;

}

static int ov9750_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	ov9750_write(sd, 0x3509, (unsigned char)(value & 0xff));
	ov9750_write(sd, 0x3508, (unsigned char)((value>>8) & 0xff));
	return 0;
}

static int ov9750_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ov9750_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ov9750_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &ov9750_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = ov9750_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int ov9750_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	if (enable) {
		ret = ov9750_write_array(sd, ov9750_stream_on);
		printk("ov9750 stream on\n");
	}
	else {
		ret = ov9750_write_array(sd, ov9750_stream_off);
		printk("ov9750 stream off\n");
	}
	return ret;
}

static int ov9750_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov9750_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov9750_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	int ret = 0;
	unsigned int pclk = 0;
	unsigned int hts = 0;
	unsigned int vts = 0;
	unsigned char val = 0;
	unsigned int newformat = 0; //the format is 24.8

	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
		/*printk("warn: fps(%d) no in range\n", fps);*/
		return -1;
	}
	pclk = OV9750_SUPPORT_PCLK;

	val = 0;
	ret += ov9750_read(sd, 0x380c, &val);
	hts = val<<8;
	val = 0;
	ret += ov9750_read(sd, 0x380d, &val);
	hts |= val;
	if (0 != ret) {
		printk("err: ov9750 read err\n");
		return ret;
	}

	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret += ov9750_write(sd, 0x3208, 0x00);
	ret += ov9750_write(sd, 0x380f, vts & 0xff);
	ret += ov9750_write(sd, 0x380e, (vts >> 8) & 0xff);
	ret += ov9750_write(sd, 0x3208, 0x10);
	/*ret += ov9750_write(sd, 0x320b, 0x00);*/
	ret += ov9750_write(sd, 0x3208, 0xa0);
	if (0 != ret) {
		printk("err: ov9750_write err\n");
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

static int ov9750_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &ov9750_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &ov9750_win_sizes[0];
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
static int ov9750_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"ov9750_reset");
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
		ret = gpio_request(pwdn_gpio,"ov9750_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = ov9750_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an ov9750 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "ov9750 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ov9750_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long ov9750_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = ov9750_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = ov9750_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = ov9750_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = ov9750_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = ov9750_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			ret = ov9750_write_array(sd, ov9750_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			ret = ov9750_write_array(sd, ov9750_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = ov9750_set_fps(sensor, ctrl->value);
			break;
		default:
			break;;
	}
	return 0;
}
static long ov9750_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = ov9750_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov9750_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov9750_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov9750_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov9750_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops ov9750_core_ops = {
	.g_chip_ident = ov9750_g_chip_ident,
	.reset = ov9750_reset,
	.init = ov9750_init,
	.s_power = ov9750_s_power,
	.ioctl = ov9750_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov9750_g_register,
	.s_register = ov9750_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov9750_video_ops = {
	.s_stream = ov9750_s_stream,
	.s_parm = ov9750_s_parm,
	.g_parm = ov9750_g_parm,
};

static const struct v4l2_subdev_ops ov9750_ops = {
	.core = &ov9750_core_ops,
	.video = &ov9750_video_ops,
};

static int ov9750_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &ov9750_win_sizes[0];
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
	clk_set_rate(sensor->mclk, 24000000);
	clk_enable(sensor->mclk);

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	ov9750_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = ov9750_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = ov9750_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(ov9750_win_sizes); i++)
		ov9750_win_sizes[i].mbus_code = mbus;

	/*
	 *(volatile unsigned int*)(0xb000007c) = 0x20000020;
	 while(*(volatile unsigned int*)(0xb000007c) & (1 << 28));
	 */

	 /*
		convert sensor-gain into isp-gain,
	 */
	ov9750_attr.max_again = 0x3f446;
	ov9750_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &ov9750_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &ov9750_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int ov9750_remove(struct i2c_client *client)
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

static const struct i2c_device_id ov9750_id[] = {
	{ "ov9750", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9750_id);

static struct i2c_driver ov9750_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ov9750",
	},
	.probe		= ov9750_probe,
	.remove		= ov9750_remove,
	.id_table	= ov9750_id,
};

static __init int init_ov9750(void)
{
	return i2c_add_driver(&ov9750_driver);
}

static __exit void exit_ov9750(void)
{
	i2c_del_driver(&ov9750_driver);
}

module_init(init_ov9750);
module_exit(exit_ov9750);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov9750 sensors");
MODULE_LICENSE("GPL");
