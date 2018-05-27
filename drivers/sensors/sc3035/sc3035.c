/*
 * sc3035.c
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

#define SC3035_CHIP_ID_H		(0x30)
#define SC3035_CHIP_ID_L		(0x35)
#define SC3035_REG_END		    0xffff
#define SC3035_REG_DELAY		0xfffe
#define SC3035_SUPPORT_2048_1536_PCLK (99000*1000)
#define SC3035_SUPPORT_1536_1536_PCLK (75600*1000)
#define SENSOR_OUTPUT_MAX_FPS 25
#define SENSOR_OUTPUT_MIN_FPS 5
#define DRIVE_CAPABILITY_2

typedef enum {
	resolution_2048_1536 =0,
	resolution_1536_1536,
};

static int reset_gpio = GPIO_PA(18);
module_param(reset_gpio, int, S_IRUGO);
MODULE_PARM_DESC(reset_gpio, "Reset GPIO NUM");

static int pwdn_gpio = -1;
module_param(pwdn_gpio, int, S_IRUGO);
MODULE_PARM_DESC(pwdn_gpio, "Power down GPIO NUM");

static int sensor_gpio_func = DVP_PA_HIGH_10BIT;
module_param(sensor_gpio_func, int, S_IRUGO);
MODULE_PARM_DESC(sensor_gpio_func, "Sensor GPIO function");

static int sensor_max_resolution = resolution_2048_1536;
module_param(sensor_max_resolution, int, S_IRUGO);
MODULE_PARM_DESC(sensor_max_resolution, "Sensor Max Fps set interface");

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

struct again_lut sc3035_again_lut[] = {
	{0x0010, 0},
	{0x0011, 5731},
	{0x0012, 11136},
	{0x0013, 16248},
	{0x0014, 21097},
	{0x0015, 25710},
	{0x0016, 30109},
	{0x0017, 34312},
	{0x0018, 38336},
	{0x0019, 42195},
	{0x001a, 45904},
	{0x001b, 49472},
	{0x001c, 52910},
	{0x001d, 56228},
	{0x001e, 59433},
	{0x001f, 62534},
	{0x0020, 65536},
	{0x0021, 68445},
	{0x0022, 71267},
	{0x0023, 74008},
	{0x0024, 76672},
	{0x0025, 79262},
	{0x0026, 81784},
	{0x0027, 84240},
	{0x0028, 86633},
	{0x0029, 88968},
	{0x002a, 91246},
	{0x002b, 93471},
	{0x002c, 95645},
	{0x002d, 97770},
	{0x002e, 99848},
	{0x002f, 101881},
	{0x0030, 103872},
	{0x0031, 105821},
	{0x0032, 107731},
	{0x0033, 109604},
	{0x0034, 111440},
	{0x0035, 113241},
	{0x0036, 115008},
	{0x0037, 116743},
	{0x0038, 118446},
	{0x0039, 120120},
	{0x003a, 121764},
	{0x003b, 123380},
	{0x003c, 124969},
	{0x003d, 126532},
	{0x003e, 128070},
	{0x003f, 129583},
	{0x0040, 131072},
	{0x0041, 132537},
	{0x0042, 133981},
	{0x0043, 135403},
	{0x0044, 136803},
	{0x0045, 138184},
	{0x0046, 139544},
	{0x0047, 140885},
	{0x0048, 142208},
	{0x0049, 143512},
	{0x004a, 144798},
	{0x004b, 146067},
	{0x004c, 147320},
	{0x004d, 148556},
	{0x004e, 149776},
	{0x004f, 150980},
	{0x0050, 152169},
	{0x0051, 153344},
	{0x0052, 154504},
	{0x0053, 155650},
	{0x0054, 156782},
	{0x0055, 157901},
	{0x0056, 159007},
	{0x0057, 160100},
	{0x0058, 161181},
	{0x0059, 162249},
	{0x005a, 163306},
	{0x005b, 164350},
	{0x005c, 165384},
	{0x005d, 166406},
	{0x005e, 167417},
	{0x005f, 168418},
	{0x0060, 169408},
	{0x0061, 170387},
	{0x0062, 171357},
	{0x0063, 172317},
	{0x0064, 173267},
	{0x0065, 174208},
	{0x0066, 175140},
	{0x0067, 176062},
	{0x0068, 176976},
	{0x0069, 177880},
	{0x006a, 178777},
	{0x006b, 179664},
	{0x006c, 180544},
	{0x006d, 181415},
	{0x006e, 182279},
	{0x006f, 183134},
	{0x0070, 183982},
	{0x0071, 184823},
	{0x0072, 185656},
	{0x0073, 186482},
	{0x0074, 187300},
	{0x0075, 188112},
	{0x0076, 188916},
	{0x0077, 189714},
	{0x0078, 190505},
	{0x0079, 191290},
	{0x007a, 192068},
	{0x007b, 192840},
	{0x007c, 193606},
	{0x007d, 194365},
	{0x007e, 195119},
	{0x007f, 195866},
	{0x0080, 196608},
	{0x0081, 197343},
	{0x0082, 198073},
	{0x0083, 198798},
	{0x0084, 199517},
	{0x0085, 200230},
	{0x0086, 200939},
	{0x0087, 201642},
	{0x0088, 202339},
	{0x0089, 203032},
	{0x008a, 203720},
	{0x008b, 204402},
	{0x008c, 205080},
	{0x008d, 205753},
	{0x008e, 206421},
	{0x008f, 207085},
	{0x0090, 207744},
	{0x0091, 208398},
	{0x0092, 209048},
	{0x0093, 209693},
	{0x0094, 210334},
	{0x0095, 210971},
	{0x0096, 211603},
	{0x0097, 212232},
	{0x0098, 212856},
	{0x0099, 213476},
	{0x009a, 214092},
	{0x009b, 214704},
	{0x009c, 215312},
	{0x009d, 215916},
	{0x009e, 216516},
	{0x009f, 217113},
	{0x00a0, 217705},
	{0x00a1, 218294},
	{0x00a2, 218880},
	{0x00a3, 219462},
	{0x00a4, 220040},
	{0x00a5, 220615},
	{0x00a6, 221186},
	{0x00a7, 221754},
	{0x00a8, 222318},
	{0x00a9, 222880},
	{0x00aa, 223437},
	{0x00ab, 223992},
	{0x00ac, 224543},
	{0x00ad, 225091},
	{0x00ae, 225636},
	{0x00af, 226178},
	{0x00b0, 226717},
	{0x00b1, 227253},
	{0x00b2, 227785},
	{0x00b3, 228315},
	{0x00b4, 228842},
	{0x00b5, 229365},
	{0x00b6, 229886},
	{0x00b7, 230404},
	{0x00b8, 230920},
	{0x00b9, 231432},
	{0x00ba, 231942},
	{0x00bb, 232449},
	{0x00bc, 232953},
	{0x00bd, 233455},
	{0x00be, 233954},
	{0x00bf, 234450},
	{0x00c0, 234944},
	{0x00c1, 235435},
	{0x00c2, 235923},
	{0x00c3, 236410},
	{0x00c4, 236893},
	{0x00c5, 237374},
	{0x00c6, 237853},
	{0x00c7, 238329},
	{0x00c8, 238803},
	{0x00c9, 239275},
	{0x00ca, 239744},
	{0x00cb, 240211},
	{0x00cc, 240676},
	{0x00cd, 241138},
	{0x00ce, 241598},
	{0x00cf, 242056},
	{0x00d0, 242512},
	{0x00d1, 242965},
	{0x00d2, 243416},
	{0x00d3, 243865},
	{0x00d4, 244313},
	{0x00d5, 244757},
	{0x00d6, 245200},
	{0x00d7, 245641},
	{0x00d8, 246080},
	{0x00d9, 246517},
	{0x00da, 246951},
	{0x00db, 247384},
	{0x00dc, 247815},
	{0x00dd, 248243},
	{0x00de, 248670},
	{0x00df, 249095},
	{0x00e0, 249518},
	{0x00e1, 249939},
	{0x00e2, 250359},
	{0x00e3, 250776},
	{0x00e4, 251192},
	{0x00e5, 251606},
	{0x00e6, 252018},
	{0x00e7, 252428},
	{0x00e8, 252836},
	{0x00e9, 253243},
	{0x00ea, 253648},
	{0x00eb, 254051},
	{0x00ec, 254452},
	{0x00ed, 254852},
	{0x00ee, 255250},
	{0x00ef, 255647},
	{0x00f0, 256041},
	{0x00f1, 256435},
	{0x00f2, 256826},
	{0x00f3, 257216},
	{0x00f4, 257604},
	{0x00f5, 257991},
	{0x00f6, 258376},
	{0x00f7, 258760},
	{0x00f8, 259142},
	{0x00f9, 259522},
	{0x00fa, 259901},
	{0x00fb, 260279},
	{0x00fc, 260655},
	{0x00fd, 261029},
	{0x00fe, 261402},
	{0x00ff, 261773},
	{0x0100, 262144}
};

struct tx_isp_sensor_attribute sc3035_attr;

unsigned int sc3035_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = sc3035_again_lut;
	while(lut->gain <= sc3035_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = lut[0].value;
			return lut[0].gain;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == sc3035_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int sc3035_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute sc3035_attr={
	.name = "sc3035",
	.chip_id = 0x3035,
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
	.max_again = 262144,
	.max_dgain = 0,
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 1580,
	.integration_time_limit = 1580,
	.total_width = 2500,
	.total_height = 1584,
	.max_integration_time = 1580,
	.one_line_expr_in_us = 25,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 2,
	.sensor_ctrl.alloc_again = sc3035_alloc_again,
	.sensor_ctrl.alloc_dgain = sc3035_alloc_dgain,
};

static struct regval_list sc3035_init_regs_2048_1536_25fps[] = {
#if 0
	/* pclk 108M*/
	{0x0100,0x00},
	{0x4500,0x31},
	{0x3416,0x11},
	{0x4501,0xa4},
	{0x3e03,0x03},
	{0x3e08,0x00},
	{0x3e09,0x10},
	{0x3e01,0x30},
	{0x322e,0x00},
	{0x322f,0xaf},
	{0x3306,0x20},
	{0x3307,0x17},
	{0x330b,0x54},
	{0x3303,0x20},
	{0x3309,0x20},
	{0x3308,0x08},
	{0x331e,0x16},
	{0x331f,0x16},
	{0x3320,0x18},
	{0x3321,0x18},
	{0x3322,0x18},
	{0x3323,0x18},
	{0x330c,0x0b},
	{0x330f,0x07},
	{0x3310,0x42},
	{0x3324,0x07},
	{0x3325,0x07},
	{0x335b,0xca},
	{0x335e,0x07},
	{0x335f,0x10},
	{0x3334,0x00},
	{0x3F01,0x04},
	{0x3F04,0x01},
	{0x3F05,0x30},
	{0x3626,0x01},
	{0x3635,0x60},
	{0x3631,0x84},
	{0x3636,0x8d},
	{0x3633,0x3f},
	{0x3639,0x80},
	{0x3622,0x1e},
	{0x3627,0x02},
	{0x3038,0xa4},
	{0x3621,0x18},
	{0x363a,0x1c},
	{0x3637,0xbe},
	{0x3638,0x85},
	{0x363c,0x48},
	{0x5780,0xff},
	{0x5781,0x04},
	{0x5785,0x10},
	{0x301c,0xa4},
	{0x301a,0xf8},
	{0x3019,0xff},
	{0x3022,0x13},
	{0x301e,0xe0},
	{0x3662,0x82},
	{0x3d0d,0x00},
	{0x3039,0x20},
	{0x303a,0x35}, //74.25M pclk
	{0x303b,0x00},
	{0x3306,0x46},
	{0x330b,0xa0},
	{0x3038,0xf8}, //pump clk div
	{0x320c,0x05},
	{0x320d,0xdc},
	{0x320e,0x06},  //vts=1584
	{0x320f,0x30},
	{0x3202,0x00}, // ystart=48
	{0x3203,0x00},
	{0x3206,0x06}, // yend=1545   1545 rows selected
	{0x3207,0x08},
	{0x3200,0x01}, // xstart= 264
	{0x3201,0x08},
	{0x3204,0x09}, // xend = 2319  2056 cols selected
	{0x3205,0x0f},
	{0x3211,0x04},  // xstart
	{0x3213,0x04},  // ystart
	{0x3208,0x08},  //2048x1536
	{0x3209,0x00},
	{0x320a,0x06},
	{0x320b,0x00},
	{0x3312,0x06}, // sa1 timing
	{0x3340,0x04},
	{0x3341,0xd2},
	{0x3342,0x01},
	{0x3343,0x80},
	{0x335d,0x2a}, // cmp timing
	{0x3348,0x04},
	{0x3349,0xd2},
	{0x334a,0x01},
	{0x334b,0x80},
	{0x3368,0x03}, // auto precharge
	{0x3369,0x30},
	{0x336a,0x06},
	{0x336b,0x30},
	{0x3367,0x05},
	{0x330e,0x17},
	{0x3d08,0x01}, // pclk inv
	{0x303f,0x82},
	{0x3c03,0x28}, //fifo sram read position
	{0x3c00,0x45}, // Dig SRAM reset
	{0x3c03,0x02}, //anti smear
	{0x3211,0x06},
	{0x3213,0x06},
	{0x3620,0x82},
	{0x3630,0xb1},
	{0x3635,0x60},
	{0x3630,0x67},
	{0x3626,0x11},
	{0x363c,0x88}, //fine gain correction
	{0x3312,0x00},
	{0x3333,0x80},
	{0x3334,0xa0},
	{0x3620,0x62},  //0xd2
	{0x3300,0x10},
	{0x3627,0x06},
	{0x3312,0x06},
	{0x3340,0x03},
	{0x3341,0x80},
	{0x3334,0x20},
	{0x331e,0x10},
	{0x331f,0x13},
	{0x3320,0x18},
	{0x3321,0x18},
	{0x3039,0x30},
	{0x303a,0x2a}, //118.8M pclk
	{0x303b,0x00},
	{0x3640,0x00},
	{0x3641,0x01},
	{0x5000,0x21},
	{0x3340,0x04},
	{0x3342,0x02},
	{0x3343,0x60},
	{0x334a,0x02},
	{0x334b,0x60},
	{0x3306,0x66},
	{0x3367,0x01},
	{0x330b,0xff},
	{0x3300,0x20},
	{0x331f,0x10},
	{0x3f05,0xe0},
	{0x3635,0x62},
	{0x3620,0x63},
	{0x3630,0x67},
	{0x3633,0x3c},
	{0x363a,0x04},
	{0x3039,0x00},
	{0x303a,0x2E},
	{0x3030,0x06},
	{0x303f,0x84},
	{0x5000,0x01},
	{0x320e,0x07},// vts=1600
	{0x320f,0x80},
	{0x3368,0x04},
	{0x3369,0x80},
	{0x336a,0x07},
	{0x336b,0x80},
	{0x3343,0x68},
	{0x334b,0x68},
	{0x330a,0x01},
	{0x330b,0x24},
	{0x3f04,0x02},
	{0x3f05,0x10},
	/*{0x5040,0x80}, //color bar*/
	{0x0100,0x01},
