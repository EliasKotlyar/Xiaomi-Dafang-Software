/*
 * jxh62.c
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

#define JXH62_CHIP_ID_H	(0xa0)
#define JXH62_CHIP_ID_L	(0x62)

#define JXH62_REG_END		0xff
#define JXH62_REG_DELAY		0xfe

#define JXH62_SUPPORT_PCLK (36*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
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

struct tx_isp_sensor_attribute jxh62_attr;
#if 0
static inline unsigned char cale_again_register(unsigned int gain)
{
	int index = 0;
	int i = 0, p = 0;
	for(index = 3; index >= 0; index--)
		if(gain & (1 << (index + TX_ISP_GAIN_FIXED_POINT)))
			break;
	i = index;
	p = (gain >> (TX_ISP_GAIN_FIXED_POINT - 4)) & ((1 << (4 + i)) - 1);
	return (i << 4) | p;
}

unsigned int jxh62_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	unsigned int again = 0;
	unsigned int gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);

	if(gain_one < jxh62_attr.max_again){
		again = (gain_one  >> (TX_ISP_GAIN_FIXED_POINT - 4) << (TX_ISP_GAIN_FIXED_POINT - 4));
	}else{
		again = jxh62_attr.max_again;
	}
	isp_gain = log2_fixed_to_fixed(again, TX_ISP_GAIN_FIXED_POINT, shift);
	*sensor_again = cale_again_register(again);
	return isp_gain;
}
#else
static inline unsigned char cale_again_register(unsigned int gain)
{
	int index = 0;
	int i = 0, p = 0;
	for(index = 3; index >= 0; index--)
		if(gain & (1 << (index + TX_ISP_GAIN_FIXED_POINT)))
			break;
	i = index;
	p = (gain >> (TX_ISP_GAIN_FIXED_POINT + index - 4)) & 0xf;
	return (i << 4) | p;
}
static inline unsigned int cale_sensor_again_to_isp(unsigned char reg)
{
	unsigned int h,l;
	h = reg >> 4;
	l = reg & 0xf;
	return (1 << (h + TX_ISP_GAIN_FIXED_POINT)) | (l << ((TX_ISP_GAIN_FIXED_POINT + h - 4)));
}
unsigned int jxh62_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	unsigned int again = 0;
	unsigned int gain_one = 0;
	unsigned int gain_one1 = 0;

	if(isp_gain > jxh62_attr.max_again){
		isp_gain = jxh62_attr.max_again;
	}
	again = isp_gain;
	gain_one = math_exp2(isp_gain, shift, TX_ISP_GAIN_FIXED_POINT);
	*sensor_again = cale_again_register(gain_one);
	gain_one1 = cale_sensor_again_to_isp(*sensor_again);
	isp_gain = log2_fixed_to_fixed(gain_one1, TX_ISP_GAIN_FIXED_POINT, shift);
//	printk("again = %d gain_one = 0x%0x sensor_gain = 0x%0x gain_one1 = 0x%0x isp_gain = %d\n", again, gain_one, *sensor_again, gain_one1, isp_gain);
	return isp_gain;
}
#endif
unsigned int jxh62_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}
struct tx_isp_sensor_attribute jxh62_attr={
	.name = "jxh62",
	.chip_id = 0xa062,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
	.cbus_device = 0x30,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 0xff << (TX_ISP_GAIN_FIXED_POINT - 4),
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 896,
	.integration_time_limit = 896,
	.total_width = 1600,
	.total_height = 900,
	.max_integration_time = 896,
	.integration_time_apply_delay = 1,
	.again_apply_delay = 1,
	.dgain_apply_delay = 1,
	.sensor_ctrl.alloc_again = jxh62_alloc_again,
	.sensor_ctrl.alloc_dgain = jxh62_alloc_dgain,
	.one_line_expr_in_us = 44,
	//void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list jxh62_init_regs_1280_720_25fps[] = {
//[SensorType]
//0=INI_Header
//
//[INI_Header]
//InitREGdef=INI_Register
//IniVersion=20160621
//Project=4	;H62
//Width=1280
//Height=720
//disWidth=1280
//disHeight=720
//FrameWidth=1920
//FrameHeight=750
//H_pad=0
//V_pad=0
//SrOutputFormat=1
//Interface=0
//HDR_Mode=0
//MclkRate=24
//DVPClkRate=36
//MipiClkRate=144
//
//[INI_Category]
//FPS=25.00
//Start=H62_Start_2016070700.soi
//PLL=H62_PLL_2016071501.soi
//Timing=H62_Timing_2016081204.soi
//Interface=H62_Interface_2016071500.soi
//Sampling=H62_Sampling_2016090501.soi
//Power=H62_Power_2016080900.soi
//AE=H62_AE_2016081200.soi
//End=H62_End_2016070700.soi
//AutoRamp=H62_AutoRamp_2016080900.soi
//
//;PC:1191871452
//
//[INI_Register]
	{0x12,0x40},
	{0x0E,0x11},
	{0x0F,0x00},
	{0x10,0x18},
	{0x11,0x80},
	{0x19,0x68},
	{0x20,0x40},
	{0x21,0x06},
	{0x22,0x84},
	{0x23,0x03},
	{0x24,0x00},
	{0x25,0xD0},
	{0x26,0x25},
	{0x27,0x10},
	{0x28,0x15},
	{0x29,0x02},
	{0x2A,0x01},
	{0x2B,0x21},
	{0x2C,0x08},
	{0x2D,0x01},
	{0x2E,0xBB},
	{0x2F,0xC0},
	{0x41,0x88},
	{0x42,0x12},
	{0x39,0x90},
	{0x1D,0xFF},
	{0x1E,0x9F},
	{0x7A,0x80},
	{0x1F,0x20},
	{0x30,0x90},
	{0x31,0x0C},//{0x31,0x08},changed by jack 20160923
	{0x32,0xFF},
	{0x33,0x0C},//{0x33,0x08},changed by jack 20160923
	{0x34,0x4B},
	{0x35,0xA3},
	{0x36,0x06},//changed by jack 20160923
	{0x38,0x40},
	{0x3A,0x08},
	{0x56,0x02},
	{0x60,0x01},
#ifdef	DRIVE_CAPABILITY_1
	{0x0D,0x50},
#elif defined(DRIVE_CAPABILITY_2)
	{0x0D,0x5c},
#endif
	{0x57,0x80},
	{0x58,0x33},
	{0x5A,0x04},
	{0x5B,0xB6},
	{0x5C,0x08},
	{0x5D,0x67},
	{0x5E,0x04},
	{0x5F,0x08},
	{0x66,0x28},
	{0x67,0xF8},
	{0x68,0x04},
	{0x69,0x74},
	{0x6A,0x1F},
	{0x63,0x82},
	{0x6C,0xC0},
	{0x6E,0x5C},
	{0x82,0x01},
	{0x0C,0x00},
	{0x46,0xC2},
	{0x48,0x7E},
	{0x62,0x40},
	{0x7D,0x57},
	{0x7E,0x28},
	{0x80,0x00},
	{0x4A,0x05},
	{0x49,0x10},
	{0x13,0x81},
	{0x59,0x97},
	{0x12,0x00},
	{0x47,0x47},
	{JXH62_REG_DELAY,500},//sleep 500
	{0x47,0x44},
	{0x1F,0x21},
//;PC:2942318289
	{JXH62_REG_END, 0x00},	/* END MARKER */
};

