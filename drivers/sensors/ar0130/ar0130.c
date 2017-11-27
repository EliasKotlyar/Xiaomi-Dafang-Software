/*
 * ar0130.c
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

#define AR0130_CHIP_ID_H	(0x24)
#define AR0130_CHIP_ID_L	(0x02)

#define AR0130_REG_END		0xffff
#define AR0130_REG_DELAY	0xfefe

#define AR0130_SUPPORT_PCLK (50*1000*1000)
#define SENSOR_OUTPUT_MAX_FPS 30
#define SENSOR_OUTPUT_MIN_FPS 5


struct regval_list {
	unsigned short reg_num;
	unsigned short value;
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
	unsigned char a_value;
	unsigned char d_value;
	unsigned int gain;
};
u32 tmp_again=0;
u32 tmp_dgain=0;
u32 final_again =0;
u32 final_dgain=0;
u32 tmp_dcg=0;
struct again_lut ar0130_again_lut[] = {
{0x00,0x20, 0},
{0x00,0x21,2909},
{0x00,0x22,5731},
{0x00,0x23,8472},
{0x00,0x24,11136},
{0x00,0x25,13726},
{0x00,0x26,16248},
{0x00,0x27,18704},
{0x00,0x28,21097},
{0x00,0x29,23432},
{0x00,0x2a,25710},
{0x00,0x2b,27935},
{0x00,0x2c,30109},
{0x00,0x2d,32234},
{0x00,0x2e,34312},
{0x00,0x2f,36345},
{0x00,0x30,38336},
{0x00,0x31,40285},
{0x00,0x32,42195},
{0x00,0x33,44068},
{0x00,0x34,45904},
{0x00,0x35,47704},
{0x00,0x36,49472},
{0x00,0x37,51207},
{0x00,0x38,52910},
{0x00,0x39,54584},
{0x00,0x3a,56228},
{0x00,0x3b,57844},
{0x00,0x3c,59433},
{0x00,0x3d,60996},
{0x00,0x3e,62534},
{0x00,0x3f,64047},
{0x10,0x20,65536},
//{0x00,0x41,67001},
{0x10,0x21,68445},
//{0x00,0x43,69867},
{0x10,0x22,71267},
//{0x00,0x45,72648},
{0x10,0x23,74008},
//{0x00,0x47,75349},
{0x10,0x24,76672},
//{0x00,0x49,77976},
{0x10,0x25,79262},
//{0x00,0x4b,80531},
{0x10,0x26,81784},
//{0x00,0x4d,83020},
{0x10,0x27,84240},
//{0x00,0x4f,85444},
{0x10,0x28,86633},
//{0x00,0x51,87808},
{0x10,0x29,88968},
//{0x00,0x53,90114},
{0x10,0x2a,91246},
//{0x00,0x55,92365},
{0x10,0x2b,93471},
//{0x00,0x57,94564},
{0x10,0x2c,95645},
	};

struct again_lut ar0130_again_dcg_lut[] = {
{0x00, 0x21,  96713},
{0x00, 0x22,  97770},
{0x00, 0x23,  98814},
{0x00, 0x24, 101881},
{0x00, 0x25, 107731},
{0x00, 0x26, 109604},
{0x00, 0x27, 114128},
{0x00, 0x28, 117598},
{0x00, 0x29, 119287},
{0x00, 0x2a, 121764},
{0x00, 0x2b, 124178},
{0x00, 0x2c, 125754},
{0x00, 0x2d, 128070},
{0x00, 0x2e, 130330},
{0x00, 0x2f, 131807},
{0x00, 0x30, 134694},
{0x00, 0x31, 136803},
{0x00, 0x32, 138184},
{0x00, 0x33, 140217},
{0x00, 0x34, 142208},
{0x00, 0x35, 143512},
{0x00, 0x36, 145435},
{0x00, 0x37, 146696},
{0x00, 0x38, 149168},
{0x00, 0x39, 150980},
{0x00, 0x3a, 152169},
{0x00, 0x3b, 153926},
{0x00, 0x3c, 155650},
{0x00, 0x3d, 156782},
{0x00, 0x3e, 158456},
{0x00, 0x3f, 160100},

{0x10, 0x20, 161181},
{0x10, 0x21, 164350},
{0x10, 0x22, 167417},
{0x10, 0x23, 169899},
{0x10, 0x24, 172793},
{0x10, 0x25, 175140},
{0x10, 0x26, 177880},
{0x10, 0x27, 180105},
{0x10, 0x28, 182707},
{0x10, 0x29, 184823},
{0x10, 0x2a, 187300},
{0x10, 0x2b, 189316},
{0x10, 0x2c, 191680},
{0x10, 0x2d, 193986},
{0x10, 0x2e, 195866},
{0x10, 0x2f, 197709},
{0x10, 0x30, 199874},
{0x10, 0x31, 201991},
{0x10, 0x32, 203720},
{0x10, 0x33, 205753},
{0x10, 0x34, 207415},
{0x10, 0x35, 209371},
{0x10, 0x36, 210971},
{0x10, 0x37, 212544},
{0x10, 0x38, 214398},
{0x10, 0x39, 216216},
{0x10, 0x3a, 217705},
{0x10, 0x3b, 219171},
{0x10, 0x3c, 220901},
{0x10, 0x3d, 221754},
{0x10, 0x3e, 223992},
{0x10, 0x3f, 225636},

{0x20, 0x20, 227253},
{0x20, 0x21, 229886},
{0x20, 0x22, 232953},
{0x20, 0x23, 235435},
{0x20, 0x24, 237134},
/* {0x20, 0x25, 240676}, */
/* {0x20, 0x26, 243416}, */
/* {0x20, 0x27, 246298}, */
/* {0x20, 0x28, 248243}, */
/* {0x20, 0x29, 250568}, */
/* {0x20, 0x2a, 252836}, */
/* {0x20, 0x2b, 255051}, */
/* {0x20, 0x2c, 257021}, */
/* {0x20, 0x2d, 259332}, */
/* {0x20, 0x2e, 261402}, */
/* {0x20, 0x2f, 263427}, */
/* {0x20, 0x30, 265410}, */
/* {0x20, 0x31, 267353}, */
/* {0x20, 0x32, 269256}, */
/* {0x20, 0x33, 271121}, */
/* {0x20, 0x34, 272951}, */
/* {0x20, 0x35, 274746}, */
/* {0x20, 0x36, 276507}, */
/* {0x20, 0x37, 278236}, */
/* {0x20, 0x38, 280087}, */
/* {0x20, 0x39, 281602}, */
/* {0x20, 0x3a, 283241}, */
/* {0x20, 0x3b, 284853}, */
/* {0x20, 0x3c, 286580}, */
/* {0x20, 0x3d, 287995}, */
/* {0x20, 0x3e, 289666}, */
/* {0x20, 0x3f, 291036}, */

