/*
 * ov9712.c
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

#define OV9712_CHIP_ID_H	(0x97)
#define OV9712_CHIP_ID_L	(0x11)

#define OV9712_REG_END		0xff
#define OV9712_REG_DELAY	0xfe

#define TX_ISP_SUPPORT_MCLK (24*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
#define OV9712_SUPPORT_PCLK (42*1000*1000)
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

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
	unsigned char value;
	unsigned int gain;
};
struct again_lut ov9712_again_lut[] = {
	{0x0, 0},//1843},
	{0x1, 7575},
	{0x2, 12980},
	{0x3, 18092},
	{0x4, 22941},
	{0x5, 27554},
	{0x6, 31953},
	{0x7, 36156},
	{0x8, 40180},
	{0x9, 44039},
	{0xa, 47747},
	{0xb, 51316},
	{0xc, 54754},
	{0xd, 58072},
	{0xe, 61277},
	{0xf, 64378},
	{0x10, 67379},
	{0x11, 73111},
	{0x12, 78516},
	{0x13, 83628},
	{0x14, 88477},
	{0x15, 93090},
	{0x16, 97489},
	{0x17, 101692},
	{0x18, 105716},
	{0x19, 109575},
	{0x1a, 113283},
	{0x1b, 116852},
	{0x1c, 120290},
	{0x1d, 123608},
	{0x1e, 126813},
	{0x1f, 129914},
	{0x30, 132915},
	{0x31, 138647},
	{0x32, 144052},
	{0x33, 149164},
	{0x34, 154013},
	{0x35, 158626},
	{0x36, 163025},
	{0x37, 167228},
	{0x38, 171252},
	{0x39, 175111},
	{0x3a, 178819},
	{0x3b, 182388},
	{0x3c, 185826},
	{0x3d, 189144},
	{0x3e, 192349},
	{0x3f, 195450},
	{0x70, 198451},
	{0x71, 204183},
	{0x72, 209588},
	{0x73, 214700},
	{0x74, 219549},
	{0x75, 224162},
	{0x76, 228561},
	{0x77, 232764},
	{0x78, 236788},
	{0x79, 240647},
	{0x7a, 244355},
	{0x7b, 247924},
	{0x7c, 251362},
	{0x7d, 254680},
	{0x7e, 257885},
	{0x7f, 260986},
	{0xf0, 263987},
	{0xf1, 269719},
	{0xf2, 275124},
	{0xf3, 280236},
	{0xf4, 285085},
	{0xf5, 289698},
	{0xf6, 294097},
	{0xf7, 298300},
	{0xf8, 302324},
	{0xf9, 306183},
	{0xfa, 309891},
	{0xfb, 313460},
	{0xfc, 316898},
	{0xfd, 320216},
	{0xfe, 323421},
	{0xff, 326522},
};

struct tx_isp_sensor_attribute ov9712_attr;

unsigned int ov9712_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = ov9712_again_lut;

	while(lut->gain <= ov9712_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = 0;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == ov9712_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}
unsigned int ov9712_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}
struct tx_isp_sensor_attribute ov9712_attr={
	.name = "ov9712",
	.chip_id = 0x9711,
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
	.max_again = 260986,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 991,
	.integration_time_limit = 991,
	.total_width = 1688,
	.total_height = 995,
	.max_integration_time = 991,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 1,
	.dgain_apply_delay = 1,
	.sensor_ctrl.alloc_again = ov9712_alloc_again,
	.sensor_ctrl.alloc_dgain = ov9712_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};

static struct regval_list ov9712_init_regs_1280_800_25fps[] = {
	/*
	   @@ DVP interface 1280*800 25fps
	 */
	{0x12, 0x80}, // reset
	{0x09, 0x10},
	{0x1e, 0x07},
	{0x5f, 0x18},
	{0x69, 0x04},
	{0x65, 0x2a},
	{0x68, 0x0a},
	{0x39, 0x28},
	{0x4d, 0x90},
	{0xc1, 0x80},
	{0x0c, 0x30},
	{0x6d, 0x02},
	{0x96, 0x01}, //DSP options enable AWB Enable ok
	{0xbc, 0x68},
	{0x12, 0x00},
	{0x3b, 0x00}, //DSP Downsample
	{0x97, 0x80}, // bit3 colorbar
	{0x17, 0x25},
	{0x18, 0xa2},
	{0x19, 0x01},
	{0x1a, 0xca},
	{0x03, 0x0a},
	{0x32, 0x07},
	{0x98, 0x00},
	{0x99, 0x00},
	{0x9a, 0x00},
	{0x57, 0x00},
	{0x58, 0xc8},
	{0x59, 0xa0},
	{0x4c, 0x13},
	{0x4b, 0x36},
	{0x3d, 0xe3},//25fps
	{0x3e, 0x03},
	//       {0x3d, 0xc4},//10fps
	//       {0x3e, 0x0a},
	{0xbd, 0xa0},
	{0xbe, 0xc8},
	{0x41, 0x84}, //AVERAGE
	{0x4e, 0x55}, //AVERAGE
	{0x4f, 0x55},
	{0x50, 0x55},
	{0x51, 0x55},
	{0x24, 0x55}, //Exposure windows
	{0x25, 0x40},
	{0x26, 0xa1},
	{0x5c, 0x52},
