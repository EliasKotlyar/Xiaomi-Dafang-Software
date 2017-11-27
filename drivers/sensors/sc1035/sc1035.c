/*
 * sc1035.c
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

#define SC1035_CHIP_ID_H		(0xf0)
#define SC1035_CHIP_ID_L		(0x00)

#define SC1035_REG_END		        0xff
#define SC1035_REG_DELAY		0xfe

#define SC1035_SUPPORT_PCLK (54*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 25
#define SENSOR_OUTPUT_MIN_FPS 5
#define DRIVE_CAPABILITY_1

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

static int sensor_gpio_func = DVP_PA_12BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");


struct again_lut {
	unsigned char value;
	unsigned int gain;
};
struct again_lut sc1035_again_lut[] = {
	{0x0, 0},
	{0x1, 2794},
	{0x2, 5509},
	{0x3, 9011},
	{0x4, 12388},
	{0x5, 15648},
	{0x6, 19572},
	{0x7, 23339},
	{0x8, 26962},
	{0x9, 31134},
	{0xa, 35130},
	{0xb, 39587},
	{0xc, 44437},
	{0xd, 49050},
	{0xe, 54517},
	{0xf, 59684},
	{0x10, 65535},
	{0x11, 68329},
	{0x12, 71489},
	{0x13, 74546},
	{0x14, 77923},
	{0x15, 81583},
	{0x16, 85107},
	{0x17, 88874},
	{0x18, 92497},
	{0x19, 96669},
	{0x1a, 100990},
	{0x1b, 105433},
	{0x1c, 109972},
	{0x1d, 114866},
	{0x1e, 120052},
	{0x1f, 125470},
	{0x93, 128676},
	{0x94, 132010},
	{0x95, 135457},
	{0x96, 139217},
	{0x97, 142834},
	{0x98, 146718},
	{0x99, 150834},
	{0x9a, 154962},
	{0x9b, 159443},
	{0x9c, 164056},
	{0x9d, 168931},
	{0x9e, 174018},
	{0x9f, 179414},
	{0xb0, 185054},
	{0xb1, 187947},
	{0xb2, 191005},
	{0xb3, 194332},
	{0xb4, 197662},
	{0xb5, 201105},
	{0xb6, 204752},
	{0xb7, 208473},
	{0xb8, 212253},
	{0xb9, 216369},
	{0xba, 220589},
	{0xbb, 224978},
	{0xbc, 229591},
	{0xbd, 234466},
	{0xbe, 239553},
	{0xbf, 245020},
	{0xf0, 250589},
	{0xf1, 253482},
	{0xf2, 256603},
	{0xf3, 259867},
	{0xf4, 263256},
	{0xf5, 266640},
	{0xf6, 270342},
	{0xf7, 274060},
	{0xf8, 277788},
	{0xf9, 281904},
	{0xfa, 286169},
	{0xfb, 290513},
	{0xfc, 295168},
	{0xfd, 300001},
	{0xfe, 305125},
	{0xff, 310590}
};

struct tx_isp_sensor_attribute sc1035_attr;

unsigned int sc1035_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = sc1035_again_lut;
	while(lut->gain <= sc1035_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = 0;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == sc1035_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int sc1035_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return 0;
}
struct tx_isp_sensor_attribute sc1035_attr={
	.name = "sc1035",
	.chip_id = 0xf000,
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
	.max_again = 310397,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 1196,
	.integration_time_limit = 1196,
	.total_width = 1800,
	.total_height = 1200,
	.max_integration_time = 1196,
	.one_line_expr_in_us = 33,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 2,
	.sensor_ctrl.alloc_again = sc1035_alloc_again,
	.sensor_ctrl.alloc_dgain = sc1035_alloc_dgain,
	//void priv; /* point to struct tx_isp_sensor_board_info */
};


