/*
 * sp1409.c
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

#define SP1409_CHIP_ID_H		(0x14)
#define SP1409_CHIP_ID_L		(0x09)

#define SP1409_REG_END			0xff
#define SP1409_REG_DELAY		0xfefe

#define SP1409_SUPPORT_PCLK (20000*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
#define DRIVE_CAPABILITY_1

struct regval_list {
	unsigned char reg_num;
	unsigned char value;
};

static int reset_gpio = -1;
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_LOW_10BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

struct again_lut {
	unsigned int rpc;
	unsigned char vrefl;
	unsigned int gain;
};
unsigned int final_vrefl=0;
struct again_lut sp1409_again_lut[] = {
	{0x10,0x0,0},
	{0x11,0x05,573},
	{0x12,0x0a,11136},
	{0x13,0x12,16248},
	{0x14,0x1a,21097},
	{0x15,0x21,25710},
	{0x16,0x27,30109},
	{0x17,0x2c,34312},
	{0x18,0x31,38336},
	{0x19,0x36,42195},
	{0x1a,0x3a,45904},
	{0x1b,0x3e,49472},
	{0x1c,0x42,52910},
	{0x1d,0x45,56228},
	{0x1e,0x48,59433},
	{0x1f,0x4b,62534},
	{0x20,0x4E,65536},
	{0x22,0x53,71267},
	{0x24,0x58,76672},
	{0x26,0x5c,81784},
	{0x28,0x60,86633},
	{0x2a,0x63,91246},
	{0x2c,0x66,95645},
	{0x2e,0x69,99848},
	{0x30,0x6B,103872},
	{0x32,0x6D,107731},
	{0x34,0x6F,111440},
	{0x36,0x71,115008},
	{0x38,0x73,118446},
	{0x3a,0x75,121764},
	{0x3c,0x77,124969},
	{0x3e,0x78,128070},
	{0x40,0x7a,131072},
	{0x44,0x7c,136803},
	{0x48,0x7e,142208},
	{0x4c,0x80,147320},
	{0x50,0x82,152169},
	{0x54,0x84,156782},
	{0x58,0x86,161181},
	{0x5c,0x87,165384},
	{0x60,0x88,169408},
	{0x64,0x89,173267},
	{0x68,0x8a,176976},
	{0x6c,0x8b,180544},
	{0x70,0x8c,183982},
	{0x74,0x8d,187300},
	{0x78,0x8e,190505},
	{0x7c,0x8f,193606},
	{0x80,0x90,196608},
	{0x88,0x91,202339},
	{0x90,0x92,207744},
	{0x98,0x93,212856},
	{0xa0,0x94,217705},
	{0xa8,0x95,222318},
	{0xb0,0x95,226717},
	{0xb9,0x96,231432},
	{0xc0,0x96,234944},
	{0xc8,0x97,238803},
	{0xd0,0x97,242512},
	{0xd8,0x98,246080},
	{0xe0,0x98,249518},
	{0xe8,0x99,252836},
	{0xf0,0x99,256041},
	{0xf8,0x99,259142},
	{0x100,0x9a,262144},
};

struct tx_isp_sensor_attribute sp1409_attr;
unsigned int sp1409_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = sp1409_again_lut;
	while(lut->gain <= sp1409_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = 0x10;
			final_vrefl=0;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->rpc;
			final_vrefl=(lut-1)->vrefl;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == sp1409_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = 0x100;
				final_vrefl=0x9a;
				return lut->gain;
			}
		}

		lut++;
	}
	/* printk("lut->rpc ======0x%x,final_vrefl ===========0x%x isp_gain==========%d\n",lut->rpc,final_vrefl,isp_gain); */
	return isp_gain;
}

unsigned int sp1409_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute sp1409_attr={
	.name = "sp1409",
	.chip_id = 0x1409,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
	.cbus_device = 0x34,
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
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 895,
	.integration_time_limit = 895,
	.total_width = 1328,
	.total_height = 899,
	.max_integration_time = 895,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = sp1409_alloc_again,
	.sensor_ctrl.alloc_dgain = sp1409_alloc_dgain,
	.one_line_expr_in_us = 44,
};

