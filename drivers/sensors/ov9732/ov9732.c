/*
 * ov9732.c
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

#define OV9732_CHIP_ID_H	(0x97)
#define OV9732_CHIP_ID_L	(0x32)

#define OV9732_REG_END		0xffff
#define OV9732_REG_DELAY	0xfffe

#define OV9732_SUPPORT_MCLK  (24*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
#define DRIVE_CAPABILITY_1

#define OV9732_USE_AGAIN_ONLY

struct regval_list {
	unsigned short reg_num;
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

struct tx_isp_sensor_attribute ov9732_attr;

static uint32_t fix_point_mult2(uint32_t a, uint32_t b)
{
	uint32_t x1,x2,x;
	uint32_t a1,a2,b1,b2;
	uint32_t mask = (((unsigned int)0xffffffff)>>(32-TX_ISP_GAIN_FIXED_POINT));
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

uint32_t ov9732_gainone_to_reg(uint32_t gain_one, uint16_t *regs)
{
	uint32_t gain_one1 = 0;
	uint16_t regsa = 0;
	uint16_t regsd = 0;
	uint32_t max_gain_one = 0;
	uint32_t max_gain_one_a = 0;
	uint32_t max_gain_one_d = 0;
	uint32_t min_gain_one_d = 0;
	uint32_t mask_gain_one_a = 0;
	uint32_t gain_one_a = 0;
	uint32_t gain_one_d = 0;

	uint32_t div_l = 0;
	uint32_t div_l1 = 0;
	uint32_t div_s = 0;
	uint32_t loop = 0;

	max_gain_one_a = 0xf8<<(TX_ISP_GAIN_FIXED_POINT-4);
	max_gain_one_d = 0x1ff<<(TX_ISP_GAIN_FIXED_POINT-8);
	min_gain_one_d = 0x100<<(TX_ISP_GAIN_FIXED_POINT-8);
	max_gain_one = fix_point_mult2(max_gain_one_a, max_gain_one_d);
	mask_gain_one_a = ((~0) >> (TX_ISP_GAIN_FIXED_POINT-4)) << (TX_ISP_GAIN_FIXED_POINT-4);

	if (gain_one >= max_gain_one) {
		gain_one_a = max_gain_one_a;
		gain_one_d = max_gain_one_d;
		regsa = 0xf8; regsd = 0xff;
		goto done;
	}
	if ((gain_one & mask_gain_one_a) < max_gain_one_a) {
		gain_one_a = gain_one & mask_gain_one_a;
		regsa = gain_one_a >>  (TX_ISP_GAIN_FIXED_POINT-4);
	} else {
		gain_one_a = max_gain_one_a;
		regsa = 0xf8;
	}

	div_l = max_gain_one_d;
	div_l1 = max_gain_one_d;
	div_s = min_gain_one_d;
	loop = 0;

	while ((div_l - div_s) > 1) {
		loop++;
		if (loop > 32) goto err_div;
		gain_one_d = (div_s + div_l)/2;
		if (gain_one > fix_point_mult2(gain_one_a, gain_one_d)) {
			div_s = gain_one_d;
		} else {
			div_l = gain_one_d;
		}
	}
	gain_one_d = div_s;
	regsd = 0xff & (gain_one_d >> (TX_ISP_GAIN_FIXED_POINT-8));
done:

	gain_one1 = fix_point_mult2(gain_one_a, (gain_one_d>>(TX_ISP_GAIN_FIXED_POINT-8))<<(TX_ISP_GAIN_FIXED_POINT-8));
	*regs = ( regsa<<8 ) | regsd;
	//printk("info:  gain_one = 0x%08x, gain_one1 = 0x%08x, sensor_again = 0x%08x\n", gain_one, gain_one1, *regs);
	return gain_one1;
err_div:

	printk("err: %s,%s,%d err_div loop = %d\n", __FILE__, __func__, __LINE__, loop);
	gain_one1 = 0x10000;
	*regs = 0x1000;;
	return gain_one1;
}
unsigned int ov9732_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

#ifdef OV9732_USE_AGAIN_ONLY
	unsigned int gain_one = 0;
	unsigned int gain_one1 = 0;
	uint16_t regs = 0;
	unsigned int isp_gain1 = 0;
	uint32_t mask;
	/* low 4 bits are fraction bits */
	gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
	if (gain_one >= (uint32_t)(15.5*(1<<TX_ISP_GAIN_FIXED_POINT)))
		gain_one = (uint32_t)(15.5*(1<<TX_ISP_GAIN_FIXED_POINT));

	regs = gain_one>>(TX_ISP_GAIN_FIXED_POINT-4);
	mask = ~0;
	mask = mask >> (TX_ISP_GAIN_FIXED_POINT-4);
	mask = mask << (TX_ISP_GAIN_FIXED_POINT-4);
	gain_one1 = gain_one&mask;
	isp_gain1 = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
	*sensor_again = regs;
	//printk("info:  isp_gain = 0x%08x, isp_gain1 = 0x%08x, gain_one = 0x%08x, gain_one1 = 0x%08x, sensor_again = 0x%08x\n", isp_gain, isp_gain1, gain_one, gain_one1, regs);
	return isp_gain1;
