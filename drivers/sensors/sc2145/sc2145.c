/*
 * sc2145.c
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

#define SC2145_CHIP_ID_H	(0x21)
#define SC2145_CHIP_ID_L	(0x45)
#define SC2145_REG_END		0xffff
#define SC2145_REG_DELAY	0xfffe
#define SC2145_SUPPORT_PCLK_FPS_30 (74250*1000)
#define SC2145_SUPPORT_PCLK_FPS_15 (37125*1000)
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

static int sensor_max_fps = TX_SENSOR_MAX_FPS_25;
module_param(sensor_max_fps, int, S_IRUGO);
MODULE_PARM_DESC(sensor_max_fps, "Sensor Max Fps set interface");

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

struct again_lut sc2145_again_lut[] = {
#if 0
	/*3e03:0x0b,gain map1*/
	{0x0310, 0},
	{0x0311, 5731},
	{0x0312, 11136},
	{0x0313, 16247},
	{0x0314, 21097},
	{0x0315, 25710},
	{0x0316, 30108},
	{0x0317, 34311},
	{0x0318, 38335},
	{0x0319, 42195},
	{0x031a, 45903},
	{0x031b, 49471},
	{0x031c, 52910},
	{0x031d, 56227},
	{0x031e, 59433},
	{0x031f, 62533},
	{0x0710, 65535},
	{0x0711, 71266},
	{0x0712, 76671},
	{0x0713, 81782},
	{0x0714, 86632},
	{0x0715, 91245},
	{0x0716, 95643},
	{0x0717, 99846},
	{0x0718, 103870},
	{0x0719, 107730},
	{0x071a, 111438},
	{0x071b, 115006},
	{0x071c, 118445},
	{0x071d, 121762},
	{0x071e, 124968},
	{0x071f, 128068},
	{0x0f10, 131070},
	{0x0f11, 136801},
	{0x0f12, 142206},
	{0x0f13, 147317},
	{0x0f14, 152167},
	{0x0f15, 156780},
	{0x0f16, 161178},
	{0x0f17, 165381},
	{0x0f18, 169405},
	{0x0f19, 173265},
	{0x0f1a, 176973},
	{0x0f1b, 180541},
	{0x0f1c, 183980},
	{0x0f1d, 187297},
	{0x0f1e, 190503},
	{0x0f1f, 193603},
	{0x1f10, 196605},
	{0x1f11, 202336},
	{0x1f12, 207741},
	{0x1f13, 212852},
	{0x1f14, 217702},
	{0x1f15, 222315},
	{0x1f16, 226713},
	{0x1f17, 230916},
	{0x1f18, 234940},
	{0x1f19, 238800},
	{0x1f1a, 242508},
	{0x1f1b, 246076},
	{0x1f1c, 249515},
	{0x1f1d, 252832},
	{0x1f1e, 256038},
	{0x1f1f, 259138}
#endif

	/*3e03:0x03 gain map2*/
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
};

struct tx_isp_sensor_attribute sc2145_attr;

unsigned int sc2145_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = sc2145_again_lut;
	while(lut->gain <= sc2145_attr.max_again) {
		if(isp_gain == 0) {
			*sensor_again = lut[0].value;
			return lut[0].gain;
		}
		else if(isp_gain < lut->gain) {
			*sensor_again = (lut - 1)->value;
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == sc2145_attr.max_again) && (isp_gain >= lut->gain)) {
				*sensor_again = lut->value;
				return lut->gain;
			}
		}

		lut++;
	}

	return isp_gain;
}

unsigned int sc2145_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute sc2145_attr={
	.name = "sc2145",
	.chip_id = 0x2145,
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
	.max_again = 258760,
	.max_dgain = 0,
	.min_integration_time = 4,
	.min_integration_time_native = 4,
	.max_integration_time_native = 2700-4,
	.integration_time_limit = 2700-4,
	.total_width = 2200,
	.total_height = 1350,
	.max_integration_time = 2700-4,
	.one_line_expr_in_us = 29,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 2,
	.sensor_ctrl.alloc_again = sc2145_alloc_again,
	.sensor_ctrl.alloc_dgain = sc2145_alloc_dgain,
};