/* {0x30, 0x20, 292655}, */
/* {0x30, 0x21, 295552}, */
/* {0x30, 0x22, 298363}, */
/* {0x30, 0x23, 301093}, */
/* {0x30, 0x24, 303746}, */
/* {0x30, 0x25, 306327}, */
/* {0x30, 0x26, 308840}, */
/* {0x30, 0x27, 311287}, */
/* {0x30, 0x28, 313779}, */
/* {0x30, 0x29, 316104}, */
/* {0x30, 0x2a, 318372}, */
/* {0x30, 0x2b, 320587}, */
/* {0x30, 0x2c, 322752}, */
/* {0x30, 0x2d, 324868}, */
/* {0x30, 0x2e, 326938}, */
	};



/* //{0x00,0x59,96713}, */
/* {0x10,0x2d,97770}, */
/* //{0x00,0x5b,98814}, */
/* {0x10,0x2e,99848}, */
/* //{0x00,0x5d,100870}, */
/* {0x10,0x2f,101881}, */
/* //{0x00,0x5f,102882}, */
/* {0x10,0x30,103872}, */
/* //{0x00,0x61,104851}, */
/* {0x10,0x31,105821}, */
/* //{0x00,0x63,106781}, */
/* {0x10,0x32,107731}, */
/* //{0x00,0x65,108672}, */
/* {0x10,0x33,109604}, */
/* //{0x00,0x67,110526}, */
/* {0x10,0x34,111440}, */
/* //{0x00,0x69,112344}, */
/* {0x10,0x35,113241}, */
/* //{0x00,0x6b,114128}, */
/* {0x10,0x36,115008}, */
/* //{0x00,0x6d,115879}, */
/* {0x10,0x37,116743}, */
/* //{0x00,0x6f,117598}, */
/* {0x10,0x38,118446}, */
/* //{0x00,0x71,119287}, */
/* {0x10,0x39,120120}, */
/* //{0x00,0x73,120946}, */
/* {0x10,0x3a,121764}, */
/* //{0x00,0x75,122576}, */
/* {0x10,0x3b,123380}, */
/* //{0x00,0x77,124178}, */
/* {0x10,0x3c,124969}, */
/* //{0x00,0x79,125754}, */
/* {0x10,0x3d,126532}, */
/* //{0x00,0x7b,127304}, */
/* {0x10,0x3e,128070}, */
/* //{0x00,0x7d,128829}, */
/* {0x10,0x3f,129583}, */
/* //{0x00,0x7f,130330}, */

/* {0x20,0x20,131072}, */
/* {0x20,0x21,133981}, */
/* {0x20,0x22,136803}, */
/* {0x20,0x23,139544}, */
/* {0x20,0x24,142208}, */
/* {0x20,0x25,144798}, */
/* {0x20,0x26,147320}, */
/* {0x20,0x27,149776}, */
/* {0x20,0x28,152169}, */
/* {0x20,0x29,154504}, */
/* {0x20,0x2a,156782}, */
/* {0x20,0x2b,159007}, */
/* {0x20,0x2c,161181}, */
/* {0x20,0x2d,163306}, */
/* {0x20,0x2e,165384}, */
/* {0x20,0x2f,167417}, */
/* {0x20,0x30,169408}, */
/* {0x20,0x31,171357}, */
/* {0x20,0x32,173267}, */
/* {0x20,0x33,175140}, */
/* {0x20,0x34,176976}, */
/* {0x20,0x35,178777}, */
/* {0x20,0x36,180544}, */
/* {0x20,0x37,182279}, */
/* {0x20,0x38,183982}, */
/* {0x20,0x39,185656}, */
/* {0x20,0x3a,187300}, */
/* {0x20,0x3b,188916}, */
/* {0x20,0x3c,190505}, */
/* {0x20,0x3d,192068}, */
/* {0x20,0x3e,193606}, */
/* {0x20,0x3f,195119}, */