#else
	unsigned int gain_one = 0;
	unsigned int gain_one1 = 0;
	uint16_t regs = 0;
	unsigned int isp_gain1 = 0;
	/* low 4 bits are fraction bits */
	gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
	if (gain_one >= (uint32_t)(16*(1<<TX_ISP_GAIN_FIXED_POINT)))
		gain_one = (uint32_t)(16*(1<<TX_ISP_GAIN_FIXED_POINT));

	gain_one1 = ov9732_gainone_to_reg(gain_one, &regs);

	isp_gain1 = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
	*sensor_again = regs;
	return isp_gain1;

#endif
}
unsigned int ov9732_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}
struct tx_isp_sensor_attribute ov9732_attr={
	.name = "ov9732",
	.chip_id = 0x9732,
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
	.max_again =0x3f446,
	.max_dgain = 0,
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 0x3c4-4,
	.integration_time_limit = 0x3c4-4,
	.total_width = 0x5c6,
	.total_height = 0x3c4,
	.max_integration_time = 0x3c4-4,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 2,
	.sensor_ctrl.alloc_again = ov9732_alloc_again,
	.sensor_ctrl.alloc_dgain = ov9732_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list ov9732_init_regs_1280_720_25fps[] = {
	/*
	  @@ DVP interface 1280*720 25fps
	*/
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x3001, 0x3f},
	{0x3002, 0xff},
	{0x3007, 0x00},
#ifdef	DRIVE_CAPABILITY_1
	{0x3009, 0x03},//pad driver
#elif defined(DRIVE_CAPABILITY_2)
	{0x3009,0x23},
#elif defined(DRIVE_CAPABILITY_3)
	{0x3009,0x43},
#elif defined(DRIVE_CAPABILITY_4)
	{0x3009,0x63},
#endif
	{0x3010, 0x00},
	{0x3011, 0x00},
	{0x3014, 0x36},
	{0x301e, 0x15},
	{0x3030, 0x09},
	{0x3080, 0x02},
	{0x3081, 0x3c},
	{0x3082, 0x04},
	{0x3083, 0x00},
	{0x3084, 0x02},
	{0x3085, 0x01},
	{0x3086, 0x01},
	{0x3089, 0x01},
	{0x308a, 0x00},
	{0x3103, 0x01},
	{0x3600, 0xf6},
	{0x3601, 0x72},
	{0x3610, 0x0c},
	{0x3611, 0xf0},
	{0x3612, 0x35},
	{0x3654, 0x10},
	{0x3655, 0x77},
	{0x3656, 0x77},
	{0x3657, 0x07},
	{0x3658, 0x22},
	{0x3659, 0x22},
	{0x365a, 0x02},
	{0x3700, 0x1f},
	{0x3701, 0x10},
	{0x3702, 0x0c},
	{0x3703, 0x07},
	{0x3704, 0x3c},
	{0x3705, 0x41},
	{0x370d, 0x10},
	{0x3710, 0x0c},
	{0x3783, 0x08},
	{0x3784, 0x05},
	{0x3785, 0x55},
	{0x37c0, 0x07},
	{0x3800, 0x00},
	{0x3801, 0x04},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x05},
	{0x3805, 0x0b},
	{0x3806, 0x02},
	{0x3807, 0xdb},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x02},
	{0x380b, 0xd0},
	{0x380c, 0x05},
	{0x380d, 0xc6},
	{0x380e, 0x03},
	{0x380f, 0xc4}, /* 25fps */
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3816, 0x00},
	{0x3817, 0x00},
	{0x3818, 0x00},
	{0x3819, 0x04},
	{0x3820, 0x10},
	{0x3821, 0x00},
	{0x382c, 0x06},
	{0x3500, 0x00},
	{0x3501, 0x31},
	{0x3502, 0x00},
	{0x3503, 0x3f},
	{0x3504, 0x00},
	{0x3505, 0x00},
	{0x3509, 0x10},
	{0x350a, 0x00},
	{0x350b, 0x40},
	{0x3d00, 0x00},
	{0x3d01, 0x00},
	{0x3d02, 0x00},
	{0x3d03, 0x00},
	{0x3d04, 0x00},
	{0x3d05, 0x00},
	{0x3d06, 0x00},
	{0x3d07, 0x00},
	{0x3d08, 0x00},
	{0x3d09, 0x00},
	{0x3d0a, 0x00},
	{0x3d0b, 0x00},
	{0x3d0c, 0x00},
	{0x3d0d, 0x00},
	{0x3d0e, 0x00},
	{0x3d0f, 0x00},
	{0x3d80, 0x00},
	{0x3d81, 0x00},
	{0x3d82, 0x38},
	{0x3d83, 0xa4},
	{0x3d84, 0x00},
	{0x3d85, 0x00},
	{0x3d86, 0x1f},
	{0x3d87, 0x03},
	{0x3d8b, 0x00},
	{0x3d8f, 0x00},
	{0x4001, 0xe0},//BLC
	{0x4004, 0x00},
	{0x4005, 0x02},
	{0x4006, 0x01},
	{0x4007, 0x40},
	{0x4009, 0x0b},
	{0x4300, 0x03},
	{0x4301, 0xff},
	{0x4304, 0x00},
	{0x4305, 0x00},
	{0x4309, 0x00},
	{0x4600, 0x00},
	{0x4601, 0x04},
	{0x4800, 0x04},
	{0x4805, 0x00},
	{0x4821, 0x3c},
	{0x4823, 0x3c},
	{0x4837, 0x2d},
	{0x4a00, 0x00},
	{0x4f00, 0x80},
	{0x4f01, 0x10},
	{0x4f02, 0x00},
	{0x4f03, 0x00},
	{0x4f04, 0x00},
	{0x4f05, 0x00},
	{0x4f06, 0x00},
	{0x4f07, 0x00},
	{0x4f08, 0x00},
	{0x4f09, 0x00},
	{0x5000, 0x0f},//eanble dpc
	{0x500c, 0x00},
	{0x500d, 0x00},
	{0x500e, 0x00},
	{0x500f, 0x00},
	{0x5010, 0x00},
	{0x5011, 0x00},
	{0x5012, 0x00},
	{0x5013, 0x00},
	{0x5014, 0x00},
	{0x5015, 0x00},
	{0x5016, 0x00},
	{0x5017, 0x00},
	{0x5080, 0x00}, //color bar
	{0x5180, 0x01},
	{0x5181, 0x00},
	{0x5182, 0x01},
	{0x5183, 0x00},
	{0x5184, 0x01},
	{0x5185, 0x00},
	{0x5708, 0x06},
	{0x5781, 0x00},
	{0x5782, 0x77},//decrease dpc strength
	{0x5783, 0x0f},
	{OV9732_REG_END, 0x00},	/* END MARKER */
};
/*
 * the order of the ov9732_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ov9732_win_sizes[] = {
	/* 1280*720 */
	{
		.width		= 1280,
		.height		= 720,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov9732_init_regs_1280_720_25fps,
	}
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ov9732_stream_on[] = {
	{0x0100, 0x01},
	{OV9732_REG_END, 0x00},	/* END MARKER */
};