static struct regval_list sc2145_init_regs_1920_1080_25fps[] = {
	{0x0100,0x00},
	{0x3c00,0x45}, //FIFO RESET
	{0x3039,0xd0}, //PLL1 BYPASS
	{0x3907,0x01}, //RNC BLC
	{0x3908,0xc0},
	{0x3416,0x10},
	{0x3200,0x00},
	{0x3201,0x00},
	{0x3204,0x07},
	{0x3205,0x9f},
	{0x322c,0x07},
	{0x322d,0xa8},
	{0x322e,0x07},
	{0x322f,0xff},
	{0x3400,0x53},
	{0x3401,0x1e},
	{0x3402,0x04},
	{0x3403,0x30},
	{0x3637,0x87},//RAMP
	{0x3623,0x02},//analog
	{0x3620,0xc4},
	{0x3621,0x18},
	{0x3635,0x03},
	{0x3300,0x10},//EQ
	{0x3306,0xe0},
	{0x330a,0x01},
	{0x330b,0xa0},
	{0x3333,0x00},
	{0x3334,0x20},
	{0x3039,0x00}, //74.25M pclk
	{0x303a,0x35},
	{0x303b,0x0c},
	{0x3035,0xca},
	{0x320c,0x08}, //0x898 for 30fps
	{0x320d,0x98},
	{0x320e,0x05},
	{0x320f,0x46},
	{0x3211,0x10},
	{0x3213,0x10},
	{0x301c,0xa4},//close mipi
	{0x3018,0xff},
	{0x3d08,0x01}, //pclk inv
	{0x337f,0x03}, //new auto precharge  330e in 3372
	{0x3368,0x04},
	{0x3369,0x00},
	{0x336a,0x00},
	{0x336b,0x00},
	{0x3367,0x08},
	{0x330e,0x40},//0x3630/0x3635 auto ctrl
	{0x3670,0x0b}, //bit[3] for 3635 in 3687, bit[1] for 3630 in 3686,bit[0] for 3620 in 3685
	{0x3674,0xa0}, //3630 value <gain0
	{0x3675,0x90}, //3630 value between gain0 and gain1
	{0x3676,0x40}, //3630 value > gain1
	{0x367c,0x07}, //gain0
	{0x367d,0x0f}, //gain1
	{0x3677,0x0a}, //3635 value <gain0
	{0x3678,0x07}, //3635 value between gain0 and gain1
	{0x3679,0x07}, //3635 value > gain1
	{0x367e,0x07}, //gain0
	{0x367f,0x1f}, //gain1
	{0x3671,0xc2}, //3620 value <gain0   11.23
	{0x3672,0xc2}, //3620 value between gain0 and gain1  11.23
	{0x3673,0x63}, //3620 value > gain1  11.23
	{0x367a,0x07}, //gain0
	{0x367b,0x1f}, //gain1
	{0x3e03,0x03}, //AE
	{0x3e01,0x46},
	{0x3e08,0x00},
	{0x3401,0x1e},
	{0x3402,0x00},
	{0x3403,0x48}, //increase rnc col num to 72+16=88
	{0x5781,0x08}, //dpc
	{0x5782,0x08},
	{0x5785,0x20},
	{0x57a2,0x01},
	{0x57a3,0xf1},//fullwell
	{0x3637,0x86},
	{0x3635,0x03},//  gain <16 0a
	{0x3622,0x0e},
	{0x3630,0x00},
	{0x3631,0x80}, //gain <2  0x81
	{0x3633,0x54},//fullwell adjust 0907
	{0x3637,0x87},
	{0x3674,0xd0},
	{0x3677,0x07},
	{0x3633,0x74},
	{0x330b,0xa8},
	{0x3333,0x80}, //col fpn
	{0x3334,0xa0},
	{0x3300,0x20}, //shading
	{0x3632,0x40}, //gain >8 0x42 //0908 update rnc num
	{0x3403,0x58},
	{0x3416,0x11},
	{0x3302,0x28}, //rst go low point to cancel left column fpn from rst
	{0x3309,0x20}, //ramp gap to cancel right column fpn from tx
	{0x331f,0x17},
	{0x3321,0x1a},
	{0x3677,0x0b}, //high txvdd when gain<2
	{0x3678,0x08}, //3635 value between gain0 and gain1
	{0x3679,0x06}, //3635 value > gain1
	{0x3306,0xd0},//ECO
	{0x322e,0x08}, //rnc
	{0x322f,0x37},
	{0x3403,0x78},
	{0x3679,0x08},//1118
	{0x3679,0x06},
	{0x3620,0xc4}, //0x64//1122
	{0x3637,0x84},
	{0x3638,0x84},//high tem_20170110
	{0x3e15,0x02},
	{0x3e16,0x5f},
	{0x3e17,0x10},
	{0x3e18,0x00},
	{0x3e19,0x13},
	{0x3e1a,0x60},
	{0x3e1b,0x12},
	{0x3e1c,0x60},
	{0x3e1d,0x62},//20170314
	{0x330e,0x20},
	{0x3308,0x20},
	{0x3632,0x70},
	{0x3676,0x38},//20170314B
	{0x3676,0x0f},//24M PLL
	{0x3039,0x50},
	{0x303a,0x5f},
	{0x303b,0x05}, // PLL1
	{0x3034,0x25},
	{0x3035,0x2a}, // PLL2
	{0x3637,0x83}, // ramp
	{0x330b,0xd8},
	{0x3306,0xe8}, // counter timing
	{0x3621,0x08}, // ini clk
	{0x3223,0x50}, // first frame
	{0x3364,0x05},
	{0x3640,0x02},
	{SC2145_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc2145_init_regs_1920_1080_15fps[] = {
	{0x0100,0x00},
	{0x3c00,0x45}, //FIFO RESET
	{0x3039,0xd0}, //PLL1 BYPASS
	{0x3907,0x01}, //RNC BLC
	{0x3908,0xc0},
	{0x3416,0x10},
	{0x3200,0x00},
	{0x3201,0x00},
	{0x3204,0x07},
	{0x3205,0x9f},
	{0x322c,0x07},
	{0x322d,0xa8},
	{0x322e,0x07},
	{0x322f,0xff},
	{0x3400,0x53},
	{0x3401,0x1e},
	{0x3402,0x04},
	{0x3403,0x30},
	{0x3637,0x87},//RAMP
	{0x3623,0x02},//analog
	{0x3620,0xc4},
	{0x3621,0x18},
	{0x3635,0x03},//Timing
	{0x3300,0x10},//EQ
	{0x3306,0xe0},
	{0x330a,0x01},
	{0x330b,0xa0},
	{0x3333,0x00},
	{0x3334,0x20},
	{0x3039,0x00}, //74.25M pclk
	{0x303a,0x35},
	{0x303b,0x0c},
	{0x3035,0xca},
	{0x320c,0x08}, //0x898 for 30fps
	{0x320d,0x98},
	{0x3211,0x10},
	{0x3213,0x10},
	{0x301c,0xa4},//close mipi
	{0x3018,0xff},
	{0x3d08,0x01}, //pclk inv
	{0x337f,0x03}, //new auto precharge  330e in 3372
	{0x3368,0x04},
	{0x3369,0x00},
	{0x336a,0x00},
	{0x336b,0x00},
	{0x3367,0x08},
	{0x330e,0x40},//0x3630/0x3635 auto ctrl
	{0x3670,0x0b}, //bit[3] for 3635 in 3687, bit[1] for 3630 in 3686,bit[0] for 3620 in 3685
	{0x3674,0xa0}, //3630 value <gain0
	{0x3675,0x90}, //3630 value between gain0 and gain1
	{0x3676,0x40}, //3630 value > gain1
	{0x367c,0x07}, //gain0
	{0x367d,0x0f}, //gain1
	{0x3677,0x0a}, //3635 value <gain0
	{0x3678,0x07}, //3635 value between gain0 and gain1
	{0x3679,0x07}, //3635 value > gain1
	{0x367e,0x07}, //gain0
	{0x367f,0x1f}, //gain1
	{0x3671,0xc2}, //3620 value <gain0   11.23
	{0x3672,0xc2}, //3620 value between gain0 and gain1  11.23
	{0x3673,0x63}, //3620 value > gain1  11.23
	{0x367a,0x07}, //gain0
	{0x367b,0x1f}, //gain1
	{0x3e03,0x03}, //AE
	{0x3e01,0x46},
	{0x3e08,0x00},
	{0x3401,0x1e},
	{0x3402,0x00},
	{0x3403,0x48}, //increase rnc col num to 72+16=88
	{0x5781,0x08}, //dpc
	{0x5782,0x08},
	{0x5785,0x20},
	{0x57a2,0x01},
	{0x57a3,0xf1},//fullwell
	{0x3637,0x86},
	{0x3635,0x03},//  gain <16 0a
	{0x3622,0x0e},
	{0x3630,0x00},
	{0x3631,0x80}, //gain <2  0x81
	{0x3633,0x54},//fullwell adjust 0907
	{0x3637,0x87},
	{0x3674,0xd0},
	{0x3677,0x07},
	{0x3633,0x74},
	{0x330b,0xa8},
	{0x3333,0x80}, //col fpn
	{0x3334,0xa0},
	{0x3300,0x20}, //shading
	{0x3632,0x40}, //gain >8 0x42//0908 update rnc num
	{0x3403,0x58},
	{0x3416,0x11},
	{0x3302,0x28}, //rst go low point to cancel left column fpn from rst
	{0x3309,0x20}, //ramp gap to cancel right column fpn from tx
	{0x331f,0x17},
	{0x3321,0x1a},
	{0x3677,0x0b}, //high txvdd when gain<2
	{0x3678,0x08}, //3635 value between gain0 and gain1
	{0x3679,0x06}, //3635 value > gain1
	{0x3306,0xd0},//ECO
	{0x322e,0x08}, //rnc
	{0x322f,0x37},
	{0x3403,0x78},
	{0x3679,0x08},//1118
	{0x3679,0x06},
	{0x3620,0xc4}, //0x64//1122
	{0x3637,0x84},
	{0x3638,0x84},//high tem_20170110
	{0x3e15,0x02},
	{0x3e16,0x5f},
	{0x3e17,0x10},
	{0x3e18,0x00},
	{0x3e19,0x13},
	{0x3e1a,0x60},
	{0x3e1b,0x12},
	{0x3e1c,0x60},
	{0x3e1d,0x62},//20170314
	{0x330e,0x20},
	{0x3308,0x20},
	{0x3632,0x70},
	{0x3676,0x38},//20170314B
	{0x3676,0x0f},//24M PLL
	{0x3039,0x50},
	{0x303a,0x5f},
	{0x303b,0x0d}, // PLL1
	{0x3034,0x25},
	{0x3035,0x2a}, // PLL2
	{0x3637,0x83}, // ramp
	{0x330a,0x00},
	{0x330b,0xf8},
	{0x3306,0xe8}, // counter timing
	{0x3621,0x08}, // ini clk
	{0x3223,0x50}, // first frame
	{0x3364,0x05},
	{0x330e,0x18},
	{SC2145_REG_END, 0x00},	/* END MARKER */
};

static struct tx_isp_sensor_win_setting sc2145_win_sizes[] = {
	/* 1920*1080 */
	{
		.width		= 1920,
		.height		= 1080,
		.fps		= 25 << 16 | 1,
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= sc2145_init_regs_1920_1080_25fps,
	}
};

static enum v4l2_mbus_pixelcode sc2145_mbus_code[] = {
	V4L2_MBUS_FMT_SBGGR10_1X10,
	V4L2_MBUS_FMT_SBGGR12_1X12,
};

/*
 * the part of driver was fixed.
 */

static struct regval_list sc2145_stream_on[] = {

	{0x0100, 0x01},
	{SC2145_REG_END, 0x00},	/* END MARKER */
};

static struct regval_list sc2145_stream_off[] = {
	{0x0100, 0x00},
	{SC2145_REG_END, 0x00},	/* END MARKER */
};

int sc2145_read(struct v4l2_subdev *sd, uint16_t reg, unsigned char *value)
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

int sc2145_write(struct v4l2_subdev *sd, uint16_t reg, unsigned char value)
{
	int ret;
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

static int sc2145_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val;
	while (vals->reg_num != SC2145_REG_END) {
		if (vals->reg_num == SC2145_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = sc2145_read(sd, vals->reg_num, &val);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int sc2145_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != SC2145_REG_END) {
		if (vals->reg_num == SC2145_REG_DELAY) {
			msleep(vals->value);
		} else {
			ret = sc2145_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

static int sc2145_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sc2145_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	int ret;
	unsigned char v;
	ret = sc2145_read(sd, 0x3107, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC2145_CHIP_ID_H)
		return -ENODEV;
	*ident = v;

	ret = sc2145_read(sd, 0x3108, &v);
	pr_debug("-----%s: %d ret = %d, v = 0x%02x\n", __func__, __LINE__, ret,v);
	if (ret < 0)
		return ret;
	if (v != SC2145_CHIP_ID_L)
		return -ENODEV;
	*ident = (*ident << 8) | v;
	return 0;
}

static int sc2145_set_integration_time(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret += sc2145_write(sd, 0x3e00, (unsigned char)((value & 0xf000) >> 12));
	ret += sc2145_write(sd, 0x3e01, (unsigned char)((value >> 4) & 0xff));
	ret += sc2145_write(sd, 0x3e02, (unsigned char)((value & 0x0f) << 4));
	if (ret < 0)
		return ret;
	return 0;
}

static int sc2145_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	int ret = 0;
	ret += sc2145_write(sd, 0x3e09, (unsigned char)(value & 0xff));
	ret += sc2145_write(sd, 0x3e08, (unsigned char)((value & 0xff00) >> 8));
	if (ret < 0)
		return ret;
	return 0;
}

static int sc2145_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sc2145_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sc2145_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &sc2145_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;

	switch (sensor_max_fps) {
	case TX_SENSOR_MAX_FPS_25:
		wsize->regs = sc2145_init_regs_1920_1080_25fps;
		wsize->fps = 25 << 16 | 1;
		break;
	case TX_SENSOR_MAX_FPS_15:
		wsize->regs = sc2145_init_regs_1920_1080_15fps;
		wsize->fps = 15 << 16 | 1;
		break;
	default:
		printk("Now we do not support this framerate!!!\n");
	}
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = sc2145_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int sc2145_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	if (enable) {
		ret = sc2145_write_array(sd, sc2145_stream_on);
		pr_debug("sc2145 stream on\n");
	}
	else {
		ret = sc2145_write_array(sd, sc2145_stream_off);
		pr_debug("sc2145 stream off\n");
	}
	return ret;
}

static int sc2145_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sc2145_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int sc2145_set_fps(struct tx_isp_sensor *sensor, int fps)
{
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_notify_argument arg;
	unsigned int pclk;
	unsigned short hts;
	unsigned short vts = 0;
	unsigned char tmp;
	unsigned int newformat = 0;
	unsigned int max_fps = 0;

	int ret = 0;
	switch (sensor_max_fps) {
	case TX_SENSOR_MAX_FPS_25:
		pclk = SC2145_SUPPORT_PCLK_FPS_30;
		max_fps = SENSOR_OUTPUT_MAX_FPS;
		break;
	case TX_SENSOR_MAX_FPS_15:
		pclk = SC2145_SUPPORT_PCLK_FPS_15;
		max_fps = TX_SENSOR_MAX_FPS_15;
		break;
	default:
		printk("Now we do not support this framerate!!!\n");
	}

	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (max_fps << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8)){
		printk("warn: fps(%d) no in range\n", fps);
		return -1;
	}
	ret = sc2145_read(sd, 0x320c, &tmp);
	hts = tmp;
	ret += sc2145_read(sd, 0x320d, &tmp);
	if(ret < 0)
		return -1;
	hts = ((hts << 8) + tmp);

	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);

	ret = sc2145_write(sd, 0x320f, (unsigned char)(vts & 0xff));
	ret += sc2145_write(sd, 0x320e, (unsigned char)(vts >> 8));
	if(ret < 0){
		printk("err: sc2145_write err\n");
		return ret;
	}
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts*2 - 4;
	sensor->video.attr->integration_time_limit = vts*2 - 4;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts*2 - 4;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	return ret;
}

static int sc2145_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;

	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &sc2145_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &sc2145_win_sizes[0];
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

static int sc2145_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"sc2145_reset");
		if(!ret){
			gpio_direction_output(reset_gpio, 1);
			msleep(10);
			gpio_direction_output(reset_gpio, 0);
			msleep(10);
			gpio_direction_output(reset_gpio, 1);
			msleep(10);
		}else{
			printk("gpio requrest fail %d\n",reset_gpio);
		}
	}
	if (pwdn_gpio != -1) {
		ret = gpio_request(pwdn_gpio, "sc2145_pwdn");
		if (!ret) {
			gpio_direction_output(pwdn_gpio, 1);
			msleep(50);
			gpio_direction_output(pwdn_gpio, 0);
			msleep(10);
		} else {
			printk("gpio requrest fail %d\n", pwdn_gpio);
		}
	}
	ret = sc2145_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an sc2145 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "sc2145 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int sc2145_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static long sc2145_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = sc2145_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = sc2145_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = sc2145_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = sc2145_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = sc2145_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		ret = sc2145_write_array(sd, sc2145_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		ret = sc2145_write_array(sd, sc2145_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = sc2145_set_fps(sensor, ctrl->value);
		break;
	default:
		break;;
	}
	return 0;
}