/* static struct regval_list jxh62_init_version_80[] = { */

/* 	{0x27,0x46}, */
/* 	{0x2C,0x00}, */
/* 	{0x63,0x19}, */
/* 	{JXH62_REG_END, 0x00},	/\* END MARKER *\/ */
/* }; */

/* static struct regval_list jxh62_init_version_81[] = { */

/* 	{0x27,0x3c}, */
/* 	{0x2C,0x04}, */
/* 	{0x63,0x51}, */
/* 	{JXH62_REG_END, 0x00},	/\* END MARKER *\/ */
/* }; */

/*
 * the order of the jxh62_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting jxh62_win_sizes[] = {
	/* 1280*800 */
	{
		.width		= 1280,
		.height		= 720,
		.fps		= 25 << 16 | 1, /* 12.5 fps */
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= jxh62_init_regs_1280_720_25fps,
	}
};
static enum v4l2_mbus_pixelcode jxh62_mbus_code[] = {
	V4L2_MBUS_FMT_SGBRG8_1X8,
	V4L2_MBUS_FMT_SBGGR10_1X10,
};
/*
 * the part of driver was fixed.
 */

static struct regval_list jxh62_stream_on[] = {
	{0x12, 0x00},

	{JXH62_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list jxh62_stream_off[] = {
	/* Sensor enter LP11*/
	{0x12, 0x40},

	{JXH62_REG_END, 0x00},	/* END MARKER */
};

int jxh62_read(struct v4l2_subdev *sd, unsigned char reg,
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

int jxh62_write(struct v4l2_subdev *sd, unsigned char reg,
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

static int jxh62_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != JXH62_REG_END) {
		if (vals->reg_num == JXH62_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = jxh62_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		/*printk("vals->reg_num:0x%02x, vals->value:0x%02x\n",vals->reg_num, val);*/
		vals++;
	}
	return 0;
}
static int jxh62_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != JXH62_REG_END) {
		if (vals->reg_num == JXH62_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = jxh62_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		//printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);
		//mdelay(200);
		vals++;
	}
	return 0;
}

static int jxh62_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int jxh62_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = jxh62_read(sd, 0x0a, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != JXH62_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = jxh62_read(sd, 0x0b, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != JXH62_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int jxh62_set_integration_time(struct v4l2_subdev *sd, int value)
{
	unsigned int expo = value;
	int ret = 0;
	/* printk("sensor expo write value 0x%x\n",expo); */
	jxh62_write(sd, 0x01, (unsigned char)(expo & 0xff));
	jxh62_write(sd, 0x02, (unsigned char)((expo >> 8) & 0xff));
	if (ret < 0)
		return ret;

	return 0;
}
static int jxh62_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[6:0] */
	/* printk("sensor again write value 0x%x\n",value); */
	jxh62_write(sd, 0x00, (unsigned char)(value & 0x7f));
	return 0;
}
static int jxh62_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[7] if gain > 2X set 0; if gain > 4X set 1 */
	return 0;
}

static int jxh62_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
#if 0
	int ret = 0;
	int black = 0;
	unsigned char h,l;
	unsigned char reg = 0xff;
	unsigned int * v = (unsigned int *)(value);
	ret = jxh62_read(sd, 0x48, &h);
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
	ret = jxh62_read(sd, reg, &l);
	if (ret < 0)
		return ret;
	*v = (black | l);
#endif
	return 0;
}

static int jxh62_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &jxh62_win_sizes[0];
	int ret = 0;
	unsigned char val;