/* {0x30,0x20,196608}, */
/* {0x30,0x21,199517}, */
/* {0x30,0x22,202339}, */
/* {0x30,0x23,205080}, */
/* {0x30,0x24,207744}, */
/* {0x30,0x25,210334}, */
/* {0x30,0x26,212856}, */
/* {0x30,0x27,215312}, */
/* {0x30,0x28,217705}, */
/* {0x30,0x29,220040}, */
/* {0x30,0x2a,222318}, */
/* {0x30,0x2b,224543}, */
/* {0x30,0x2c,226717}, */
/* {0x30,0x2d,228842}, */
/* {0x30,0x2e,230920}, */
/* {0x30,0x2f,232953}, */
/* {0x30,0x30,234944}, */
/* {0x30,0x31,236893}, */
/* {0x30,0x32,238803}, */
/* {0x30,0x33,240676}, */
/* {0x30,0x34,242512}, */
/* {0x30,0x35,244313}, */
/* {0x30,0x36,246080}, */
/* {0x30,0x37,247815}, */
/* {0x30,0x38,249518}, */
/* {0x30,0x39,251192}, */
/* {0x30,0x3a,252836}, */
/* {0x30,0x3b,254452}, */
/* {0x30,0x3c,256041}, */
/* {0x30,0x3d,257604}, */
/* {0x30,0x3e,259142}, */
/* {0x30,0x3f,260655}, */
/* {0x30,0x40,262144}, */
/* }; */

/* struct again_lut ar0130_again_lut[] = { */
/* 	{0x1300,0}, */
/* 	{0x1301,65536}, */
/* 	{0x1302,131072}, */
/* 	{0x1303,196608}, */
/* }; */

struct tx_isp_sensor_attribute ar0130_attr;
//#define AR0130IR_MAX_DIGITAL_GAIN     ((1<<8)-1)   // x15.992
//#define AR0130IR_MAX_ANALOGL_GAIN     ((3<<LOG2_GAIN_SHIFT)+(8<<(LOG2_GAIN_SHIFT-4)))	// Limited to x12.0 (x8*x1.5) as recommended in data sheet 192 81
/*
 * the part of driver maybe modify about different sensor and different board.
 */


/*
 * the configure of gpio should be in accord with product-board.
 * if one is redundant, please config -1.
 */
/* struct tx_isp_sensor_board_info ar0130_board = { */
/* #ifdef CONFIG_BOARD_BOX */
/* 	.reset_gpio = GPIO_PF(30), */
/* 	.pwdn_gpio = GPIO_PF(31), */
/* #elif defined(CONFIG_BOARD_BOOTES) */
/* 	.reset_gpio = GPIO_PF(28), */
/* 	.pwdn_gpio = GPIO_PF(29), */
/* #else */
/* 	.reset_gpio = GPIO_PF(0), */
/* 	.pwdn_gpio = GPIO_PF(1), */
/* #endif */
/* 	.mclk_name = "cgu_cim", */
/* }; */
static struct regval_list ar0130_init_regs_1280_720[] = {

