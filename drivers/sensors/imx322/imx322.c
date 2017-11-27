/*
 * imx322.c
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

#define IMX322_CHIP_ID_H	(0x55)
#define IMX322_CHIP_ID_L	(0xec)

#define IMX322_REG_END		0xffff
#define IMX322_REG_DELAY	0xfffe

#define IMX322_SUPPORT_PCLK (37125*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
#define AGAIN_MAX_DB 0x50
#define DGAIN_MAX_DB 0x3c
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

unsigned int imx322_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{

	uint16_t again=(isp_gain*20)>>LOG2_GAIN_SHIFT;
	// Limit Max gain
	if(again>AGAIN_MAX_DB+DGAIN_MAX_DB) again=AGAIN_MAX_DB+DGAIN_MAX_DB;
	*sensor_again=again;
	isp_gain= (((int32_t)again)<<LOG2_GAIN_SHIFT)/20;
	return isp_gain;
}

unsigned int imx322_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	*sensor_dgain = 0;
	return 0;
}

struct tx_isp_sensor_attribute imx322_attr = {
	.name = "imx322",
	.chip_id = 0x55f1,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
	.cbus_device = 0x1a,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_SONY_MODE,
		.blanking = {
			.vblanking = 17,
			.hblanking = 47,
		},
	},
	.max_again = 586420,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 1125-2,
	.integration_time_limit = 1125-1,
	.total_width = 1984,
	.total_height = 1125,
	.max_integration_time = 1125-1,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = imx322_alloc_again,
	.sensor_ctrl.alloc_dgain = imx322_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};


static struct regval_list imx322_init_regs_1920_1080_30fps[] = {
/* inclk 37.135M clk*/
	{0x3000,0x31},
	{0x0008,0xf0},
	{0x0009,0x00},
	{0x0112,0x0c},
	{0x0113,0x0c},
	{0x0340,0x04},
	{0x0341,0x65},
	{0x0342,0x05},        //LINE_LENGTH 30fps 0x44c
	{0x0343,0x28},        //LINE_LENGTH
	{0x3002,0x0F},        //MODE
	{0x3027,0x20},
	{0x3011,0x00}, //date rete 00 1x inclk 01 2x inclk
	{0x3012,0x82},
	{0x3016,0x3c},
	{0x3017,0x00},
	{0x301f,0x73},
	{0x3021,0x00},//modify
	{0x3022,0x40},
	{0x3027,0x20},
	{0x3117,0x0D},
	{0x302c,0x00},
	{0x307a,0x00},
	{0x307b,0x00},
	{0x309a,0x94}, //30fps 26
	{0x309b,0x02},
	{0x30CE,0x16},        //PRES
	{0x30CF,0x82},        //DRES
	{0x30D0,0x00},        //DRES
	{0x302c,0x00},
	{0x301E,0x00},//gain
	{0x0202,0x01},
	{0x0203,0x00},//expo
	{0x3008,0x00},
	{0x3009,0x00},
	{0x0100,0x00},
	{IMX322_REG_DELAY, 0x14},	/* END MARKER */
	{IMX322_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the imx322_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting imx322_win_sizes[] = {
	/* 1280*960 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SRGGB12_1X12,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= imx322_init_regs_1920_1080_30fps,
	}
};

static enum v4l2_mbus_pixelcode imx322_mbus_code[] = {
	V4L2_MBUS_FMT_SRGGB10_1X10,
	V4L2_MBUS_FMT_SRGGB12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list imx322_stream_on[] = {
	{0x0100,0x01},
	{IMX322_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list imx322_stream_off[] = {
	{0x0100,0x00},
	{IMX322_REG_END, 0x00},	/* END MARKER */
};

int imx322_read(struct v4l2_subdev *sd, uint16_t reg,
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

int imx322_write(struct v4l2_subdev *sd, uint16_t reg,
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

static int imx322_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != IMX322_REG_END) {
		if (vals->reg_num == IMX322_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = imx322_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
			printk("{0x%0x, 0x%02x}\n",vals->reg_num, val);
		}
		vals++;
	}
	return 0;
}
static int imx322_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != IMX322_REG_END) {
		if (vals->reg_num == IMX322_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = imx322_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int imx322_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int imx322_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;

	ret = imx322_read(sd, 0x31f3, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != IMX322_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = imx322_read(sd, 0x31f2, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != IMX322_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int imx322_set_integration_time(struct v4l2_subdev *sd, int int_time)
{
	int ret = 0;
	char value = 0;
	unsigned short shs = 0;
	unsigned short vmax = 0;
	ret = imx322_read(sd, 0x0341, &value);
	vmax = value;
	ret = imx322_read(sd, 0x0340, &value);
	vmax |= value << 8;
	shs = vmax - int_time - 1;

	ret = imx322_write(sd, 0x0203, (unsigned char)(shs & 0xff));
	if (ret < 0)
		return ret;
	ret = imx322_write(sd, 0x0202, (unsigned char)((shs >> 8) & 0xff));
	if (ret < 0)
		return ret;
	return 0;

}

static int imx322_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	/* printk("sensor again write value 0x%x\n",value); */
	ret = imx322_write(sd, 0x301e, (unsigned char)(value & 0xff));
	if (ret < 0)
		return ret;
	return 0;
}

static int imx322_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int imx322_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int imx322_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &imx322_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = imx322_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int imx322_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	if (enable) {
		ret = imx322_write_array(sd, imx322_stream_on);
		pr_debug("imx322 stream on\n");
		/* imx322_read_array(sd,imx322_init_regs_1920_1080_30fps); */
	}
	else {
		ret = imx322_write_array(sd, imx322_stream_off);
		pr_debug("imx322 stream off\n");
	}
//	imx322_read_array(sd, imx322_init_regs_1280_960_25fps);
	return ret;
}

static int imx322_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int imx322_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int imx322_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	int ret;
	unsigned int vts = 0;
	struct tx_isp_notify_argument arg;

	if(fps==983041){  //15fps
		ret += imx322_write(sd, 0x0340, 0x07);
		ret += imx322_write(sd, 0x0341, 0x30);
		/* ret += imx322_write(sd, 0x309a, 0x4c); */
		/* ret += imx322_write(sd, 0x309b, 0x04); */
		if (0 != ret) {
			printk("err: imx322_write err\n");
			return ret;
		}
		vts=0x730;
	}
	else{
		ret += imx322_write(sd, 0x0340, 0x04);
		ret += imx322_write(sd, 0x0341, 0x65);
		/* ret += imx322_write(sd, 0x309a, 0x94); */
		/* ret += imx322_write(sd, 0x309b, 0x02); */
		if (0 != ret) {
			printk("err: imx322_write err\n");
			return ret;
		}
		vts=0x465;
	}

	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 4;
	sensor->video.attr->integration_time_limit = vts - 4;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 4;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

	return 0;
#if 0
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
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}
	pclk = IMX322_SUPPORT_PCLK;

	val = 0;
	ret += imx322_read(sd, 0x380c, &val);
	hts = val<<8;
	val = 0;
	ret += imx322_read(sd, 0x380d, &val);
	hts |= val;
	if (0 != ret) {
		printk("err: imx322 read err\n");
		return ret;
	}
	vts = (pclk << 4) / (hts * (newformat >> 4));
	ret += imx322_write(sd, 0x380f, vts&0xff);
	ret += imx322_write(sd, 0x380e, (vts>>8)&0xff);
	if (0 != ret) {
		printk("err: imx322_write err\n");
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
#endif
}

static int imx322_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &imx322_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &imx322_win_sizes[0];
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
static int imx322_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"imx322_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 0);
			msleep(5);
			gpio_direction_output(reset_gpio, 1);
			msleep(5);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"imx322_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = imx322_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an imx322 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "imx322 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int imx322_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long imx322_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = imx322_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = imx322_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = imx322_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = imx322_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = imx322_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		ret = imx322_write_array(sd, imx322_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		ret = imx322_write_array(sd, imx322_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = imx322_set_fps(sensor, ctrl->value);
		break;
	default:
		break;;
	}
	return 0;
}
static long imx322_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = imx322_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int imx322_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = imx322_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 1;
	return ret;
}

