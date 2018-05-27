/*
 * gc2033.c
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

#define GC2033_CHIP_ID_H	(0x20)
#define GC2033_CHIP_ID_L	(0x33)

#define GC2033_FLAG_END		0xff
#define GC2033_FLAG_DELAY	0x00
#define GC2033_PAGE_REG	    0xfe

#define GC2033_SUPPORT_PCLK (96*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
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

const unsigned int  ANALOG_GAIN_1 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.0*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_2 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.42*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_3 =		(1<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.99*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_4 =		(2<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.86*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_5 =		(4<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.05*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_6 =		(5<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.78*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_7 =		(8<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.23*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_8 =		(11<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.72*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_9 =		(16<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.55*(1<<TX_ISP_GAIN_FIXED_POINT)));
const unsigned int  ANALOG_GAIN_10 =		(22<<TX_ISP_GAIN_FIXED_POINT)|(unsigned int)((0.68*(1<<TX_ISP_GAIN_FIXED_POINT)));

struct tx_isp_sensor_attribute gc2033_attr;

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

#define  ANALOG_GAIN_MAX (fix_point_mult2(ANALOG_GAIN_10, (0xf<<TX_ISP_GAIN_FIXED_POINT) + (0x3f<<(TX_ISP_GAIN_FIXED_POINT-6))))
unsigned int gc2033_gainone_to_reg(unsigned int gain_one, unsigned int *regs)
{
	unsigned int gain_one1 = 0;
	unsigned int gain_tmp = 0;
	unsigned char regb6 = 0;
	unsigned char regb1 =0x1;
	unsigned char regb2 = 0;
	int i,j;
	unsigned int gain_one_max = fix_point_mult2(ANALOG_GAIN_10, (0xf<<TX_ISP_GAIN_FIXED_POINT) + (0x3f<<(TX_ISP_GAIN_FIXED_POINT-6)));
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
	} else if (gain_one < gain_one_max) {
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
	} else {
		gain_one1 = gain_one_max;
		regb6 = 0x09;
		regb1 = 0xf;
		regb2 = 0x3f;
		goto done;
	}
	gain_one1 = ANALOG_GAIN_1;
	done:
	*regs = (regb6<<12)|(regb1<<8)|(regb2);
	return gain_one1;
}
unsigned int gc2033_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	unsigned int gain_one = 0;
	unsigned int gain_one1 = 0;
	unsigned int regs = 0;
	unsigned int isp_gain1 = 0;
	gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
	gain_one1 = gc2033_gainone_to_reg(gain_one, &regs);
	isp_gain1 = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
	*sensor_again = regs;
	return isp_gain1;
}
unsigned int gc2033_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}
/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct tx_isp_sensor_attribute gc2033_attr={
	.name = "gc2033",
	.chip_id = 0x2033,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
	.cbus_device = 0x37,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 276471,//321573,
	.max_dgain = 0,
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 1348,
	.integration_time_limit = 1348,
	.total_width = 2840,
	.total_height = 1352,
	.max_integration_time = 1348,
	.one_line_expr_in_us = 30,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 2,
	.sensor_ctrl.alloc_again = gc2033_alloc_again,
	.sensor_ctrl.alloc_dgain = gc2033_alloc_dgain,
};
static struct regval_list gc2033_init_regs_1920_1080_25fps_dvp[] = {
	/*SYS*/
	{0xf2, 0x00},
	{0xf6, 0x00},
	{0xfc, 0x06},
	{0xf7, 0x01},
	{0xf8, 0x07},
	{0xf9, 0x06},
	{0xfa, 0x00},
	{0xfc, 0x0e},
	/*ANALOG & CISCTL*/
	{0xfe, 0x00},
	{0x03, 0x03},
	{0x04, 0xf6},
	{0x05, 0x02}, //HB
	{0x06, 0xc6},
	{0x86, 0x51},
	{0x8e, 0x0c},/*drop frame while change VB*/
	{0x07, 0x00},
	{0x08, 0xf8},
	{0x09, 0x00},
	{0x0a, 0x00}, //row start
	{0x0b, 0x00},
	{0x0c, 0x00}, //col start
	{0x0d, 0x04},
	{0x0e, 0x40}, //height 1088
	{0x0f, 0x07},
	{0x10, 0x88}, //width 1928
	{0x12, 0xe2},
	{0x17, 0x54},
	{0x18, 0x02},
	{0x19, 0x0d},
	{0x1a, 0x18},
	{0x1c, 0x6c},
	{0x1d, 0x12},
	{0x20, 0x54},
	{0x21, 0x2c},
	{0x23, 0xf0},
	{0x24, 0xc1},
	{0x25, 0x18},
	{0x26, 0x64},
	{0x28, 0x20},
	{0x29, 0x08},
	{0x2a, 0x08},
	{0x2b, 0x48},
	{0x2d, 0x1c},
	{0x2f, 0x40},
	{0x30, 0x99},
	{0x34, 0x00},
	{0x38, 0x80},
	{0x3b, 0x12},
	{0x3d, 0xb0},
	{0xcc, 0x8a},
	{0xcd, 0x99},
	{0xcf, 0x70},
	{0xd0, 0xcb},
	{0xd2, 0xc1},
	{0xd8, 0x80},
	{0xda, 0x14},
	{0xdc, 0x24},
	{0xe1, 0x14},
	{0xe3, 0xf0},
	{0xe4, 0xfa},
	{0xe6, 0x1f},
	{0xe8, 0x02},
	{0xe9, 0x02},
	{0xea, 0x03},
	{0xeb, 0x03},
	/*ISP*/
	{0xfe, 0x00},
	{0x80, 0x5c},
	{0x88, 0x23},
	{0x89, 0x03},
	{0x90, 0x01},
	{0x92, 0x02},
	{0x94, 0x02},
	{0x95, 0x04}, //crop win height
	{0x96, 0x38},
	{0x97, 0x07}, //crop win width
	{0x98, 0x80},
	/*BLK*/
	{0xfe, 0x00},
	{0x40, 0x22},
	{0x43, 0x07},
	{0x4e, 0x3c},
	{0x4f, 0x00},
	{0x60, 0x00},
	{0x61, 0x80},
	/*GAIN*/
	{0xfe, 0x00},
	{0xb0, 0x58},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb6, 0x00},
	{0xfe, 0x01},
	{0x01, 0x00},
	{0x02, 0x01},
	{0x03, 0x02},
	{0x04, 0x03},
	{0x05, 0x04},
	{0x06, 0x05},
	{0x07, 0x06},
	{0x08, 0x0e},
	{0x09, 0x16},
	{0x0a, 0x1e},
	{0x0b, 0x36},
	{0x0c, 0x3e},
	{0x0d, 0x56},
	{0x0e, 0x5e},
	/*DNDD*/
	{0xfe, 0x02},
	{0x81, 0x05},
	/*dark sun*/
	{0xfe, 0x01},
	{0x54, 0x77},
	{0x58, 0x00},
	{0x5a, 0x05},
	/*MIPI*/
	{0xfe, 0x03},
	{0x01, 0x00},
	{0x02, 0x00},
	{0x03, 0x00},
	{0x06, 0x00},
	{0x10, 0x00},
	{0x15, 0x00},
	{0x36, 0x83},
	{0x8f, 0x64},
	/*pad enable*/
	{0xfe, 0x00},
	{0xf2, 0x0f},

  	{GC2033_FLAG_END, 0x00},	/* END MARKER */

};