	{0x301A, 0x0001}, 	// RESET_REGISTER
	{AR0130_REG_DELAY, 100}, //ms
	{0x301A, 0x10D8}, 	// RESET_REGISTER

//Linear Mode Setup
//AR0130 Rev1 Linear sequencer load 8-2-2011
	{0x3088, 0x8000}, 		// SEQ_CTRL_PORT
	{0x3086, 0x0225}, 		// SEQ_DATA_PORT
	{0x3086, 0x5050}, 		// SEQ_DATA_PORT
	{0x3086, 0x2D26}, 		// SEQ_DATA_PORT
	{0x3086, 0x0828}, 		// SEQ_DATA_PORT
	{0x3086, 0x0D17}, 		// SEQ_DATA_PORT
	{0x3086, 0x0926}, 		// SEQ_DATA_PORT
	{0x3086, 0x0028}, 		// SEQ_DATA_PORT
	{0x3086, 0x0526}, 		// SEQ_DATA_PORT
	{0x3086, 0xA728}, 		// SEQ_DATA_PORT
	{0x3086, 0x0725}, 		// SEQ_DATA_PORT
	{0x3086, 0x8080}, 		// SEQ_DATA_PORT
	{0x3086, 0x2917}, 		// SEQ_DATA_PORT
	{0x3086, 0x0525}, 		// SEQ_DATA_PORT
	{0x3086, 0x0040}, 		// SEQ_DATA_PORT
	{0x3086, 0x2702}, 		// SEQ_DATA_PORT
	{0x3086, 0x1616}, 		// SEQ_DATA_PORT
	{0x3086, 0x2706}, 		// SEQ_DATA_PORT
	{0x3086, 0x1736}, 		// SEQ_DATA_PORT
	{0x3086, 0x26A6}, 		// SEQ_DATA_PORT
	{0x3086, 0x1703}, 		// SEQ_DATA_PORT
	{0x3086, 0x26A4}, 		// SEQ_DATA_PORT
	{0x3086, 0x171F}, 		// SEQ_DATA_PORT
	{0x3086, 0x2805}, 		// SEQ_DATA_PORT
	{0x3086, 0x2620}, 		// SEQ_DATA_PORT
	{0x3086, 0x2804}, 		// SEQ_DATA_PORT
	{0x3086, 0x2520}, 		// SEQ_DATA_PORT
	{0x3086, 0x2027}, 		// SEQ_DATA_PORT
	{0x3086, 0x0017}, 		// SEQ_DATA_PORT
	{0x3086, 0x1E25}, 		// SEQ_DATA_PORT
	{0x3086, 0x0020}, 		// SEQ_DATA_PORT
	{0x3086, 0x2117}, 		// SEQ_DATA_PORT
	{0x3086, 0x1028}, 		// SEQ_DATA_PORT
	{0x3086, 0x051B}, 		// SEQ_DATA_PORT
	{0x3086, 0x1703}, 		// SEQ_DATA_PORT
	{0x3086, 0x2706}, 		// SEQ_DATA_PORT
	{0x3086, 0x1703}, 		// SEQ_DATA_PORT
	{0x3086, 0x1747}, 		// SEQ_DATA_PORT
	{0x3086, 0x2660}, 		// SEQ_DATA_PORT
	{0x3086, 0x17AE}, 		// SEQ_DATA_PORT
	{0x3086, 0x2500}, 		// SEQ_DATA_PORT
	{0x3086, 0x9027}, 		// SEQ_DATA_PORT
	{0x3086, 0x0026}, 		// SEQ_DATA_PORT
	{0x3086, 0x1828}, 		// SEQ_DATA_PORT
	{0x3086, 0x002E}, 		// SEQ_DATA_PORT
	{0x3086, 0x2A28}, 		// SEQ_DATA_PORT
	{0x3086, 0x081E}, 		// SEQ_DATA_PORT
	{0x3086, 0x0831}, 		// SEQ_DATA_PORT
	{0x3086, 0x1440}, 		// SEQ_DATA_PORT
	{0x3086, 0x4014}, 		// SEQ_DATA_PORT
	{0x3086, 0x2020}, 		// SEQ_DATA_PORT
	{0x3086, 0x1410}, 		// SEQ_DATA_PORT
	{0x3086, 0x1034}, 		// SEQ_DATA_PORT
	{0x3086, 0x1400}, 		// SEQ_DATA_PORT
	{0x3086, 0x1014}, 		// SEQ_DATA_PORT
	{0x3086, 0x0020}, 		// SEQ_DATA_PORT
	{0x3086, 0x1400}, 		// SEQ_DATA_PORT
	{0x3086, 0x4013}, 		// SEQ_DATA_PORT
	{0x3086, 0x1802}, 		// SEQ_DATA_PORT
	{0x3086, 0x1470}, 		// SEQ_DATA_PORT
	{0x3086, 0x7004}, 		// SEQ_DATA_PORT
	{0x3086, 0x1470}, 		// SEQ_DATA_PORT
	{0x3086, 0x7003}, 		// SEQ_DATA_PORT
	{0x3086, 0x1470}, 		// SEQ_DATA_PORT
	{0x3086, 0x7017}, 		// SEQ_DATA_PORT
	{0x3086, 0x2002}, 		// SEQ_DATA_PORT
	{0x3086, 0x1400}, 		// SEQ_DATA_PORT
	{0x3086, 0x2002}, 		// SEQ_DATA_PORT
	{0x3086, 0x1400}, 		// SEQ_DATA_PORT
	{0x3086, 0x5004}, 		// SEQ_DATA_PORT
	{0x3086, 0x1400}, 		// SEQ_DATA_PORT
	{0x3086, 0x2004}, 		// SEQ_DATA_PORT
	{0x3086, 0x1400}, 		// SEQ_DATA_PORT
	{0x3086, 0x5022}, 		// SEQ_DATA_PORT
	{0x3086, 0x0314}, 		// SEQ_DATA_PORT
	{0x3086, 0x0020}, 		// SEQ_DATA_PORT
	{0x3086, 0x0314}, 		// SEQ_DATA_PORT
	{0x3086, 0x0050}, 		// SEQ_DATA_PORT
	{0x3086, 0x2C2C}, 		// SEQ_DATA_PORT
	{0x3086, 0x2C2C}, 		// SEQ_DATA_PORT
	{0x309E, 0x0000}, 		// DCDS_PROG_START_ADDR
	{0x30E4, 0x6372}, 		// ADC_BITS_6_7
	{0x30E2, 0x7253}, 		// ADC_BITS_4_5
	{0x30E0, 0x5470}, 		// ADC_BITS_2_3
	{0x30E6, 0xC4CC}, 		// ADC_CONFIG1
	{0x30E8, 0x8050}, 		// ADC_CONFIG2
	{AR0130_REG_DELAY, 200}, //ms

	{0x30EA, 0x8c00},
	{0x30Ba, 0x000b},

