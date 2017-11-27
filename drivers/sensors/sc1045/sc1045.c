/*
 * sc1045.c
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

#define SC1045_CHIP_ID_H		(0x10)
#define SC1045_CHIP_ID_L		(0x45)

#define SC1045_REG_END		        0xff
#define SC1045_REG_DELAY		0xfe

#define SC1045_SUPPORT_PCLK (43200*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
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

static int sensor_gpio_func = DVP_PA_LOW_10BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

struct again_lut {
	unsigned char value;
	unsigned int gain;
};

struct again_lut sc1045_again_lut[] = {
	{0x0, 0},
	{0x1, 5402},
	{0x2, 10512},
	{0x3, 15368},
	{0x4, 19979},
	{0x5, 24376},
	{0x6, 28577},
	{0x7, 32606},
	{0x8, 36464},
	{0x9, 40171},
	{0xa, 43738},
	{0xb, 47181},
	{0xc, 50497},
	{0xd, 53701},
	{0xe, 56800},
	{0xf, 59806},
	{0x10, 65536},
	{0x11, 70938},
	{0x12, 76048},
	{0x13, 80904},
	{0x14, 85515},
	{0x15, 89912},
	{0x16, 94113},
	{0x17, 98142},
	{0x18, 102000},
	{0x19, 105707},
	{0x1a, 109274},
	{0x1b, 112717},
	{0x1c, 116033},
	{0x1d, 119237},
	{0x1e, 122336},
	{0x1f, 125342},
	{0x30, 131072},
	{0x31, 136474},
	{0x32, 141584},
	{0x33, 146440},
	{0x34, 151051},
	{0x35, 155448},
	{0x36, 159649},
	{0x37, 163678},
	{0x38, 167536},
	{0x39, 171243},
	{0x3a, 174810},
	{0x3b, 178253},
	{0x3c, 181569},
	{0x3d, 184773},
	{0x3e, 187872},
	{0x3f, 190878},
	{0x70, 196608},
	{0x71, 202010},
	{0x72, 207120},
	{0x73, 211976},
	{0x74, 216587},
	{0x75, 220984},
	{0x76, 225185},
	{0x77, 229214},
	{0x78, 233072},
	{0x79, 236779},
	{0x7a, 240346},
	{0x7b, 243789},
	{0x7c, 247105},
	{0x7d, 250309},
	{0x7e, 253408},
	{0x7f, 256414},
	{0xf0, 262144},
	{0xf1, 267546},
	{0xf2, 272656},
	{0xf3, 277512},
	{0xf4, 282123},
	{0xf5, 286520},
	{0xf6, 290721},
	{0xf7, 294750},
	{0xf8, 298608},
	{0xf9, 302315},
	{0xfa, 305882},
	{0xfb, 309325},
	{0xfc, 312641},
	{0xfd, 315845},
	{0xfe, 318944},
	{0xff, 321950},
};

struct tx_isp_sensor_attribute sc1045_attr;

unsigned int sc1045_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = sc1045_again_lut;
	while(lut->gain <= sc1045_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = 0;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == sc1045_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int sc1045_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute sc1045_attr={
	.name = "sc1045",
	.chip_id = 0x1045,
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
	.max_again = 256414,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 895,
	.integration_time_limit = 895,
	.total_width = 1920,
	.total_height = 900,
	.max_integration_time = 895,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = sc1045_alloc_again,
	.sensor_ctrl.alloc_dgain = sc1045_alloc_dgain,
	.one_line_expr_in_us = 44,
};

static struct regval_list sc1045_init_regs_1280_720_25fps[] = {
	{0x3003, 0x01}, //soft reset
	{0x3000, 0x00}, //pause for reg writing
	{0x3010, 0x01}, //output format 43.2M 1920X900 25fps
	{0x3011, 0xbe},
	{0x320e, 0x03},
	{0x320f, 0x84},
	{0x3004, 0x45},
	{0x3e03, 0x03}, //exp and gain
	{0x3600, 0x94}, //0607 reduce power
	{0x3610, 0x03},
	{0x3634, 0x00}, // reduce power
	{0x3620, 0x84},
	{0x3631, 0x85}, // txvdd 0910
#ifdef	DRIVE_CAPABILITY_1
	{0x3640, 0x01},
#elif defined(DRIVE_CAPABILITY_2)
	{0x3640, 0x02},
#elif defined(DRIVE_CAPABILITY_3)
	{0x3640, 0x03},
#endif
	{0x3907, 0x03},
	{0x3622, 0x0e},
	{0x3633, 0x2c}, //0825
	{0x3630, 0x88},
	{0x3635, 0x80},
	{0X3310, 0X83}, //prechg tx auto ctrl [5] 0825
	{0x3336, 0x00},
	{0x3337, 0x96},
	{0x3338, 0x03},
	{0x3339, 0x84},
	{0X331E, 0xa0},
	{0x3335, 0x10},
	{0X3315, 0X44},
	{0X3308, 0X40},
	{0X3415, 0X00},
	{0X3416, 0X40},
	{0x3330, 0x0d}, // sal_en timing,cancel the fpn in low light
	{0x3320, 0x05}, //0825
	{0x3321, 0x60},
	{0x3322, 0x02},
	{0x3323, 0xc0},
	{0x3303, 0xa0},
	{0x3304, 0x60},
	{0x3d04, 0x04}, //0806    vsync gen mode
	{0x3d08, 0x03},
	/* {0x3000, 0x01}, //recover   */
	{SC1045_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the sc1045_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sc1045_win_sizes[] = {
	/* 1280*960 */
	{
		.width		= 1280,
		.height		= 720,
		.fps		= 25 << 16 | 1, /* 25 fps */
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= sc1045_init_regs_1280_720_25fps,
	}
};

/*
 * the part of driver was fixed.
 */

static struct regval_list sc1045_stream_on[] = {

	{0x3000, 0x01},
	{SC1045_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc1045_stream_off[] = {

	{0x3000, 0x00},
	{SC1045_REG_END, 0x00},	/* END MARKER */
};

int sc1045_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value)
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

int sc1045_write(struct v4l2_subdev *sd, unsigned short reg, unsigned char value)
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

static int sc1045_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SC1045_REG_END) {
		if (vals->reg_num == SC1045_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = sc1045_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		/*printk("read vals->reg_num:0x%02x, vals->value:0x%02x\n",vals->reg_num, val);*/

		vals++;
	}

	return 0;
}

static int sc1045_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SC1045_REG_END) {
		if (vals->reg_num == SC1045_REG_DELAY) {
			if (vals->value >= (1000 / HZ))
				msleep(vals->value);
			else
				mdelay(vals->value);
		} else {
			ret = sc1045_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}

		vals++;
	}

	return 0;
}