#endif
#if 1
	/* 99M pclk */
	{0x0100,0x00},
	{0x4500,0x31},
	{0x3416,0x11},
	{0x4501,0xa4},
	{0x3e03,0x03},
	{0x3e08,0x00},
	{0x3e09,0x10},
	{0x3e01,0x30},
	{0x322e,0x00},
	{0x322f,0xaf},
	{0x3306,0x20},
	{0x3307,0x17},
	{0x330b,0x54},
	{0x3303,0x20},
	{0x3309,0x20},
	{0x3308,0x08},
	{0x331e,0x16},
	{0x331f,0x16},
	{0x3320,0x18},
	{0x3321,0x18},
	{0x3322,0x18},
	{0x3323,0x18},
	{0x330c,0x0b},
	{0x330f,0x07},
	{0x3310,0x42},
	{0x3324,0x07},
	{0x3325,0x07},
	{0x335b,0xca},
	{0x335e,0x07},
	{0x335f,0x10},
	{0x3334,0x00},
	{0x3F01,0x04},
	{0x3F04,0x01},
	{0x3F05,0x30},
	{0x3626,0x01},
	{0x3635,0x60},
	{0x3631,0x84},
	{0x3636,0x8d},
	{0x3633,0x3f},
	{0x3639,0x80},
	{0x3622,0x1e},
	{0x3627,0x02},
	{0x3038,0xa4},
	{0x3621,0x18},
	{0x363a,0x1c},
	{0x3637,0xbe},
	{0x3638,0x85},
	{0x363c,0x48}, // ramp cur
	{0x5780,0xff}, // dpc
	{0x5781,0x04},
	{0x5785,0x10},
	{0x301c,0xa4}, // dig
	{0x301a,0xf8}, // ana
	{0x3019,0xff},
	{0x3022,0x13},
	{0x301e,0xe0}, // [4] 0:close tempsens
	{0x3662,0x82},
	{0x3d0d,0x00}, // close random code
	{0x3039,0x20},
	{0x303a,0x35}, //74.25M pclk
	{0x303b,0x00},
	{0x3306,0x46},
	{0x330b,0xa0},
	{0x3038,0xf8}, //pump clk div
	{0x320c,0x04},  //hts=2500
	{0x320d,0xe2},
	{0x320e,0x06},  //vts=1650
	{0x320f,0x30},
	{0x3202,0x00}, // ystart=48
	{0x3203,0x00},
	{0x3206,0x06}, // yend=1545   1545 rows selected
	{0x3207,0x08},
	{0x3200,0x01}, // xstart= 264
	{0x3201,0x08},
	{0x3204,0x09}, // xend = 2319  2056 cols selected
	{0x3205,0x0f},
	{0x3211,0x04},  // xstart
	{0x3213,0x04},  // ystart
	{0x3208,0x08},  //2048x1536
	{0x3209,0x00},
	{0x320a,0x06},
	{0x320b,0x00},
	{0x3312,0x06}, // sa1 timing
	{0x3340,0x04},
	{0x3341,0xd2},
	{0x3342,0x01},
	{0x3343,0x80},
	{0x335d,0x2a}, // cmp timing
	{0x3348,0x04},
	{0x3349,0xd2},
	{0x334a,0x01},
	{0x334b,0x80},
	{0x3368,0x03}, // auto precharge
	{0x3369,0x30},
	{0x336a,0x06},
	{0x336b,0x30},
	{0x3367,0x05},
	{0x330e,0x17},
	{0x3d08,0x01}, // pclk inv
	{0x303f,0x82},
	{0x3c03,0x28}, //fifo sram read position
	{0x3c00,0x45}, // Dig SRAM reset
	{0x3c03,0x02}, //anti smear
	{0x3211,0x06},
	{0x3213,0x06},
	{0x3620,0x82},
	{0x3630,0xb1},
	{0x3635,0x60},
	{0x3630,0x67},
	{0x3626,0x11},
	{0x363c,0x88},
	{0x3312,0x00},
	{0x3333,0x80},
	{0x3334,0xa0},
	{0x3620,0x62},
	{0x3300,0x10},
	{0x3627,0x06},
	{0x3340,0x03},
	{0x3341,0x80},
	{0x3334,0x20},
	{0x331e,0x10},
	{0x331f,0x13},
	{0x3320,0x18},
	{0x3321,0x18},
	{0x3633,0x3e},
	{0x331f,0x11},
	{0x363a,0x04},
	{0x3f05,0x76},
	{0x3620,0x63},
	{0x3367,0x02},
	{0x3211,0x04},  // xstart
	{0x5000,0x21},
	{0x5000,0x27},
	{0x3039,0x50},
	{0x303a,0x1f},
	{0x330b,0xc0},
	{0x3306,0x46},
	{0x3035,0xba},
	{0x3343,0xe0},
	{0x334b,0xe0},	//{0x5040,0x80}, //color bar