	{0x3082, 0x0029}, 	// OPERATION_MODE_CTRL
				//AR0130 Rev1 Optimized settings 8-2-2011
	{0x301E, 0x00C8},	// DATA_PEDESTAL
	{0x3EDA, 0x0F03},	// DAC_LD_14_15
	{0x3EDE, 0xC005},	// DAC_LD_18_19
	{0x3ED8, 0x09EF},	// DAC_LD_12_13
	{0x3EE2, 0xA46B},	// DAC_LD_22_23
	{0x3EE0, 0x047D},	// DAC_LD_20_21
	{0x3EDC, 0x0070},	// DAC_LD_16_17
	{0x3044, 0x0404},	// DARK_CONTROL
	{0x3EE6, 0x4303},	// DAC_LD_26_27
	{0x3EE4, 0xD208},	// DAC_LD_24_25
	{0x3ED6, 0x00BD},	// DAC_LD_10_11
	{0x30B0, 0x1300},	// DIGITAL_TEST
	{0x30D4, 0xE007},	// COLUMN_CORRECTION
	{0x301A, 0x10DC},	// RESET_REGISTER
	{AR0130_REG_DELAY,500}, // delay 500ms
	{0x301A, 0x10D8}, 	// RESET_REGISTER
	{AR0130_REG_DELAY,500}, // delay 500ms
	{0x3044, 0x0400},	// DARK_CONTROL
	/* {0x3044, 0x000},	// DARK_CONTROL */

	{0x3012, 0x0160},	// COARSE_INTEGRATION_TIME
	//720p 25fps Setup
	{0x3032, 0x0000}, 	// DIGITAL_BINNING
	{0x3002, 0x0002}, 	//Y_ADDR_START = 2
	{0x3004, 0x0000}, 	//X_ADDR_START = 0
	/* {0x3006, 0x02D1}, 	//Y_ADDR_END = 721 */
	{0x3006, 0x03C1},
	{0x3008, 0x04FF}, 	//X_ADDR_END = 1279
	{0x300A, 0x04BC}, 	//FRAME_LENGTH_LINES = 1212
	{0x300C, 0x0672}, 	//LINE_LENGTH_PCK = 1650
	//Enable Parallel Mode
	{0x301A, 0x10D8}, 	// RESET_REGISTER
	{0x31D0, 0x0001}, 	// HDR_COMP
	{0x3064, 0x1802},       // Diable embeded data
	//PLL Enabled 24Mhz to 50Mhz
	{0x306e, 0x9211},
	{0x302C, 0x0001}, 		// VT_SYS_CLK_DIV //
	{0x302A, 0x000C}, 		// VT_PIX_CLK_DIV
	{0x302E, 0x0002}, //0x0003	// PRE_PLL_CLK_DIV
	{0x3030, 0x0032}, 		// PLL_MULTIPLIER//
	{0x30B0, 0x1300}, 		// DIGITAL_TEST
	{AR0130_REG_DELAY,100}, //ms
	/* {0x3070, 0x2}, */          //color bar
	{AR0130_REG_END, 0x0000},	/* END MARKER */
};
/* static struct regval_list ar0130_init_regs_25_fps[] = { */
/* 	{0x300A, 0x04BC},	//FRAME_LENGTH_LINES = 1010 */
/* 	{AR0130_REG_END, 0x0000},	/\* END MARKER *\/ */
/* }; */

/* static struct regval_list ar0130_init_regs_30_fps[] = { */
/* 	{0x300A, 0x03f2},	//FRAME_LENGTH_LINES = 1010 */
/* 	{AR0130_REG_END, 0x0000},	/\* END MARKER *\/ */
/* }; */
/*
 * the order of the ar0130_win_sizes is [full_resolution, preview_resolution].
 */
static struct tx_isp_sensor_win_setting ar0130_win_sizes[] = {
	/* 1280*800 */
	{
		.width		= 1280,
		.height		= 960,
		.fps		= 25<<16|1,
		.mbus_code	= V4L2_MBUS_FMT_SGRBG12_1X12,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs 		= ar0130_init_regs_1280_720,
		/* .fast		= ar0130_init_regs_25_fps, */
		/* .slow		= ar0130_init_regs_30_fps, */
	}
};

static enum v4l2_mbus_pixelcode ar0130_mbus_code[] = {
	V4L2_MBUS_FMT_SGRBG10_1X10,
	V4L2_MBUS_FMT_SGRBG12_1X12,
};
/*
 * the part of driver was fixed.
 */

static struct regval_list ar0130_stream_on[] = {
	{0x301A, 0x10DC},

	{AR0130_REG_END, 0x0000},	/* END MARKER */
};

static struct regval_list ar0130_stream_off[] = {
	/* Sensor enter LP11*/
	{0x301A, 0x10D8},

	{AR0130_REG_END, 0x0000},	/* END MARKER */
};