static struct regval_list sp1409_init_regs_1280_720_25fps[] = {
	{0xfd, 0x00},
	{0x24, 0x01},
	{0x27, 0x36},
	{0x26, 0x02},
	{0x28, 0x00},
	{0x40, 0x00},
	{0x25, 0x00},
	{0x41, 0x01},
	{0x2e, 0x01}, //
	{0x89, 0x02},
	{0x90, 0x01},
	{0x69, 0x05},
	{0x6a, 0x00},
	{0x6b, 0x02},
	{0x6c, 0xD0},
	{0xfd, 0x01},
	{0x1f, 0x00},
	{0x1e, 0x00},

	{0xfd, 0x00},
	{0x3d, 0x00},
	{0x43, 0x03},
	{0x45, 0x05},
	{0x46, 0x02},
	{0x47, 0x02},
	{0x49, 0x00},
	{0x3f, 0x08},
	{0x1d, 0x55},
	{0x1e, 0x55},
	//{0x38, 0x00},
	//{0x55, 0x00},


	{0xfd, 0x01},
	{0x15, 0x00},
	{0x0a, 0x00},
	{0x2e, 0x0a},
	{0x30, 0x20},
	{0x31, 0x02},
	{0x33, 0x02},
	{0x34, 0x40},
	{0x36, 0x0A},
	{0x39, 0x01},
	{0x3a, 0x67},
	{0x3c, 0x07},
	{0x3d, 0x03},
	{0x3e, 0x00},
	{0x3f, 0x0B},
	{0x40, 0x00},
	{0x41, 0x3B},
	{0x4a, 0x49},
	{0x4b, 0x03},
	{0x4c, 0x90},
	{0x4f, 0x00},
	{0x50, 0x10},
	{0x4d, 0x02},
	{0x4e, 0xD8},
	{0x5c, 0x03},
	{0x64, 0x01},
	{0x65, 0xD0},
	{0x75, 0x05},
	{0x7b, 0x01},
	{0x7c, 0x55},
	{0x20, 0x30},
	{0x7e, 0x01},
	{0x1a, 0x01},
	{0x1b, 0x87},
	{0xfe, 0x01},

	{0xfd, 0x01},
	{0x77, 0x00},
	{0x78, 0x07},
	{0xfd, 0x02},
	{0x20, 0x00},
	{0x50, 0x03},
	{0x53, 0x03},
	{0x51, 0x03},
	{0x52, 0x04},
	{0x31, 0x01},
	{0x32, 0x81},
	{0x33, 0x03},
	{0x34, 0x80},
	{0x35, 0x00},
	{0x30, 0x00},//auto blc 0x09
	{0x80, 0x0f},//bad pixel
	{0x81, 0x1b},
 	{0x92, 0x20},

	{0xfd, 0x01},
	{0x16, 0x00},
	{0x17, 0x9c},
	{0xfd, 0x01},

	{0x06, 0x05},
	{0x07, 0x00},
	{0x08, 0x02},
	{0x09, 0xD0},

	{0xfd, 0x01},
	{0x02, 0x00},
	{0x03, 0x18},
	{0x04, 0x00},
	{0x05, 0x10},

	{0xfd, 0x01},
	{0x0c, 0x02},
	{0x0d, 0x9A},
	{0xfd, 0x01},
	{0x24, 0x20},
	{0x22, 0x00},
	{0x23, 0x10},
	{0xfd, 0x02},
	{0x70, 0x10},
	{0x60, 0x00},
	{0x61, 0x80},
	{0x62, 0x00},
	{0x63, 0x80},
	{0x64, 0x00},
	{0x65, 0x80},
	{0x66, 0x00},
	{0x67, 0x80},
	{0x68, 0x00},
	{0x69, 0x40},
	{0x6a, 0x00},
	{0x6b, 0x40},
	{0x6c, 0x00},
	{0x6d, 0x40},
	{0x6e, 0x00},
	{0x6f, 0x40},
	/* {0xfd, 0x02}, */
	/* {0x20, 0x01},//color */
	{0xfe, 0x01},
	{0xfe, 0x01},
	{0xfd, 0x01},
	{SP1409_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the sp1409_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sp1409_win_sizes[] = {
	/* 1280*960 */
	{
		.width		= 1280,
		.height		= 720,
		.fps		= 25 << 16 | 1, /* 25 fps */
		.mbus_code	= V4L2_MBUS_FMT_SRGGB10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= sp1409_init_regs_1280_720_25fps,
	}
};

/*
 * the part of driver was fixed.
 */

static struct regval_list sp1409_stream_on[] = {
	{0xfd, 0x01},
	{0x1e, 0x00},
	{0xfe, 0x02},
	{SP1409_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sp1409_stream_off[] = {
	{0xfd, 0x01},
	{0x1e, 0xff},
	{0xfd, 0x00},
	{0xfe, 0x02},
	{SP1409_REG_END, 0x00},	/* END MARKER */
};

int sp1409_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value)
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

int sp1409_write(struct v4l2_subdev *sd, unsigned char reg, unsigned char value)
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

static int sp1409_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SP1409_REG_END) {
		if (vals->reg_num == SP1409_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = sp1409_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		/* printk("read vals->reg_num:0x%02x, vals->value:0x%02x\n",vals->reg_num, val); */

		vals++;
	}

	return 0;
}

static int sp1409_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SP1409_REG_END) {
		if (vals->reg_num == SP1409_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = sp1409_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}

		vals++;
	}

