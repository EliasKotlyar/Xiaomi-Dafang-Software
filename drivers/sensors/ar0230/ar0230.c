/*
 * ar0230.c
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

#define AR0230_CHIP_ID_H	(0x00)
#define AR0230_CHIP_ID_L	(0x56)
#define AR0230_REG_END		0xffff
#define AR0230_REG_DELAY	0xfffe
#define AR0230_SUPPORT_SCLK (37125000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5
#define LCG 0x0
#define HCG 0x1

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_12BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

static int data_interface = TX_SENSOR_DATA_INTERFACE_DVP;
module_param(data_interface, int, S_IRUGO);
MODULE_PARM_DESC(data_interface, "Sensor Date interface");

struct regval_list {
	unsigned short reg_num;
	unsigned short value;
};

/*
 * the part of driver maybe modify about different sensor and different board.
 */
struct again_lut {
	/* unsigned char mode; */
	unsigned int value;
	unsigned int gain;
};

/* static unsigned char again_mode = LCG; */
struct again_lut ar0230_again_lut[] = {
	{ 0x0,	 0 },
	{ 0x1,	 2794 },
	{ 0x2,	 6397 },
	{ 0x3,	 9011 },
	{ 0x4,	 12388 },
	{ 0x5,	 16447 },
	{ 0x6,	 19572 },
	{ 0x7,	 23340 },
	{ 0x8,	 26963 },
	{ 0x9,	 31135 },
	{ 0xa,	 35780 },
	{ 0xb,	 39588 },
	{ 0xc,	 44438 },
	{ 0xd,	 49051 },
	{ 0xe,	 54517 },
	{ 0xf,	 59685 },
	{ 0x10,	 65536 },
	{ 0x12,	 71490 },
	{ 0x14,	 78338 },
	{ 0x16,	 85108 },
	{ 0x18,	 92854 },
	{ 0x1a,	 100992 },
	{ 0x1c,	 109974 },
	{ 0x1e,	 120053 },
	{ 0x20,	 131072 },
	{ 0x22,	 137247 },
	{ 0x24,	 143667 },
	{ 0x26,	 150644 },
	{ 0x28,	 158212 },
	{ 0x2a,	 166528 },
	{ 0x2c,	 175510 },
	{ 0x2e,	 185457 },
	{ 0x30,	 196608 },
	{ 0x32,	 202783 },
	{ 0x34,	 209203 },
	{ 0x36,	 216276 },
	{ 0x38,	 223748 },
	{ 0x3a,	 232064 },
	{ 0x3c,	 241046 },
	{ 0x3e,	 250993 },
	{ 0x40,	 262144 },

#if 0
	{LCG,	 0xb,	  39588},
	{LCG,	 0xc,	  44438},
	{LCG,	 0xd,	  49051},
	{LCG,	 0xe,	  54517},
	{LCG,	 0xf,	  59685},
	{LCG,	 0x10,	  65536},
	{LCG,	 0x12,	  71490},
	{LCG,	 0x14,	  78338},
	{LCG,	 0x16,	  85108},
	{LCG,	 0x18,	  92854},
	/* {HCG,	 0x0,	  93910}, */
	/* {HCG,	 0x1,	  97010}, */
	{HCG,	 0x2,	  100012},
	{HCG,	 0x3,	  103239},
	{HCG,	 0x4,	  106666},
	{HCG,	 0x5,	  109974},
	{HCG,	 0x6,	  113454},
	{HCG,	 0x7,	  117360},
	{HCG,	 0x8,	  121110},
	{HCG,	 0x9,	  125221},
	{HCG,	 0xa,	  129402},
	{HCG,	 0xb,	  133636},
	{HCG,	 0xc,	  138348},
	{HCG,	 0xd,	  143252},
	{HCG,	 0xe,	  148310},
	{HCG,	 0xf,	  153670},
	{HCG,	 0x10,	  159446},
	{HCG,	 0x12,	  165548},
	{HCG,	 0x14,	  172049},
	{HCG,	 0x16,	  179133},
	{HCG,	 0x18,	  186646},
	{HCG,	 0x1a,	  194938},
	{HCG,	 0x1c,	  203884},
	{HCG,	 0x1e,	  213846},
	{HCG,	 0x20,	  224982},
	{HCG,	 0x22,	  231084},
	{HCG,	 0x24,	  237585},
	{HCG,	 0x26,	  244598},
	{HCG,	 0x28,	  252182},
	{HCG,	 0x2a,	  260414},
	{HCG,	 0x2c,	  269420},
	{HCG,	 0x2e,	  279382},
	{HCG,	 0x30,	  290518},
	{HCG,	 0x32,	  296661},
	{HCG,	 0x34,	  303160},
	{HCG,	 0x36,	  310169},
	{HCG,	 0x38,	  317685},
	{HCG,	 0x3a,	  325980},
	{HCG,	 0x3c,	  334956},
	{HCG,	 0x3e,	  344918},
	{HCG,	 0x40,	  356054},
#endif
};
struct tx_isp_sensor_attribute ar0230_attr;

