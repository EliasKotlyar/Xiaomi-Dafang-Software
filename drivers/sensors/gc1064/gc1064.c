/*
 * gc1064.c
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

#define GC1064_CHIP_ID_H	(0x10)
#define GC1064_CHIP_ID_L	(0x24)

#define GC1064_FLAG_END	      0x00
#define GC1064_FLAG_DELAY    0xff
#define GC1064_PAGE_REG	      0xfe

#define GC1064_SUPPORT_PCLK (48*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 25
#define SENSOR_OUTPUT_MIN_FPS 5
#define DRIVE_CAPABILITY_2

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

const unsigned int  ANALOG_GAIN_1 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.0*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_2 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.65*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_3 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.87*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_4 =		(3<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.08*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_5 =		(3<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.5*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_6 =		(5<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.82*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_7 =		(6<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.7*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_8 =		(10<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.7*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_9 =		(15<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.8*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_10 =	(21<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.4*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_11 =	(30<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.8*(1<<TX_ISP_GAIN_FIXED_POINT)));

struct tx_isp_sensor_attribute gc1064_attr;

unsigned int fix_point_mult2(unsigned int a, unsigned int b)
{
	unsigned int x1,x2,x;
	unsigned int a1,a2,b1,b2;
	unsigned int mask = (((unsigned int)0xffffffff)>>(32-TX_ISP_GAIN_FIXED_POINT));
	a1 = a>>TX_ISP_GAIN_FIXED_POINT;
	a2 = a&mask;
	b1 = b>>TX_ISP_GAIN_FIXED_POINT;
	b2 = b&mask;

	x1 = a1*b1;
	x1 += (a1*b2)>>TX_ISP_GAIN_FIXED_POINT;
	x1 += (a2*b1)>>TX_ISP_GAIN_FIXED_POINT;

	x2 = (a1*b2)&mask;
	x2 += (a2*b1)&mask;
	x2 += (a2*b2)>>TX_ISP_GAIN_FIXED_POINT;

	x = (x1<<TX_ISP_GAIN_FIXED_POINT)+x2;
	return x;
}

unsigned int fix_point_mult3(unsigned int a, unsigned int b, unsigned int c)
{
	unsigned int x = 0;
	x = fix_point_mult2(a,b);
	x = fix_point_mult2(x,c);
	return x;
}

#define  ANALOG_GAIN_MAX (fix_point_mult2(ANALOG_GAIN_11, (0xf<<TX_ISP_GAIN_FIXED_POINT) + (0x3f<<(TX_ISP_GAIN_FIXED_POINT-6))))

unsigned int gc1064_gainone_to_reg(unsigned int gain_one, unsigned int *regs)
{
	unsigned int gain_one1 = 0;
	unsigned int gain_tmp = 0;
	unsigned char regb6 = 0;
	unsigned char regb1 =0x1;
	unsigned char regb2 = 0;
	int i,j;
	unsigned int gain_one_max = fix_point_mult2(ANALOG_GAIN_11, (0xf<<TX_ISP_GAIN_FIXED_POINT) + (0x3f<<(TX_ISP_GAIN_FIXED_POINT-6)));

	if (gain_one < ANALOG_GAIN_1) {
		gain_one1 = ANALOG_GAIN_1;
		regb6 = 0x00;
		regb1 = 0x01;
		regb2 = 0x00;
		goto done;
	} else if (gain_one < (ANALOG_GAIN_2)) {
		gain_one1 = gain_tmp = ANALOG_GAIN_1;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_1, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x00;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_3) {
		gain_one1 = gain_tmp = ANALOG_GAIN_2;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_2, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x01;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_4) {
		gain_one1 = gain_tmp = ANALOG_GAIN_3;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_3, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x02;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_5) {
		gain_one1 = gain_tmp = ANALOG_GAIN_4;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_4, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x03;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_6) {
		gain_one1 = gain_tmp = ANALOG_GAIN_5;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_5, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x04;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_7) {
		gain_one1 = gain_tmp = ANALOG_GAIN_6;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_6, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x05;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_8) {
		gain_one1 = gain_tmp = ANALOG_GAIN_7;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_7, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x06;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_9) {
		gain_one1 = gain_tmp = ANALOG_GAIN_8;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_8, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x07;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_10) {
		gain_one1 = gain_tmp = ANALOG_GAIN_9;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_9, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x08;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < ANALOG_GAIN_11) {
		gain_one1 = gain_tmp = ANALOG_GAIN_10;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_10, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x09;
				regb1 = i;
				regb2 = j;
			}
	} else if (gain_one < gain_one_max) {
		gain_one1 = gain_tmp = ANALOG_GAIN_11;
		regb1 = 0;
		regb2 = 0;
		for (i = 1; i <= 0xf; i++ )
			for (j = 0; j <= 0x3f; j++) {
				gain_tmp = fix_point_mult2(ANALOG_GAIN_11, (i<<TX_ISP_GAIN_FIXED_POINT)+(j<<(TX_ISP_GAIN_FIXED_POINT-6)));
				if (gain_one < gain_tmp) {
					goto done;
				}
				gain_one1 = gain_tmp;
				regb6 = 0x0a;
				regb1 = i;
				regb2 = j;
			}
	} else {
		gain_one1 = gain_one_max;
		regb6 = 0x08;
		regb1 = 0xf;
		regb2 = 0x3f;
		goto done;
	}
	gain_one1 = ANALOG_GAIN_1;

done:
	*regs = (regb6<<12)|(regb1<<8)|(regb2);
	return gain_one1;
}

unsigned int gc1064_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	unsigned int gain_one = 0;
	unsigned int gain_one1 = 0;
	unsigned int regs = 0;
	unsigned int isp_gain1 = 0;

	gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
	gain_one1 = gc1064_gainone_to_reg(gain_one, &regs);
	isp_gain1 = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
	*sensor_again = regs;
	return isp_gain1;
}
unsigned int gc1064_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return 0;
}

/*
 * the part of driver maybe modify about different sensor and different board.
 */