	if(!enable)
		return ISP_SUCCESS;

	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = jxh62_write_array(sd, wsize->regs);
	ret = jxh62_read(sd, 0x09, &val);
	/* if (val == 0x00 || val == 0x80) */
	/* 	jxh62_write_array(sd, jxh62_init_version_80); */
	/* else if(val == 0x81) */
	/* 	jxh62_write_array(sd, jxh62_init_version_81); */
	/* if (ret) */
	/* 	return ret; */
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int jxh62_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = jxh62_write_array(sd, jxh62_stream_on);
		pr_debug("jxh62 stream on\n");
	}
	else {
		ret = jxh62_write_array(sd, jxh62_stream_off);
		pr_debug("jxh62 stream off\n");
	}
	return ret;
}

static int jxh62_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	/*printk("functiong:%s, line:%d\n", __func__, __LINE__); */
	return 0;
}

static int jxh62_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	/*printk("functiong:%s, line:%d\n", __func__, __LINE__); */
	return 0;
}


static int jxh62_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = JXH62_SUPPORT_PCLK;
	unsigned short hts;
	unsigned short vts = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	unsigned int vts_tmp = 0;

	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;
	ret += jxh62_read(sd, 0x21, &tmp);
	hts = tmp;
	ret += jxh62_read(sd, 0x20, &tmp);
	if(ret < 0)
		return -1;
	hts = (hts << 8) + tmp;
	/*vts = (pclk << 4) / (hts * (newformat >> 4));*/
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
#if 0
	ret = jxh62_write(sd, 0x22, (unsigned char)(vts & 0xff));
	ret += jxh62_write(sd, 0x23, (unsigned char)(vts >> 8));
	if(ret < 0)
		return -1;