unsigned int ar0230_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = ar0230_again_lut;

	while(lut->gain <= ar0230_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = 0;
			/* again_mode = LCG; */
			return 0;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			/* again_mode = (lut - 1)->mode; */
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == ar0230_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				/* again_mode = lut->mode; */
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;

}

unsigned int ar0230_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute ar0230_attr={
	.name = "ar0230",
	.chip_id = 0x0056,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_16BITS | V4L2_SBUS_MASK_ADDR_16BITS,
	.cbus_device = 0x10,
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
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 0x0546-4,
	.integration_time_limit = 0x0546-4,
	.total_width = 0x44c,
	.total_height = 0x0546,
	.max_integration_time = 0x0546-4,
	.integration_time_apply_delay = 1,
	.again_apply_delay = 1,
	.dgain_apply_delay = 0,
	.sensor_ctrl.alloc_again = ar0230_alloc_again,
	.sensor_ctrl.alloc_dgain = ar0230_alloc_dgain,
	//	void priv; /* point to struct tx_isp_sensor_board_info */
};


static struct regval_list ar0230_init_regs_1920_1080_30fps_mipi[] = {

	{AR0230_REG_END, 0x00},	/* END MARKER */
};


static struct regval_list ar0230_init_regs_1920_1080_30fps_dvp[] = {
	{0x301A, 0x0001},	// RESET_REGISTER
	{AR0230_REG_DELAY,200},
	{0x301A, 0x10D8},	// RESET_REGISTER
	{AR0230_REG_DELAY,200},
	{0x3088, 0x8242},
	{0x3086, 0x4558},
	{0x3086, 0x729B},
	{0x3086, 0x4A31},
	{0x3086, 0x4342},
	{0x3086, 0x8E03},
	{0x3086, 0x2A14},
	{0x3086, 0x4578},
	{0x3086, 0x7B3D},
	{0x3086, 0xFF3D},
	{0x3086, 0xFF3D},
	{0x3086, 0xEA2A},
	{0x3086, 0x043D},
	{0x3086, 0x102A},
	{0x3086, 0x052A},
	{0x3086, 0x1535},
	{0x3086, 0x2A05},
	{0x3086, 0x3D10},
	{0x3086, 0x4558},
	{0x3086, 0x2A04},
	{0x3086, 0x2A14},
	{0x3086, 0x3DFF},
	{0x3086, 0x3DFF},
	{0x3086, 0x3DEA},
	{0x3086, 0x2A04},
	{0x3086, 0x622A},
	{0x3086, 0x288E},
	{0x3086, 0x0036},
	{0x3086, 0x2A08},
	{0x3086, 0x3D64},
	{0x3086, 0x7A3D},
	{0x3086, 0x0444},
	{0x3086, 0x2C4B},
	{0x3086, 0x8F03},
	{0x3086, 0x430D},
	{0x3086, 0x2D46},
	{0x3086, 0x4316},
	{0x3086, 0x5F16},
	{0x3086, 0x530D},
	{0x3086, 0x1660},
	{0x3086, 0x3E4C},
	{0x3086, 0x2904},
	{0x3086, 0x2984},
	{0x3086, 0x8E03},
	{0x3086, 0x2AFC},
	{0x3086, 0x5C1D},
	{0x3086, 0x5754},
	{0x3086, 0x495F},
	{0x3086, 0x5305},
	{0x3086, 0x5307},
	{0x3086, 0x4D2B},
	{0x3086, 0xF810},
	{0x3086, 0x164C},
	{0x3086, 0x0955},
	{0x3086, 0x562B},
	{0x3086, 0xB82B},
	{0x3086, 0x984E},
	{0x3086, 0x1129},
	{0x3086, 0x9460},
	{0x3086, 0x5C19},
	{0x3086, 0x5C1B},
	{0x3086, 0x4548},
	{0x3086, 0x4508},
	{0x3086, 0x4588},
	{0x3086, 0x29B6},
	{0x3086, 0x8E01},
	{0x3086, 0x2AF8},
	{0x3086, 0x3E02},
	{0x3086, 0x2AFA},
	{0x3086, 0x3F09},
	{0x3086, 0x5C1B},
	{0x3086, 0x29B2},
	{0x3086, 0x3F0C},
	{0x3086, 0x3E03},
	{0x3086, 0x3E15},
	{0x3086, 0x5C13},
	{0x3086, 0x3F11},
	{0x3086, 0x3E0F},
	{0x3086, 0x5F2B},
	{0x3086, 0x902A},
	{0x3086, 0xF22B},
	{0x3086, 0x803E},
	{0x3086, 0x063F},
	{0x3086, 0x0660},
	{0x3086, 0x29A2},
	{0x3086, 0x29A3},
	{0x3086, 0x5F4D},
	{0x3086, 0x1C2A},
	{0x3086, 0xFA29},
	{0x3086, 0x8345},
	{0x3086, 0xA83E},
	{0x3086, 0x072A},
	{0x3086, 0xFB3E},
	{0x3086, 0x2945},
	{0x3086, 0x8824},
	{0x3086, 0x3E08},
	{0x3086, 0x2AFA},
	{0x3086, 0x5D29},
	{0x3086, 0x9288},
	{0x3086, 0x102B},
	{0x3086, 0x048B},
	{0x3086, 0x1686},
	{0x3086, 0x8D48},
	{0x3086, 0x4D4E},
	{0x3086, 0x2B80},
	{0x3086, 0x4C0B},
	{0x3086, 0x603F},
	{0x3086, 0x302A},
	{0x3086, 0xF23F},
	{0x3086, 0x1029},
	{0x3086, 0x8229},
	{0x3086, 0x8329},
	{0x3086, 0x435C},
	{0x3086, 0x155F},
	{0x3086, 0x4D1C},
	{0x3086, 0x2AFA},
	{0x3086, 0x4558},
	{0x3086, 0x8E00},
	{0x3086, 0x2A98},
	{0x3086, 0x3F0A},
	{0x3086, 0x4A0A},
	{0x3086, 0x4316},
	{0x3086, 0x0B43},
	{0x3086, 0x168E},
	{0x3086, 0x032A},
	{0x3086, 0x9C45},
	{0x3086, 0x783F},
	{0x3086, 0x072A},
	{0x3086, 0x9D3E},
	{0x3086, 0x305D},
	{0x3086, 0x2944},
	{0x3086, 0x8810},
	{0x3086, 0x2B04},
	{0x3086, 0x530D},
	{0x3086, 0x4558},
	{0x3086, 0x3E08},
	{0x3086, 0x8E01},
	{0x3086, 0x2A98},
	{0x3086, 0x8E00},
	{0x3086, 0x769C},
	{0x3086, 0x779C},
	{0x3086, 0x4644},
	{0x3086, 0x1616},
	{0x3086, 0x907A},
	{0x3086, 0x1244},
	{0x3086, 0x4B18},
	{0x3086, 0x4A04},
	{0x3086, 0x4316},
	{0x3086, 0x0643},
	{0x3086, 0x1605},
	{0x3086, 0x4316},
	{0x3086, 0x0743},
	{0x3086, 0x1658},
	{0x3086, 0x4316},
	{0x3086, 0x5A43},
	{0x3086, 0x1645},
	{0x3086, 0x588E},
	{0x3086, 0x032A},
	{0x3086, 0x9C45},
	{0x3086, 0x787B},
	{0x3086, 0x3F07},
	{0x3086, 0x2A9D},
	{0x3086, 0x530D},
	{0x3086, 0x8B16},
	{0x3086, 0x863E},
	{0x3086, 0x2345},
	{0x3086, 0x5825},
	{0x3086, 0x3E10},
	{0x3086, 0x8E01},
	{0x3086, 0x2A98},
	{0x3086, 0x8E00},
	{0x3086, 0x3E10},
	{0x3086, 0x8D60},
	{0x3086, 0x1244},
	{0x3086, 0x4B2C},
	{0x3086, 0x2C2C},
	{0x2436, 0x000E},
	{0x320C, 0x0180},
	{0x320E, 0x0300},
	{0x3210, 0x0500},
	{0x3204, 0x0B6D},
	{0x30FE, 0x0080},
	{0x3ED8, 0x7B99},
	{0x3EDC, 0x9BA8},
	{0x3EDA, 0x9B9B},
	{0x3092, 0x006F},
	{0x3EEC, 0x1C04},
	{0x30BA, 0x779C},
	{0x3EF6, 0xA70F},
	{0x3044, 0x0410},
	{0x3ED0, 0xFF44},
	{0x3ED4, 0x031F},
	{0x30FE, 0x0080},
	{0x3EE2, 0x8866},
	{0x3EE4, 0x6623},
	{0x3EE6, 0x2263},
	{0x30E0, 0x4283},
	{0x30F0, 0x1283},
	{0x301A, 0x10D8},	//RESET_REGISTER
	{0x30B0, 0x0118},	//DIGITAL_TEST enable low power
	{0x31AC, 0x0C0C},
	{0x302A, 0x0008},	//VT_PIX_CLK_DIV
	{0x302C, 0x0001},	//VT_SYS_CLK_DIV
	{0x302E, 0x0002},	//PRE_PLL_CLK_DIV
	{0x3030, 0x002C},	//PLL_MULTIPLIER
	{0x3036, 0x000C},	//OP_PIX_CLK_DIV
	{0x3038, 0x0001},	//OP_SYS_CLK_DIV
	{0x3002, 0x0000},	//Y_ADDR_START
	{0x3004, 0x0008},	//X_ADDR_START
	{0x3006, 0x0437},	    //Y_ADDR_END
	{0x3008, 0x0787},	//X_ADDR_END
	{0x300A, 0x0546},    //1125 vts
	{0x300C, 0x044c},    //1100 hts
	{0x3012, 0x0416},    //1046
	{0x30A2, 0x0001},
	{0x30A6, 0x0001},
	{0x3040, 0x0000},
	{0x31AE, 0x0301},
	{0x3082, 0x0009},
	{0x30BA, 0x769C},
	{0x3096, 0x0080},
	{0x3098, 0x0080},
	{0x31E0, 0x0200},
	{0x318C, 0x0000},
	{0x2400, 0x0003},
	{0x301E, 0x00A8},
	{0x2450, 0x0000},
	{0x320A, 0x0080},
	{0x31D0, 0x0001},
	{0x3200, 0x0000},
	{0x31D0, 0x0000},
	{0x3176, 0x0080},
	{0x3178, 0x0080},
	{0x317A, 0x0080},
	{0x317C, 0x0080},
	{0x3060, 0x000B},	// ANALOG_GAIN 1.5x Minimum analog Gain for LCG
	{0x3206, 0x0B08},
	{0x3208, 0x1E13},
	{0x3202, 0x0080},
	{0x3200, 0x0002},
	{0x3100, 0x0000},
	{0x3064, 0x1802},	//Disable Embedded Data and Stats
	{0x31C6, 0x0400},	//HISPI_CONTROL_STATUS: HispiSP
	{0x306E, 0x9210},	//DATAPATH_SELECT[9]=1 VDD_SLVS=1.8V
	{0x3060, 0x000f},
	{0x305e, 0x0080},
	{0x3012, 0xffff},
	/* {0x3070, 0x0002},       //color bar */
	{AR0230_REG_DELAY,33},
	{AR0230_REG_END, 0x00},	/* END MARKER */
};