#endif
	{SC3035_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc3035_init_regs_1536_1536_25fps[] = {

	{0x0100,0x00},
	{0x4500,0x31}, // rnc sel
	{0x3416,0x11},
	{0x4501,0xa4}, // bit ctrl
	{0x3e03,0x03}, // aec
	{0x3e08,0x00},
	{0x3e09,0x10},
	{0x3e01,0x30},
	{0x322e,0x00},
	{0x322f,0xaf},
	{0x3306,0x20},
	{0x3307,0x17},
	{0x330b,0x54},
	{0x3303,0x20},
	{0x3309,0x20},
	{0x3308,0x08},
	{0x331e,0x16},
	{0x331f,0x16},
	{0x3320,0x18},
	{0x3321,0x18},
	{0x3322,0x18},
	{0x3323,0x18},
	{0x330c,0x0b},
	{0x330f,0x07},
	{0x3310,0x42},
	{0x3324,0x07},
	{0x3325,0x07},
	{0x335b,0xca},
	{0x335e,0x07},
	{0x335f,0x10},
	{0x3334,0x00},
	{0x3F01,0x04},
	{0x3F04,0x01},
	{0x3F05,0x30},
	{0x3626,0x01},
	{0x3635,0x60},
	{0x3631,0x84},
	{0x3636,0x8d},
	{0x3633,0x3f},
	{0x3639,0x80},
	{0x3622,0x1e},
	{0x3627,0x02},
	{0x3038,0xa4},
	{0x3621,0x18},
	{0x363a,0x1c},
	{0x3637,0xbe},
	{0x3638,0x85},
	{0x363c,0x48}, // ramp cur
	{0x5780,0xff}, // dpc
	{0x5781,0x04},
	{0x5785,0x10},
	{0x301c,0xa4}, // dig
	{0x301a,0xf8}, // ana
	{0x3019,0xff},
	{0x3022,0x13},
	{0x301e,0xe0}, // [4] 0:close tempsens
	{0x3662,0x82},
	{0x3d0d,0x00}, // close random code
	{0x3039,0x00},
	{0x303a,0x32}, //75.6M pclk
	{0x303b,0x02},
	{0x3306,0x46},
	{0x330b,0xa0},
	{0x3038,0xf8}, //pump clk div
	{0x320c,0x03},  // hts=1920
	{0x320d,0xc0},
	{0x320e,0x06},  // vts=1575
	{0x320f,0x27},
	{0x3202,0x00}, // ystart=0
	{0x3203,0x00},
	{0x3206,0x06}, // yend=1544   1545 rows selected
	{0x3207,0x08},
	{0x3200,0x02}, // xstart= 520
	{0x3201,0x08},
	{0x3204,0x08}, // xend = 2063  1544 cols selected
	{0x3205,0x0f},
	{0x3211,0x04},  // xstart
	{0x3213,0x04},  // ystart
	{0x3208,0x06},  // 1536x1536
	{0x3209,0x00},
	{0x320a,0x06},
	{0x320b,0x00},
	{0x3312,0x06}, // sa1 timing
	{0x3340,0x03},
	{0x3341,0x74},
	{0x3342,0x01},
	{0x3343,0x80},
	{0x335d,0x2a}, // cmp timing
	{0x3348,0x03},
	{0x3349,0x74},
	{0x334a,0x01},
	{0x334b,0x80},
	{0x3368,0x03}, // auto precharge
	{0x3369,0x27},
	{0x336a,0x06},
	{0x336b,0x27},
	{0x3367,0x02},
	{0x330e,0x17},
	{0x3d08,0x00}, // pclk inv
	{0x303f,0x82},
	{0x3c03,0x28}, //fifo sram read position
	{0x3c00,0x45}, // Dig SRAM reset
	{0x3c03,0x02}, //anti smear
	{0x3211,0x06},
	{0x3213,0x06},
	{0x3620,0x82},
	{0x3630,0xb1},
	{0x3635,0x60},
	{0x3630,0x67},
	{0x3626,0x11},
	{0x363c,0x88}, //fine gain correction
	{0x3312,0x00},
	{0x3333,0x80},
	{0x3334,0xa0},
	{0x3620,0x62},
	{0x3300,0x10},
	{0x3627,0x06},
	{0x3312,0x06},
	{0x3340,0x02},
	{0x3341,0x80},
	{0x3348,0x03},
	{0x3349,0xb0},
	{0x3334,0x20},
	{0x331e,0x10},
	{0x331f,0x13},
	{0x3320,0x18},
	{0x3321,0x18},
	{0x3300,0x18},
	{0x3343,0xa8},
	{0x334b,0xa8},
	{0x331f,0x10},
	{0x3633,0x3d}, //for better dark current
	{0x363a,0x04},
	{0x3208,0x06},
	{0x3209,0x00},
	{0x3633,0x3c},
	{0x363a,0x04},
	{0x3633,0x3d},
	{0x3640,0x01},
	{0x3f05,0x60},
	{0x3039,0x50},
	{0x303a,0x01},
	{0x303b,0x02},
	{0x3035,0xba},
	{SC3035_REG_END, 0x00},	/* END MARKER */
};


/*
 * the order of the sc3035_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting sc3035_win_sizes[] = {
	/* 2048*1536 */
	{
		.width		= 2048,
		.height		= 1536,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= sc3035_init_regs_2048_1536_25fps,
	},
	/* 1536*1536 */
	{
		.width		= 1536,
		.height		= 1536,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= sc3035_init_regs_1536_1536_25fps,
	}
};