	return 0;
}

static int sp1409_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sp1409_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	int ret;
	unsigned char v;
	unsigned char page=0;
	sp1409_read(sd, 0xfd, &page);
	if(page!=0){
		sp1409_write(sd, 0xfd, 0x0);
		sp1409_write(sd, 0xfe, 0x2);
	}

	ret = sp1409_read(sd, 0x04, &v);
	/* printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v); */
	if (ret < 0)
		return ret;

	if (v != SP1409_CHIP_ID_H)
		return -ENODEV;
	*ident = v;
	ret = sp1409_read(sd, 0x05, &v);
	/* printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v); */
	if (ret < 0)
		return ret;

	if (v != SP1409_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;

	return 0;
}

static int sp1409_set_integration_time(struct v4l2_subdev *sd, int value)
{
	unsigned int expo = value;
	int ret = 0;
	unsigned char page=0;
	sp1409_read(sd, 0xfd, &page);
	if(page!=1){
		sp1409_write(sd, 0xfd, 0x01);
		sp1409_write(sd, 0xfe, 0x02);
	}
	ret += sp1409_write(sd, 0x0c, (unsigned char)((expo >> 8) & 0xff));
	ret += sp1409_write(sd, 0x0d, (unsigned char)(expo & 0xff));
	sp1409_write(sd, 0xfe, 0x02);
	if (ret < 0)
		return ret;

	return 0;
}

static int sp1409_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	unsigned char page=0;
	sp1409_read(sd, 0xfd, &page);
	if(page!=1){
		sp1409_write(sd, 0xfd, 0x01);
		sp1409_write(sd, 0xfe, 0x02);
	}
	/* printk("a gain value=0x%x,final_verfl=0x%x\n",value,final_vrefl); */
	sp1409_write(sd, 0x22, (unsigned char)(value>>8));
	sp1409_write(sd, 0x23, (unsigned char)(value&0xff));
	sp1409_write(sd, 0x7f, final_vrefl);
	sp1409_write(sd, 0xfe, 0x02);
	if (ret < 0)
		return ret;

	return 0;
}

static int sp1409_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[7] if gain > 2X set 0; if gain > 4X set 1 */
	/* int ret = 0; */

	/* ret = sp1409_write(sd, 0x3610, (unsigned char)value); */
	/* if (ret < 0) */
	/* 	return ret; */
	return 0;
}

static int sp1409_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sp1409_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &sp1409_win_sizes[0];
	int ret = 0;

	if(!enable)
		return ISP_SUCCESS;

	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = sp1409_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int sp1409_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sp1409_write_array(sd, sp1409_stream_on);
		printk("sp1409 stream on\n");
	}
	else {
		ret = sp1409_write_array(sd, sp1409_stream_off);
		printk("sp1409 stream off\n");
	}
	return ret;
}

static int sp1409_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sp1409_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}