/*
 * the order of the ar0230_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ar0230_win_sizes[] = {
	/* 1280*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG12_1X12,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ar0230_init_regs_1920_1080_30fps_dvp,
	}
};

static enum v4l2_mbus_pixelcode ar0230_mbus_code[] = {
	V4L2_MBUS_FMT_SGRBG12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list ar0230_stream_on_dvp[] = {
	{0x301A, 0x10DC},	//RESET_REGISTER
	{AR0230_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ar0230_stream_off_dvp[] = {
	{0x301A, 0x10D8},	//RESET_REGISTER
	{AR0230_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ar0230_stream_on_mipi[] = {
	{AR0230_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list ar0230_stream_off_mipi[] = {
	{AR0230_REG_END, 0x00},	/* END MARKER */
};

int ar0230_read(struct v4l2_subdev *sd, unsigned short reg,
		unsigned char *value)
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
			.len	= 2,
			.buf	= value,
		}
	};
	int ret;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}

int ar0230_write(struct v4l2_subdev *sd, unsigned short reg,
		unsigned short value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	unsigned char buf[4] = {reg >> 8, reg & 0xff, value >> 8, value & 0xff};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 4,
		.buf	= buf,
	};
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int ar0230_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val[2];
	while (vals->reg_num != AR0230_REG_END) {
		if (vals->reg_num == AR0230_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ar0230_read(sd, vals->reg_num, val);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}
static int ar0230_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != AR0230_REG_END) {
		if (vals->reg_num == AR0230_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ar0230_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int ar0230_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int ar0230_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v[2];
	int ret;

	ret = ar0230_read(sd, 0x3000, v);
	if (ret < 0)
		return ret;
	if (v[0] != AR0230_CHIP_ID_H)
		return -ENODEV;
	if (v[1] != AR0230_CHIP_ID_L)
		return -ENODEV;
	return 0;
}

static int ar0230_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret;

	ret = ar0230_write(sd,0x3012, value);
	if (ret < 0)
		return ret;
	return 0;

}