#if 0
static struct regval_list gc2033_init_regs_1920_1080_15fps_dvp[] = {
	/*mclk=24mhz,pclk=60mhz,frame_rate=15fps,row_time=47.33us*/
	/*pixel_line_total=2840,line_frame_total=1408*/
	/*SYS*/
	{0xf2, 0x00},
	{0xf6, 0x00},
	{0xfc, 0x06},
	{0xf7, 0x01},
	{0xf8, 0x04},
	{0xf9, 0x06},
	{0xfa, 0x00},
	{0xfc, 0x0e},
	/*ANALOG & CISCTL*/
	{0xfe, 0x00},
	{0x03, 0x03},
	{0x04, 0xf6},
	{0x05, 0x02},
	{0x06, 0xc6},
	{0x86, 0x51},
	{0x8e, 0x0c},/*drop frame while change VB*/
	{0x07, 0x01},
	{0x08, 0x30},
	{0x09, 0x00},
	{0x0a, 0x00}, //row start
	{0x0b, 0x00},
	{0x0c, 0x00}, //col start
	{0x0d, 0x04},
	{0x0e, 0x40}, //height 1088
	{0x0f, 0x07},
	{0x10, 0x88}, //width 1928
	{0x12, 0xe2},
	{0x17, 0x54},
	{0x18, 0x02},
	{0x19, 0x0d},
	{0x1a, 0x18},
	{0x1c, 0x6c},
	{0x1d, 0x12},
	{0x20, 0x54},
	{0x21, 0x2c},
	{0x23, 0xf0},
	{0x24, 0xc1},
	{0x25, 0x18},
	{0x26, 0x64},
	{0x28, 0x20},
	{0x29, 0x08},
	{0x2a, 0x08},
	{0x2b, 0x48},
	{0x2d, 0x1c},
	{0x2f, 0x40},
	{0x30, 0x99},
	{0x34, 0x00},
	{0x38, 0x80},
	{0x3b, 0x12},
	{0x3d, 0xb0},
	{0xcc, 0x8a},
	{0xcd, 0x99},
	{0xcf, 0x70},
	{0xd0, 0xcb},
	{0xd2, 0xc1},
	{0xd8, 0x80},
	{0xda, 0x14},
	{0xdc, 0x24},
	{0xe1, 0x14},
	{0xe3, 0xf0},
	{0xe4, 0xfa},
	{0xe6, 0x1f},
	{0xe8, 0x02},
	{0xe9, 0x02},
	{0xea, 0x03},
	{0xeb, 0x03},
	/*ISP*/
	{0xfe, 0x00},
	{0x80, 0x5c},
	{0x88, 0x23},
	{0x89, 0x03},
	{0x90, 0x01},
	{0x92, 0x02},
	{0x94, 0x02},
	{0x95, 0x04}, //crop win height
	{0x96, 0x38},
	{0x97, 0x07}, //crop win width
	{0x98, 0x80},
	/*BLK*/
	{0xfe, 0x00},
	{0x40, 0x22},
	{0x43, 0x07},
	{0x4e, 0x3c},
	{0x4f, 0x00},
	{0x60, 0x00},
	{0x61, 0x80},
	/*GAIN*/
	{0xfe, 0x00},
	{0xb0, 0x58},
	{0xb1, 0x01},
	{0xb2, 0x00},
	{0xb6, 0x00},
	{0xfe, 0x01},
	{0x01, 0x00},
	{0x02, 0x01},
	{0x03, 0x02},
	{0x04, 0x03},
	{0x05, 0x04},
	{0x06, 0x05},
	{0x07, 0x06},
	{0x08, 0x0e},
	{0x09, 0x16},
	{0x0a, 0x1e},
	{0x0b, 0x36},
	{0x0c, 0x3e},
	{0x0d, 0x56},
	{0x0e, 0x5e},
	/*DNDD*/
	{0xfe, 0x02},
	{0x81, 0x05},
	{0x80, 0x5c},/*enable sensor DD*/
	/*dark sun*/
	{0xfe, 0x01},
	{0x54, 0x77},
	{0x58, 0x00},
	{0x5a, 0x05},
	/*MIPI*/
	{0xfe, 0x03},
	{0x01, 0x00},
	{0x02, 0x00},
	{0x03, 0x00},
	{0x06, 0x00},
	{0x10, 0x00},
	{0x15, 0x00},
	{0x36, 0x83},
	{0x8f, 0x64},
	/*pad enable*/
	{0xfe, 0x00},
	{0xf2, 0x0f},