#ifdef	DRIVE_CAPABILITY_1
	{0x5d, 0x00},
#elif defined(DRIVE_CAPABILITY_2)
	{0x5d, 0x10},
#elif defined(DRIVE_CAPABILITY_3)
	{0x5d, 0x20},
#elif defined(DRIVE_CAPABILITY_4)
	{0x5d, 0x30},
#endif
	{0x5d, 0x00},
	{0x11, 0x01},
	{0x2a, 0x98},
	{0x2b, 0x06},
	{0x2d, 0x00},
	{0x2e, 0x00},
	{0x13, 0x80},//manual exposure & gain ok
	{0x14, 0x40}, //gain ceiling 8X
	{0x4a, 0x00},/* banding step remove for isp calibration */
	{0x49, 0xce},
	{0x22, 0x03},
	{0x09, 0x00},
	{0x60, 0x9d},
	// {0x10, 0x10},
	// {0x16, 0x10},
	// {0x00, 0x0f},
	{0x10, 0x00},//low bit
	{0x16, 0x01},//high bit
	{0x00, 0x00},
//awb
	{0x38,0x00},
	{0x01,0x40},
	{0x02,0x40},
	{0x05,0x40},
	{0x06,0x00},
	{0x07,0x00},
	{OV9712_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the ov9712_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ov9712_win_sizes[] = {
	/* 1280*800 */
	{
		.width		= 1280,
		.height		= 800,
		.fps		= 25 << 16 | 1, /* 12.5 fps */
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ov9712_init_regs_1280_800_25fps,
	}
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ov9712_stream_on[] = {
	{0x09, 0x00},

	{OV9712_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ov9712_stream_off[] = {
	/* Sensor enter LP11*/
	{0x09, 0x10},

	{OV9712_REG_END, 0x00},	/* END MARKER */
};

int ov9712_read(struct v4l2_subdev *sd, unsigned char reg,
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

int ov9712_write(struct v4l2_subdev *sd, unsigned char reg,
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

static int ov9712_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != OV9712_REG_END) {
		if (vals->reg_num == OV9712_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov9712_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			vals->value = val;
		}
		/*printk("vals->reg_num:0x%02x, vals->value:0x%02x\n",vals->reg_num, val);*/
		vals++;
	}
	return 0;
}
static int ov9712_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != OV9712_REG_END) {
		if (vals->reg_num == OV9712_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ov9712_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		/*printk("vals->reg_num:%x, vals->value:%x\n",vals->reg_num, vals->value);*/
		vals++;
	}
	return 0;
}

static int ov9712_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int ov9712_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;
	ret = ov9712_read(sd, 0x0a, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;
	if (v != OV9712_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = ov9712_read(sd, 0x0b, &v);
	/*printk("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);*/
	if (ret < 0)
		return ret;
	if (v != OV9712_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int ov9712_set_integration_time(struct v4l2_subdev *sd, int value)
{
	unsigned int expo = value;
	int ret = 0;

	ov9712_write(sd, 0x10, (unsigned char)(expo & 0xff));
	ov9712_write(sd, 0x16, (unsigned char)((expo >> 8) & 0xff));
	if (ret < 0)
		return ret;

	/*[15:0] = 0x16,0x10 */
	return 0;
}
static int ov9712_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[6:0] */
	ov9712_write(sd, 0x00, (unsigned char)(value & 0x7f));
	return 0;
}
static int ov9712_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[7] if gain > 2X set 0; if gain > 4X set 1 */
	return 0;
}

static int ov9712_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
#if 0
	int ret = 0;
	int black = 0;
	unsigned char h,l;
	unsigned char reg = 0xff;
	unsigned int * v = (unsigned int *)(value);
	ret = ov9712_read(sd, 0x48, &h);
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
	ret = ov9712_read(sd, reg, &l);
	if (ret < 0)
		return ret;
	*v = (black | l);
#endif
	return 0;
}

static int ov9712_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &ov9712_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = ov9712_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int ov9712_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	unsigned char val_h;
	unsigned char val_l;
	if (enable) {
		ret = ov9712_write_array(sd, ov9712_stream_on);
		printk("ov9712 stream on\n");
	}
	else {
		ret = ov9712_write_array(sd, ov9712_stream_off);
		printk("ov9712 stream off\n");
	}
	return ret;
}

static int ov9712_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov9712_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ov9712_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = OV9712_SUPPORT_PCLK;
	unsigned short hts;
	unsigned short vts = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;
	ret += ov9712_read(sd, 0x2b, &tmp);
	hts = tmp;
	ret += ov9712_read(sd, 0x2a, &tmp);
	if(ret < 0)
		return -1;
	hts = (hts << 8) + tmp;
	/*vts = (pclk << 4) / (hts * (newformat >> 4));*/
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret = ov9712_write(sd, 0x3d, (unsigned char)(vts & 0xff));
	ret += ov9712_write(sd, 0x3e, (unsigned char)(vts >> 8));
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

static int ov9712_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &ov9712_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &ov9712_win_sizes[0];
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
		if(sensor->priv != wsize){
			ret = ov9712_write_array(sd, wsize->regs);
			if(!ret)
				sensor->priv = wsize;
		}
		sensor->video.fps = wsize->fps;
		arg.value = (int)&sensor->video;
		sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	}
	return ret;
}
static int ov9712_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"ov9712_reset");
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
		ret = gpio_request(pwdn_gpio,"ov9712_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = ov9712_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an ov9712 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "ov9712 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ov9712_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long ov9712_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = ov9712_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = ov9712_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = ov9712_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = ov9712_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = ov9712_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			ret = ov9712_write_array(sd, ov9712_stream_off);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			ret = ov9712_write_array(sd, ov9712_stream_on);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = ov9712_set_fps(sensor, ctrl->value);
			break;
		default:
			break;;
	}
	return ret;
}
static long ov9712_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = ov9712_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return ret;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov9712_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov9712_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov9712_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov9712_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops ov9712_core_ops = {
	.g_chip_ident = ov9712_g_chip_ident,
	.reset = ov9712_reset,
	.init = ov9712_init,
	.s_power = ov9712_s_power,
	.ioctl = ov9712_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov9712_g_register,
	.s_register = ov9712_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov9712_video_ops = {
	.s_stream = ov9712_s_stream,
	.s_parm = ov9712_s_parm,
	.g_parm = ov9712_g_parm,
};

static const struct v4l2_subdev_ops ov9712_ops = {
	.core = &ov9712_core_ops,
	.video = &ov9712_video_ops,
};

static int ov9712_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &ov9712_win_sizes[0];
	int ret;

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

	ov9712_attr.dvp.gpio = sensor_gpio_func;
	 /*
		convert sensor-gain into isp-gain,
	 */
	ov9712_attr.max_again = 260986;
	ov9712_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &ov9712_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &ov9712_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int ov9712_remove(struct i2c_client *client)
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

static const struct i2c_device_id ov9712_id[] = {
	{ "ov9712", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9712_id);

static struct i2c_driver ov9712_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ov9712",
	},
	.probe		= ov9712_probe,
	.remove		= ov9712_remove,
	.id_table	= ov9712_id,
};

static __init int init_ov9712(void)
{
	return i2c_add_driver(&ov9712_driver);
}

static __exit void exit_ov9712(void)
{
	i2c_del_driver(&ov9712_driver);
}

module_init(init_ov9712);
module_exit(exit_ov9712);

MODULE_DESCRIPTION("A low-level driver for OmniVision ov9712 sensors");
MODULE_LICENSE("GPL");