struct tx_isp_sensor_attribute gc1064_attr={
	.name = "gc1064",
	.chip_id = 0x1024,
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
	},
	.max_again = 0x5091D,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 920,
	.integration_time_limit = 920,
	.total_width = 2078,
	.total_height = 924,
	.max_integration_time = 920,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = gc1064_alloc_again,
	.sensor_ctrl.alloc_dgain = gc1064_alloc_dgain,
//	void priv; /* point to struct tx_isp_sensor_board_info */
};

/*
 * the configure of gpio should be in accord with product-board.
 * if one is redundant, please config -1.
 */
/*struct tx_isp_sensor_board_info gc1064_board = {
#ifdef CONFIG_BOARD_BOX
	.reset_gpio = GPIO_PF(30),
	.pwdn_gpio = GPIO_PF(31),
#elif defined(CONFIG_BOARD_BOOTES)
	.reset_gpio = GPIO_PF(28),
	.pwdn_gpio = GPIO_PF(29),
#elif defined(CONFIG_BOARD_MANGO)
	.reset_gpio = GPIO_PA(18),
	.pwdn_gpio = -1,
#else
	.reset_gpio = GPIO_PF(0),
	.pwdn_gpio = GPIO_PF(1),
#endif
	.mclk_name = "cgu_cim",
};
*/
static struct regval_list gc1064_init_regs_1280_720[] = {