int ar0130_read(struct v4l2_subdev *sd, unsigned short reg,
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


static int ar0130_write(struct v4l2_subdev *sd, unsigned short reg,
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

static int ar0130_read_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	unsigned char val[2];
	unsigned short value = 0;
	while (vals->reg_num != AR0130_REG_END) {
		if (vals->reg_num == AR0130_REG_DELAY) {
				msleep(vals->value);
		} else {
			ret = ar0130_read(sd, vals->reg_num, val);
			if (ret < 0)
				return ret;
		}
		value = (val[0]<<8)|(val[1]);
		/*printk("vals->reg_num:%x, vvals->value:0x%x\n",vals->reg_num, value);*/
		vals++;
	}
	return 0;
}
static int ar0130_write_array(struct v4l2_subdev *sd, struct regval_list *vals)
{
	int ret;
	while (vals->reg_num != AR0130_REG_END) {
		if (vals->reg_num == AR0130_REG_DELAY) {
				msleep(vals->value);
		} else {
			/*printk("vals->reg_num=0x%x,vals->value =0x%x\n",vals->reg_num,vals->value);*/
			ret = ar0130_write(sd, vals->reg_num, vals->value);
			if (ret < 0)
				return ret;
		}
		vals++;
	}
	return 0;
}

unsigned int ar0130_alloc_again(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_again)
{
	struct again_lut *lut = ar0130_again_lut;
	struct again_lut *dcg_lut = ar0130_again_dcg_lut;
	while((lut->gain <= ar0130_attr.max_again)||(dcg_lut->gain <= ar0130_attr.max_again)) {
		if(isp_gain < 95645){
		if(isp_gain == 0) {
			tmp_again = 0x1300;
			tmp_dgain = 0x20;
			tmp_dcg =0x0;
			*sensor_again = 0;
			return 0;
		}
		else if(isp_gain < lut->gain) {
			tmp_again = 0x1300|((lut - 1)->a_value);
			tmp_dgain = (lut - 1)->d_value;
			tmp_dcg =0x0;
			*sensor_again = (lut - 1)->d_value;
			/* printk(" 11isp value ==%d,again sensor value ==0x%x, dgain value ===0x%x tmp_dcg ===%x \n",isp_gain,tmp_again,tmp_dgain,tmp_dcg); */
			return (lut - 1)->gain;
		}
		else{
			if((lut->gain == ar0130_attr.max_again) && (isp_gain >= lut->gain)) {
				tmp_again = 0x1300|((lut)->a_value);
				tmp_dgain = (lut)->d_value;
				tmp_dcg =0x0;
				*sensor_again = (lut)->d_value;
				return lut->gain;
			}
		}

		lut++;

		}else{

		if(isp_gain <96713) {
			tmp_again = 0x1300;
			tmp_dgain = 0x21;
			tmp_dcg =0x4;
			*sensor_again = 0;
			return 0;
		}
		else if(isp_gain < dcg_lut->gain) {
			tmp_again = 0x1300|((dcg_lut - 1)->a_value);
			tmp_dgain = (dcg_lut - 1)->d_value;
			tmp_dcg =0x4;
				*sensor_again = (dcg_lut - 1)->d_value;
			return (dcg_lut - 1)->gain;
		}
		else{
			if((dcg_lut->gain == ar0130_attr.max_again) && (isp_gain >= dcg_lut->gain)) {
				tmp_again = 0x1300|((dcg_lut)->a_value);
				tmp_dgain = (dcg_lut)->d_value;
				tmp_dcg =0x4;
				*sensor_again = (dcg_lut)->d_value;
				return dcg_lut->gain;
			}
		}

		dcg_lut++;


		}
	}
	return isp_gain;
}

unsigned int ar0130_alloc_dgain(unsigned int isp_gain, unsigned char shift, unsigned int *sensor_dgain)
{
	return isp_gain;
}

struct tx_isp_sensor_attribute ar0130_attr={
	.name = "ar0130",
	.chip_id = 0x0051,
	.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C,
	.cbus_mask = V4L2_SBUS_MASK_SAMPLE_8BITS | V4L2_SBUS_MASK_ADDR_8BITS,
	.cbus_device = 0x10,
	.dbus_type = TX_SENSOR_DATA_INTERFACE_DVP,
	.dvp = {
		.mode = SENSOR_DVP_HREF_MODE,
		.blanking = {
			.vblanking = 0,
			.hblanking = 0,
		},
	},
	.max_again = 237134,
	.max_dgain = 0,
	.min_integration_time = 1,
	.min_integration_time_native = 1,
	.max_integration_time_native = 1212,
	.integration_time_limit = 1212,
	.total_width = 1650,
	.total_height = 1212,
	.max_integration_time = 1212,
	.integration_time_apply_delay = 2,
	.again_apply_delay = 2,
	.dgain_apply_delay = 2,
	.sensor_ctrl.alloc_again = ar0130_alloc_again,
	.sensor_ctrl.alloc_dgain = ar0130_alloc_dgain,
//	void priv; /* point to struct tx_isp_sensor_board_info */
};



static int ar0130_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int ar0130_detect(struct v4l2_subdev *sd, unsigned int *ident)
{
	unsigned char v[2];
	int ret;
	ret = ar0130_read(sd, 0x3000, v);
	if (ret < 0)
		return ret;
	if (v[0] != AR0130_CHIP_ID_H)
		return -ENODEV;
	if (v[1] != AR0130_CHIP_ID_L)
		return -ENODEV;
	return 0;}

static int ar0130_set_integration_time(struct v4l2_subdev *sd, int value)
{
	ar0130_write(sd,0x3012, value);
	/*[15:0] = 0x16,0x10 */
	return 0;
}
static int ar0130_set_analog_gain(struct v4l2_subdev *sd, int value)
{
	/* ar0130_write(sd,0x30B0, 0x1330); */
	/* ar0130_write(sd, 0x305e, 0x40); */
	/* ar0130_write(sd, 0x3100,0x00 ); */
	/*printk("aaa tmp_again=========0x%x final_again ===0x%x, tmp_dgain ========0x%x,final_dgain ===0x%x, tmp_dcg =0x%x\n",tmp_again,final_again,tmp_dgain,final_dgain,tmp_dcg); */
	if((final_again!=tmp_again)||(final_dgain!=tmp_dgain))
	{
		ar0130_write(sd,0x30B0, tmp_again);
		ar0130_write(sd, 0x305e, tmp_dgain);
		ar0130_write(sd, 0x3100, tmp_dcg);
		final_again = tmp_again;
		final_dgain = tmp_dgain;
	}
	return 0;
}
static int ar0130_set_digital_gain(struct v4l2_subdev *sd, int value)
{
	/* 0x00 bit[7] if gain > 2X set 0; if gain > 4X set 1 */
	return 0;
}

static int ar0130_get_black_pedestal(struct v4l2_subdev *sd, int value)
{
#if 0
	int ret = 0;
	int black = 0;
	unsigned char h,l;
	unsigned char reg = 0xff;
	unsigned int * v = (unsigned int *)(value);
	ret = ar0130_read(sd, 0x48, &h);
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
	ret = ar0130_read(sd, reg, &l);
	if (ret < 0)
		return ret;
	*v = (black | l);
#endif
	return 0;
}

static int ar0130_init(struct v4l2_subdev *sd, u32 enable)
{
	struct tx_isp_sensor *sensor = (container_of(sd, struct tx_isp_sensor, sd));
	struct tx_isp_notify_argument arg;
	struct tx_isp_sensor_win_setting *wsize = &ar0130_win_sizes[0];
	int ret = 0;
	if(!enable)
		return ISP_SUCCESS;
	sensor->video.mbus.width = wsize->width;
	sensor->video.mbus.height = wsize->height;
	sensor->video.mbus.code = wsize->mbus_code;
	sensor->video.mbus.field = V4L2_FIELD_NONE;
	sensor->video.mbus.colorspace = wsize->colorspace;
	sensor->video.fps = wsize->fps;
	ret = ar0130_write_array(sd, wsize->regs);
	if (ret)
		return ret;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
	sensor->priv = wsize;
	return 0;
}

static int ar0130_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;
	if (enable) {
		ret = ar0130_write_array(sd, ar0130_stream_on);
		udelay(100000);
		ar0130_write(sd,0x30Ba, 0x0);
		printk("ar0130 stream on\n");
	}
	else {
		ret = ar0130_write_array(sd, ar0130_stream_off);
		printk("ar0130 stream off\n");
	}
	return ret;
}