  	{GC2033_FLAG_END, 0x00},	/* END MARKER */
};
#endif

/*
 * the order of the gc2033_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting gc2033_win_sizes[] = {
	/* 1920*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25<<16|1,
		.mbus_code	= V4L2_MBUS_FMT_SRGGB10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= gc2033_init_regs_1920_1080_25fps_dvp,
	}
};

static enum v4l2_mbus_pixelcode gc2033_mbus_code[] = {
	V4L2_MBUS_FMT_SRGGB10_1X10,
	V4L2_MBUS_FMT_SBGGR12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list gc2033_stream_on[] = {
	{ 0xf2, 0x8f},
	{GC2033_FLAG_END, 0x00},	/* END MARKER */
};

static struct regval_list gc2033_stream_off[] = {
	{ 0xf2, 0x80},
	{GC2033_FLAG_END, 0x00},	/* END MARKER */
};

int gc2033_read(struct v4l2_subdev *sd, unsigned char reg, unsigned char *value)
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

static int gc2033_write(struct v4l2_subdev *sd, unsigned char reg, unsigned char value)
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


static int gc2033_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != GC2033_FLAG_END) {
		if (vals->reg_num == GC2033_FLAG_DELAY) {
				msleep(vals->value);
		} else {
			ret = gc2033_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			if (vals->reg_num == GC2033_PAGE_REG){
				val &= 0xf8;
				val |= (vals->value & 0x07);
				ret = gc2033_write(sd, vals->reg_num, val);
				ret = gc2033_read(sd, vals->reg_num, &val);
			}
		}
		vals++;
	}
	return 0;
}
static int gc2033_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != GC2033_FLAG_END) {
		if (vals->reg_num == GC2033_FLAG_DELAY) {
				msleep(vals->value);
		} else {
			ret = gc2033_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int gc2033_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int gc2033_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = gc2033_read(sd, 0xf0, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != GC2033_CHIP_ID_H)
		return -ENODEV;
	ret = gc2033_read(sd, 0xf1, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != GC2033_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int gc2033_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret = gc2033_write(sd, 0x4, value&0xff);
	if (ret < 0) {
		printk("gc2033_write error  %d\n" ,__LINE__);
		return ret;
	}
	ret = gc2033_write(sd, 0x3, (value&0x1f00)>>8);
	if (ret < 0) {
		printk("gc2033_write error  %d\n" ,__LINE__ );
		return ret;
	}
	return 0;
}
static int gc2033_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	int tmp = 0;
	ret = gc2033_write(sd, 0xfe, 0x00);
	ret += gc2033_write(sd, 0xb6, (value >> 12) & 0xf);
	ret += gc2033_write(sd, 0xb1, (value >> 8) & 0xf);
	ret += gc2033_write(sd, 0xb2, (value << 2) & 0xff);
	if (ret < 0) {
		printk("gc2033_write error  %d" ,__LINE__ );
		return ret;
	}
	/*** dark sun ***/
	ret = gc2033_read(sd, 0xb6, &tmp);
	if (ret < 0) {
		return ret;
	}
	if(tmp >= 0x08){
		ret = gc2033_write(sd, 0x21, 0x2c);
		if (ret < 0)
			return ret;
	}
	else{
		ret = gc2033_write(sd, 0x21, 0x28);
		if (ret < 0)
			return ret;
	}
	return 0;
}
static int gc2033_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int gc2033_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int gc2033_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &gc2033_win_sizes[0];
	int ret = 0;

	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = gc2033_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int gc2033_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = gc2033_write_array(sd, gc2033_stream_on);
		pr_debug("gc2033 stream on\n");
	}
	else {
		ret = gc2033_write_array(sd, gc2033_stream_off);
		pr_debug("gc2033 stream off\n");
	}
	return ret;
}