	{0xfe,0x80},
	{0xfe,0x80},
	{0xfe,0x80},
	{0xf2,0x0f},
	{0xf6,0x00},
	{0xfc,0xc6},
	{0xf7,0xb9},
	{0xf8,0x03},
	{0xf9,0x0e},
	{0xfa,0x00},
	{0xfe,0x00},
  /////////////////////////////////////////////////////
  ////////////////   ANALOG & CISCTL   ////////////////
  /////////////////////////////////////////////////////
  	{0x03,0x02},
	{0x04,0xb5}, //exposure
	{0x05,0x01},
	{0x06,0x6f}, //H blanking
	{0x07,0x00},
	{0x08,0xb4}, // v blanking
	{0x0d,0x02},
	{0x0e,0xd8}, //728 windows heigh
	{0x0f,0x05},
	{0x10,0x08}, //1288 window width
	{0x11,0x00},
	{0x12,0x18},
	{0x16,0xc0},
	{0x17,0x14},
	{0x19,0x06},
	{0x1b,0x4f},
	{0x1c,0x11},
	{0x1d,0x10}, //exp<1frame 图像闪烁
	{0x1e,0xf8}, //fc 左侧发紫
	{0x1f,0x38},
	{0x20,0x81},
	{0x21,0x1f}, //6f//2f
	{0x22,0xc0}, //c2 左侧发紫
	{0x23,0x82}, //f2 左侧发紫
#ifdef DRIVE_CAPABILITY_1
	{0x24,0xe3},
#elif defined(DRIVE_CAPABILITY_2)
	{0x24,0x2f},
#endif
	{0x25,0xd4},
	{0x26,0xa8},
	{0x29,0x3f}, //54//3f
	{0x2a,0x00},
	{0x2b,0x00}, //00--powernoise	 03---强光拖尾
	{0x2c,0xe0}, //左右range不一致
	{0x2d,0x0a},
	{0x2e,0x00},
	{0x2f,0x16}, //1f--横闪线
	{0x30,0x00},
	{0x31,0x01},
	{0x32,0x02},
	{0x33,0x03},
	{0x34,0x04},
	{0x35,0x05},
	{0x36,0x06},
	{0x37,0x07},
	{0x38,0x0f},
	{0x39,0x17},
	{0x3a,0x1f},
	{0x3f,0x18}, //关掉Vclamp 电压
	////////////////////////////////////////////////
	/////////////////	  ISP	//////////////////////
	////////////////////////////////////////////////
	{0xfe,0x00},
	{0x8a,0x00},
	{0x8c,0x02},
	{0x8e,0x02},
	{0x8f,0x15},
	{0x90,0x01},
	{0x94,0x02},
	{0x95,0x02},
	{0x96,0xd0},
	{0x97,0x05},
	{0x98,0x00},
	////////////////////////////////////////////////
	/////////////////	 MIPI	/////////////////////
	////////////////////////////////////////////////
	{0xfe,0x03},
	{0x01,0x00},
	{0x02,0x00},
	{0x03,0x00},
	{0x06,0x00},
	{0x10,0x00},
	{0x15,0x00},
	///////////////////////////////////////////////
	////////////////	 BLK	/////////////////////
	///////////////////////////////////////////////
	{0xfe,0x00},
	{0x18,0x02},
	{0x1a,0x11},
	{0x40,0x23}, //2b
	{0x5e,0x00},
	{0x66,0x80},
	////////////////////////////////////////////////
	///////////////// Dark SUN /////////////////////
	////////////////////////////////////////////////
	{0xfe,0x00},
	{0xcc,0x25},
	{0xce,0xf3},
	///////////////////////////////////////////////
	////////////////	 Gain	/////////////////////
	///////////////////////////////////////////////
	{0xfe,0x00},
	{0xb0,0x50},
	{0xb3,0x40},
	{0xb4,0x40},
	{0xb5,0x40},
	{0xb6,0x00},
	//disable sensor dpc
//	{0x8b,0x22},
	///////////////////////////////////////////////
	////////////////	  pad enable   ///////////////
	///////////////////////////////////////////////
	{0xf2,0x0f},
	{0xfe,0x00},
  	{GC1064_FLAG_END, 0x00},	/* END MARKER */

};

/*
 * the order of the gc1064_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting gc1064_win_sizes[] = {
	/* 1280*720 */
	{
		.width		= 1280,
		.height		= 720,
		.fps		= 25<<16|1,
		.mbus_code	= V4L2_MBUS_FMT_SRGGB10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= gc1064_init_regs_1280_720,
	}
};

/*
 * the part of driver was fixed.
 */

