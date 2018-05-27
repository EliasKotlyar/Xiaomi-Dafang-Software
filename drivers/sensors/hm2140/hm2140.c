/*
 * hm2140.c
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

#define HM2140_CHIP_ID_H	(0x21)
#define HM2140_CHIP_ID_L	(0x40)

#define HM2140_REG_END		0xffff
#define HM2140_REG_DELAY	0xfffe

#define HM2140_SUPPORT_SCLK (76789800)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_LOW_10BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

static int data_interface = TX_SENSOR_DATA_INTERFACE_DVP;
module_param(data_interface, int, S_IRUGO);
MODULE_PARM_DESC(data_interface, "Sensor Date interface");

struct regval_list {
	uint16_t reg_num;
	unsigned char value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
	unsigned int value;
	unsigned int gain;
};
struct again_lut hm2140_again_lut[] = {
	{0x0, 0	     },
	{0x1, 5731   },
	{0x2, 11136  },
	{0x3, 16248  },
	{0x4, 21097  },
	{0x5, 25710  },
	{0x6, 30109  },
	{0x7, 34312  },
	{0x8, 38336  },
	{0x9, 42195  },
	{0xa, 45904  },
	{0xb, 49472  },
	{0xc, 52910  },
	{0xd, 56228  },
	{0xe, 59433  },
	{0xf, 62534  },
	{0x10, 65536 },
	{0x11, 71267 },
	{0x12, 76672 },
	{0x13, 81784 },
	{0x14, 86633 },
	{0x15, 91246 },
	{0x16, 95645 },
	{0x17, 99848 },
	{0x18, 103872},
	{0x19, 107731},
	{0x1a, 111440},
	{0x1b, 115008},
	{0x1c, 118446},
	{0x1d, 121764},
	{0x1e, 124969},
	{0x1f, 128070},
	{0x20, 131072},
	{0x21, 136803},
	{0x22, 142208},
	{0x23, 147320},
	{0x24, 152169},
	{0x25, 156782},
	{0x26, 161181},
	{0x27, 165384},
	{0x28, 169408},
	{0x29, 173267},
	{0x2a, 176976},
	{0x2b, 180544},
	{0x2c, 183982},
	{0x2d, 187300},
	{0x2e, 190505},
	{0x2f, 193606},
	{0x30, 196608},
	{0x31, 202339},
	{0x32, 207744},
	{0x33, 212856},
	{0x34, 217705},
	{0x35, 222318},
	{0x36, 226717},
	{0x37, 230920},
	{0x38, 234944},
	{0x39, 238803},
	{0x3a, 242512},
	{0x3b, 246080},
	{0x3c, 249518},
	{0x3d, 252836},
	{0x3e, 256041},
	{0x3f, 259142},
};
struct tx_isp_sensor_attribute hm2140_attr;

unsigned int hm2140_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = hm2140_again_lut;

	while(lut->gain <= hm2140_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = lut->value;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == hm2140_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int hm2140_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return 0;
}

struct tx_isp_mipi_bus hm2140_mipi={
	.clk = 800,
	.lans = 2,
};
struct tx_isp_dvp_bus hm2140_dvp={
	.mode = SENSOR_DVP_HREF_MODE,
	.blanking = {
		.vblanking = 0,
		.hblanking = 0,
	},
};

struct tx_isp_sensor_attribute hm2140_attr={
	.name = "hm2140",
	.chip_id = 0x2140,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_16BITS,
	.cbus_device = 0x24,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},

	},
	.max_again = 259142,
	.max_dgain = 0,
	.min_integration_time = 2,
	.min_integration_time_native = 2,
	.max_integration_time_native = 0x534 - 2,
	.integration_time_limit = 0x534 - 2,
	.total_width = 0x902,
	.total_height = 0x534,
	.max_integration_time = 0x534 - 2,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = hm2140_alloc_again,
	.sensor_ctrl.alloc_dgain = hm2140_alloc_dgain,
};

static struct regval_list hm2140_init_regs_1920_1080_25fps_mipi[] = {

	{HM2140_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list hm2140_init_regs_1920_1080_25fps_dvp[] = {
//1080p@25fps
	{0x0103, 0x00},
	{0x5227, 0x74},
	{0x030B, 0x11},
	{0x0307, 0x00},
	{0x0309, 0x00},
	{0x030A, 0x0A},
	{0x030D, 0x02},
	{0x030F, 0x10},
	{0x5235, 0x24},
	{0x5236, 0x92},
	{0x5237, 0x23},
	{0x5238, 0xDC},
	{0x5239, 0x01},
	{0x0100, 0x02},
	{0x4001, 0x00},
	{0x4002, 0x2B},
	{0x0101, 0x00},
	{0x4026, 0xBB},
	{0x0202, 0x02},
	{0x0203, 0xEC},
	{0x0340, 0x05},//vts
	{0x0341, 0x34},
	{0x0342, 0x09},//hts
	{0x0343, 0x02},
	{0x0344, 0x00},
	{0x0345, 0x04},
	{0x0346, 0x00},
	{0x0347, 0x04},
	{0x0348, 0x07},
	{0x0349, 0x83},
	{0x034A, 0x04},
	{0x034B, 0x3B},
	{0x034C, 0x07},
	{0x034D, 0x80},
	{0x034E, 0x04},
	{0x034F, 0x38},
	{0x0360, 0x00},
	{0x0350, 0x73},
	{0x5015, 0xB3},
	{0x50DD, 0x01},
	{0x50CB, 0xA3},
	{0x5004, 0x40},
	{0x5005, 0x28},
	{0x504D, 0x7F},
	{0x504E, 0x00},
	{0x5040, 0x07},
	{0x5011, 0x00},
	{0x501D, 0x4C},
	{0x5011, 0x0B},
	{0x5012, 0x01},
	{0x5013, 0x03},
	{0x4131, 0x01},
	{0x5282, 0xFF},
	{0x5283, 0x07},
	{0x5010, 0x20},
	{0x4132, 0x20},
	{0x50D5, 0xE0},
	{0x50D7, 0x12},
	{0x50BB, 0x14},
	{0x50B7, 0x00},
	{0x50B8, 0x35},
	{0x50B9, 0x11},
	{0x50BA, 0xF0},
	{0x50B3, 0x24},
	{0x50B4, 0x00},
	{0x50FA, 0x02},
	{0x509B, 0x01},
	{0x50AA, 0xFF},
	{0x50AB, 0x25},
	{0x509C, 0x00},
	{0x50AD, 0x0C},
	{0x5096, 0x00},
	{0x50A1, 0x12},
	{0x50AF, 0x31},
	{0x50A0, 0x11},
	{0x50A2, 0x22},
	{0x509D, 0x20},
	{0x50AC, 0x55},
	{0x50AE, 0x26},
	{0x509E, 0x03},
	{0x509F, 0x01},
	{0x5097, 0x12},
	{0x5099, 0x00},
	{0x50B5, 0x00},
	{0x50B6, 0x10},
	{0x5094, 0x08},
	{0x5200, 0x43},
	{0x5201, 0xC0},
	{0x5202, 0x00},
	{0x5203, 0x00},
	{0x5204, 0x00},
	{0x5205, 0x05},
	{0x5206, 0xA1},
	{0x5207, 0x01},
	{0x5208, 0x05},
	{0x5209, 0x0C},
	{0x520A, 0x00},
	{0x520B, 0x45},
	{0x520C, 0x15},
	{0x520D, 0x40},
	{0x520E, 0x50},
	{0x520F, 0x10},
	{0x5214, 0x40},
	{0x5215, 0x14},
	{0x5216, 0x00},
	{0x5217, 0x02},
	{0x5218, 0x07},
	{0x521C, 0x00},
	{0x521E, 0x00},
	{0x522A, 0x3F},
	{0x522C, 0x00},
	{0x5230, 0x00},
	{0x5232, 0x05},
	{0x523A, 0x20},
	{0x523B, 0x34},
	{0x523C, 0x03},
	{0x523D, 0x00},
	{0x523E, 0x00},
	{0x523F, 0x70},
	{0x50E8, 0x16},
	{0x50E9, 0x00},
	{0x50EB, 0x0F},
	{0x4B11, 0x0F},
	{0x4B12, 0x0F},
	{0x4B31, 0x04},
	{0x4B3B, 0x02},
	{0x4B44, 0x80},
	{0x4B45, 0x00},
	{0x4B47, 0x00},
	{0x4B4E, 0x30},
	{0x4020, 0x60},
	{0x5100, 0x1B},
	{0x5101, 0x2B},
	{0x5102, 0x3B},
	{0x5103, 0x4B},
	{0x5104, 0x5F},
	{0x5105, 0x6F},
	{0x5106, 0x7F},
	{0x5108, 0x00},
	{0x5109, 0x00},
	{0x510A, 0x00},
	{0x510B, 0x00},
	{0x510C, 0x00},
	{0x510D, 0x00},
	{0x510E, 0x00},
	{0x5110, 0x0E},
	{0x5111, 0x0E},
	{0x5112, 0x0E},
	{0x5113, 0x0E},
	{0x5114, 0x0E},
	{0x5115, 0x0E},
	{0x5116, 0x0E},
	{0x5118, 0x09},
	{0x5119, 0x09},
	{0x511A, 0x09},
	{0x511B, 0x09},
	{0x511C, 0x09},
	{0x511D, 0x09},
	{0x511E, 0x09},
	{0x5120, 0xEA},
	{0x5121, 0x6A},
	{0x5122, 0x6A},
	{0x5123, 0x6A},
	{0x5124, 0x6A},
	{0x5125, 0x6A},
	{0x5126, 0x6A},
	{0x5140, 0x0B},
	{0x5141, 0x1B},
	{0x5142, 0x2B},
	{0x5143, 0x3B},
	{0x5144, 0x4B},
	{0x5145, 0x5B},
	{0x5146, 0x6B},
	{0x5148, 0x02},
	{0x5149, 0x02},
	{0x514A, 0x02},
	{0x514B, 0x02},
	{0x514C, 0x02},
	{0x514D, 0x02},
	{0x514E, 0x02},
	{0x5150, 0x08},
	{0x5151, 0x08},
	{0x5152, 0x08},
	{0x5153, 0x08},
	{0x5154, 0x08},
	{0x5155, 0x08},
	{0x5156, 0x08},
	{0x5158, 0x02},
	{0x5159, 0x02},
	{0x515A, 0x02},
	{0x515B, 0x02},
	{0x515C, 0x02},
	{0x515D, 0x02},
	{0x515E, 0x02},
	{0x5160, 0x66},
	{0x5161, 0x66},
	{0x5162, 0x66},
	{0x5163, 0x66},
	{0x5164, 0x66},
	{0x5165, 0x66},
	{0x5166, 0x66},
	{0x5180, 0x00},
	{0x5189, 0x00},
	{0x5192, 0x00},
	{0x519B, 0x00},
	{0x51A4, 0x00},
	{0x51AD, 0x00},
	{0x51B6, 0x00},
	{0x51C0, 0x00},
	{0x5181, 0x00},
	{0x518A, 0x00},
	{0x5193, 0x00},
	{0x519C, 0x00},
	{0x51A5, 0x00},
	{0x51AE, 0x00},
	{0x51B7, 0x00},
	{0x51C1, 0x00},
	{0x5182, 0x85},
	{0x518B, 0x85},
	{0x5194, 0x85},
	{0x519D, 0x85},
	{0x51A6, 0x85},
	{0x51AF, 0x85},
	{0x51B8, 0x85},
	{0x51C2, 0x85},
	{0x5183, 0x52},
	{0x518C, 0x52},
	{0x5195, 0x52},
	{0x519E, 0x52},
	{0x51A7, 0x52},
	{0x51B0, 0x52},
	{0x51B9, 0x52},
	{0x51C3, 0x52},
	{0x5184, 0x00},
	{0x518D, 0x00},
	{0x5196, 0x08},
	{0x519F, 0x08},
	{0x51A8, 0x08},
	{0x51B1, 0x08},
	{0x51BA, 0x08},
	{0x51C4, 0x08},
	{0x5185, 0x73},
	{0x518E, 0x73},
	{0x5197, 0x73},
	{0x51A0, 0x73},
	{0x51A9, 0x73},
	{0x51B2, 0x73},
	{0x51BB, 0x73},
	{0x51C5, 0x73},
	{0x5186, 0x34},
	{0x518F, 0xA4},
	{0x5198, 0x34},
	{0x51A1, 0x34},
	{0x51AA, 0x34},
	{0x51B3, 0x3F},
	{0x51BC, 0x3F},
	{0x51C6, 0x3F},
	{0x5187, 0x40},
	{0x5190, 0x38},
	{0x5199, 0x20},
	{0x51A2, 0x08},
	{0x51AB, 0x04},
	{0x51B4, 0x04},
	{0x51BD, 0x02},
	{0x51C7, 0x01},
	{0x5188, 0x20},
	{0x5191, 0x40},
	{0x519A, 0x40},
	{0x51A3, 0x40},
	{0x51AC, 0x40},
	{0x51B5, 0x78},
	{0x51BE, 0x78},
	{0x51C8, 0x78},
	{0x51E1, 0x07},
	{0x51E3, 0x07},
	{0x51E5, 0x07},
	{0x51ED, 0x00},
	{0x51EE, 0x00},
	{0x4002, 0x2B},
	{0x3132, 0x00},
	{0x4024, 0x00},
	{0x5229, 0xFD},
	{0x4002, 0x2B},
	{0x3110, 0x03},
	{0x373D, 0x18},
	{0xBAA2, 0xC0},
	{0xBAA2, 0x40},
	{0xBA90, 0x01},
	{0xBA93, 0x02},
	{0x350D, 0x01},
	{0x3514, 0x00},
	{0x350C, 0x01},
	{0x3519, 0x00},
	{0x351A, 0x01},
	{0x351B, 0x1E},
	{0x351C, 0x90},
	{0x351E, 0x05},
	{0x351D, 0x05},
	{0x4B20, 0x9E},
	{0x4B18, 0x00},
	{0x4B3E, 0x00},
	{0x4B0E, 0x21},
	{0x4800, 0xAC},
	{0x0104, 0x01},
	{0x0104, 0x00},
	{0x4801, 0xAE},
	{0x0000, 0x00},
	/* {0x4026, 0xBA},//color bar */
	/* {0x0601, 0x02},//color bar */
	/* {0x0100, 0x01},  */
	{HM2140_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the hm2140_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting hm2140_win_sizes[] = {
	/* 1920*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= hm2140_init_regs_1920_1080_25fps_dvp,
	}
};

static enum v4l2_mbus_pixelcode hm2140_mbus_code[] = {
	V4L2_MBUS_FMT_SBGGR10_1X10,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list hm2140_stream_on_dvp[] = {
	{0x0100, 0x01},
	{HM2140_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list hm2140_stream_off_dvp[] = {
	{0x0100, 0x00},
	{HM2140_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list hm2140_stream_on_mipi[] = {

	{HM2140_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list hm2140_stream_off_mipi[] = {

	{HM2140_REG_END, 0x00},	/* END MARKER */
};

int hm2140_read(struct v4l2_subdev *sd, uint16_t reg,
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

int hm2140_write(struct v4l2_subdev *sd, uint16_t reg,
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

static int hm2140_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != HM2140_REG_END) {
		if (vals->reg_num == HM2140_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = hm2140_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		printk("vals->reg_num:0x%02x, vals->value:0x%02x\n",vals->reg_num, val);
		vals++;
	}
	return 0;
}
static int hm2140_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != HM2140_REG_END) {
		if (vals->reg_num == HM2140_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = hm2140_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int hm2140_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int hm2140_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v;
	int ret;

	ret = hm2140_read(sd, 0x0000, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != HM2140_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = hm2140_read(sd, 0x0001, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != HM2140_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int hm2140_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret = hm2140_write(sd, 0x0203, (unsigned char)(value & 0xff));
	ret += hm2140_write(sd, 0x0202, (unsigned char)((value >> 8) & 0xff));
	ret += hm2140_write(sd, 0x0104, 0x01);

	if (ret < 0)
		return ret;

	return 0;
}

static int hm2140_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;

	ret = hm2140_write(sd, 0x0205, (unsigned char)(value & 0xff));
	ret += hm2140_write(sd, 0x0104, 0x01);
	if (ret < 0)
		return ret;

	return 0;
}

static int hm2140_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int hm2140_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int hm2140_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &hm2140_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = hm2140_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int hm2140_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	unsigned char val;
	if (enable) {
		if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
			ret = hm2140_write_array(sd, hm2140_stream_on_dvp);
			msleep(100);
			while(0){
				hm2140_read(sd, 0x0005, &val);
				printk("0005 is 0x%x\n",val);
				hm2140_read(sd, 0x0100, &val);
				printk("0x0100 is 0x%x\n",val);
				hm2140_read(sd, 0x34c, &val);
				printk("0x34c is 0x%x\n",val);
				hm2140_read(sd, 0x34d, &val);
				printk("0x34d is 0x%x\n",val);
				hm2140_read(sd, 0x34e, &val);
				printk("0x34e is 0x%x\n",val);
				hm2140_read(sd, 0x34f, &val);
				printk("0x34f is 0x%x\n",val);
			}
		} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
			ret = hm2140_write_array(sd, hm2140_stream_on_mipi);

		}else{
			printk("Don't support this Sensor Data interface\n");
		}
		pr_debug("hm2140 stream on\n");

	}
	else {
		if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
			ret = hm2140_write_array(sd, hm2140_stream_off_dvp);
		} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
			ret = hm2140_write_array(sd, hm2140_stream_off_mipi);

		}else{
			printk("Don't support this Sensor Data interface\n");
		}
		pr_debug("hm2140 stream off\n");
	}
	return ret;
}