static int gc2033_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int gc2033_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int gc2033_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = GC2033_SUPPORT_PCLK;
	unsigned short win_high=0;
	unsigned short vts = 0;
	unsigned short hb=0;
	unsigned short vb = 0;
	unsigned short hts=0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}
	ret = gc2033_read(sd, 0x05, &tmp);
	hb = tmp;
	ret += gc2033_read(sd, 0x06, &tmp);
	if(ret < 0)
		return -1;
	hb = (hb << 8) + tmp;
	hts = hb*4;

	ret = gc2033_read(sd, 0x0d, &tmp);
	win_high = tmp;
	ret += gc2033_read(sd, 0x0e, &tmp);
	if(ret < 0)
		return -1;
	win_high = (win_high << 8) + tmp;
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	vb = vts - win_high - 16;
	ret = gc2033_write(sd, 0x08, (unsigned char)(vb & 0xff));
	ret += gc2033_write(sd, 0x07, (unsigned char)(vb >> 8));
	if(ret < 0)
		return -1;
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 4;
	sensor->video.attr->integration_time_limit = vts - 4;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 4;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return ret;
}

static int gc2033_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &gc2033_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &gc2033_win_sizes[0];
	}

	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		if(sensor->priv != wsize){
			ret = gc2033_write_array(sd, wsize->regs);
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
static int gc2033_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"gc2033_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(20);
			gpio_direction_output(reset_gpio, 0);
			msleep(20);
			gpio_direction_output(reset_gpio, 1);
			msleep(10);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"gc2033_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
			msleep(10);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = gc2033_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an gc2033 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "gc2033 chip found @ 0x%02x (%s)\n",
		client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int gc2033_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long gc2033_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = gc2033_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = gc2033_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = gc2033_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = gc2033_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = gc2033_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		ret = gc2033_write_array(sd, gc2033_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		ret = gc2033_write_array(sd, gc2033_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = gc2033_set_fps(sensor, ctrl->value);
		break;
	default:
		break;
	}
	return 0;
}
static long gc2033_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = gc2033_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int gc2033_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = gc2033_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int gc2033_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	gc2033_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops gc2033_core_ops = {
	.g_chip_ident = gc2033_g_chip_ident,
	.reset = gc2033_reset,
	.init = gc2033_init,
	.s_power = gc2033_s_power,
	.ioctl = gc2033_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = gc2033_g_register,
	.s_register = gc2033_s_register,
#endif
};

static const struct v4l2_subdev_video_ops gc2033_video_ops = {
	.s_stream = gc2033_s_stream,
	.s_parm = gc2033_s_parm,
	.g_parm = gc2033_g_parm,
};

static const struct v4l2_subdev_ops gc2033_ops = {
	.core = &gc2033_core_ops,
	.video = &gc2033_video_ops,
};

static int gc2033_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &gc2033_win_sizes[0];
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

	gc2033_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = gc2033_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = gc2033_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(gc2033_win_sizes); i++)
		gc2033_win_sizes[i].mbus_code = mbus;

	 /*
		convert sensor-gain into isp-gain,
	 */
	gc2033_attr.max_again = 276471;//321573;/*log2_fixed_to_fixed(gc2033_attr.max_again, TX_ISP_GAIN_FIXED_POINT, LOG2_GAIN_SHIFT);*/
	gc2033_attr.max_dgain = gc2033_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &gc2033_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &gc2033_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("@@@@@@@probe ok ------->gc2033\n");
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int gc2033_remove(struct i2c_client *client)
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

static const struct i2c_device_id gc2033_id[] = {
	{ "gc2033", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc2033_id);

static struct i2c_driver gc2033_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "gc2033",
	},
	.probe		= gc2033_probe,
	.remove		= gc2033_remove,
	.id_table	= gc2033_id,
};

static __init int init_gc2033(void)
{
	return i2c_add_driver(&gc2033_driver);
}

static __exit void exit_gc2033(void)
{
	i2c_del_driver(&gc2033_driver);
}

module_init(init_gc2033);
module_exit(exit_gc2033);

MODULE_DESCRIPTION("A low-level driver for Gcoreinc gc2033 sensors");
MODULE_LICENSE("GPL");