static enum v4l2_mbus_pixelcode sc3035_mbus_code[] = {
	V4L2_MBUS_FMT_SBGGR10_1X10,
	V4L2_MBUS_FMT_SBGGR12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list sc3035_stream_on[] = {

	{0x0100, 0x01},
	{SC3035_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc3035_stream_off[] = {

	{0x0100, 0x00},
	{SC3035_REG_END, 0x00},	/* END MARKER */
};

int sc3035_read(struct v4l2_subdev *sd, uint16_t reg, unsigned char *value)
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

int sc3035_write(struct v4l2_subdev *sd, uint16_t reg, unsigned char value)
{
	int ret;;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	uint8_t buf[3] = {(reg>>8)&0xff, reg&0xff, value};
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

static int sc3035_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SC3035_REG_END) {
		if (vals->reg_num == SC3035_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = sc3035_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int sc3035_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SC3035_REG_END) {
		if (vals->reg_num == SC3035_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = sc3035_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int sc3035_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sc3035_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	int ret;
	unsigned char v;
	ret = sc3035_read(sd, 0x3107, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC3035_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = sc3035_read(sd, 0x3108, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC3035_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int sc3035_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret += sc3035_write(sd, 0x3e00, (unsigned char)((value & 0xf000) >> 12));
	ret += sc3035_write(sd, 0x3e01, (unsigned char)((value >> 4) & 0xff));
	ret += sc3035_write(sd, 0x3e02, (unsigned char)((value & 0x0f) << 4));
	if (ret < 0)
		return ret;
	return 0;
}

static int sc3035_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	unsigned char v;

	ret += sc3035_write(sd, 0x3e09, (unsigned char)(value & 0xff));
	ret += sc3035_write(sd, 0x3e08, (unsigned char)((value & 0xff00) >> 8));
	if (ret < 0)
		return ret;

	ret += sc3035_read(sd, 0x3109, &v);
	if(v == 0x02){
		if(value < 0x20){
			ret += sc3035_write(sd, 0x3620, 0xd2);
		}else if(value <= 0x100){
			ret += sc3035_write(sd, 0x3620, 0x62);
		}
	}else if(v == 0x03){
		ret += sc3035_write(sd, 0x3620, 0x62);
	}

	/* denoise logic */
	if (value < 0x20) {
		ret += sc3035_write(sd, 0x3630, 0xa9);
		ret += sc3035_write(sd, 0x3627, 0x02);
		ret += sc3035_write(sd, 0x3635, 0x65);
		if (ret < 0)
			return ret;
	}
	else if (value <= 0x100){
		ret += sc3035_write(sd, 0x3630, 0x67);
		ret += sc3035_write(sd, 0x3627, 0x02);
		ret += sc3035_write(sd, 0x3635, 0x65);
		if (ret < 0)
			return ret;
	}
/*	else{
		ret += sc3035_write(sd, 0x3630, 0x84);
		ret += sc3035_write(sd, 0x3635, 0x52);
		ret += sc3035_write(sd, 0x3620, 0x62);
		ret += sc3035_write(sd, 0x3315, 0x00);
	    if (ret < 0)
			return ret;
	}
*/
	return 0;
}

static int sc3035_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sc3035_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sc3035_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = 0;

	if(!enable)
		return ISP_SUCCESS;

	switch (sensor_max_resolution) {
	case resolution_2048_1536:
		wsize = &sc3035_win_sizes[0];
		break;
	case resolution_1536_1536:
		wsize = &sc3035_win_sizes[1];
		break;
	default:
		printk("Now we do not support this resolution!!!\n");
	}

	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = sc3035_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int sc3035_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sc3035_write_array(sd, sc3035_stream_on);
		pr_debug("sc3035 stream on\n");
	}
	else {
		ret = sc3035_write_array(sd, sc3035_stream_off);
		pr_debug("sc3035 stream off\n");
	}
	return ret;
}

static int sc3035_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sc3035_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sc3035_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	unsigned int pclk;
	unsigned short hts;
	unsigned short vts = 0;
	unsigned short drop_frame_reg = 0;
	unsigned char tmp;
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	unsigned int max_fps = 0;

	switch (sensor_max_resolution) {
	case resolution_2048_1536:
		pclk = SC3035_SUPPORT_2048_1536_PCLK;
		max_fps = SENSOR_OUTPUT_MAX_FPS;
		break;
	case resolution_1536_1536:
		pclk = SC3035_SUPPORT_1536_1536_PCLK;
		max_fps = SENSOR_OUTPUT_MAX_FPS;
		break;
	default:
		printk("Now we do not support this resolution!!!\n");
	}

	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}
	ret = sc3035_read(sd, 0x320c, &tmp);
	hts = tmp;
	ret += sc3035_read(sd, 0x320d, &tmp);
	if(ret < 0)
		return -1;
	hts = ((hts << 8) + tmp)*2;

	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	drop_frame_reg = vts - 0x300;

	ret = sc3035_write(sd, 0x320f, (unsigned char)(vts & 0xff));
	ret += sc3035_write(sd, 0x320e, (unsigned char)(vts >> 8));
	if(ret < 0){
		printk("err: sc3035_write err\n");
		return ret;
	}
	ret = sc3035_write(sd, 0x336b, (unsigned char)(vts & 0xff));
	ret += sc3035_write(sd, 0x336a, (unsigned char)(vts >> 8));
	ret += sc3035_write(sd, 0x3369, (unsigned char)(drop_frame_reg & 0xff));
	ret += sc3035_write(sd, 0x3368, (unsigned char)(drop_frame_reg >> 8));
	if(ret < 0){
		printk("err: sc3035_write err\n");
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
}

static int sc3035_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	switch (sensor_max_resolution) {
		case resolution_2048_1536:
			if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
				wsize = &sc3035_win_sizes[0];
			}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
				wsize = &sc3035_win_sizes[0];
			}
			break;
		case resolution_1536_1536:
			if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
				wsize = &sc3035_win_sizes[1];
			}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
				wsize = &sc3035_win_sizes[1];
			}
			break;
		default:
			printk("Now we do not support this resolution!!!\n");
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