static int hm2140_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int hm2140_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int hm2140_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	int ret = 0;
	unsigned int sclk = 0;
	unsigned int hts = 0;
	unsigned int vts = 0;
	unsigned char val = 0;
	unsigned int newformat = 0; //the format is 24.8
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)) {
		return -1;
	}
	sclk = HM2140_SUPPORT_SCLK;

	val = 0;
	ret += hm2140_read(sd, 0x342, &val);
	hts = val<<8;
	val = 0;
	ret += hm2140_read(sd, 0x343, &val);
	hts |= val;
	if (0 != ret) {
		printk("err: hm2140 read err\n");
		return ret;
	}

	vts = sclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret += hm2140_write(sd, 0x341, vts & 0xff);
	ret += hm2140_write(sd, 0x340, (vts >> 8) & 0xff);
	ret += hm2140_write(sd, 0x0104, 0x01);
	if (0 != ret) {
		printk("err: hm2140_write err\n");
		return ret;
	}
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 2;
	sensor->video.attr->integration_time_limit = vts - 2;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 2;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);

	return ret;
}

static int hm2140_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &hm2140_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &hm2140_win_sizes[0];
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
static int hm2140_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"hm2140_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 0);
			msleep(10);
			gpio_direction_output(reset_gpio, 1);
			msleep(10);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"hm2140_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 0);
			msleep(10);
			gpio_direction_output(pwdn_gpio, 1);
			msleep(10);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = hm2140_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an hm2140 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "hm2140 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int hm2140_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long hm2140_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = hm2140_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = hm2140_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = hm2140_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = hm2140_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = hm2140_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
				ret = hm2140_write_array(sd, hm2140_stream_off_dvp);
			} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = hm2140_write_array(sd, hm2140_stream_off_mipi);

			}else{
				printk("Don't support this Sensor Data interface\n");
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
				ret = hm2140_write_array(sd, hm2140_stream_on_dvp);
			} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = hm2140_write_array(sd, hm2140_stream_on_mipi);

			}else{
				printk("Don't support this Sensor Data interface\n");
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = hm2140_set_fps(sensor, ctrl->value);
			break;
		default:
			pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
			break;
	}
	return 0;
}
static long hm2140_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = hm2140_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int hm2140_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = hm2140_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int hm2140_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	hm2140_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops hm2140_core_ops = {
	.g_chip_ident = hm2140_g_chip_ident,
	.reset = hm2140_reset,
	.init = hm2140_init,
	.s_power = hm2140_s_power,
	.ioctl = hm2140_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = hm2140_g_register,
	.s_register = hm2140_s_register,
#endif
};