static struct regval_list gc1064_stream_on[] = {
	{ 0xfe, 0x03},
	{ 0x10, 0x91},
	/* {GC1064_FLAG_DELAY, 0x00, 1000},  */
	{ 0xfe, 0x00},
	{GC1064_FLAG_END, 0x00},	/* END MARKER */
};

static struct regval_list gc1064_stream_off[] = {
	{ 0xfe, 0x03},
	{ 0x10 ,0x81},//add
	/* {GC1064_FLAG_DELAY, 0x00, 1000},  */
	{ 0xfe ,0x00},
	{GC1064_FLAG_END, 0x00},	/* END MARKER */
};

int gc1064_read(struct v4l2_subdev *sd, unsigned char reg,
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


int gc1064_write(struct v4l2_subdev *sd, unsigned char reg,
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


static int gc1064_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != GC1064_FLAG_END) {
		if(vals->reg_num == GC1064_FLAG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = gc1064_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			if (vals->reg_num == GC1064_PAGE_REG){
				val &= 0xf8;
				val |= (vals->value & 0x07);
				ret = gc1064_write(sd, vals->reg_num, val);
				ret = gc1064_read(sd, vals->reg_num, &val);
			}
		}
		vals++;
	}
	return 0;
}
static int gc1064_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != GC1064_FLAG_END) {
		if (vals->reg_num == GC1064_FLAG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = gc1064_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int gc1064_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int gc1064_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = gc1064_read(sd, 0xf0, &v);
	if (ret < 0)
		return ret;
	ret = gc1064_read(sd, 0xf1, &v);
	if (ret < 0)
		return ret;
	if (v != GC1064_CHIP_ID_L)
		return -ENODEV;

	return 0;
}

static int gc1064_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret = gc1064_write(sd, 0x4, value&0xff);
	if (ret < 0) {
		printk("gc1064_write error\n");
		goto err_gc1064_wr;
	}
	ret = gc1064_write(sd, 0x3, (value&0x1f00)>>8);
	if (ret < 0) {
		printk("gc1064_write error\n");
		goto err_gc1064_wr;
	}
	return 0;
err_gc1064_wr:
	return ret;
}
static int gc1064_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret = gc1064_write(sd, 0xb6, (value>>12)&0xf);
	if (ret < 0) {
		printk("gc1064_write error\n");
		goto err_gc1064_wr;
	}
	ret = gc1064_write(sd, 0xb1, (value>>8)&0xf);
	if (ret < 0) {
		printk("gc1064_write error\n");
		goto err_gc1064_wr;
	}
	ret = gc1064_write(sd, 0xb2, (value<<2)&0xff);
	if (ret < 0) {
		printk("gc1064_write error\n");
		goto err_gc1064_wr;
	}
	return 0;
err_gc1064_wr:
	return ret;
}
static int gc1064_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int gc1064_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
#if 0
	int ret = 0;
	int black = 0;
	unsigned char h,l;
	unsigned char reg = 0xff;
	unsigned int * v = (unsigned int *)(value);
	ret = gc1064_read(sd, 0x48, &h);
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
	ret = gc1064_read(sd, reg, &l);
	if (ret < 0)
		return ret;
	*v = (black | l);
#endif
	return 0;
}

static int gc1064_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &gc1064_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = gc1064_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int gc1064_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	if (enable) {
		ret = gc1064_write_array(sd, gc1064_stream_on);
		printk("gc1064 stream on\n");
	}
	else {
		ret = gc1064_write_array(sd, gc1064_stream_off);
		printk("gc1064 stream off\n");
	}
	return ret;
}