static struct regval_list ov9732_stream_off[] = {
	{0x0100, 0x00},
	{OV9732_REG_END, 0x00},	/* END MARKER */
};

static int ov9732_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg >> 8, reg & 0xff};
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

static int ov9732_write(struct v4l2_subdev *sd, unsigned short reg, unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
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

static int ov9732_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV9732_REG_END) {
		if (vals->reg_num == OV9732_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov9732_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		/* printk("vals->reg_num:0x%02x, vals->value:0x%02x\n",vals->reg_num, val); */
		vals++;
	}
	return 0;
}
static int ov9732_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV9732_REG_END) {
		if (vals->reg_num == OV9732_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov9732_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		/* printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value); */
		vals++;
	}
	return 0;
}

static int ov9732_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int ov9732_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = ov9732_read(sd, 0x300a, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;
	if (v != OV9732_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = ov9732_read(sd, 0x300b, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;
	if (v != OV9732_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int ov9732_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	unsigned int expo = value<<4;

	ret = ov9732_write(sd, 0x3502, (unsigned char)(expo & 0xff));
	if (ret < 0)
		return ret;
	ret = ov9732_write(sd, 0x3501, (unsigned char)((expo >> 8) & 0xff));
	if (ret < 0)
		return ret;
	ret = ov9732_write(sd, 0x3500, (unsigned char)((expo >> 16) & 0xf));
	if (ret < 0)
		return ret;
	return 0;
}
static int ov9732_set_analog_gain(struct v4l2_subdev *sd, int value)
{

	/* 0x00 bit[6:0] */
#ifdef OV9732_USE_AGAIN_ONLY
	/* again */
	ov9732_write(sd, 0x350b, (unsigned char)(value & 0xff));
#else
	/* again */
	ov9732_write(sd, 0x350b, (unsigned char)((value >> 8) & 0xff));
	/* dgain */
	ov9732_write(sd, 0x5181, (unsigned char)(value & 0xff));
	ov9732_write(sd, 0x5183, (unsigned char)(value & 0xff));
	ov9732_write(sd, 0x5185, (unsigned char)(value & 0xff));
#endif
	return 0;
}
static int ov9732_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[7] if gain > 2X set 0; if gain > 4X set 1 */
	return 0;
}

static int ov9732_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
#if 0
	int ret = 0;
	int black = 0;
	unsigned char h,l;
	unsigned char reg = 0xff;
	unsigned int * v = (unsigned int *)(value);
	ret = ov9732_read(sd, 0x48, &h);
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
	ret = ov9732_read(sd, reg, &l);
	if (ret < 0)
		return ret;
	*v = (black | l);
#endif
	return 0;
}

static int ov9732_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &ov9732_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = ov9732_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int ov9732_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	unsigned char val_h;
	unsigned char val_l;
	if (enable) {
		ret = ov9732_write_array(sd, ov9732_stream_on);
		printk("ov9732 stream on\n");
	}
	else {
		ret = ov9732_write_array(sd, ov9732_stream_off);
		printk("ov9732 stream off\n");
	}
	return ret;
}