static struct regval_list sc1035_init_regs_1280_960_25fps[] = {
#if 0
	{0x3000, 0x01},
	{0x3003, 0x01},
	{0x3400, 0x53},//RNC_enable
	{0x3416, 0xc0},//RNC(BLC)_Target
	{0x3d08, 0x01},
	{0x5000, 0x00},//close sensor isp
	{0x3e03, 0x03},//close sensor AEC
	{0x3e01, 0x23},
	{0x3e02, 0x00},
	{0x3e09, 0x10},
	{0x3e0f, 0x14},//close digital gain
	{0x3928, 0x00},
	{0x3622, 0x2e},
	{0x3630, 0x58},
	{0x3612, 0x00},
	{0x3632, 0x41},
	{0x3635, 0x04},
	{0x3500, 0x10},
	{0x3631, 0x80},
	{0x3620, 0x44},
	{0x3633, 0x7c},
	{0x3780, 0x0b},
	{0x3300, 0x33},
	{0x3301, 0x61},
	{0x3302, 0x30},
	{0x3303, 0x56},
	{0x3304, 0xa0},
	{0x3305, 0x72},
	{0x331e, 0x56},
	{0x321e, 0x00},
	{0x321f, 0x0a},
	{0x3216, 0x0a},
	{0x3115, 0x0a},
	{0x3332, 0x38},
	{0x5054, 0x82},
	{0x3907, 0x01},
	{0x3908, 0xc0},
	{0x3010, 0x08},//PLL_CTRL
	{0x3011, 0xe6},
	{0x3004, 0x04},//SC_REG04
	{0x320c, 0x07},/* HTS */
	{0x320d, 0x08},
	{0x320e, 0x04},/* VTS */
	{0x320f, 0xb0},
	{0x3210, 0x00},
	{0x3211, 0x62},
	{0x3212, 0x00},
	{0x3213, 0x08},
	{0x3208, 0x05},
	{0x3209, 0x00},
	{0x320a, 0x03},
	{0x320b, 0xc0},
	{0x3202, 0x00},
	{0x3203, 0x08},
	{0x3206, 0x03},
	{0x3207, 0xcf},
	{0x3330, 0x0d},//sa1timing
	{0x3320, 0x06},
	{0x3321, 0xE8},
	{0x3322, 0x01},
	{0x3323, 0xf0},
	{0x503d, 0x00},//color bar bit [7]
	{0x3315, 0x44},//bl_en all high
	{0x3301, 0x38},
	{0x3308, 0x40},
	{0x3610, 0x03},
	{0x3600, 0x7c},
	{0x3000, 0x00},
#else
	{0x3000, 0x01},//manualstreamenbale
	{0x3003, 0x01},//softreset
	{0x3400, 0x53},
	{0x3416, 0xc0},
	{0x3d08, 0x01},
#ifdef	DRIVE_CAPABILITY_1
	{0x3640, 0x01},
#elif defined(DRIVE_CAPABILITY_2)
	{0x3640, 0x02},
#elif defined(DRIVE_CAPABILITY_3)
	{0x3640, 0x03},
#endif
	{0x3e03, 0x03},
	{0x3928, 0x00},
	{0x3630, 0x58},
	{0x3612, 0x00},
	{0x3632, 0x41},
	{0x3635, 0x04},
	{0x3500, 0x10},
	{0x3620, 0x44},
	{0x3633, 0x7c},
	{0x3780, 0x0b},
	{0x3300, 0x33},
	{0x3301, 0x38},
	{0x3302, 0x30},
	{0x3303, 0x66},
	{0x3304, 0xa0},
	{0x3305, 0x72},
	{0x331e, 0x56},
	{0x321e, 0x00},
	{0x321f, 0x0a},
	{0x3216, 0x0a},
	{0x3332, 0x38},
	{0x5054, 0x82},
	{0x3622, 0x26},
	{0x3907, 0x02},
	{0x3908, 0x00},
	{0x3601, 0x18},
	{0x3315, 0x44},
	{0x3308, 0x40},
	{0x3223, 0x22},//vysncmode[5]
	{0x3e0e, 0x50},
	{0x3101, 0x9b},
	{0x3114, 0x03},
	{0x3115, 0xd1},
	{0x3211, 0x60},
	{0x5780, 0xff},
	{0x5781, 0x7f},
	{0x5785, 0xff},
	{0x5000, 0x00},
	{0x3e0f, 0x14},
	{0x3631, 0x82},
	{0x3010, 0x08},
	{0x3011, 0xe6},
	{0x3004, 0x04},
	{0x3610, 0x2b},
	{0x320c, 0x07}, //HTS 1800
	{0x320d, 0x08},
	{0x320e, 0x04}, //VTS 1200
	{0x320f, 0xb0},
	{0x3210, 0x00},
	{0x3211, 0x60},
	{0x3212, 0x00},
	{0x3213, 0x09},
	{0x3208, 0x05},
	{0x3209, 0x00},
	{0x320a, 0x03},
	{0x320b, 0xc0},
	{0x3202, 0x00},
	{0x3203, 0x08},
	{0x3206, 0x03},
	{0x3207, 0xcf},
	{0x3330, 0x0d},
	{0x3320, 0x06},
	{0x3321, 0xe8},
	{0x3322, 0x01},
	{0x3323, 0xc0},
	/* {0x503d, 0x00},//color bar bit [7] */
	{0x3600, 0x54},
	{0x3000, 0x00},
#endif
	{SC1035_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the sc1035_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sc1035_win_sizes[] = {
	/* 1280*960 */
	{
		.width		= 1280,
		.height		= 960,
		.fps		= 25 << 16 | 1, /* 25 fps */
		.mbus_code	= V4L2_MBUS_FMT_SGRBG12_1X12,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= sc1035_init_regs_1280_960_25fps,
	}
};

static enum v4l2_mbus_pixelcode sc1035_mbus_code[] = {
	V4L2_MBUS_FMT_SGRBG10_1X10,
	V4L2_MBUS_FMT_SGRBG12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list sc1035_stream_on[] = {

	{0x3000, 0x01},
	{SC1035_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc1035_stream_off[] = {

	{0x3000, 0x00},
	{SC1035_REG_END, 0x00},	/* END MARKER */
};

int sc1035_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value)
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

int sc1035_write(struct v4l2_subdev *sd, unsigned short reg, unsigned char value)
{
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
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

static int sc1035_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SC1035_REG_END) {
		if (vals->reg_num == SC1035_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = sc1035_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		/*printk("read vals->reg_num:0x%02x, vals->value:0x%02x\n",vals->reg_num, val);*/

		vals++;
	}

	return 0;
}

static int sc1035_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SC1035_REG_END) {
		if (vals->reg_num == SC1035_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = sc1035_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}

		//printk("write vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);

		vals++;
	}

	return 0;
}

static int sc1035_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sc1035_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	int ret;
	unsigned char v;

	ret = sc1035_read(sd, 0x580b, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;

	if (v != SC1035_CHIP_ID_H)
		return -ENODEV;
	*ident = v;
	ret = sc1035_read(sd, 0x3c05, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;

	if (v != SC1035_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;

	return 0;
}

static int sc1035_set_integration_time(struct v4l2_subdev *sd, int value)
{
	unsigned int expo = value;
	int ret = 0;
	int exposure_thresh = sc1035_attr.max_integration_time * 7 / 10;
	unsigned char reg_value = 0xe0;
	if (value <= exposure_thresh)
		reg_value = 0xe0;
	else if (value < sc1035_attr.max_integration_time)
		reg_value = 709 - (693 * value / (sc1035_attr.max_integration_time));
	else
		reg_value =  0x10;

	sc1035_write(sd, 0x331e, reg_value);
	ret += sc1035_write(sd, 0x3e01, (unsigned char)((expo >> 4) & 0xff));
	ret += sc1035_write(sd, 0x3e02, (unsigned char)((expo & 0x0f) << 4));

	if (ret < 0)
		return ret;

	return 0;
}
static int sc1035_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	ret = sc1035_write(sd, 0x3e09, (unsigned char)value);
	if (ret < 0)
		return ret;

	return 0;
}
static int sc1035_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[7] if gain > 2X set 0; if gain > 4X set 1 */
	return 0;
}

static int sc1035_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sc1035_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &sc1035_win_sizes[0];
	int ret = 0;

	if(!enable)
		return ISP_SUCCESS;

	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = sc1035_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int sc1035_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sc1035_write_array(sd, sc1035_stream_on);
		printk("sc1035 stream on\n");
	}
	else {
		ret = sc1035_write_array(sd, sc1035_stream_off);
		printk("sc1035 stream off\n");
	}
	return ret;
}