static int sp1409_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = SP1409_SUPPORT_PCLK;
	unsigned short hts;
	unsigned short vts_pre;
	unsigned short vsize_eff;
	unsigned short vblank;
	unsigned short vts = 0;
	unsigned char tmp,tmp1,tmp2;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	unsigned char page=0;
	/* fps=PCLK*1000000/(vts*hts) */

	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;

	sp1409_read(sd, 0xfd, &page);
	if(page!=1){
		sp1409_write(sd, 0xfd, 0x01);

	}
	ret += sp1409_read(sd, 0x25, &tmp);
	hts = tmp;
	ret += sp1409_read(sd, 0x26, &tmp);
	sp1409_write(sd, 0xfe, 0x02);
	if(ret < 0)
		return -1;
	hts = (hts << 8) + tmp;

	ret += sp1409_read(sd, 0x28, &tmp);
	vts_pre = tmp;
	ret += sp1409_read(sd, 0x27, &tmp);
	sp1409_write(sd, 0xfe, 0x02);
	if(ret < 0)
		return -1;
	vts_pre = (vts_pre << 8) + tmp;

	ret += sp1409_read(sd, 0x17, &tmp);
	vblank = tmp;
	ret += sp1409_read(sd, 0x16, &tmp);
	sp1409_write(sd, 0xfe, 0x02);
	if(ret < 0)
		return -1;
	vblank = (vblank << 8) + tmp;

	vsize_eff=vts_pre-vblank;
	/*vts = (pclk << 4) / (hts * (newformat >> 4));*/
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret = sp1409_write(sd, 0x17, (unsigned char)((vts - vsize_eff) & 0xff));
	ret += sp1409_write(sd, 0x16, (unsigned char)((vts - vsize_eff) >> 8));
	sp1409_write(sd, 0xfe, 0x02);
	ret += sp1409_read(sd, 0x17, &tmp1);
	ret += sp1409_read(sd, 0x16, &tmp2);

	/* printk("hts======0x%x,vts ========0x%x,vsize_eff========0x%x vblank====0x%x   read 1716 0x%x\n",hts,vts,vsize_eff,vblank,tmp1<<8|tmp2); */
	if(ret < 0)
		return -1;
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 5;
	sensor->video.attr->integration_time_limit = vts - 5;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 5;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return 0;
}

static int sp1409_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	/* printk("functiong:%s, line:%d\n", __func__, __LINE__); */
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &sp1409_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &sp1409_win_sizes[0];
	}
	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		if(sensor->priv != wsize){
			ret = sp1409_write_array(sd, wsize->regs);
			if(!ret)
				sensor->priv = wsize;
		}
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}

static int sp1409_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"sp1409_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			mdelay(10);
			gpio_direction_output(reset_gpio, 0);
			mdelay(10);
			gpio_direction_output(reset_gpio, 1);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if (pwdn_gpio != -1) {
		ret = gpio_request(pwdn_gpio, "sp1409_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			mdelay(50);
			gpio_direction_output(pwdn_gpio, 0);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = sp1409_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sp1409 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "sp1409 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sp1409_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static long sp1409_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = sp1409_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = sp1409_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = sp1409_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = sp1409_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = sp1409_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		//	ret = sp1409_write_array(sd, sp1409_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		//	ret = sp1409_write_array(sd, sp1409_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = sp1409_set_fps(sensor, ctrl->value);
		break;
	default:
		printk("do not support ctrl->cmd ====%d\n",ctrl->cmd);
		break;;
	}
	return 0;
}

static long sp1409_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = sp1409_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sp1409_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = sp1409_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int sp1409_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	sp1409_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops sp1409_core_ops = {
	.g_chip_ident = sp1409_g_chip_ident,
	.reset = sp1409_reset,
	.init = sp1409_init,
	.s_power = sp1409_s_power,
	.ioctl = sp1409_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sp1409_g_register,
	.s_register = sp1409_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sp1409_video_ops = {
	.s_stream = sp1409_s_stream,
	.s_parm = sp1409_s_parm,
	.g_parm = sp1409_g_parm,
};

static const struct v4l2_subdev_ops sp1409_ops = {
	.core = &sp1409_core_ops,
	.video = &sp1409_video_ops,
};

static int sp1409_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &sp1409_win_sizes[0];
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

	sp1409_attr.dvp.gpio = sensor_gpio_func;
	/*
	  convert sensor-gain into isp-gain,
	*/
	sp1409_attr.max_again = sp1409_attr.max_again;
	sp1409_attr.max_dgain = sp1409_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &sp1409_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &sp1409_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int sp1409_remove(struct i2c_client *client)
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

static const struct i2c_device_id sp1409_id[] = {
	{ "sp1409", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sp1409_id);

static struct i2c_driver sp1409_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "sp1409",
	},
	.probe		= sp1409_probe,
	.remove		= sp1409_remove,
	.id_table	= sp1409_id,
};

static __init int init_sp1409(void)
{
	return i2c_add_driver(&sp1409_driver);
}

static __exit void exit_sp1409(void)
{
	i2c_del_driver(&sp1409_driver);
}

module_init(init_sp1409);
module_exit(exit_sp1409);

MODULE_DESCRIPTION("A low-level driver for OmniVision sp1409 sensors");
MODULE_LICENSE("GPL");