static int sc3035_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"sc3035_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(10);
			gpio_direction_output(reset_gpio, 0);
			msleep(10);
			gpio_direction_output(reset_gpio, 1);
			msleep(8);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if (pwdn_gpio != -1) {
		ret = gpio_request(pwdn_gpio, "sc3035_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			msleep(120);
			gpio_direction_output(pwdn_gpio, 0);
			msleep(10);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = sc3035_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sc3035 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "sc3035 chip found @ 0x%02x (%s)\n",
		client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sc3035_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static long sc3035_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = sc3035_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = sc3035_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = sc3035_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = sc3035_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = sc3035_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		ret = sc3035_write_array(sd, sc3035_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		ret = sc3035_write_array(sd, sc3035_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = sc3035_set_fps(sensor, ctrl->value);
		break;
	default:
		pr_debug("do not support ctrl->cmd ====%d\n",ctrl->cmd);
		break;
	}
	return 0;
}

static long sc3035_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = sc3035_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc3035_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = sc3035_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int sc3035_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	sc3035_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops sc3035_core_ops = {
	.g_chip_ident = sc3035_g_chip_ident,
	.reset = sc3035_reset,
	.init = sc3035_init,
	.s_power = sc3035_s_power,
	.ioctl = sc3035_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sc3035_g_register,
	.s_register = sc3035_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sc3035_video_ops = {
	.s_stream = sc3035_s_stream,
	.s_parm = sc3035_s_parm,
	.g_parm = sc3035_g_parm,
};

static const struct v4l2_subdev_ops sc3035_ops = {
	.core = &sc3035_core_ops,
	.video = &sc3035_video_ops,
};

static int sc3035_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = NULL; //&sc3035_win_sizes[0];
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

	sc3035_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
	case DVP_PA_LOW_10BIT:
	case DVP_PA_HIGH_10BIT:
		mbus = sc3035_mbus_code[0];
		break;
	case DVP_PA_12BIT:
		mbus = sc3035_mbus_code[1];
		break;
	default:
		goto err_set_sensor_gpio;
	}

	switch (sensor_max_resolution) {
		case resolution_2048_1536:
			wsize = &sc3035_win_sizes[0];

			sc3035_attr.max_integration_time_native = 1580;
			sc3035_attr.integration_time_limit = 1580;
			sc3035_attr.total_width = 2500;
			sc3035_attr.total_height = 1584;
			sc3035_attr.max_integration_time = 1580;
			break;
		case resolution_1536_1536:

			wsize = &sc3035_win_sizes[1];

			sc3035_attr.max_integration_time_native = 1571;
			sc3035_attr.integration_time_limit = 1571;
			sc3035_attr.total_width = 1920 ;
			sc3035_attr.total_height = 1575;
			sc3035_attr.max_integration_time = 1571;
			break;
		default:
			printk("Now we do not support this resolution!!!\n");
	}

	for(i = 0; i < ARRAY_SIZE(sc3035_win_sizes); i++)
		sc3035_win_sizes[i].mbus_code = mbus;

	/*
	  convert sensor-gain into isp-gain,
	*/
	sc3035_attr.max_again = 262144;
	sc3035_attr.max_dgain = 0; //sc3035_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &sc3035_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &sc3035_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("@@@@@@@probe ok ------->sc3035\n");
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int sc3035_remove(struct i2c_client *client)
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

static const struct i2c_device_id sc3035_id[] = {
	{ "sc3035", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc3035_id);

static struct i2c_driver sc3035_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "sc3035",
	},
	.probe		= sc3035_probe,
	.remove		= sc3035_remove,
	.id_table	= sc3035_id,
};

static __init int init_sc3035(void)
{
	return i2c_add_driver(&sc3035_driver);
}

static __exit void exit_sc3035(void)
{
	i2c_del_driver(&sc3035_driver);
}

module_init(init_sc3035);
module_exit(exit_sc3035);

MODULE_DESCRIPTION("A low-level driver for Smartsenstech sc3035 sensors");
MODULE_LICENSE("GPL");