static int ar0130_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ar0130_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

static int ar0130_set_fps(struct tx_isp_sensor *sensor, int fps)
{
#if 1
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	unsigned int pclk = AR0130_SUPPORT_PCLK;
	unsigned short hts=0;
	unsigned short vts = 0;
	unsigned char tmp[2];
	unsigned int newformat = 0; //the format is 24.8
	int ret = 0;
	/* the format of fps is 16/16. for example 25 << 16 | 2, the value is 25/2 fps. */
	newformat = (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
	if(newformat > (SENSOR_OUTPUT_MAX_FPS << 8) || newformat < (SENSOR_OUTPUT_MIN_FPS << 8))
		return -1;
	ret += ar0130_read(sd, 0x300C, tmp);
	hts =tmp[0];
	/* ret += ar0130_read(sd, 0x320d, &tmp); */
	if(ret < 0)
		return -1;
	hts = (hts << 8) + tmp[1];
	/*vts = (pclk << 4) / (hts * (newformat >> 4));*/
	vts = pclk * (fps & 0xffff) / hts / ((fps & 0xffff0000) >> 16);
	ret = ar0130_write(sd, 0x300A, vts);
	if(ret < 0)
		return -1;
	sensor->video.fps = fps;
	sensor->video.attr->max_integration_time_native = vts - 4;
	sensor->video.attr->integration_time_limit = vts - 4;
	sensor->video.attr->total_height = vts;
	sensor->video.attr->max_integration_time = vts - 4;
	arg.value = (int)&sensor->video;
	sd->v4l2_dev->notify(sd, TX_ISP_NOTIFY_SYNC_VIDEO_IN, &arg);
#endif
	return 0;
}

static int ar0130_set_mode(struct tx_isp_sensor *sensor, int value)
{
	struct tx_isp_notify_argument arg;
	struct v4l2_subdev *sd = &sensor->sd;
	struct tx_isp_sensor_win_setting *wsize = NULL;
	int ret = ISP_SUCCESS;
	if(value == TX_ISP_SENSOR_FULL_RES_MAX_FPS){
		wsize = &ar0130_win_sizes[0];
	}else if(value == TX_ISP_SENSOR_PREVIEW_RES_MAX_FPS){
		wsize = &ar0130_win_sizes[0];
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
			ret = ar0130_write_array(sd, wsize->regs);
			if(!ret){
				sensor->priv = wsize;
			}
		}
	}
	return ret;
}
static int ar0130_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned int ident = 0;
	int ret = ISP_SUCCESS;

	if(reset_gpio != -1){
		ret = gpio_request(reset_gpio,"ar0130_reset");
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
		ret = gpio_request(pwdn_gpio,"ar0130_pwdn");
		if(!ret){
			/* gpio_direction_output(ar0130_board.pwdn_gpio, 1); */
			/* msleep(150); */
			gpio_direction_output(pwdn_gpio, 0);
		}else{
			printk("gpio requrest fail %d\n",pwdn_gpio);
		}
	}
	ret = ar0130_detect(sd, &ident);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an ar0130 chip.\n",
			client->addr, client->adapter->name);
		return ret;
	}
	v4l_info(client, "ar0130 chip found @ 0x%02x (%s)\n",
		 client->addr, client->adapter->name);
	return v4l2_chip_ident_i2c_client(client, chip, ident, 0);
}