static int gc1064_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int gc1064_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int gc1064_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = GC1064_SUPPORT_PCLK;
	unsigned short win_width=0;
	unsigned short win_high=0;
	unsigned short vts = 0;
	unsigned short hb=0;
	unsigned short sh_delay=0;
	unsigned short vb = 0;
	unsigned short hts=0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;
	ret = gc1064_read(sd, 0x5, &tmp);
	hb = tmp;
	ret = gc1064_read(sd, 0x6, &tmp);
	if(ret < 0)
		return -1;
	hb = (hb << 8) + tmp;
	ret = gc1064_read(sd, 0xf, &tmp);
	win_width = tmp;
	ret = gc1064_read(sd, 0x10, &tmp);
	if(ret < 0)
		return -1;
	win_width = (win_width << 8) + tmp;

	ret = gc1064_read(sd, 0x12, &tmp);
	sh_delay = tmp;

	hts=win_width+2*(hb+sh_delay+4);

	ret = gc1064_read(sd, 0xd, &tmp);
	win_high = tmp;
	ret = gc1064_read(sd, 0xe, &tmp);
	if(ret < 0)
		return -1;
	win_high = (win_high << 8) + tmp;
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	vb=vts-win_high-16;
	ret = gc1064_write(sd, 0x8, (unsigned char)(vb & 0xff));
	ret += gc1064_write(sd, 0x7, (unsigned char)(vb >> 8));
	if(ret < 0)
		return -1;
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 4;
	sensor->video.attr->integration_time_limit = vts - 4;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 4;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return 0;
}

static int gc1064_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &gc1064_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &gc1064_win_sizes[0];
	}

	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		if(sensor->priv != wsize){
			ret = gc1064_write_array(sd, wsize->regs);
			if(!ret){
				sensor->priv = wsize;
			}
		}
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}
static int gc1064_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"gc1064_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			mdelay(20);
			gpio_direction_output(reset_gpio, 0);
			mdelay(20);
			gpio_direction_output(reset_gpio, 1);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"gc1064_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			mdelay(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = gc1064_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an gc1064 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "gc1064 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int gc1064_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long gc1064_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = gc1064_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = gc1064_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = gc1064_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = gc1064_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = gc1064_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		ret = gc1064_write_array(sd, gc1064_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		ret = gc1064_write_array(sd, gc1064_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = gc1064_set_fps(sensor, ctrl->value);
		break;
	default:
		break;;
	}
	return 0;
}
static long gc1064_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = gc1064_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int gc1064_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = gc1064_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int gc1064_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	gc1064_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops gc1064_core_ops = {
	.g_chip_ident = gc1064_g_chip_ident,
	.reset = gc1064_reset,
	.init = gc1064_init,
	.s_power = gc1064_s_power,
	.ioctl = gc1064_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = gc1064_g_register,
	.s_register = gc1064_s_register,
#endif
};

static const struct v4l2_subdev_video_ops gc1064_video_ops = {
	.s_stream = gc1064_s_stream,
	.s_parm = gc1064_s_parm,
	.g_parm = gc1064_g_parm,
};

static const struct v4l2_subdev_ops gc1064_ops = {
	.core = &gc1064_core_ops,
	.video = &gc1064_video_ops,
};

static int gc1064_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &gc1064_win_sizes[0];
	int ret =0;
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
		return PTR_ERR(sensor->mclk);
	}
	clk_set_rate(sensor->mclk, 24000000);
	clk_enable(sensor->mclk);

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	 /*
		convert sensor-gain into isp-gain,
	 */
	gc1064_attr.max_again = log2_fixed_to_fixed(gc1064_attr.max_again, TX_ISP_GAIN_FIXED_POINT, LOG2_GAIN_SHIFT);
	gc1064_attr.max_dgain = gc1064_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &gc1064_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &gc1064_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
	kfree(sensor);

	return -1;
}

static int gc1064_remove(struct i2c_client *client)
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

static const struct i2c_device_id gc1064_id[] = {
	{ "gc1064", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc1064_id);

static struct i2c_driver gc1064_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "gc1064",
	},
	.probe		= gc1064_probe,
	.remove		= gc1064_remove,
	.id_table	= gc1064_id,
};

static __init int init_gc1064(void)
{
	return i2c_add_driver(&gc1064_driver);
}

static __exit void exit_gc1064(void)
{
	i2c_del_driver(&gc1064_driver);
}

module_init(init_gc1064);
module_exit(exit_gc1064);

MODULE_DESCRIPTION("A low-level driver for OmniVision gc1064 sensors");
MODULE_LICENSE("GPL");