static int ar0230_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret;

#if 0
	if (again_mode == LCG){
		ret = ar0230_write(sd,0x3202, 0x0080);
		ret += ar0230_write(sd,0x3206, 0x0B08);
		ret += ar0230_write(sd,0x3208, 0x1E13);
		ret += ar0230_write(sd,0x3100, 0x00);

	} else if (again_mode == HCG){
		ret = ar0230_write(sd,0x3202, 0x00B0);
		ret += ar0230_write(sd,0x3206, 0x1C0E);
		ret += ar0230_write(sd,0x3208, 0x4E39);
		ret += ar0230_write(sd,0x3100, 0x04);
	} else {
		printk("Do not support this Again mode!\n");
	}

#endif
	ret = ar0230_write(sd,0x3060, value);
	if (ret < 0)
		return ret;

	return 0;
}

static int ar0230_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ar0230_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int ar0230_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &ar0230_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = ar0230_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int ar0230_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
			ret = ar0230_write_array(sd, ar0230_stream_on_dvp);
		} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
			ret = ar0230_write_array(sd, ar0230_stream_on_mipi);

		}else{
			printk("Don't support this Sensor Data interface\n");
		}
		pr_debug("ar0230 stream on\n");

	}
	else {
		if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
			ret = ar0230_write_array(sd, ar0230_stream_off_dvp);
		} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
			ret = ar0230_write_array(sd, ar0230_stream_off_mipi);

		}else{
			printk("Don't support this Sensor Data interface\n");
		}
		pr_debug("ar0230 stream off\n");
	}
	return ret;
}