static int sc1045_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sc1045_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	int ret;
	unsigned char v;

	ret = sc1045_read(sd, 0x3107, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;

	if (v != SC1045_CHIP_ID_H)
		return -ENODEV;
	*ident = v;
	ret = sc1045_read(sd, 0x3108, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;

	if (v != SC1045_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;

	return 0;
}

static int sc1045_set_integration_time(struct v4l2_subdev *sd, int value)
{
	unsigned int expo = value;
	int ret = 0;

	ret += sc1045_write(sd, 0x3e01, (unsigned char)((expo >> 4) & 0xff));
	ret += sc1045_write(sd, 0x3e02, (unsigned char)((expo & 0x0f) << 4));
	if (ret < 0)
		return ret;

	return 0;
}

static int sc1045_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	/* if ((value & 0x80) != 0) */
	/* 	ret = sc1045_write(sd, 0x3610, 0x1b); */
	/* else  */
	/* 	ret = sc1045_write(sd, 0x3610, 0x03); */

	ret += sc1045_write(sd, 0x3e09, (unsigned char)(value & 0x7f));
	if (ret < 0)
		return ret;

	return 0;
}

static int sc1045_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[7] if gain > 2X set 0; if gain > 4X set 1 */
	/* int ret = 0; */

	/* ret = sc1045_write(sd, 0x3610, (unsigned char)value); */
	/* if (ret < 0) */
	/* 	return ret; */
	return 0;
}

static int sc1045_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sc1045_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &sc1045_win_sizes[0];
	int ret = 0;

	if(!enable)
		return ISP_SUCCESS;

	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = sc1045_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int sc1045_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sc1045_write_array(sd, sc1045_stream_on);
		printk("sc1045 stream on\n");
	}
	else {
		ret = sc1045_write_array(sd, sc1045_stream_off);
		printk("sc1045 stream off\n");
	}
	return ret;
}