static int imx322_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	imx322_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops imx322_core_ops = {
	.g_chip_ident = imx322_g_chip_ident,
	.reset = imx322_reset,
	.init = imx322_init,
	.s_power = imx322_s_power,
	.ioctl = imx322_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = imx322_g_register,
	.s_register = imx322_s_register,
#endif
};

static const struct v4l2_subdev_video_ops imx322_video_ops = {
	.s_stream = imx322_s_stream,
	.s_parm = imx322_s_parm,
	.g_parm = imx322_g_parm,
};

static const struct v4l2_subdev_ops imx322_ops = {
	.core = &imx322_core_ops,
	.video = &imx322_video_ops,
};

static int imx322_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &imx322_win_sizes[0];
	enum v4l2_mbus_pixelcode mbus;
	int i = 0;
	int ret;
	unsigned long rate = 0;

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

	rate = clk_get_rate(clk_get_parent(sensor->mclk));
	if (((rate / 1000) % 37125) != 0) {
		struct clk *vpll;
		vpll = clk_get(NULL,"vpll");
		if (IS_ERR(vpll)) {
			pr_warning("get vpll failed\n");
		} else {
			rate = clk_get_rate(vpll);
			if (((rate / 1000) % 37125) == 0) {
				ret = clk_set_parent(sensor->mclk, vpll);
				if (ret < 0)
					pr_err("set mclk parent as vpll err\n");
			}
		}
	}

	clk_set_rate(sensor->mclk, 37125000);
	clk_enable(sensor->mclk);
	pr_debug("mclk=%lu\n", clk_get_rate(sensor->mclk));

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	imx322_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
	case DVP_PA_LOW_10BIT:
	case DVP_PA_HIGH_10BIT:
		mbus = imx322_mbus_code[0];
		break;
	case DVP_PA_12BIT:
		mbus = imx322_mbus_code[1];
		break;
	default:
		goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(imx322_win_sizes); i++)
		imx322_win_sizes[i].mbus_code = mbus;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &imx322_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &imx322_ops);
	v4l2_set_subdev_hostdata(sd, sensor);
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int imx322_remove(struct i2c_client *client)
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

static const struct i2c_device_id imx322_id[] = {
	{ "imx322", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx322_id);

static struct i2c_driver imx322_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "imx322",
	},
	.probe		= imx322_probe,
	.remove		= imx322_remove,
	.id_table	= imx322_id,
};

static __init int init_imx322(void)
{
	return i2c_add_driver(&imx322_driver);
}

static __exit void exit_imx322(void)
{
	i2c_del_driver(&imx322_driver);
}

module_init(init_imx322);
module_exit(exit_imx322);

MODULE_DESCRIPTION("A low-level driver for OmniVision imx322 sensors");
MODULE_LICENSE("GPL");