static int ar0130_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}
static long ar0130_ops_private_ioctl(struct tx_isp_sensor *sensor, struct isp_private_ioctl *ctrl)
{
	struct v4l2_subdev *sd = &sensor->sd;
	long ret = 0;
	switch(ctrl->cmd){
	case TX_ISP_PRIVATE_IOCTL_SENSOR_INT_TIME:
		ret = ar0130_set_integration_time(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_AGAIN:
		ret = ar0130_set_analog_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_DGAIN:
		ret = ar0130_set_digital_gain(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_BLACK_LEVEL:
		ret = ar0130_get_black_pedestal(sd, ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_RESIZE:
		ret = ar0130_set_mode(sensor,ctrl->value);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_PREPARE_CHANGE:
		ret = ar0130_write_array(sd, ar0130_stream_off);
		break;
	case TX_ISP_PRIVATE_IOCTL_SUBDEV_FINISH_CHANGE:
		ret = ar0130_write_array(sd, ar0130_stream_on);
		break;
	case TX_ISP_PRIVATE_IOCTL_SENSOR_FPS:
		ret = ar0130_set_fps(sensor, ctrl->value);
		break;
	default:
		break;;
	}
	return 0;
}
static long ar0130_ops_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct tx_isp_sensor *sensor =container_of(sd, struct tx_isp_sensor, sd);
	int ret;
	switch(cmd){
	case VIDIOC_ISP_PRIVATE_IOCTL:
		ret = ar0130_ops_private_ioctl(sensor, arg);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ar0130_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val[2];
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ar0130_read(sd, reg->reg & 0xffff, val);
	reg->val = val[0];
	reg->val = (reg->val<<8)+val[1];
	reg->size = 2;
	return ret;
}

static int ar0130_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ar0130_write(sd, reg->reg & 0xffff, reg->val & 0xffff);
	return 0;
}
#endif

static const struct v4l2_subdev_core_ops ar0130_core_ops = {
	.g_chip_ident = ar0130_g_chip_ident,
	.reset = ar0130_reset,
	.init = ar0130_init,
	.s_power = ar0130_s_power,
	.ioctl = ar0130_ops_ioctl,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ar0130_g_register,
	.s_register = ar0130_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ar0130_video_ops = {
	.s_stream = ar0130_s_stream,
	.s_parm = ar0130_s_parm,
	.g_parm = ar0130_g_parm,
};

static const struct v4l2_subdev_ops ar0130_ops = {
	.core = &ar0130_core_ops,
	.video = &ar0130_video_ops,
};

static int ar0130_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct tx_isp_video_in *video;
	struct tx_isp_sensor *sensor;
	struct tx_isp_sensor_win_setting *wsize = &ar0130_win_sizes[0];
	enum v4l2_mbus_pixelcode mbus;
	int i = 0;
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

	ar0130_attr.dvp.gpio = sensor_gpio_func;

	switch(sensor_gpio_func){
		case DVP_PA_LOW_10BIT:
		case DVP_PA_HIGH_10BIT:
			mbus = ar0130_mbus_code[0];
			break;
		case DVP_PA_12BIT:
			mbus = ar0130_mbus_code[1];
			break;
		default:
			goto err_set_sensor_gpio;
	}

	for(i = 0; i < ARRAY_SIZE(ar0130_win_sizes); i++)
		ar0130_win_sizes[i].mbus_code = mbus;

	sd = &sensor->sd;
	video = &sensor->video;
	sensor->video.attr = &ar0130_attr;
	sensor->video.vi_max_width = wsize->width;
	sensor->video.vi_max_height = wsize->height;
	v4l2_i2c_subdev_init(sd, client, &ar0130_ops);
	v4l2_set_subdev_hostdata(sd, sensor);
	return 0;
err_set_sensor_gpio:
	clk_disable(sensor->mclk);
	clk_put(sensor->mclk);
err_get_mclk:
	kfree(sensor);

	return -1;
}

static int ar0130_remove(struct i2c_client *client)
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

static const struct i2c_device_id ar0130_id[] = {
	{ "ar0130", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0130_id);

static struct i2c_driver ar0130_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ar0130",
	},
	.probe		= ar0130_probe,
	.remove		= ar0130_remove,
	.id_table	= ar0130_id,
};

static __init int init_ar0130(void)
{
	return i2c_add_driver(&ar0130_driver);
}

static __exit void exit_ar0130(void)
{
	i2c_del_driver(&ar0130_driver);
}

module_init(init_ar0130);
module_exit(exit_ar0130);

MODULE_DESCRIPTION("A low-level driver for OmniVision ar0130 sensors");
MODULE_LICENSE("GPL");