static int ov9732_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov9732_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov9732_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	int ret = 0;
	unsigned int mclk = 0;
	unsigned int pclk = 0;
	unsigned int hts = 0;
	unsigned int vts = 0;
	unsigned char val = 0;

	unsigned int newformat = 0; //the format is 24.8
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || fps < (SENSOR_OUTPUT_MIN_FPS << 8)) {
		/*printk("warn: fps(%d) no in range\n", fps);*/
		return -1;
	}

	mclk = OV9732_SUPPORT_MCLK;
	/* prediv */
	val = 0;
	ret += ov9732_read(sd, 0x3080, &val);
	val &= 0x7;
	switch (val) {
		case 0:
			pclk = mclk/1;
			break;
		case 1:
			pclk = mclk*2/3;
			break;
		case 2:
			pclk = mclk/2;
			break;
		case 3:
			pclk = mclk*2/5;
			break;
		case 4:
			pclk = mclk/3;
			break;
		case 5:
			pclk = mclk/4;
			break;
		case 6:
			pclk = mclk/5;
			break;
		case 7:
			pclk = mclk/6;
			break;
		default:
			break;
	}
	/* mul */
	val = 0;
	ret += ov9732_read(sd, 0x3081, &val);
	pclk *= val;
	/* sysdiv */
	val = 0;
	ret += ov9732_read(sd, 0x3082, &val);
	pclk /= ((val&0xf)+1);
	/* pixdiv */
	val = 0;
	ret += ov9732_read(sd, 0x3083, &val);
	if (val&0x1)
		pclk /= 4;
	else
		pclk /= 2;
	/* pixdiv */
	pclk /= 2;
	/*printk("info: pclk = %d\n", pclk);*/

	val = 0;
	ret += ov9732_read(sd, 0x380c, &val);
	hts = val<<8;
	val = 0;
	ret += ov9732_read(sd, 0x380d, &val);
	hts |= val;
	/*printk("info: hts = %d\n", hts);*/
	if (0 != ret) {
		printk("err: ov9732 read err\n");
		return ret;
	}

	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

	/*printk("info: vts = %d\n", vts);*/
	ret += ov9732_write(sd, 0x380f, vts&0xff);
	ret += ov9732_write(sd, 0x380e, (vts>>8)&0xff);
	if (0 != ret) {
		printk("err: ov9732_write err\n");
		return ret;
	}
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 4;
	sensor->video.attr->integration_time_limit = vts - 4;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 4;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return 0;
}