static long sc2145_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = sc2145_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int sc2145_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = sc2145_read(sd, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int sc2145_s_register(struct v4l2_subdev *sd, const struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	sc2145_write(sd, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops sc2145_core_ops = {
	.g_chip_ident = sc2145_g_chip_ident,
	.reset = sc2145_reset,
	.init = sc2145_init,
	.s_power = sc2145_s_power,
	.ioctl = sc2145_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = sc2145_g_register,
	.s_register = sc2145_s_register,
#endif
};

static const struct v4l2_subdev_video_ops sc2145_video_ops = {
	.s_stream = sc2145_s_stream,
	.s_parm = sc2145_s_parm,
	.g_parm = sc2145_g_parm,
};

static const struct v4l2_subdev_ops sc2145_ops = {
	.core = &sc2145_core_ops,
	.video = &sc2145_video_ops,
};

static int sc2145_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &sc2145_win_sizes[0];
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

	sc2145_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
	case DVP_PA_LOW_10BIT:
	case DVP_PA_HIGH_10BIT:
		mbus = sc2145_mbus_code[0];
		break;
	case DVP_PA_12BIT:
		mbus = sc2145_mbus_code[1];
		break;
	default:
		goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(sc2145_win_sizes); i++)
		sc2145_win_sizes[i].mbus_code = mbus;

	/*
	  convert sensor-gain into isp-gain,
	*/
	sc2145_attr.max_again = 262144;
	sc2145_attr.max_dgain = 0; //sc2145_attr.max_dgain;
	sd = &sensor->sd;
	video = &sensor->video;

	switch (sensor_max_fps) {
	case TX_SENSOR_MAX_FPS_25:
		sc2145_attr.max_integration_time_native = 2700-4;
		sc2145_attr.integration_time_limit = 2700-4;
		sc2145_attr.total_width = 2200;
		sc2145_attr.total_height = 1350;
		sc2145_attr.max_integration_time = 2700-4;
		break;
	case TX_SENSOR_MAX_FPS_15:
		sc2145_attr.max_integration_time_native = 2250-4;
		sc2145_attr.integration_time_limit = 2250-4;
		sc2145_attr.total_width = 2200;
		sc2145_attr.total_height = 1125;
		sc2145_attr.max_integration_time = 2250-4;
		break;
	default:
		printk("Now we do not support this framerate!!!\n");
	}

	sensor->video.attr = &sc2145_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &sc2145_ops);
	v4l2_set_subdev_hostdata(sd, sensor);

	pr_debug("probe ok ------->sc2145\n");
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int sc2145_remove(struct i2c_client *client)
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

static const struct i2c_device_id sc2145_id[] = {
	{ "sc2145", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc2145_id);

static struct i2c_driver sc2145_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "sc2145",
	},
	.probe		= sc2145_probe,
	.remove		= sc2145_remove,
	.id_table	= sc2145_id,
};

static __init int init_sc2145(void)
{
	return i2c_add_driver(&sc2145_driver);
}

static __exit void exit_sc2145(void)
{
	i2c_del_driver(&sc2145_driver);
}

module_init(init_sc2145);
module_exit(exit_sc2145);

MODULE_DESCRIPTION("A low-level driver for SmartSens sc2145 sensors");
MODULE_LICENSE("GPL");