#else
	jxh62_write(sd, 0xc0, 0x22);
	jxh62_write(sd, 0xc1, (unsigned char)(vts & 0xff));
	jxh62_write(sd, 0xc2, 0x23);
	jxh62_write(sd, 0xc3, (unsigned char)(vts >> 8));
	ret = jxh62_read(sd, 0x1f, &tmp);
	pr_debug("before register 0x1f value : 0x%02x\n", tmp);
	if(ret < 0)
		return -1;
	tmp |= (1 << 7); //set bit[7],  register group write function,  auto clean
	jxh62_write(sd, 0x1f, tmp);
	pr_debug("after register 0x1f value : 0x%02x\n", tmp);
#endif


	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 4;
	sensor->video.attr->integration_time_limit = vts - 4;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 4;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return 0;
}

static int jxh62_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &jxh62_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &jxh62_win_sizes[0];
	}
	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		if(sensor->priv != wsize){
			ret = jxh62_write_array(sd, wsize->regs);
			if(!ret)
				sensor->priv = wsize;
		}
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}
static int jxh62_g_chip_ident(struct v4l2_subdev *sd,
			      struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"jxh62_reset");
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
		ret = gpio_request(pwdn_gpio, "jxh62_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			msleep(50);
			gpio_direction_output(pwdn_gpio, 0);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = jxh62_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an jxh62 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "jxh62 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int jxh62_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long jxh62_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = jxh62_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = jxh62_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = jxh62_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = jxh62_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = jxh62_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		//	ret = jxh62_write_array(sd, jxh62_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		//	ret = jxh62_write_array(sd, jxh62_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = jxh62_set_fps(sensor, ctrl->value);
		break;
	default:
		pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
		break;;
	}
	return 0;
}
static long jxh62_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = jxh62_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int jxh62_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = jxh62_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int jxh62_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	jxh62_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops jxh62_core_ops = {
	.g_chip_ident = jxh62_g_chip_ident,
	.reset = jxh62_reset,
	.init = jxh62_init,
	.s_power = jxh62_s_power,
	.ioctl = jxh62_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = jxh62_g_register,
	.s_register = jxh62_s_register,
#endif
};

static const struct v4l2_subdev_video_ops jxh62_video_ops = {
	.s_stream = jxh62_s_stream,
	.s_parm = jxh62_s_parm,
	.g_parm = jxh62_g_parm,
};

static const struct v4l2_subdev_ops jxh62_ops = {
	.core = &jxh62_core_ops,
	.video = &jxh62_video_ops,
};

static int jxh62_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &jxh62_win_sizes[0];
	enum v4l2_mbus_pixelcode mbus;
	int i = 0;
	int ret = -1;

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
	clk_set_rate(sensor->mclk, 24000000);//MCLK=24MHz
	clk_enable(sensor->mclk);

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	jxh62_attr.dvp.gpio = sensor_gpio_func;
	switch(sensor_gpio_func){
	case DVP_PA_LOW_8BIT:
	case DVP_PA_HIGH_8BIT:
		mbus = jxh62_mbus_code[0];
		break;
	case DVP_PA_LOW_10BIT:
	case DVP_PA_HIGH_10BIT:
		mbus = jxh62_mbus_code[1];
		break;
	default:
		goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(jxh62_win_sizes); i++)
		jxh62_win_sizes[i].mbus_code = mbus;
	/*
	  convert sensor-gain into isp-gain,
	*/
	jxh62_attr.max_again = 	log2_fixed_to_fixed(jxh62_attr.max_again, TX_ISP_GAIN_FIXED_POINT, LOG2_GAIN_SHIFT);
	jxh62_attr.max_dgain = jxh62_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &jxh62_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &jxh62_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int jxh62_remove(struct i2c_client *client)
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

static const struct i2c_device_id jxh62_id[] = {
	{ "jxh62", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, jxh62_id);

static struct i2c_driver jxh62_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "jxh62",
	},
	.probe		= jxh62_probe,
	.remove		= jxh62_remove,
	.id_table	= jxh62_id,
};

static __init int init_jxh62(void)
{
	return i2c_add_driver(&jxh62_driver);
}

static __exit void exit_jxh62(void)
{
	i2c_del_driver(&jxh62_driver);
}

module_init(init_jxh62);
module_exit(exit_jxh62);

MODULE_DESCRIPTION("A low-level driver for SOI jxh62 sensors");
MODULE_LICENSE("GPL");