static int ar0230_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ar0230_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ar0230_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = AR0230_SUPPORT_SCLK;
	unsigned short hts=0;
	unsigned short vts = 0;
	unsigned char tmp[2];
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;
	ret += ar0230_read(sd, 0x300C, tmp);
	hts =tmp[0];
	if(ret < 0)
		return -1;
	hts = (hts << 8) + tmp[1];
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret = ar0230_write(sd, 0x300A, vts);
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

static int ar0230_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &ar0230_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &ar0230_win_sizes[0];
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
static int ar0230_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;
	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"ar0230_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(20);
			gpio_direction_output(reset_gpio, 0);
			msleep(20);
			gpio_direction_output(reset_gpio, 1);
			msleep(20);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if(pwdn_gpio != -1){
		ret = gpio_request(pwdn_gpio,"ar0230_pwdn");
		if(!ret){
			gpio_direction_output(pwdn_gpio, 1);
			msleep(150);
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = ar0230_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
				"chip found @ 0x%x (%s) is not an ar0230 chip.\n",
				client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "ar0230 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ar0230_s_power(struct v4l2_subdev *sd, int on)
{

	return 0;
}
static long ar0230_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
		case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
			ret = ar0230_set_integration_time(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
			ret = ar0230_set_analog_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
			ret = ar0230_set_digital_gain(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
			ret = ar0230_get_black_pedestal(sd, ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
			ret = ar0230_set_mode(sensor,ctrl->value);
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
			if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
				ret = ar0230_write_array(sd, ar0230_stream_off_dvp);
			} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = ar0230_write_array(sd, ar0230_stream_off_mipi);

			}else{
				printk("Don't support this Sensor Data interface\n");
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
			if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
				ret = ar0230_write_array(sd, ar0230_stream_on_dvp);
			} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
				ret = ar0230_write_array(sd, ar0230_stream_on_mipi);

			}else{
				printk("Don't support this Sensor Data interface\n");
			}
			break;
		case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
			ret = ar0230_set_fps(sensor, ctrl->value);
			break;
		default:
			pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
			break;;
	}
	return 0;
}
static long ar0230_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
		case VIDIOC_ISP_PRIVATE_IOCTL:
			ret = ar0230_ops_private_ioctl(sensor, arg);
			break;
		default:
			return -1;
			break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ar0230_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val[2];
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ar0230_read(sd, reg->reg & 0xffff, val);
	reg->val = val[0];
	reg->val = (reg->val<<8)+val[1];
	reg->size = 2;
	return ret;
}