static const struct v4l2_subdev_video_ops hm2140_video_ops = {
	.s_stream = hm2140_s_stream,
	.s_parm = hm2140_s_parm,
	.g_parm = hm2140_g_parm,
};

static const struct v4l2_subdev_ops hm2140_ops = {
	.core = &hm2140_core_ops,
	.video = &hm2140_video_ops,
};

static int hm2140_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &hm2140_win_sizes[0];
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

	hm2140_attr.dbus_type = data_interface;
	if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
		wsize->regs = hm2140_init_regs_1920_1080_25fps_dvp;
		memcpy((void*)(&(hm2140_attr.dvp)),(void*)(&hm2140_dvp),sizeof(hm2140_dvp));
	} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
		wsize->regs = hm2140_init_regs_1920_1080_25fps_mipi;
		memcpy((void*)(&(hm2140_attr.mipi)),(void*)(&hm2140_mipi),sizeof(hm2140_mipi));
	} else{
		printk("Don't support this Sensor Data Output Interface.\n");
		goto err_set_sensor_data_interface;
	}

	hm2140_attr.max_again = 259142;
	hm2140_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &hm2140_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &hm2140_ops);
	v4l2_set_subdev_hostdata(sd, sensor);
	pr_debug("probe ok ------->hm2140\n");
	return 0;
err_set_sensor_data_interface:
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int hm2140_remove(struct i2c_client *client)
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

static const struct i2c_device_id hm2140_id[] = {
	{ "hm2140", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, hm2140_id);

static struct i2c_driver hm2140_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "hm2140",
	},
	.probe		= hm2140_probe,
	.remove		= hm2140_remove,
	.id_table	= hm2140_id,
};

static __init int init_hm2140(void)
{
	return i2c_add_driver(&hm2140_driver);
}

static __exit void exit_hm2140(void)
{
	i2c_del_driver(&hm2140_driver);
}

module_init(init_hm2140);
module_exit(exit_hm2140);

MODULE_DESCRIPTION("A low-level driver for OmniVision hm2140 sensors");
MODULE_LICENSE("GPL");