static int ov9732_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &ov9732_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &ov9732_win_sizes[0];
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
static int ov9732_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"ov9732_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(20);
			gpio_direction_output(reset_gpio, 0);
			msleep(20);
			gpio_direction_output(reset_gpio, 1);
			msleep(1);
		}else{
			printk("gpio requrest fail %d\n", reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"ov9732_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = ov9732_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an ov9732 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "ov9732 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ov9732_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long ov9732_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = ov9732_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = ov9732_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = ov9732_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = ov9732_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = ov9732_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		ret = ov9732_write_array(sd, ov9732_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		ret = ov9732_write_array(sd, ov9732_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = ov9732_set_fps(sensor, ctrl->value);
		break;
	default:
		break;;
	}
	return 0;
}
static long ov9732_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = ov9732_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov9732_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov9732_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov9732_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov9732_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops ov9732_core_ops = {
	.g_chip_ident = ov9732_g_chip_ident,
	.reset = ov9732_reset,
	.init = ov9732_init,
	.s_power = ov9732_s_power,
	.ioctl = ov9732_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov9732_g_register,
	.s_register = ov9732_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov9732_video_ops = {
	.s_stream = ov9732_s_stream,
	.s_parm = ov9732_s_parm,
	.g_parm = ov9732_g_parm,
};

static const struct v4l2_subdev_ops ov9732_ops = {
	.core = &ov9732_core_ops,
	.video = &ov9732_video_ops,
};

static int ov9732_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &ov9732_win_sizes[0];
	int ret = -1;

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

	ov9732_attr.dvp.gpio = sensor_gpio_func;
	/*
	 *(volatile unsigned int*)(0xb000007c) = 0x20000020;
	 while(*(volatile unsigned int*)(0xb000007c) & (1 << 28));
	*/

	/*
	  convert sensor-gain into isp-gain,
	*/
#ifdef OV9732_USE_AGAIN_ONLY
	ov9732_attr.max_again = 0x3f446;
#else
	ov9732_attr.max_again = 0x40000;
#endif
	ov9732_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &ov9732_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &ov9732_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);
	return -1;

}

static int ov9732_remove(struct i2c_client *client)
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

static const struct i2c_device_id ov9732_id[] = {
	{ "ov9732", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9732_id);

static struct i2c_driver ov9732_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ov9732",
	},
	.probe		= ov9732_probe,
	.remove		= ov9732_remove,
	.id_table	= ov9732_id,
};

static __init int init_ov9732(void)
{
	return i2c_add_driver(&ov9732_driver);
}

static __exit void exit_ov9732(void)
{
	i2c_del_driver(&ov9732_driver);
}

module_init(init_ov9732);
module_exit(exit_ov9732);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov9732 sensors");
MODULE_LICENSE("GPL");