static int sc1035_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sc1035_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}


static int sc1035_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = SC1035_SUPPORT_PCLK;
	unsigned short hts;
	unsigned short vts = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;
	ret += sc1035_read(sd, 0x320c, &tmp);
	hts = tmp;
	ret += sc1035_read(sd, 0x320d, &tmp);
	if(ret < 0)
		return -1;
	hts = (hts << 8) + tmp;
	/*vts = (pclk << 4) / (hts * (newformat >> 4));*/
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret = sc1035_write(sd, 0x320f, (unsigned char)(vts & 0xff));
	ret += sc1035_write(sd, 0x320e, (unsigned char)(vts >> 8));
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

static int sc1035_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &sc1035_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &sc1035_win_sizes[0];
	}
	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		if(sensor->priv != wsize){
			ret = sc1035_write_array(sd, wsize->regs);
			if(!ret)
				sensor->priv = wsize;
		}
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}
static int sc1035_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"sc1035_reset");
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
		ret = gpio_request(pwdn_gpio, "sc1035_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			mdelay(50);
			gpio_direction_output(pwdn_gpio, 0);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = sc1035_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an sc1035 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "sc1035 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sc1035_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long sc1035_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = sc1035_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = sc1035_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = sc1035_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = sc1035_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = sc1035_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		//	ret = sc1035_write_array(sd, sc1035_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		//	ret = sc1035_write_array(sd, sc1035_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = sc1035_set_fps(sensor, ctrl->value);
			break;
		default:
			break;;
	}
	return 0;
}
static long sc1035_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = sc1035_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc1035_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = sc1035_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int sc1035_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	sc1035_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops sc1035_core_ops = {
	.g_chip_ident = sc1035_g_chip_ident,
	.reset = sc1035_reset,
	.init = sc1035_init,
	.s_power = sc1035_s_power,
	.ioctl = sc1035_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sc1035_g_register,
	.s_register = sc1035_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sc1035_video_ops = {
	.s_stream = sc1035_s_stream,
	.s_parm = sc1035_s_parm,
	.g_parm = sc1035_g_parm,
};

static const struct v4l2_subdev_ops sc1035_ops = {
	.core = &sc1035_core_ops,
	.video = &sc1035_video_ops,
};

static int sc1035_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &sc1035_win_sizes[0];
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

	sc1035_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = sc1035_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = sc1035_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(sc1035_win_sizes); i++)
		sc1035_win_sizes[i].mbus_code = mbus;

	 /*
		convert sensor-gain into isp-gain,
	 */
	sc1035_attr.max_again = 310397;
	sc1035_attr.max_dgain = sc1035_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &sc1035_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &sc1035_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int sc1035_remove(struct i2c_client *client)
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

static const struct i2c_device_id sc1035_id[] = {
	{ "sc1035", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc1035_id);

static struct i2c_driver sc1035_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "sc1035",
	},
	.probe		= sc1035_probe,
	.remove		= sc1035_remove,
	.id_table	= sc1035_id,
};

static __init int init_sc1035(void)
{
	return i2c_add_driver(&sc1035_driver);
}

static __exit void exit_sc1035(void)
{
	i2c_del_driver(&sc1035_driver);
}

module_init(init_sc1035);
module_exit(exit_sc1035);

MODULE_DESCRIPTION("A low-level driver for OmniVision sc1035 sensors");
MODULE_LICENSE("GPL");