static int sc1045_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sc1045_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}


static int sc1045_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = SC1045_SUPPORT_PCLK;
	unsigned short hts;
	unsigned short vts = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;
	ret += sc1045_read(sd, 0x320c, &tmp);
	hts = tmp;
	ret += sc1045_read(sd, 0x320d, &tmp);
	if(ret < 0)
		return -1;
	hts = (hts << 8) + tmp;
	/*vts = (pclk << 4) / (hts * (newformat >> 4));*/
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret = sc1045_write(sd, 0x320f, (unsigned char)(vts & 0xff));
	ret += sc1045_write(sd, 0x320e, (unsigned char)(vts >> 8));
	ret = sc1045_write(sd, 0x3339, (unsigned char)(vts & 0xff));
	ret += sc1045_write(sd, 0x3338, (unsigned char)(vts >> 8));
	ret = sc1045_write(sd, 0x3337, (unsigned char)((vts - 0x2ee) & 0xff));
	ret += sc1045_write(sd, 0x3336, (unsigned char)((vts - 0x2ee) >> 8));
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

static int sc1045_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &sc1045_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &sc1045_win_sizes[0];
	}
	if(wsize){
		sensor->video.mbus.width = wsize->width;
		sensor->video.mbus.height = wsize->height;
		sensor->video.mbus.code = wsize->mbus_code;
		sensor->video.mbus.field = V4L2_FIELD_NONE;
		sensor->video.mbus.colorspace = wsize->colorspace;
		if(sensor->priv != wsize){
			ret = sc1045_write_array(sd, wsize->regs);
			if(!ret)
				sensor->priv = wsize;
		}
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}

static int sc1045_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"sc1045_reset");
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
		ret = gpio_request(pwdn_gpio, "sc1045_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			mdelay(50);
			gpio_direction_output(pwdn_gpio, 0);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = sc1045_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an sc1045 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "sc1045 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sc1045_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static long sc1045_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = sc1045_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = sc1045_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = sc1045_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = sc1045_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = sc1045_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		//	ret = sc1045_write_array(sd, sc1045_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		//	ret = sc1045_write_array(sd, sc1045_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = sc1045_set_fps(sensor, ctrl->value);
			break;
		default:
			break;;
	}
	return 0;
}

static long sc1045_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = sc1045_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc1045_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = sc1045_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int sc1045_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	sc1045_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops sc1045_core_ops = {
	.g_chip_ident = sc1045_g_chip_ident,
	.reset = sc1045_reset,
	.init = sc1045_init,
	.s_power = sc1045_s_power,
	.ioctl = sc1045_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sc1045_g_register,
	.s_register = sc1045_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sc1045_video_ops = {
	.s_stream = sc1045_s_stream,
	.s_parm = sc1045_s_parm,
	.g_parm = sc1045_g_parm,
};

static const struct v4l2_subdev_ops sc1045_ops = {
	.core = &sc1045_core_ops,
	.video = &sc1045_video_ops,
};

static int sc1045_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &sc1045_win_sizes[0];
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

	sc1045_attr.dvp.gpio = sensor_gpio_func;
	 /*
		convert sensor-gain into isp-gain,
	 */
	sc1045_attr.max_again = sc1045_attr.max_again;
	sc1045_attr.max_dgain = sc1045_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &sc1045_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &sc1045_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int sc1045_remove(struct i2c_client *client)
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

static const struct i2c_device_id sc1045_id[] = {
	{ "sc1045", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc1045_id);

static struct i2c_driver sc1045_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "sc1045",
	},
	.probe		= sc1045_probe,
	.remove		= sc1045_remove,
	.id_table	= sc1045_id,
};

static __init int init_sc1045(void)
{
	return i2c_add_driver(&sc1045_driver);
}

static __exit void exit_sc1045(void)
{
	i2c_del_driver(&sc1045_driver);
}

module_init(init_sc1045);
module_exit(exit_sc1045);

MODULE_DESCRIPTION("A low-level driver for OmniVision sc1045 sensors");
MODULE_LICENSE("GPL");