static int ar0230_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ar0230_write(sd, reg->reg & 0xffff, reg->val & 0xffff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops ar0230_core_ops = {
	.g_chip_ident = ar0230_g_chip_ident,
	.reset = ar0230_reset,
	.init = ar0230_init,
	.s_power = ar0230_s_power,
	.ioctl = ar0230_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ar0230_g_register,
	.s_register = ar0230_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ar0230_video_ops = {
	.s_stream = ar0230_s_stream,
	.s_parm = ar0230_s_parm,
	.g_parm = ar0230_g_parm,
};

static const struct v4l2_subdev_ops ar0230_ops = {
	.core = &ar0230_core_ops,
	.video = &ar0230_video_ops,
};

static int ar0230_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &ar0230_win_sizes[0];
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
	clk_set_rate(sensor->mclk, 27000000);
	clk_enable(sensor->mclk);

	ret = set_sensor_gpio_function(sensor_gpio_func);
	if (ret < 0)
		goto err_set_sensor_gpio;

	ar0230_attr.dbus_type = data_interface;
	if (data_interface == TX_SENSOR_DATA_INTERFACE_DVP){
		wsize->regs = ar0230_init_regs_1920_1080_30fps_dvp;
	} else if (data_interface == TX_SENSOR_DATA_INTERFACE_MIPI){
		wsize->regs = ar0230_init_regs_1920_1080_30fps_mipi;
	} else{
		printk("Don't support this Sensor Data Output Interface.\n");
		goto err_set_sensor_data_interface;
	}
#if 0
	ar0230_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = ar0230_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = ar0230_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(ar0230_win_sizes); i++)
		ar0230_win_sizes[i].mbus_code = mbus;

#endif
	 /*
		convert sensor-gain into isp-gain,
	 */
	ar0230_attr.max_again = 262144;
	ar0230_attr.max_dgain = 0;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &ar0230_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &ar0230_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("probe ok ------->ar0230\n");
	return 0;
err_set_sensor_data_interface:
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int ar0230_remove(struct i2c_client *client)
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

static const struct i2c_device_id ar0230_id[] = {
	{ "ar0230", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0230_id);

static struct i2c_driver ar0230_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ar0230",
	},
	.probe		= ar0230_probe,
	.remove		= ar0230_remove,
	.id_table	= ar0230_id,
};

static __init int init_ar0230(void)
{
	return i2c_add_driver(&ar0230_driver);
}

static __exit void exit_ar0230(void)
{
	i2c_del_driver(&ar0230_driver);
}

module_init(init_ar0230);
module_exit(exit_ar0230);

MODULE_DESCRIPTION("A low-level driver for OmniVision ar0230 sensors");
MODULE_LICENSE("GPL");
