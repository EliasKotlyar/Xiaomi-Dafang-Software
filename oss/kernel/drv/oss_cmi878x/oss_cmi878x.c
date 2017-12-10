/*
 * Purpose: Driver for C-Media CMI8788 PCI audio controller.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

#include "oss_cmi878x_cfg.h"
#include <oss_pci.h>
#include <uart401.h>
#include <ac97.h>

#define CMEDIA_VENDOR_ID	0x13F6
#define CMEDIA_CMI8788		0x8788

/*
 * CM8338 registers definition
 */

#define RECA_ADDR		(devc->base+0x00)
#define RECA_SIZE		(devc->base+0x04)
#define RECA_FRAG		(devc->base+0x06)
#define RECB_ADDR		(devc->base+0x08)
#define RECB_SIZE		(devc->base+0x0C)
#define RECB_FRAG		(devc->base+0x0E)
#define RECC_ADDR		(devc->base+0x10)
#define RECC_SIZE		(devc->base+0x14)
#define RECC_FRAG		(devc->base+0x16)
#define SPDIF_ADDR		(devc->base+0x18)
#define SPDIF_SIZE		(devc->base+0x1C)
#define SPDIF_FRAG		(devc->base+0x1E)
#define MULTICH_ADDR		(devc->base+0x20)
#define MULTICH_SIZE		(devc->base+0x24)
#define MULTICH_FRAG		(devc->base+0x28)
#define FPOUT_ADDR		(devc->base+0x30)
#define FPOUT_SIZE		(devc->base+0x34)
#define FPOUT_FRAG		(devc->base+0x36)

#define DMA_START		(devc->base+0x40)
#define CHAN_RESET		(devc->base+0x42)
#define MULTICH_MODE		(devc->base+0x43)
#define IRQ_MASK		(devc->base+0x44)
#define IRQ_STAT		(devc->base+0x46)
#define MISC_REG		(devc->base+0x48)
#define REC_FORMAT		(devc->base+0x4A)
#define PLAY_FORMAT		(devc->base+0x4B)
#define REC_MODE		(devc->base+0x4C)
#define FUNCTION		(devc->base+0x50)

#define I2S_MULTICH_FORMAT	(devc->base+0x60)
#define I2S_ADC1_FORMAT		(devc->base+0x62)
#define I2S_ADC2_FORMAT		(devc->base+0x64)
#define I2S_ADC3_FORMAT		(devc->base+0x66)

#define SPDIF_FUNC		(devc->base+0x70)
#define SPDIFOUT_CHAN_STAT	(devc->base+0x74)
#define SPDIFIN_CHAN_STAT	(devc->base+0x78)

#define I2C_ADDR		(devc->base+0x90)
#define I2C_MAP		(devc->base+0x91)
#define I2C_DATA		(devc->base+0x92)
#define I2C_CTRL		(devc->base+0x94)

#define SPI_CONTROL		(devc->base+0x98)
#define SPI_DATA		(devc->base+0x99)

#define MPU401_DATA		(devc->base+0xA0)
#define MPU401_COMMAND		(devc->base+0xA1)
#define MPU401_CONTROL		(devc->base+0xA2)

#define GPI_DATA		(devc->base+0xA4)
#define GPI_IRQ_MASK		(devc->base+0xA5)
#define GPIO_DATA		(devc->base+0xA6)
#define GPIO_CONTROL		(devc->base+0xA8)
#define GPIO_IRQ_MASK		(devc->base+0xAA)
#define DEVICE_SENSE		(devc->base+0xAC)

#define PLAY_ROUTING		(devc->base+0xC0)
#define REC_ROUTING		(devc->base+0xC2)
#define REC_MONITOR		(devc->base+0xC3)
#define MONITOR_ROUTING		(devc->base+0xC4)

#define AC97_CTRL		(devc->base+0xD0)
#define AC97_INTR_MASK		(devc->base+0xD2)
#define AC97_INTR_STAT		(devc->base+0xD3)
#define AC97_OUT_CHAN_CONFIG	(devc->base+0xD4)
#define AC97_IN_CHAN_CONFIG	(devc->base+0xD8)
#define AC97_CMD_DATA		(devc->base+0xDC)

#define CODEC_VERSION		(devc->base+0xE4)
#define CTRL_VERSION		(devc->base+0xE6)

/* Device IDs */
#define ASUS_VENDOR_ID		0x1043
#define SUBID_XONAR_D2		0x8269
#define SUBID_XONAR_D2X		0x82b7
#define SUBID_XONAR_D1		0x834f
#define SUBID_XONAR_DX		0x8275
#define SUBID_XONAR_STX 	0x835c
#define SUBID_XONAR_DS		0x838e
#define SUBID_XONAR_ST		0x835d

#define SUBID_GENERIC		0x0000

/* Xonar specific */
#define XONAR_DX_FRONTDAC	0x9e
#define XONAR_DX_SURRDAC	0x30
#define XONAR_STX_FRONTDAC	0x98
#define XONAR_ST_FRONTDAC	0x98
#define XONAR_ST_CLOCK		0x9c
#define XONAR_DS_FRONTDAC	0x1
#define XONAR_DS_SURRDAC	0x0
#define XONAR_MCLOCK_128	0x00
#define XONAR_MCLOCK_256	0x10
#define XONAR_MCLOCK_512	0x20

/* defs for AKM 4396 DAC */
#define AK4396_CTL1        0x00
#define AK4396_CTL2        0x01
#define AK4396_CTL3        0x02
#define AK4396_LchATTCtl   0x03
#define AK4396_RchATTCtl   0x04

/* defs for CS4398 DAC */
#define CS4398_CHIP_ID	  0x01
#define CS4398_MODE_CTRL  0x02
#define CS4398_MIXING	  0x03
#define CS4398_MUTE_CTRL  0x04
#define CS4398_VOLA       0x05
#define CS4398_VOLB       0x06
#define CS4398_RAMP_CTRL  0x07
#define CS4398_MISC_CTRL  0x08
#define CS4398_MISC2_CTRL 0x09

#define CS4398_POWER_DOWN (1<<7)	/* Obvious */
#define CS4398_CPEN	  (1<<6)	/* Control Port Enable */
#define CS4398_FREEZE	  (1<<5)	/* Freezes registers, unfreeze to 
					 * accept changed registers */
#define CS4398_MCLKDIV2   (1<<4)	/* Divide MCLK by 2 */
#define	CS4398_MCLKDIV3   (1<<3)	/* Divive MCLK by 3 */
#define CS4398_I2S	  (1<<4)	/* Set I2S mode */

#define CS4398_SINGLE_SPEED 	0x00
#define CS4398_DOUBLE_SPEED	0x01
#define CS4398_QUAD_SPEED	0x02

/* defs for CS4362A DAC */
#define CS4362A_MODE1_CTRL 	0x01
#define CS4362A_MODE2_CTRL	0x02
#define CS4362A_MODE3_CTRL	0x03
#define CS4362A_FILTER_CTRL	0x04
#define CS4362A_INVERT_CTRL	0x05
#define CS4362A_MIX1_CTRL	0x06
#define CS4362A_VOLA_1		0x07
#define CS4362A_VOLB_1		0x08
#define CS4362A_MIX2_CTRL	0x09
#define CS4362A_VOLA_2		0x0A
#define CS4362A_VOLB_2		0x0B
#define CS4362A_MIX3_CTRL	0x0C
#define CS4362A_VOLA_3		0x0D
#define CS4362A_VOLB_3		0x0E
#define CS4362A_CHIP_REV	0x12

/* CS4362A Reg 01h */
#define CS4362A_CPEN		(1<<7)
#define CS4362A_FREEZE		(1<<6)
#define CS4362A_MCLKDIV		(1<<5)
#define CS4362A_DAC3_ENABLE	(1<<3)
#define CS4362A_DAC2_ENABLE	(1<<2)
#define CS4362A_DAC1_ENABLE	(1<<1)
#define CS4362A_POWER_DOWN	(1)

/* CS4362A Reg 02h */
#define CS4362A_DIF_LJUST	0x00
#define CS4362A_DIF_I2S		0x10
#define CS4362A_DIF_RJUST16	0x20
#define CS4362A_DIF_RJUST24	0x30
#define CS4362A_DIF_RJUST20	0x40
#define CS4362A_DIF_RJUST18	0x50

/* CS4362A Reg 03h */
#define CS4362A_RAMP_IMMEDIATE  0x00
#define CS4362A_RAMP_ZEROCROSS	0x40
#define CS4362A_RAMP_SOFT	0x80
#define CS4362A_RAMP_SOFTZERO   0xC0
#define CS4362A_SINGLE_VOL	0x20
#define CS4362A_RAMP_ERROR	0x10
#define CS4362A_MUTEC_POL	0x08
#define CS4362A_AUTOMUTE	0x04
#define CS4362A_SIX_MUTE	0x00
#define CS4362A_ONE_MUTE	0x01
#define CS4362A_THREE_MUTE	0x03

/* CS4362A Reg 04h */
#define CS4362A_FILT_SEL	0x10
#define CS4362A_DEM_NONE	0x00
#define CS4362A_DEM_44KHZ	0x02
#define CS4362A_DEM_48KHZ	0x04
#define CS4362A_DEM_32KHZ	0x06
#define CS4362A_RAMPDOWN	0x01

/* CS4362A Reg 05h */
#define CS4362A_INV_A3 		(1<<4)
#define CS4362A_INV_B3 		(1<<5)
#define CS4362A_INV_A2		(1<<2)
#define CS4362A_INV_B2		(1<<3)
#define CS4362A_INV_A1		(1)
#define CS4362A_INV_B1		(1<<1)

/* CS4362A Reg 06h, 09h, 0Ch */
/* ATAPI crap, does anyone still use analog CD playback? */
#define CS4362A_SINGLE_SPEED 	0x00
#define CS4362A_DOUBLE_SPEED	0x01
#define CS4362A_QUAD_SPEED	0x02

/* CS4362A Reg 07h, 08h, 0Ah, 0Bh, 0Dh, 0Eh */
/* Volume registers */
#define CS4362A_VOL_MUTE	0x80


/* 0-100. Start at -96dB. */
#define CS4398_VOL(x) \
	((x) == 0 ? 0xFF : (0xC0 - ((x)*192/100)))
/* 0-100. Start at -96dB. Bit 7 is mute. */
#define CS4362A_VOL(x) \
	(char)((x) == 0 ? 0xFF : (0x60 - ((x)*96/100)))

#define UNUSED_CMI9780_CONTROLS ( \
        SOUND_MASK_VOLUME | \
        SOUND_MASK_PCM | \
        SOUND_MASK_REARVOL | \
        SOUND_MASK_CENTERVOL | \
        SOUND_MASK_SIDEVOL | \
        SOUND_MASK_SPEAKER | \
        SOUND_MASK_ALTPCM | \
        SOUND_MASK_VIDEO | \
        SOUND_MASK_DEPTH | \
        SOUND_MASK_MONO | \
        SOUND_MASK_PHONE \
        )

typedef struct cmi8788_portc
{
  int speed, bits, channels;
  int open_mode;
  int trigger_bits;
  int audio_enabled;
  int audiodev;
  int dac_type;
#define ADEV_MULTICH	0
#define ADEV_FRONTPANEL	2
#define ADEV_SPDIF	3
  int adc_type;
#define ADEV_I2SADC1	0
#define ADEV_I2SADC2	1
#define ADEV_I2SADC3	2
  int play_dma_start, rec_dma_start;
  int play_irq_mask, rec_irq_mask;
  int play_chan_reset, rec_chan_reset;
  int min_rate, max_rate, min_chan, max_chan;
}
cmi8788_portc;

#define MAX_PORTC 4

typedef struct cmi8788_devc
{
  oss_device_t *osdev;
  oss_native_word base;
  int fm_attached;
  int irq;
  int dma_len;
  volatile unsigned char intr_mask;
  int model;
#define MDL_CMI8788		1
  char *chip_name;

  /* Audio parameters */
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  oss_mutex_t dac_mutex;
  int open_mode;
  cmi8788_portc portc[MAX_PORTC];

  /* Mixer */
  ac97_devc ac97devc, fp_ac97devc;
  int ac97_mixer_dev, fp_mixer_dev, cmi_mixer_dev;
  int playvol[4];
  int recvol;
  int mute;

  /* uart401 */
  oss_midi_inputbyte_t midi_input_intr;
  int midi_opened, midi_disabled;
  volatile unsigned char input_byte;
  int midi_dev;
  int mpu_attached;
}
cmi8788_devc;

static const char xd2_codec_map[4] = {
                0, 1, 2, 4
        };

static void cmi8788uartintr (cmi8788_devc * devc);
static int reset_cmi8788uart (cmi8788_devc * devc);
static void enter_uart_mode (cmi8788_devc * devc);

static int
ac97_read (void *devc_, int reg)
{
  cmi8788_devc *devc = devc_;
  oss_native_word flags;
  int val, data;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  val = 0L;
  val |= reg << 16;
  val |= 1 << 23;		/*ac97 read the reg address */
  val |= 0 << 24;		/*codec 0 */
  OUTL (devc->osdev, val, AC97_CMD_DATA);
  oss_udelay (200);
  data = INL (devc->osdev, AC97_CMD_DATA) & 0xFFFF;
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return data;
}

static int
ac97_write (void *devc_, int reg, int data)
{
  cmi8788_devc *devc = devc_;
  oss_native_word flags;
  int val;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  val = 0L;
  val |= data & 0xFFFF;
  val |= reg << 16;
  val |= 0 << 23;		/*ac97 write operation */
  val |= 0 << 24;		/*on board codec */
  OUTL (devc->osdev, val, AC97_CMD_DATA);
  oss_udelay (200);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 1;
}

static int
fp_ac97_read (void *devc_, int reg)
{
  cmi8788_devc *devc = devc_;
  oss_native_word flags;
  int data, val;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  val = 0L;
  val |= reg << 16;
  val |= 1 << 23;		/*ac97 read the reg address */
  val |= 1 << 24;		/*fp codec1 */
  OUTL (devc->osdev, val, AC97_CMD_DATA);
  oss_udelay (200);
  data = INL (devc->osdev, AC97_CMD_DATA) & 0xFFFF;
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return data;
}

static int
fp_ac97_write (void *devc_, int reg, int data)
{
  cmi8788_devc *devc = devc_;
  oss_native_word flags;
  int val;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  val = 0L;
  val |= data & 0xFFFF;
  val |= reg << 16;
  val |= 0 << 23;		/*ac97 write operation */
  val |= 1 << 24;		/*fp codec1 */
  OUTL (devc->osdev, val, AC97_CMD_DATA);
  oss_udelay (200);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 1;
}

static int
spi_write (cmi8788_devc *devc, int codec_num, unsigned char reg, int val)
{
  oss_native_word flags;
  unsigned int tmp;
  int latch, shift, count;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  /* check if SPI is busy */
   count = 10;
   while ((INB(devc->osdev, SPI_CONTROL) & 0x1) && count-- > 0) {
    	oss_udelay(10);
  }

  if (devc->model == SUBID_XONAR_DS) {
	shift = 9;
	latch = 0;
  }
  else {
	shift = 8;
	latch = 0x80;
  }

  /* 2 byte data/reg info to be written */
  tmp = val;
  tmp |= (reg << shift);

  /* write 2-byte data values */
  OUTB (devc->osdev, tmp & 0xff, SPI_DATA + 0);
  OUTB (devc->osdev, (tmp >> 8) & 0xff, SPI_DATA + 1);

  /* Latch high, clock=160, Len=2byte, mode=write */
  tmp = (INB (devc->osdev, SPI_CONTROL) & ~0x7E) | latch | 0x1;

  /* now address which codec you want to send the data to */
  tmp |= (codec_num << 4);

  /* send the command to write the data */
  OUTB (devc->osdev, tmp, SPI_CONTROL);

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 1;
}

static int
i2c_write (cmi8788_devc *devc, unsigned char codec_num, unsigned char reg,
		unsigned char data)
{
  oss_native_word flags;
  int count = 50;

  /* Wait for it to stop being busy */
  MUTEX_ENTER(devc->dac_mutex, flags);
  while((INW(devc->osdev, I2C_CTRL) & 0x1) && (count > 0))
  {
	oss_udelay(10);
	count--;
  }
  if(count == 0)
  {
	cmn_err(CE_WARN, "Time out on Two-Wire interface (busy).");
  	MUTEX_EXIT(devc->dac_mutex, flags);
	return OSS_EIO;
  }
  MUTEX_EXIT(devc->dac_mutex, flags);

  MUTEX_ENTER_IRQDISABLE(devc->low_mutex, flags);
  /* first write the Register Address into the MAP register */
  OUTB (devc->osdev, reg, I2C_MAP);

  /* now write the data */
  OUTB (devc->osdev, data, I2C_DATA);

  /* select the codec number to address */
  OUTB (devc->osdev, codec_num, I2C_ADDR);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  oss_udelay(100); 

  return 1;

}

static void
cs4398_init (cmi8788_devc *devc, int codec_addr)
{

  /* Fast Two-Wire. Reduces the wire ready time. */
  OUTW(devc->osdev, 0x0100, I2C_CTRL);

    /* Power down, enable control mode */
    i2c_write(devc, codec_addr, CS4398_MISC_CTRL, 
    CS4398_CPEN | CS4398_POWER_DOWN);
   
  
  /* Left justified PCM (DAC and 8788 support I2S, but doesn't work.
   * Setting it introduces clipping like hell)*/
  i2c_write(devc, codec_addr, CS4398_MODE_CTRL, 0x00);
  /* That's the DAC default, set anyway.  */
  i2c_write(devc, codec_addr, 3, 0x09);
  /* PCM auto-mute - DAC Default (PCM & DSD AutoMute & AutoPolarity)*/
  i2c_write(devc, codec_addr, 4, 0xC0);
  /* Vol A+B to -64dB. */
  i2c_write(devc, codec_addr, 5, 0x80);
  i2c_write(devc, codec_addr, 6, 0x80);
  /* Soft-ramping - DAC Default (Soft Ramp with RampUp & RampDown)*/
  i2c_write(devc, codec_addr, 7, 0xB0);
  /* Remove power down flag. */
  i2c_write(devc, codec_addr, CS4398_MISC_CTRL, CS4398_CPEN);
}

static void
cs4362a_init(cmi8788_devc *devc, int codec_addr)
{
  
  /* Fast Two-Wire. Reduces the wire ready time. */
  OUTW(devc->osdev, 0x0100, I2C_CTRL);

  /* Power down and enable control port. */ 
  i2c_write(devc, codec_addr, CS4362A_MODE1_CTRL, CS4362A_CPEN | CS4362A_POWER_DOWN);
  /* Left-justified PCM */
  i2c_write(devc, codec_addr, CS4362A_MODE2_CTRL, CS4362A_DIF_LJUST);
  /* Ramp & Automute, re-set DAC defaults. */
  i2c_write(devc, codec_addr, CS4362A_MODE3_CTRL, 0x84); 
  /* Filter control, DAC defs. */
  i2c_write(devc, codec_addr, CS4362A_FILTER_CTRL, 0);
  /* Invert control, DAC defs. */
  i2c_write(devc, codec_addr, CS4362A_INVERT_CTRL, 0);
  /* Mixing control, DAC defs. */
  i2c_write(devc, codec_addr, CS4362A_MIX1_CTRL, 0x24);
  i2c_write(devc, codec_addr, CS4362A_MIX2_CTRL, 0x24);
  i2c_write(devc, codec_addr, CS4362A_MIX3_CTRL, 0x24);
  /* Volume to -64dB. */
  i2c_write(devc, codec_addr, CS4362A_VOLA_1, 0x40);
  i2c_write(devc, codec_addr, CS4362A_VOLB_1, 0x40);
  i2c_write(devc, codec_addr, CS4362A_VOLA_2, 0x40);
  i2c_write(devc, codec_addr, CS4362A_VOLB_2, 0x40);
  i2c_write(devc, codec_addr, CS4362A_VOLA_3, 0x40);
  i2c_write(devc, codec_addr, CS4362A_VOLB_3, 0x40);
  /* Power up. */
  i2c_write(devc, codec_addr, CS4362A_MODE1_CTRL, CS4362A_CPEN);
}


static void
cs2000_init(cmi8788_devc *devc, int addr)
{

  /* Fast Two-Wire. Reduces the wire ready time. */
  OUTW(devc->osdev, 0x0100, I2C_CTRL);

  /* first set global config reg  to freeze */
  i2c_write (devc, addr, 0x5, 0x08); 

  /* set dev ctrl reg to  output enabled (0) */
  i2c_write (devc, addr, 0x2, 0);

  /* set dev cfg1 reg to  defaults and enable */
  i2c_write (devc, addr, 0x3, 0x0 | (0 << 3) | 0x0 | 0x1);

  /* set dev cfg2 reg to defaults and enable */
  i2c_write (devc, addr, 0x4, (0 << 1) | 0x0);

  /* set ratio0 MSB to 0 */
  i2c_write (devc, addr, 0x06, 0x00); 
  /* set ratio0 MSB+8 to 0x10 */
  i2c_write (devc, addr, 0x07, 0x10);
  /* set ratio0 LSB+15 to 0 */
  i2c_write (devc, addr, 0x08, 0x00);
  /* set ratio0 LSB+7 to 0 */
  i2c_write (devc, addr, 0x09, 0x00);

  /* Func reg1 to ref clock div 1 */
  i2c_write (devc, addr, 0x16, 0x10);

  /* set func reg2 to 0 */
  i2c_write (devc, addr, 0x17, 0);

  /* set global cfg to run */
  i2c_write (devc, addr, 0x05, 0x1);
}


static unsigned int
mix_scale (int vol, int bits)
{
  vol = mix_cvt[vol];
  return (vol * ((1 << bits) - 1) / 100);
}


static void
cmi8788_generic_set_play_volume (cmi8788_devc *devc, int codec_id, int left, int right)

{
  spi_write (devc, codec_id, AK4396_LchATTCtl | 0x20, mix_scale(left, 8));
  spi_write (devc, codec_id, AK4396_RchATTCtl | 0x20, mix_scale(right, 8));
}

static void
xonar_d1_set_play_volume(cmi8788_devc * devc, int codec_id, int left, int right)
{
  switch(codec_id)
  {
    case 0:
      i2c_write(devc, XONAR_DX_FRONTDAC, CS4398_VOLA, CS4398_VOL(left));
      i2c_write(devc, XONAR_DX_FRONTDAC, CS4398_VOLB, CS4398_VOL(right));
      break;
    case 1:
      i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_VOLA_1, CS4362A_VOL(left));
      i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_VOLB_1, CS4362A_VOL(right));
      break;
    case 2:
      i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_VOLA_2, CS4362A_VOL(left));
      i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_VOLB_2, CS4362A_VOL(right));
      break;
    case 3:
      i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_VOLA_3, CS4362A_VOL(left));
      i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_VOLB_3, CS4362A_VOL(right));
      break;
  }
}

static void
xonar_d2_set_play_volume(cmi8788_devc *devc, int codec_id, int left, int right)
{
  spi_write (devc, xd2_codec_map[codec_id], 16, mix_scale(left,8));
  spi_write (devc, xd2_codec_map[codec_id], 17, mix_scale(right,8));
}

static void
xonar_stx_set_play_volume(cmi8788_devc * devc, int codec_id, int left, int right)
{
  if (codec_id == 0)
  {
      i2c_write(devc, XONAR_STX_FRONTDAC, 16, mix_scale(left,8));
      i2c_write(devc, XONAR_STX_FRONTDAC, 17, mix_scale(right,8));
  }
}

static void
xonar_st_set_play_volume(cmi8788_devc * devc, int codec_id, int left, int right)
{
  if (codec_id == 0)
  {
      i2c_write(devc, XONAR_ST_FRONTDAC, 16, mix_scale(left,8));
      i2c_write(devc, XONAR_ST_FRONTDAC, 17, mix_scale(right,8));
  }
}

static void
xonar_ds_set_play_volume(cmi8788_devc * devc, int codec_id, int left, int right)
{
  switch (codec_id)
   {
    	case 0:      /* front */
		spi_write (devc, XONAR_DS_FRONTDAC, 0, mix_scale(left,7)|0x80);
		spi_write (devc, XONAR_DS_FRONTDAC, 1, mix_scale(right,7)|0x180);
		spi_write (devc, XONAR_DS_FRONTDAC, 3, mix_scale(left,7)|0x80);
		spi_write (devc, XONAR_DS_FRONTDAC, 4, mix_scale(right,7)|0x180);
                break;

	case 1:      /* side */
                spi_write (devc, XONAR_DS_SURRDAC, 0, mix_scale(left,7)|0x80);
                spi_write (devc, XONAR_DS_SURRDAC, 1, mix_scale(right,7)|0x180);
                break;
	case 2:      /* rear */
                spi_write (devc, XONAR_DS_SURRDAC, 4, mix_scale(left,7)|0x80);
                spi_write (devc, XONAR_DS_SURRDAC, 5, mix_scale(right,7)|0x180);
                break;
	case 3:      /* center */
                spi_write (devc, XONAR_DS_SURRDAC, 6, mix_scale(left,7)|0x80);
                spi_write (devc, XONAR_DS_SURRDAC, 7, mix_scale(right,7)|0x180);
                break;
   }
}

static int
cmi8788_set_rec_volume (cmi8788_devc * devc, int value)
{
  unsigned char left, right;

  left = value & 0xff;
  right = (value >> 8) & 0xff;

  if (left > 100)
	left = 100;
  if (right > 100)
	right = 100;

  devc->recvol = left | (right << 8);

  spi_write (devc, XONAR_DS_FRONTDAC, 0xe, mix_scale(left,8));
  spi_write (devc, XONAR_DS_FRONTDAC, 0xf, mix_scale(right,8));

  return devc->recvol;
}



static int
cmi8788_set_play_volume (cmi8788_devc * devc, int codec_id, int value)
{
  int left, right;

  left = (value & 0x00FF);
  right = (value & 0xFF00) >> 8;

  if (left > 100)
	left = 100;
  if (right > 100)
        right = 100;

  devc->playvol[codec_id] = left | (right<<8);

  switch(devc->model)
  {
    case SUBID_XONAR_D1:
    case SUBID_XONAR_DX:
      xonar_d1_set_play_volume(devc, codec_id, left, right);
      break;
    case SUBID_XONAR_D2:
    case SUBID_XONAR_D2X:
      xonar_d2_set_play_volume(devc, codec_id, left, right);
      break;
    case SUBID_XONAR_STX:
      xonar_stx_set_play_volume(devc, codec_id, left, right);
      break;
    case SUBID_XONAR_ST:
      xonar_st_set_play_volume(devc, codec_id, left, right);
      break;
    case SUBID_XONAR_DS:
      xonar_ds_set_play_volume(devc, codec_id, left, right);
      break;
    default:
      cmi8788_generic_set_play_volume (devc, codec_id, left, right);
      break;
  }
  return devc->playvol[codec_id];
}

static int
cmi8788intr (oss_device_t * osdev)
{
  cmi8788_devc *devc = (cmi8788_devc *) osdev->devc;
  unsigned int intstat;
  int i;
  int serviced = 0;


  intstat = INW (devc->osdev, IRQ_STAT);
  if (intstat != 0)
    serviced = 1;
  else
    return 0;

  for (i = 0; i < MAX_PORTC; i++)
    {
      cmi8788_portc *portc = &devc->portc[i];

      /* Handle Playback Ints */
      if ((intstat & portc->play_irq_mask)
	  && (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	{

	  /* Acknowledge the interrupt by disabling and enabling the irq */
	  OUTW (devc->osdev,
		INW (devc->osdev, IRQ_MASK) & ~portc->play_irq_mask,
		IRQ_MASK);
	  OUTW (devc->osdev,
		INW (devc->osdev, IRQ_MASK) | portc->play_irq_mask, IRQ_MASK);

	  /* process buffer */
	  oss_audio_outputintr (portc->audiodev, 0);
	}

      /* Handle Record Ints */
      if ((intstat & portc->rec_irq_mask)
	  && (portc->trigger_bits & PCM_ENABLE_INPUT))
	{
	  /* disable the interrupt first */
	  OUTW (devc->osdev,
		INW (devc->osdev, IRQ_MASK) & ~portc->rec_irq_mask, IRQ_MASK);
	  /* enable the interrupt mask */
	  OUTW (devc->osdev,
		INW (devc->osdev, IRQ_MASK) | portc->rec_irq_mask, IRQ_MASK);
	  oss_audio_inputintr (portc->audiodev, 0);
	}

    }

  /* MPU interrupt */
  if (intstat & 0x1000)
    {
      cmi8788uartintr (devc);
    }

  return serviced;
}

static int
cmi8788_audio_set_rate (int dev, int arg)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->speed;

  if (arg > portc->max_rate)
    arg = portc->max_rate;
  if (arg < portc->min_rate)
    arg = portc->min_rate;

  portc->speed = arg;
  return portc->speed;
}

static short
cmi8788_audio_set_channels (int dev, short arg)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;

  if (audio_engines[dev]->flags & ADEV_STEREOONLY)
    arg = 2;

  if (arg == 1)
    arg = 2;

  if (arg>8)
     arg=8;
  if ((arg != 2) && (arg != 4) && (arg != 6) && (arg != 8))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
cmi8788_audio_set_format (int dev, unsigned int arg)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (audio_engines[dev]->flags & ADEV_16BITONLY)
    arg = AFMT_S16_LE;

  if (!(arg & (AFMT_S16_LE | AFMT_AC3 | AFMT_S32_LE)))
    return portc->bits;
  portc->bits = arg;
  return portc->bits;
}

/*ARGSUSED*/
static int
cmi8788_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void cmi8788_audio_trigger (int dev, int state);

static void
cmi8788_audio_reset (int dev)
{
  cmi8788_audio_trigger (dev, 0);
}

static void
cmi8788_audio_reset_input (int dev)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;
  cmi8788_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
cmi8788_audio_reset_output (int dev)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;
  cmi8788_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
cmi8788_audio_open (int dev, int mode, int open_flags)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;
  cmi8788_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (devc->open_mode & mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  devc->open_mode |= mode;

  portc->open_mode = mode;
  portc->audio_enabled &= ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
cmi8788_audio_close (int dev, int mode)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;
  cmi8788_devc *devc = audio_engines[dev]->devc;
  
  /* 
   * muting AB, front, channels when we're done with playing audio - 'pop' & click free
   * when stopping sound or when doing soundoff
   * Only for D1/DX models - not sure if the problem appears on other Xonars
   * Also only for DAC for front channels - not sure if the problem appears on side/rear/center DAC 
   */
  switch(devc->model)
  {
    case SUBID_XONAR_DX:
    case SUBID_XONAR_D1:
      i2c_write(devc,XONAR_DX_FRONTDAC, CS4398_MUTE_CTRL, 0xD8);
      oss_udelay(1000);
      break;
  }
   
  cmi8788_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
cmi8788_audio_output_block (int dev, oss_native_word buf, int count,
			    int fragsize, int intrflag)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
cmi8788_audio_start_input (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
cmi8788_audio_trigger (int dev, int state)
{
  cmi8788_portc *portc = audio_engines[dev]->portc;
  cmi8788_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      
	     /* 
	      * Delay is here for removal of dangerous DC current and pops/clicks  - they might still
	      * appear, but they won't damage anything.
	      * Maybe it's just my Xonar DX, but it's pushing out small DC current 
	      *	so we're waiting for it to settle down so to not let external amp amplify DC.
	      * We're also unmuting the AB channels that were muted after we stopped playing audio.
	      * Only for D1/DX models - not sure if the problem appears on other Xonars 
	      * Also only for DAC for front channels - not sure if the problem appears on side/rear/center DAC 
	      */
	      switch(devc->model)
	      {
		case SUBID_XONAR_DX:
		case SUBID_XONAR_D1:
		  oss_udelay(1000);
		  i2c_write(devc,XONAR_DX_FRONTDAC,CS4398_MUTE_CTRL,0xC0);
		  break;
	      }
	      
	      /* Enable Interrupt */
	      OUTW (devc->osdev,
		    INW (devc->osdev, IRQ_MASK) | portc->play_irq_mask,
		    IRQ_MASK);
	      /* enable the dma */
	      OUTW (devc->osdev,
		    INW (devc->osdev, DMA_START) | portc->play_dma_start,
		    DMA_START);
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

	      /* disable dma */
	      OUTW (devc->osdev,
		    INW (devc->osdev, DMA_START) & ~portc->play_dma_start,
		    DMA_START);
	      /* Disable Interrupt */
	      OUTW (devc->osdev,
		    INW (devc->osdev, IRQ_MASK) & ~portc->play_irq_mask,
		    IRQ_MASK);
	    }
	}
    }

  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      /* Enable Interrupt */
	      OUTW (devc->osdev,
		    INW (devc->osdev, IRQ_MASK) | portc->rec_irq_mask,
		    IRQ_MASK);

	      /* enable the channel */
	      OUTW (devc->osdev,
		    INW (devc->osdev, DMA_START) | portc->rec_dma_start,
		    DMA_START);
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      portc->audio_enabled &= ~PCM_ENABLE_INPUT;
	      /* disable channel  */
	      OUTW (devc->osdev,
		    INW (devc->osdev, DMA_START) & ~portc->rec_dma_start,
		    DMA_START);

	      /* Disable Interrupt */
	      OUTW (devc->osdev,
		    INW (devc->osdev, IRQ_MASK) & ~portc->rec_irq_mask,
		    IRQ_MASK);

	    }
	}
    }	
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
cmi8788_reset_channel (cmi8788_devc * devc, cmi8788_portc * portc,
		       int direction)
{
  if (direction == PCM_ENABLE_OUTPUT)
    {
      /* reset the channel */
      OUTB (devc->osdev,
	    INB (devc->osdev, CHAN_RESET) | portc->play_chan_reset,
	    CHAN_RESET);
      oss_udelay (10);
      OUTB (devc->osdev,
	    INB (devc->osdev, CHAN_RESET) & ~portc->play_chan_reset,
	    CHAN_RESET);
    }

  if (direction == PCM_ENABLE_INPUT)
    {
      /* reset the channel */
      OUTB (devc->osdev,
	    INB (devc->osdev, CHAN_RESET) | portc->rec_chan_reset,
	    CHAN_RESET);
      oss_udelay (10);
      OUTB (devc->osdev,
	    INB (devc->osdev, CHAN_RESET) & ~portc->rec_chan_reset,
	    CHAN_RESET);
    }

}

static int
i2s_calc_rate (int rate)
{
  int i2s_rate;

  switch (rate)
    {
    case 32000:
      i2s_rate = 0;
      break;
    case 44100:
      i2s_rate = 1;
      break;
    case 48000:
      i2s_rate = 2;
      break;
    case 64000:
      i2s_rate = 3;
      break;
    case 88200:
      i2s_rate = 4;
      break;
    case 96000:
      i2s_rate = 5;
      break;
    case 176400:
      i2s_rate = 6;
      break;
    case 192000:
      i2s_rate = 7;
      break;
    default:
      i2s_rate = 2;
      break;
    }

  return i2s_rate;
}

int
i2s_calc_bits (int bits)
{
  int i2s_bits;

  switch (bits)
    {
#if 0
    case AFMT_S24_LE:
      i2s_bits = 0x80;
      break;
#endif
    case AFMT_S32_LE:
      i2s_bits = 0xC0;
      break;
    default:			/* AFMT_S16_LE */
      i2s_bits = 0x00;
      break;
    }

  return i2s_bits;
}

/*ARGSUSED*/
static int
cmi8788_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  cmi8788_devc *devc = audio_engines[dev]->devc;
  cmi8788_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_in;
  oss_native_word flags;
  int channels, bits, i2s_bits, i2s_rate;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  switch (portc->adc_type)
    {
    case ADEV_I2SADC1:
      {
	portc->rec_dma_start = 0x1;
	portc->rec_irq_mask = 0x1;
	portc->rec_chan_reset = 0x1;
	cmi8788_reset_channel (devc, portc, PCM_ENABLE_INPUT);

	OUTL (devc->osdev, dmap->dmabuf_phys, RECA_ADDR);
	OUTW (devc->osdev, dmap->bytes_in_use / 4 - 1, RECA_SIZE);
	OUTW (devc->osdev, dmap->fragment_size / 4 - 1, RECA_FRAG);

	switch (portc->channels)
	  {
	  case 4:
	    channels = 0x1;
	    break;
	  case 6:
	    channels = 0x2;
	    break;
	  case 8:
	    channels = 0x4;
	    break;
	  default:
	    channels = 0x0;	/* Stereo */
	    break;
	  }

	OUTB (devc->osdev, (INB (devc->osdev, REC_MODE) & ~0x3) | channels,
	      REC_MODE);

	switch (portc->bits)
	  {
#if 0
	    /* The 24 bit format supported by cmi8788 is not AFMT_S24_LE */
	  case AFMT_S24_LE:
	    bits = 0x1;
	    break;
#endif
	  case AFMT_S32_LE:
	    bits = 0x2;
	    break;
	  default:		/* AFMT_S16_LE */
	    bits = 0x0;
	    break;
	  }
	OUTB (devc->osdev, (INB (devc->osdev, REC_FORMAT) & ~0x3) | bits,
	      REC_FORMAT);

	/* set up the i2s bits as well */
	i2s_bits = i2s_calc_bits (portc->bits);
	OUTB (devc->osdev,
	      (INB (devc->osdev, I2S_ADC1_FORMAT) & ~0xC0) | i2s_bits,
	      I2S_ADC1_FORMAT);

	/* setup the i2s speed */
	i2s_rate = i2s_calc_rate (portc->speed);
	OUTB (devc->osdev,
	      (INB (devc->osdev, I2S_ADC1_FORMAT) & ~0x7) | i2s_rate,
	      I2S_ADC1_FORMAT);

	break;
      }

    case ADEV_I2SADC2:
      {
	portc->rec_dma_start = 0x2;
	portc->rec_irq_mask = 0x2;
	portc->rec_chan_reset = 0x2;
	cmi8788_reset_channel (devc, portc, PCM_ENABLE_INPUT);

	OUTL (devc->osdev, dmap->dmabuf_phys, RECB_ADDR);
	OUTW (devc->osdev, dmap->bytes_in_use / 4 - 1, RECB_SIZE);
	OUTW (devc->osdev, dmap->fragment_size / 4 - 1, RECB_FRAG);

        switch (portc->bits)
          {
#if 0
          case AFMT_S24_LE:
            bits = 0x04;
            break;
#endif
          case AFMT_S32_LE:
            bits = 0x08;
            break;
          default:              /*  AFMT_S16_LE */
            bits = 0x0;
            break;
          }

        OUTB (devc->osdev, (INB (devc->osdev, REC_FORMAT) & ~0x0C) | bits,
              REC_FORMAT);

        /* setup i2s bits */
        i2s_bits = i2s_calc_bits (portc->bits);
        OUTB (devc->osdev,
              (INB (devc->osdev, I2S_ADC2_FORMAT) & ~0xC0) | i2s_bits,
              I2S_ADC2_FORMAT);

        /* setup speed */
        i2s_rate = i2s_calc_rate (portc->speed);
        OUTB (devc->osdev,
              (INB (devc->osdev, I2S_ADC2_FORMAT) & ~0x7) | i2s_rate,
              I2S_ADC2_FORMAT);

	break;
      }

    case ADEV_I2SADC3:
      {
	portc->rec_dma_start = 0x4;
	portc->rec_irq_mask = 0x4;
	portc->rec_chan_reset = 0x4;
	cmi8788_reset_channel (devc, portc, PCM_ENABLE_INPUT);

	OUTL (devc->osdev, dmap->dmabuf_phys, RECC_ADDR);
	OUTW (devc->osdev, dmap->bytes_in_use / 4 - 1, RECC_SIZE);
	OUTW (devc->osdev, dmap->fragment_size / 4 - 1, RECC_FRAG);

	switch (portc->bits)
	  {
#if 0
	  case AFMT_S24_LE:
	    bits = 0x10;
	    break;
#endif
	  case AFMT_S32_LE:
	    bits = 0x20;
	    break;
	  default:		/*  AFMT_S16_LE */
	    bits = 0x0;
	    break;
	  }

	OUTB (devc->osdev, (INB (devc->osdev, REC_FORMAT) & ~0x30) | bits,
	      REC_FORMAT);

	/* setup i2s bits */
	i2s_bits = i2s_calc_bits (portc->bits);
	OUTB (devc->osdev,
	      (INB (devc->osdev, I2S_ADC3_FORMAT) & ~0xC0) | i2s_bits,
	      I2S_ADC3_FORMAT);

	/* setup speed */
	i2s_rate = i2s_calc_rate (portc->speed);
	OUTB (devc->osdev,
	      (INB (devc->osdev, I2S_ADC3_FORMAT) & ~0x7) | i2s_rate,
	      I2S_ADC3_FORMAT);
	break;
      }
    }
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
cmi8788_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  cmi8788_devc *devc = audio_engines[dev]->devc;
  cmi8788_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_out;
  oss_native_word flags;
  int i2s_rate, rate, spdif_rate, bits = 0, i2s_bits, channels = 0;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  switch (portc->dac_type)
    {
    case ADEV_MULTICH:
      {
	portc->play_dma_start = 0x10;
	portc->play_irq_mask = 0x10;
	portc->play_chan_reset = 0x10;
	cmi8788_reset_channel (devc, portc, PCM_ENABLE_OUTPUT);

	OUTL (devc->osdev, dmap->dmabuf_phys, MULTICH_ADDR);
	OUTL (devc->osdev, dmap->bytes_in_use / 4 - 1, MULTICH_SIZE);
	OUTL (devc->osdev, dmap->fragment_size / 4 - 1, MULTICH_FRAG);

	switch (portc->channels)
	  {
	  case 2:
	    channels = 0;
	    break;
	  case 4:
	    channels = 1;
	    break;
	  case 6:
	    channels = 2;
	    break;
	  case 8:
	    channels = 3;
	    break;
	  }

	OUTB (devc->osdev,
	      (INB (devc->osdev, MULTICH_MODE) & ~0x3) | channels,
	      MULTICH_MODE);

	/* setup bits per sample */
	switch (portc->bits)
	  {
#if 0
	  case AFMT_S24_LE:
	    bits = 4;
	    break;
#endif
	  case AFMT_S32_LE:
	    bits = 8;
	    break;

	  default:		/* AFMT_S16_LE */
	    bits = 0;
	    break;
	  }
	  
	  
	  /*
	   * Setting correct MCLK/RCLK ratio and oversampling speed based on input signal
	   * We have a clock generator at 24.5760MHz, so we use ratios for that speed
	   * Only for D1/DX models and only for front (CS4398) DAC
	   * as I don't have any means of testing other channels or other cards
	   */
	   switch(devc->model)
	   {
	   case SUBID_XONAR_DX:
	   case SUBID_XONAR_D1:
		/*
		 * If we're playing up to 50KHz audio we need to set 512x MCLK/LRCK ratio
		 * and set oversampling to single mode - 128x oversampling
		 */
		 if(portc->speed <= 50000)
		 {
		   OUTW (devc->osdev, 0x010A | XONAR_MCLOCK_512, I2S_MULTICH_FORMAT);
		   i2c_write(devc, XONAR_DX_FRONTDAC, CS4398_MODE_CTRL, CS4398_SINGLE_SPEED);
		   		   
		  /* Surround sound currently doesn't work on Xonar D1/DX - if it ever starts working
		   * these three lines below are correct for setting the oversampling mode for surround DAC
		
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX1_CTRL, 0x24 | CS4362A_SINGLE_SPEED);
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX2_CTRL, 0x24 | CS4362A_SINGLE_SPEED);
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX3_CTRL, 0x24 | CS4362A_SINGLE_SPEED);
		   
		   */
		 }
		 
		/*
		 * If we're playing up to 100KHz audio we need to set 256x MCLK/LRCK ratio
		 * and set oversampling to double mode - 64x oversampling
		 */
		 else if(portc->speed <= 100000)
		 {
		   OUTW (devc->osdev, 0x010A | XONAR_MCLOCK_256, I2S_MULTICH_FORMAT);
		   i2c_write(devc, XONAR_DX_FRONTDAC, CS4398_MODE_CTRL, CS4398_DOUBLE_SPEED);
		   
		  /* Surround sound currently doesn't work on Xonar D1/DX - if it ever starts working
		   * these three lines below are correct for setting the oversampling mode for surround DAC
		
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX1_CTRL, 0x24 | CS4362A_DOUBLE_SPEED);
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX2_CTRL, 0x24 | CS4362A_DOUBLE_SPEED);
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX3_CTRL, 0x24 | CS4362A_DOUBLE_SPEED);
		   
		   */
		 }
		 
		/*
		 * If we're playing up to 200KHz audio we need to set 128x MCLK/LRCK ratio
		 * and set oversampling to double mode - 32x oversampling
		 */
		 else if(portc->speed <= 200000)
		 {
		   OUTW (devc->osdev, 0x010A | XONAR_MCLOCK_128, I2S_MULTICH_FORMAT);
		   i2c_write(devc, XONAR_DX_FRONTDAC, CS4398_MODE_CTRL, CS4398_QUAD_SPEED); 
		   
		  /* Surround sound currently doesn't work on Xonar D1/DX - if it ever starts working
		   * these three lines below are correct for setting the oversampling mode for surround DAC
		
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX1_CTRL, 0x24 | CS4362A_QUAD_SPEED);
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX2_CTRL, 0x24 | CS4362A_QUAD_SPEED);
		   i2c_write(devc, XONAR_DX_SURRDAC, CS4362A_MIX3_CTRL, 0x24 | CS4362A_QUAD_SPEED);
		   
		   */
		 }
	  
		 break;
	   }

	/* set the format bits in play format register */
	OUTB (devc->osdev, (INB (devc->osdev, PLAY_FORMAT) & ~0xC) | bits,
	      PLAY_FORMAT);

	/* setup the i2s bits in the i2s register */
	i2s_bits = i2s_calc_bits (portc->bits);
	OUTB (devc->osdev,
	      (INB (devc->osdev, I2S_MULTICH_FORMAT) & ~0xC0) | i2s_bits,
	      I2S_MULTICH_FORMAT);

	/* setup speed */
	i2s_rate = i2s_calc_rate (portc->speed);
	OUTB (devc->osdev,
	      (INB (devc->osdev, I2S_MULTICH_FORMAT) & ~0x7) | i2s_rate,
	      I2S_MULTICH_FORMAT);

	break;
      }

    case ADEV_FRONTPANEL:
      {
	portc->play_dma_start = 0x20;
	portc->play_irq_mask = 0x20;
	portc->play_chan_reset = 0x20;
	cmi8788_reset_channel (devc, portc, PCM_ENABLE_OUTPUT);

	OUTL (devc->osdev, dmap->dmabuf_phys, FPOUT_ADDR);
	OUTW (devc->osdev, dmap->bytes_in_use / 4 - 1, FPOUT_SIZE);
	OUTW (devc->osdev, dmap->fragment_size / 4 - 1, FPOUT_FRAG);
	ac97_playrate (&devc->fp_ac97devc, portc->speed);
	ac97_spdif_setup (devc->fp_mixer_dev, portc->speed, portc->bits);

	break;
      }

    case ADEV_SPDIF:
      {
	portc->play_dma_start = 0x08;
	portc->play_irq_mask = 0x08;
	portc->play_chan_reset = 0x08;
	cmi8788_reset_channel (devc, portc, PCM_ENABLE_OUTPUT);

	/* STOP SPDIF Out */
	OUTL (devc->osdev, (INL (devc->osdev, SPDIF_FUNC) & ~0x00000002),
	      SPDIF_FUNC);

	/* setup AC3 for 16bit 48Khz, Non-Audio */
	if (portc->bits == AFMT_AC3)
	  {
	    portc->bits = 16;
	    portc->channels = 2;
	    portc->speed = 48000;

	    /* set the PCM/Data bit to Non-Audio */
	    OUTL (devc->osdev,
		  (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0x0002) | 0x0002,
		  SPDIFOUT_CHAN_STAT);
	  }
	else
	  OUTL (devc->osdev,
		INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0x0002,
		SPDIFOUT_CHAN_STAT);

	OUTL (devc->osdev, dmap->dmabuf_phys, SPDIF_ADDR);
	OUTW (devc->osdev, dmap->bytes_in_use / 4 - 1, SPDIF_SIZE);
	OUTW (devc->osdev, dmap->fragment_size / 4 - 1, SPDIF_FRAG);

	/* setup number of bits/sample */
	switch (portc->bits)
	  {
	  case 16:
	    bits = 0;
	    break;
	  case 24:
	    bits = 1;
	    break;
	  case 32:
	    bits = 2;
	    break;
	  }

	OUTB (devc->osdev, (INB (devc->osdev, PLAY_FORMAT) & ~0x3) | bits,
	      PLAY_FORMAT);

	/* setup sampling rate */
	switch (portc->speed)
	  {
	  case 32000:
	    rate = 0;
	    spdif_rate = 0x3;
	    break;
	  case 44100:
	    rate = 1;
	    spdif_rate = 0x0;
	    break;
	  case 48000:
	    rate = 2;
	    spdif_rate = 0x2;
	    break;
	  case 64000:
	    rate = 3;
	    spdif_rate = 0xb;
	    break;
	  case 88200:
	    rate = 4;
	    spdif_rate = 0x8;
	    break;
	  case 96000:
	    rate = 5;
	    spdif_rate = 0xa;
	    break;
	  case 176400:
	    rate = 6;
	    spdif_rate = 0xc;
	    break;
	  case 192000:
	    rate = 7;
	    spdif_rate = 0xe;
	    break;
	  default:
	    rate = 2;		/* 48000 */
	    spdif_rate = 0x2;
	    break;
	  }

	OUTL (devc->osdev,
	      (INL (devc->osdev, SPDIF_FUNC) & ~0x0f000000) | rate << 24,
	      SPDIF_FUNC);

	/* also program the Channel status */
	OUTL (devc->osdev,
	      (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF000) | spdif_rate
	      << 12, SPDIFOUT_CHAN_STAT);

	/* Enable SPDIF Out */
	OUTL (devc->osdev,
	      (INL (devc->osdev, SPDIF_FUNC) & ~0x00000002) | 0x2,
	      SPDIF_FUNC);

	break;
      }
    }
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  
  
  return 0;
}

static int
cmi8788_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  cmi8788_devc *devc = audio_engines[dev]->devc;
  cmi8788_portc *portc = audio_engines[dev]->portc;
  unsigned int ptr = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_INPUT)
    {
      switch (portc->adc_type)
	{
	case ADEV_I2SADC1:
	  ptr = INL (devc->osdev, RECA_ADDR);
	  break;
	case ADEV_I2SADC2:
	  ptr = INL (devc->osdev, RECB_ADDR);
	  break;
	case ADEV_I2SADC3:
	  ptr = INL (devc->osdev, RECC_ADDR);
	  break;
	}
    }

  if (direction == PCM_ENABLE_OUTPUT)
    {
      switch (portc->dac_type)
	{
	case ADEV_MULTICH:
	  ptr = INL (devc->osdev, MULTICH_ADDR);
	  break;
	case ADEV_FRONTPANEL:
	  ptr = INL (devc->osdev, FPOUT_ADDR);
	  break;
	case ADEV_SPDIF:
	  ptr = INL (devc->osdev, SPDIF_ADDR);
	  break;
	}
    }

  ptr -= dmap->dmabuf_phys;
  ptr %= dmap->bytes_in_use;
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}


static audiodrv_t cmi8788_audio_driver = {
  cmi8788_audio_open,
  cmi8788_audio_close,
  cmi8788_audio_output_block,
  cmi8788_audio_start_input,
  cmi8788_audio_ioctl,
  cmi8788_audio_prepare_for_input,
  cmi8788_audio_prepare_for_output,
  cmi8788_audio_reset,
  NULL,
  NULL,
  cmi8788_audio_reset_input,
  cmi8788_audio_reset_output,
  cmi8788_audio_trigger,
  cmi8788_audio_set_rate,
  cmi8788_audio_set_format,
  cmi8788_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* cmi8788_check_input, */
  NULL,				/* cmi8788_check_output, */
  NULL,				/* cmi8788_alloc_buffer */
  NULL,				/* cmi8788_free_buffer */
  NULL,
  NULL,
  cmi8788_get_buffer_pointer
};

#define input_avail(devc) (!(cmi8788uart_status(devc)&INPUT_AVAIL))
#define output_ready(devc)      (!(cmi8788uart_status(devc)&OUTPUT_READY))

static __inline__ int
cmi8788uart_status (cmi8788_devc * devc)
{
  return INB (devc->osdev, MPU401_COMMAND);
}

static void
cmi8788uart_cmd (cmi8788_devc * devc, unsigned char cmd)
{
  OUTB (devc->osdev, cmd, MPU401_COMMAND);
}

static __inline__ int
cmi8788uart_read (cmi8788_devc * devc)
{
  return INB (devc->osdev, MPU401_DATA);
}

static __inline__ void
cmi8788uart_write (cmi8788_devc * devc, unsigned char byte)
{
  OUTB (devc->osdev, byte, MPU401_DATA);
}

#define OUTPUT_READY    0x40
#define INPUT_AVAIL     0x80
#define MPU_ACK         0xFE
#define MPU_RESET       0xFF
#define UART_MODE_ON    0x3F


static void
cmi8788uart_input_loop (cmi8788_devc * devc)
{
  while (input_avail (devc))
    {
      unsigned char c = cmi8788uart_read (devc);

      if (c == MPU_ACK)
	devc->input_byte = c;
      else if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	devc->midi_input_intr (devc->midi_dev, c);
    }
}

static void
cmi8788uartintr (cmi8788_devc * devc)
{
  cmi8788uart_input_loop (devc);
}

/*ARGSUSED*/
static int
cmi8788uart_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		  oss_midi_inputbuf_t inputbuf,
		  oss_midi_outputintr_t outputintr)
{
  cmi8788_devc *devc = (cmi8788_devc *) midi_devs[dev]->devc;

  if (devc->midi_opened)
    {
      return OSS_EBUSY;
    }

  while (input_avail (devc))
    cmi8788uart_read (devc);

  devc->midi_input_intr = inputbyte;
  devc->midi_opened = mode;
  enter_uart_mode (devc);
  devc->midi_disabled = 0;

  return 0;
}

/*ARGSUSED*/
static void
cmi8788uart_close (int dev, int mode)
{
  cmi8788_devc *devc = (cmi8788_devc *) midi_devs[dev]->devc;
  reset_cmi8788uart (devc);
  oss_udelay (10);
  enter_uart_mode (devc);
  reset_cmi8788uart (devc);
  devc->midi_opened = 0;
}


static int
cmi8788uart_out (int dev, unsigned char midi_byte)
{
  int timeout;
  cmi8788_devc *devc = (cmi8788_devc *) midi_devs[dev]->devc;
  oss_native_word flags;

  /*
   * Test for input since pending input seems to block the output.
   */

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (input_avail (devc))
    cmi8788uart_input_loop (devc);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  /*
   * Sometimes it takes about 130000 loops before the output becomes ready
   * (After reset). Normally it takes just about 10 loops.
   */

  for (timeout = 130000; timeout > 0 && !output_ready (devc); timeout--);

  if (!output_ready (devc))
    {
      cmn_err (CE_WARN, "UART timeout - Device not responding\n");
      devc->midi_disabled = 1;
      reset_cmi8788uart (devc);
      enter_uart_mode (devc);
      return 1;
    }

  cmi8788uart_write (devc, midi_byte);
  return 1;
}

/*ARGSUSED*/
static int
cmi8788uart_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t cmi8788_midi_driver = {
  cmi8788uart_open,
  cmi8788uart_close,
  cmi8788uart_ioctl,
  cmi8788uart_out,
};

static void
enter_uart_mode (cmi8788_devc * devc)
{
  int ok, timeout;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

  devc->input_byte = 0;
  cmi8788uart_cmd (devc, UART_MODE_ON);

  ok = 0;
  for (timeout = 50000; timeout > 0 && !ok; timeout--)
    if (devc->input_byte == MPU_ACK)
      ok = 1;
    else if (input_avail (devc))
      if (cmi8788uart_read (devc) == MPU_ACK)
	ok = 1;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}


void
attach_cmi8788uart (cmi8788_devc * devc)
{
  enter_uart_mode (devc);
  devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "CMI8788", "CMI8788 UART", &cmi8788_midi_driver, sizeof (midi_driver_t),
					0, devc, devc->osdev);
  devc->midi_opened = 0;
}

static int
reset_cmi8788uart (cmi8788_devc * devc)
{
  int ok, timeout, n;

  /*
   * Send the RESET command. Try again if no success at the first time.
   */

  ok = 0;

  for (n = 0; n < 2 && !ok; n++)
    {
      for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

      devc->input_byte = 0;
      cmi8788uart_cmd (devc, MPU_RESET);

      /*
       * Wait at least 25 msec. This method is not accurate so let's make the
       * loop bit longer. Cannot sleep since this is called during boot.
       */

      for (timeout = 50000; timeout > 0 && !ok; timeout--)
	if (devc->input_byte == MPU_ACK)	/* Interrupt */
	  ok = 1;
	else if (input_avail (devc))
	  if (cmi8788uart_read (devc) == MPU_ACK)
	    ok = 1;

    }



  if (ok)
    cmi8788uart_input_loop (devc);	/*
					 * Flush input before enabling interrupts
					 */

  return ok;
}


int
probe_cmi8788uart (cmi8788_devc * devc)
{
  int ok = 0;
  oss_native_word flags;

  DDB (cmn_err (CE_CONT, "Entered probe_cmi8788uart\n"));

  devc->midi_input_intr = NULL;
  devc->midi_opened = 0;
  devc->input_byte = 0;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  ok = reset_cmi8788uart (devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (ok)
    {
      DDB (cmn_err (CE_CONT, "Reset UART401 OK\n"));
    }
  else
    {
      DDB (cmn_err
	   (CE_CONT, "Reset UART401 failed (no hardware present?).\n"));
      DDB (cmn_err
	   (CE_CONT, "mpu401 status %02x\n", cmi8788uart_status (devc)));
    }

  DDB (cmn_err (CE_CONT, "cmi8788uart detected OK\n"));
  return ok;
}

void
unload_cmi8788uart (cmi8788_devc * devc)
{
  reset_cmi8788uart (devc);
}


static void
attach_mpu (cmi8788_devc * devc)
{
  devc->mpu_attached = 1;
  attach_cmi8788uart (devc);
}

/*ARGSUSED*/
static int
cmi8788_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  cmi8788_devc *devc = mixer_devs[dev]->devc;

  if (((cmd >> 8) & 0xff) == 'M')
    {
      int val;

      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    return *arg = 0;
	    break;

	  case SOUND_MIXER_PCM:
	    val = *arg;
	    return *arg = cmi8788_set_play_volume (devc, 0, val);
	    break;

	  case SOUND_MIXER_REARVOL:
	    val = *arg;
	    return *arg = cmi8788_set_play_volume (devc, 1, val);
	    break;

	  case SOUND_MIXER_CENTERVOL:
	    val = *arg;
	    return *arg = cmi8788_set_play_volume (devc, 2, val);
	    break;

	  case SOUND_MIXER_SIDEVOL:
	    val = *arg;
	    return *arg = cmi8788_set_play_volume (devc, 3, val);
	    break;

	  case SOUND_MIXER_RECLEV:
	    val = *arg;
	    return *arg = cmi8788_set_rec_volume (devc, val);
	    break;

	  default:
	    val = *arg;
	    return *arg = 0;
	    break;
	  }
      else
	switch (cmd & 0xff)	/* Return Parameter */
	  {
	  case SOUND_MIXER_RECSRC:
	    return *arg = 0;
	    break;

	  case SOUND_MIXER_DEVMASK:
            if ((devc->model == SUBID_XONAR_STX) || (devc->model == SUBID_XONAR_ST))
                *arg = SOUND_MASK_PCM;
            else
	    	*arg = SOUND_MASK_PCM | SOUND_MASK_REARVOL | 
			SOUND_MASK_CENTERVOL | SOUND_MASK_SIDEVOL;

	    if (devc->model == SUBID_XONAR_DS)
		*arg |= SOUND_MASK_RECLEV;

	    return *arg;
	    break;

	  case SOUND_MIXER_STEREODEVS:
            if ((devc->model == SUBID_XONAR_STX) || (devc->model == SUBID_XONAR_ST))
               *arg = SOUND_MASK_PCM;
            else
	       *arg = SOUND_MASK_PCM | SOUND_MASK_REARVOL | 
			SOUND_MASK_CENTERVOL | SOUND_MASK_SIDEVOL;

	    if (devc->model == SUBID_XONAR_DS)
		*arg |= SOUND_MASK_RECLEV;

	    return *arg;
	    break;

	  case SOUND_MIXER_CAPS:
	    return *arg = SOUND_CAP_EXCL_INPUT;
	    break;

	  case SOUND_MIXER_PCM:
	    return *arg = devc->playvol[0];
	    break;

	  case SOUND_MIXER_REARVOL:
	    return *arg = devc->playvol[1];
	    break;

	  case SOUND_MIXER_CENTERVOL:
	    return *arg = devc->playvol[2];
	    break;

	  case SOUND_MIXER_SIDEVOL:
	    return *arg = devc->playvol[3];
	    break;
	
	  case SOUND_MIXER_RECLEV:
	    return *arg = devc->recvol;
	    break;

	  default:
	    return *arg = 0;
	    break;
	  }
    }
  else
    return *arg = 0;

}

static mixer_driver_t cmi8788_mixer_driver = {
  cmi8788_mixer_ioctl
};

/********************Record/Play ROUTING Control *************************/

static int
cmi8788_ext (int dev, int ctrl, unsigned int cmd, int value)
{
/*
 * Access function for CMPCI mixer extension bits
 */
  cmi8788_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case 0:		/* Record Monitor */
	  value = (INB (devc->osdev, REC_MONITOR) & 0x1) ? 1 : 0;
	  break;
	case 1:		/* SPDIFIN Monitor */
	  value = (INB (devc->osdev, REC_MONITOR) & 0x10) ? 1 : 0;
	  break;
	case 2:		/* Record source select */
	  switch (devc->model)
		{
		  case SUBID_XONAR_DS:		
	  		value = (INW (devc->osdev, GPIO_DATA) & 0x40) ? 1 : 0;
			break;
		  case SUBID_XONAR_D1:
		  case SUBID_XONAR_DX:
		  case SUBID_XONAR_STX:
		  case SUBID_XONAR_ST:
			value = (INW (devc->osdev, GPIO_DATA) & 0x100) ? 1 : 0;
			break;
		  default: 
	  		value = (ac97_read (devc, 0x72) & 0x1) ? 1 : 0;
			break;
	        }
	  break;
	case 3:		/* Speaker Spread - check bit15 to see if it's set */
	  value = (INW (devc->osdev, PLAY_ROUTING) & 0x8000) ? 0 : 1;
	  break;
	case 4:		/* SPDIF IN->OUT Loopback */
	  value = (INW (devc->osdev, SPDIF_FUNC) & 0x4) ? 1 : 0;
	  break;
	case 5:
	  value = (INW (devc->osdev, GPIO_DATA) & 0x80) ? 1 : 0;
	  break;
	case 6:
	  if (!(INW (devc->osdev, GPIO_DATA) & 0x80))
		value = 0;
	  else if (INW (devc->osdev, GPIO_DATA) & 0x02)
		value = 1;
	  else
		value = 2;
	  break;
	case 7:
	  value = devc->mute;
	  break;
	
	default:
	  return OSS_EINVAL;
	  break;
	}

      return value;
    }


  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case 0:		/* Record Monitor */
	  if (value)
	    OUTB (devc->osdev, INB (devc->osdev, REC_MONITOR) | 0xF,
		  REC_MONITOR);
	  else
	    OUTB (devc->osdev, INB (devc->osdev, REC_MONITOR) & ~0xF,
		  REC_MONITOR);
	  break;

	case 1:		/* SPDIFIN Monitor */
	  if (value)
	    OUTB (devc->osdev, INB (devc->osdev, REC_MONITOR) | 0x10,
		  REC_MONITOR);
	  else
	    OUTB (devc->osdev, INB (devc->osdev, REC_MONITOR) & ~0x10,
		  REC_MONITOR);
	  break;

	case 2:
	  if (value)
          {
	     switch (devc->model)
		{
		case SUBID_XONAR_DS:
			OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | 0x40, GPIO_DATA);
			break;
		case SUBID_XONAR_D1:
		case SUBID_XONAR_DX:
		case SUBID_XONAR_STX:
		case SUBID_XONAR_ST:
			OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | 0x100, GPIO_DATA);
			break;
		}
	     ac97_write(devc, 0x72, ac97_read(devc, 0x72) | 0x1);
	  }
	  else
	  {
             switch (devc->model)
                {
                case SUBID_XONAR_DS:
                        OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) & ~0x40, GPIO_DATA);
                        break;
                case SUBID_XONAR_D1:
                case SUBID_XONAR_DX:
		case SUBID_XONAR_STX:
		case SUBID_XONAR_ST:
                        OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) & ~0x100, GPIO_DATA);
                        break;
                }

	     ac97_write(devc, 0x72, ac97_read(devc, 0x72) & ~0x1);
	  }
	  break;

	case 3:		/* Speaker Spread (clone front to all channels) */
	  if (value)
	    OUTW (devc->osdev, INW (devc->osdev, PLAY_ROUTING) & 0x00FF,
		  PLAY_ROUTING);
	  else
	    OUTW (devc->osdev,
		  (INW (devc->osdev, PLAY_ROUTING) & 0x00FF) | 0xE400,
		  PLAY_ROUTING);
	  break;

	case 4:		/* SPDIF IN->OUT Loopback */
	  if (value)
	    OUTW (devc->osdev, INW (devc->osdev, SPDIF_FUNC) | 0x4,
		  SPDIF_FUNC);
	  else
	    OUTW (devc->osdev, INW (devc->osdev, SPDIF_FUNC) & ~0x4,
		  SPDIF_FUNC);
	  break;

	case 5:
          if (value)
		OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | 0x80, GPIO_DATA) ;
	  else
		OUTW(devc->osdev, (INW(devc->osdev, GPIO_DATA) & ~0x80), GPIO_DATA);
          break;

	case 6:
          switch (value)
	  {
		/* speaker (line) output */
		case 0: OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) & ~(0x80|0x02), GPIO_DATA); 	
			 break;
		/* rear headphone output */
		case 1: OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | (0x80|0x02), GPIO_DATA);
			 break;
		case 2: OUTW (devc->osdev, (INW(devc->osdev, GPIO_DATA) | 0x80) & ~0x02, GPIO_DATA);
			break;

	  }
          break;

	case 7:
	  /* muting for Xonar ST/STX on PCM1796 */
          if (value)
	    {
		i2c_write (devc, XONAR_ST_FRONTDAC, 18, 0x01|0x30|0x80);
		devc->mute=1;
	    }
	  else
	    {
		i2c_write (devc, XONAR_ST_FRONTDAC, 18, 0x00|0x30|0x80);
		devc->mute=0;
	    }
          break;

	default:
	  return OSS_EINVAL;
	  break;
	}
      return (value);
    }
  return OSS_EINVAL;
}


/********************SPDIFOUT Control *************************/
int
spdifout_ctl (int dev, int ctrl, unsigned int cmd, int value)
{
  int tmp = 0;
  cmi8788_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case SPDIFOUT_ENABLE:	/* SPDIF OUT */
	  value = (INL (devc->osdev, SPDIF_FUNC) & 0x2) ? 1 : 0;
	  break;

	case SPDIFOUT_PRO:	/* Consumer/PRO */
	  value = (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & 0x1) ? 1 : 0;
	  break;

	case SPDIFOUT_AUDIO:	/* PCM/AC3 */
	  value = (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & 0x2) ? 1 : 0;
	  break;

	case SPDIFOUT_COPY:	/* Copy Prot */
	  value = (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & 0x4) ? 1 : 0;
	  break;

	case SPDIFOUT_PREEMPH:	/* Pre emphasis */
	  value = (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & 0x8) ? 1 : 0;
	  break;

	case SPDIFOUT_RATE:	/* Sampling Rate */
	  tmp = (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & 0xf000);
	  switch (tmp)
	    {
	    case 0x0000:	/* 44100 */
	      value = 0;
	      break;
	    case 0x2000:	/* 48000 */
	      value = 1;
	      break;
	    case 0x3000:	/* 32000 */
	      value = 2;
	      break;
	    case 0x8000:	/* 88200 */
	      value = 3;
	      break;
	    case 0xA000:	/* 96000 */
	      value = 4;
	      break;
	    case 0xB000:	/* 64000 */
	      value = 5;
	      break;
	    case 0xC000:	/* 176400 */
	      value = 6;
	      break;
	    case 0xE000:	/* 192000 */
	      value = 7;
	      break;
	    default:
	      cmn_err (CE_WARN, "unsupported SPDIF F/S rate\n");
	      break;
	    }
	  break;

	case SPDIFOUT_VBIT:	/* V Bit */
	  value = (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & 0x10000) ? 1 : 0;
	  break;

	case SPDIFOUT_ADC:	/* Analog In to SPDIF Out */
	  value = (INW (devc->osdev, PLAY_ROUTING) & 0x80) ? 1 : 0;
	  break;

	default:
	  break;
	}

      return value;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case SPDIFOUT_ENABLE:	/* Enable SPDIF OUT */
	  if (value)
	    OUTL (devc->osdev, INL (devc->osdev, SPDIF_FUNC) | 0x2,
		  SPDIF_FUNC);
	  else
	    OUTL (devc->osdev, INL (devc->osdev, SPDIF_FUNC) & ~0x2,
		  SPDIF_FUNC);
	  break;

	case SPDIFOUT_PRO:	/* consumer/pro audio */
	  if (value)
	    OUTL (devc->osdev, INL (devc->osdev, SPDIFOUT_CHAN_STAT) | 0x1,
		  SPDIFOUT_CHAN_STAT);
	  else
	    OUTL (devc->osdev, INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0x1,
		  SPDIFOUT_CHAN_STAT);
	  break;

	case SPDIFOUT_AUDIO:	/* PCM/AC3 */
	  if (value)
	    OUTL (devc->osdev, INL (devc->osdev, SPDIFOUT_CHAN_STAT) | 0x2,
		  SPDIFOUT_CHAN_STAT);
	  else
	    OUTL (devc->osdev, INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0x2,
		  SPDIFOUT_CHAN_STAT);
	  break;

	case SPDIFOUT_COPY:	/* copy prot */
	  if (value)
	    OUTL (devc->osdev, INL (devc->osdev, SPDIFOUT_CHAN_STAT) | 0x4,
		  SPDIFOUT_CHAN_STAT);
	  else
	    OUTL (devc->osdev, INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0x4,
		  SPDIFOUT_CHAN_STAT);
	  break;

	case SPDIFOUT_PREEMPH:	/* preemphasis */
	  if (value)
	    OUTL (devc->osdev, INL (devc->osdev, SPDIFOUT_CHAN_STAT) | 0x8,
		  SPDIFOUT_CHAN_STAT);
	  else
	    OUTL (devc->osdev, INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0x8,
		  SPDIFOUT_CHAN_STAT);
	  break;

	case SPDIFOUT_RATE:	/* Frequency */
	  switch (value)
	    {
	    case 0:		/* 44100 */
	      OUTL (devc->osdev,
		    (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF0000) | 0x0,
		    SPDIFOUT_CHAN_STAT);
	      break;
	    case 1:		/* 48000 */
	      OUTL (devc->osdev,
		    (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF0000) |
		    0x2000, SPDIFOUT_CHAN_STAT);
	      break;
	    case 2:		/* 32000 */
	      OUTL (devc->osdev,
		    (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF0000) |
		    0x3000, SPDIFOUT_CHAN_STAT);
	      break;
	    case 3:		/* 88000 */
	      OUTL (devc->osdev,
		    (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF0000) |
		    0x8000, SPDIFOUT_CHAN_STAT);
	      break;
	    case 4:		/* 96000 */
	      OUTL (devc->osdev,
		    (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF0000) |
		    0xA000, SPDIFOUT_CHAN_STAT);
	      break;
	    case 5:		/* 64000 */
	      OUTL (devc->osdev,
		    (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF0000) |
		    0xB000, SPDIFOUT_CHAN_STAT);
	      break;
	    case 6:		/* 176400 */
	      OUTL (devc->osdev,
		    (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF0000) |
		    0xC000, SPDIFOUT_CHAN_STAT);
	      break;
	    case 7:		/* 192000 */
	      OUTL (devc->osdev,
		    (INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0xF0000) |
		    0xE000, SPDIFOUT_CHAN_STAT);
	      break;

	    default:
	      break;
	    }
	  break;

	case SPDIFOUT_VBIT:	/* V Bit */
	  if (value)
	    OUTL (devc->osdev,
		  INL (devc->osdev, SPDIFOUT_CHAN_STAT) | 0x10000,
		  SPDIFOUT_CHAN_STAT);
	  else
	    OUTL (devc->osdev,
		  INL (devc->osdev, SPDIFOUT_CHAN_STAT) & ~0x10000,
		  SPDIFOUT_CHAN_STAT);
	  break;

	case SPDIFOUT_ADC:	/* Analog In to SPDIF Out */
	  if (value)
	    OUTW (devc->osdev, INW (devc->osdev, PLAY_ROUTING) | 0xA0,
		  PLAY_ROUTING);
	  else
	    OUTW (devc->osdev, INW (devc->osdev, PLAY_ROUTING) & ~0xE0,
		  PLAY_ROUTING);
	  break;

	default:
	  break;
	}

      return (value);
    }

  return OSS_EINVAL;
}

static int
cmi8788_mix_init (int dev)
{
  int group, parent, ctl;
  cmi8788_devc *devc = mixer_devs[dev]->hw_devc;

  if ((parent = mixer_ext_create_group (dev, 0, "EXT")) < 0)
    return parent;

/* CREATE MONITOR */
  if ((group = mixer_ext_create_group (dev, parent, "MONITOR")) < 0)
    return group;

  if ((ctl =
       mixer_ext_create_control (dev, group, 0, cmi8788_ext,
				 MIXT_ONOFF, "ANALOG", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl =
       mixer_ext_create_control (dev, group, 1, cmi8788_ext,
				 MIXT_ONOFF, "SPDIF", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

 if ((ctl =
	mixer_ext_create_control (dev, group, 2, cmi8788_ext, MIXT_ENUM,
				  "INPUTSRC", 2,
				  MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return ctl;
    mixer_ext_set_strings (dev, ctl, "Line Mic", 0);

/* Create PLAYBACK ROUTING */
  if ((group = mixer_ext_create_group (dev, parent, "ROUTING")) < 0)
    return group;

  if ((ctl =
       mixer_ext_create_control (dev, group, 3, cmi8788_ext,
				 MIXT_ONOFF, "SPREAD", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl =
       mixer_ext_create_control (dev, group, 4, cmi8788_ext,
				 MIXT_ONOFF, "SPDIF-LOOPBACK", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if (devc->model == SUBID_XONAR_D2 || devc->model == SUBID_XONAR_D2X)
    if ((ctl = mixer_ext_create_control (dev, group, 5, cmi8788_ext,
				 MIXT_ONOFF, "PCM-LOOPBACK", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
		        return ctl;

  if (devc->model == SUBID_XONAR_STX || devc->model == SUBID_XONAR_ST)

   {
    if ((ctl =
	mixer_ext_create_control (dev, group, 6, cmi8788_ext, MIXT_ENUM,
				  "OUTPUTSRC", 3,
				  MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return ctl;
    mixer_ext_set_strings (dev, ctl, "Speaker Headphone FrontHeadphone", 0);


    if ((ctl =
       mixer_ext_create_control (dev, group, 7, cmi8788_ext,
				 MIXT_ONOFF, "MUTE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
        return ctl;
   }

/* Create SPDIF OUTPUT */
  if ((group = mixer_ext_create_group (dev, 0, "SPDIF-OUT")) < 0)
    return group;

  if ((ctl =
       mixer_ext_create_control (dev, group, SPDIFOUT_ENABLE,
				 spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_ENABLE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl =
       mixer_ext_create_control (dev, group, SPDIFOUT_ADC,
				 spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_ADC/DAC", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl =
       mixer_ext_create_control (dev, group, SPDIFOUT_PRO,
				 spdifout_ctl, MIXT_ENUM,
				 "SPDOUT_Pro", 2,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "Consumer Professional", 0);

  if ((ctl =
       mixer_ext_create_control (dev, group, SPDIFOUT_AUDIO,
				 spdifout_ctl, MIXT_ENUM,
				 "SPDOUT_Audio", 2,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "Audio Data", 0);

  if ((ctl =
       mixer_ext_create_control (dev, group, SPDIFOUT_COPY,
				 spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_Copy", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl =
       mixer_ext_create_control (dev, group, SPDIFOUT_PREEMPH,
				 spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_Pre-emph", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl =
       mixer_ext_create_control (dev, group, SPDIFOUT_RATE,
				 spdifout_ctl, MIXT_ENUM,
				 "SPDOUT_Rate", 8,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  mixer_ext_set_strings (dev, ctl,
			 "44.1KHz 48KHz 32KHz 88.2KHz 96KHz 64KHz 176.4KHz 192KHz",
			 0);

  if ((ctl =
       mixer_ext_create_control (dev, group, SPDIFOUT_VBIT,
				 spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_VBit", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  return 0;
}


static void 
ac97_hwinit(cmi8788_devc *devc)
{

    /* Gpio #0 programmed as output, set CMI9780 Reg0x70 */
    ac97_write(devc, 0x70, 0x100);

    /* LI2LI,MIC2MIC; let them always on, FOE on, ROE/BKOE/CBOE off */
    ac97_write(devc, 0x62, 0x180F);

    /* change PCBeep path, set Mix2FR on, option for quality issue */
    ac97_write(devc, 0x64, 0x8043);
#if 0
   /* unmute Master Volume */
    ac97_write(devc, 0x02, 0x0);


    /* mute PCBeep, option for quality issues */
    ac97_write(devc, 0x0A, 0x8000);

    /* Record Select Control Register (Index 1Ah) */
    ac97_write(devc, 0x1A, 0x0000);

    /* set Mic Volume Register 0x0Eh umute and enable micboost */
    ac97_write(devc, 0x0E, 0x0848);

    /* set Line in Volume Register 0x10h mute */
    ac97_write(devc, 0x10, 0x8808);

    /* set CD Volume Register 0x12h mute */
    ac97_write(devc, 0x12, 0x8808);

    /* set AUX Volume Register 0x16h max */
    ac97_write(devc, 0x16, 0x0808);

    /* set record gain Register 0x1Ch to max */
    ac97_write(devc, 0x1C, 0x0F0F);
#endif
    ac97_write(devc, 0x71, 0x0001);
}

static int
init_cmi8788 (cmi8788_devc * devc)
{
  unsigned short sVal;
  unsigned short sDac;
  unsigned char bVal;
  int i, first_dev = -1, count;
  int default_vol;
/*
 * Init CMI Controller
 */
  sVal = INW (devc->osdev, CTRL_VERSION);
  if (!(sVal & 0x0008))
    {
      bVal = INB (devc->osdev, MISC_REG);
      bVal |= 0x20;
      OUTB (devc->osdev, bVal, MISC_REG);
    }

  bVal = INB (devc->osdev, FUNCTION);
  bVal |= 0x02; /* Reset codec*/
  OUTB(devc->osdev, bVal, FUNCTION);


  /* I2S to 16bit, see below. */
  sDac = 0x010A; 
 
  /* Non-generic DAC initialization */
  switch(devc->model)
  {
    case SUBID_XONAR_D1:
    case SUBID_XONAR_DX:
    case SUBID_XONAR_D2:
    case SUBID_XONAR_D2X:
    case SUBID_XONAR_STX:
    case SUBID_XONAR_DS:
      /* Must set master clock. */
      sDac |= XONAR_MCLOCK_256;
      break;
    case SUBID_XONAR_ST:
      sDac |= XONAR_MCLOCK_512;
  }

  /* Setup I2S to use 16bit instead of 24Bit */
  OUTW (devc->osdev, sDac, I2S_MULTICH_FORMAT);
  OUTW (devc->osdev, sDac, I2S_ADC1_FORMAT);
  OUTW (devc->osdev, sDac, I2S_ADC2_FORMAT);
  OUTW (devc->osdev, sDac, I2S_ADC3_FORMAT);

  /* setup Routing regs (default vals) */
  OUTW (devc->osdev, 0xE400, PLAY_ROUTING);
  OUTB (devc->osdev, 0x00, REC_ROUTING);
  OUTB (devc->osdev, 0x00, REC_MONITOR);
  OUTB (devc->osdev, 0xE4, MONITOR_ROUTING);


  /* install the CMI8788 mixer */
  if ((devc->cmi_mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
						devc->osdev,
						devc->osdev,
						"CMedia CMI8788",
						&cmi8788_mixer_driver,
						sizeof (mixer_driver_t),
						devc)) < 0)
    {
      return 0;
    }

  mixer_devs[devc->cmi_mixer_dev]->hw_devc = devc;
  mixer_devs[devc->cmi_mixer_dev]->priority = 10;	/* Possible default mixer candidate */
  mixer_ext_set_init_fn (devc->cmi_mixer_dev, cmi8788_mix_init, 25);
  
  /* Cold reset onboard AC97 */
  OUTW (devc->osdev, 0x1, AC97_CTRL);
  count = 100;
  while ((INW (devc->osdev, AC97_CTRL) & 0x2) && (count--))
    {
      OUTW (devc->osdev, (INW (devc->osdev, AC97_CTRL) & ~0x2) | 0x2,
	    AC97_CTRL);
      oss_udelay (100);
    }
  if (!count)
    cmn_err (CE_WARN, "CMI8788 AC97 not ready\n");


  sVal = INW (devc->osdev, AC97_CTRL);

  devc->ac97_mixer_dev = devc->fp_mixer_dev = -1;

  /* check if there's an onboard AC97 codec (CODEC 0)  and install the mixer */
  if (sVal & 0x10)
    {
      /* disable CODEC0 OUTPUT */
      OUTW (devc->osdev, 0, /* INW (devc->osdev, AC97_OUT_CHAN_CONFIG) & ~0xFF00,*/
	    AC97_OUT_CHAN_CONFIG);

      /* enable CODEC0 INPUT */
      OUTW (devc->osdev, 0, /* INW (devc->osdev, AC97_IN_CHAN_CONFIG) | 0x0300,*/
	    AC97_IN_CHAN_CONFIG);

      devc->ac97_mixer_dev =
	ac97_install (&devc->ac97devc, "AC97 Input Mixer", ac97_read,
		      ac97_write, devc, devc->osdev);
    }

  /* check if there's an front panel AC97 codec (CODEC1) and install the mixer */
  if (sVal & 0x20)
    {
      /* enable CODEC1 OUTPUT */
      OUTW (devc->osdev, INW (devc->osdev, AC97_OUT_CHAN_CONFIG) | 0x0033,
	    AC97_OUT_CHAN_CONFIG);
      /* enable CODEC1 INPUT */
      OUTW (devc->osdev, INW (devc->osdev, AC97_IN_CHAN_CONFIG) | 0x0033,
	    AC97_IN_CHAN_CONFIG);

      devc->fp_mixer_dev =
	ac97_install (&devc->fp_ac97devc, "AC97 Mixer (Front Panel)",
		      fp_ac97_read, fp_ac97_write, devc, devc->osdev);

      if (devc->fp_mixer_dev >= 0)
	{
	  /* enable S/PDIF */
	  devc->fp_ac97devc.spdif_slot = SPDIF_SLOT34;
	  ac97_spdifout_ctl (devc->fp_mixer_dev, SPDIFOUT_ENABLE,
			     SNDCTL_MIX_WRITE, 1);
	}
    }



    switch(devc->model) {
            case SUBID_XONAR_D1:
            case SUBID_XONAR_DX:
                    /*GPIO8 = 0x100 controls mic/line-in */
                    /*GPIO0 = 0x001controls output */
                    /*GPIO2/3 = 0x00C codec output control*/

                    /* setup for i2c communication mode */
                    OUTB(devc->osdev, INB (devc->osdev, FUNCTION) | 0x40, FUNCTION);
                    /* setup GPIO direction */
                    OUTW(devc->osdev, INW(devc->osdev, GPIO_CONTROL) | 0x010D, GPIO_CONTROL);
                    /* setup GPIO pins */
                    OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | 0x0101, GPIO_DATA);

                    /* init the front and rear dacs */
                    cs4398_init(devc, XONAR_DX_FRONTDAC);
                    cs4362a_init(devc, XONAR_DX_SURRDAC);
  		    /* initialize the codec 0 */
  		    ac97_hwinit(devc);
                    break;


            case SUBID_XONAR_D2:
            case SUBID_XONAR_D2X:
                    /*GPIO7 = 0x0080 controls mic/line-in */
                    /*GPIO8 = 0x0100 controls output */
                    /*GPIO2/3 = 0x000C codec output control*/

                    /* setup for spi communication mode */
                    OUTB(devc->osdev, (INB (devc->osdev, FUNCTION) & ~0x40) | 0x80, FUNCTION);
                    /* setup the GPIO direction */
                    OUTW(devc->osdev, INW(devc->osdev, GPIO_CONTROL) | 0x018C, GPIO_CONTROL);
	            /* setup GPIO Pins */
	            OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | 0x0100, GPIO_DATA);

                    /* for all 4 codecs: unmute, set to 24Bit SPI */
		    for (i = 0; i < 4; ++i) {
			spi_write(devc, xd2_codec_map[i], 16, mix_scale(75,8)); /* left vol*/
			spi_write(devc, xd2_codec_map[i], 17, mix_scale(75,8)); /* right vol */
			spi_write(devc, xd2_codec_map[i], 18, 0x00| 0x30 | 0x80); /* unmute/24LSB/ATLD */
		    }
  		    /* initialize the codec 0 */
  		    ac97_hwinit(devc);
		    break;

 		case SUBID_XONAR_STX:
            	    /*GPIO0 = Antipop control */
                    /*GPIO1 = frontpanel h/p control*/
                    /*GPIO7 = 0x0080 controls analog out*/
                    /*GPIO8 = 0x0100 controls mic/line in*/
                    /*GPIO2/3 = 0x000C codec input control*/

                    /* setup for i2c communication mode */
                    OUTB(devc->osdev, INB (devc->osdev, FUNCTION) | 0x40, FUNCTION);

                    /* setup the GPIO direction control register */
                    OUTW(devc->osdev, INW(devc->osdev, GPIO_CONTROL) | 0x018F, GPIO_CONTROL);
                    /* setup GPIO pins mic/output */
                    OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | 0x111, GPIO_DATA);

                    OUTW(devc->osdev, INW(devc->osdev, I2C_CTRL)|0x0100, I2C_CTRL);

                    /* initialize the PCM1796 DAC */
                    i2c_write(devc, XONAR_STX_FRONTDAC, 20, 0x40); /*OS_64*/
                    i2c_write(devc, XONAR_STX_FRONTDAC, 20, 0); /*OS_64*/
                    i2c_write(devc, XONAR_STX_FRONTDAC, 16, mix_scale(75,8));
                    i2c_write(devc, XONAR_STX_FRONTDAC, 17, mix_scale(75,8));
                    i2c_write(devc, XONAR_STX_FRONTDAC, 18, 0x00|0x30|0x80); /*unmute, 24LSB, ATLD */
                    i2c_write(devc, XONAR_STX_FRONTDAC, 19, 0); /*ATS1/FLT_SHARP*/

                    /* initialize the codec 0 */
                    ac97_hwinit(devc);
		    break;

	    case SUBID_XONAR_ST:
	    	    /*GPIO0 = enable output*/
                    /*GPIO1 = frontpanel h/p control*/
                    /*GPIO7 = 0x0080 controls analog out line/HP*/
                    /*GPIO8 = 0x0100 controls mic/line in*/
                    /*GPIO2/3 = 0x000C codec input control*/

                    /* setup for i2c communication mode */
                    OUTB(devc->osdev, INB (devc->osdev, FUNCTION) | 0x40, FUNCTION);

                    /* setup the GPIO direction control register */
                    OUTW(devc->osdev, INW(devc->osdev, GPIO_CONTROL) | 0x01FF, GPIO_CONTROL);
                    /* setup GPIO pins mic/output */
                    OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | 0x101, GPIO_DATA);
                    OUTW(devc->osdev, INW(devc->osdev, I2C_CTRL)|0x0100, I2C_CTRL);
                     /* initialize the CS2000 clock chip */
		    cs2000_init (devc, XONAR_ST_CLOCK);

                    /* initialize the PCM1796 DAC */
                    i2c_write(devc, XONAR_ST_FRONTDAC, 20, 0); /*normal*/
                    i2c_write(devc, XONAR_ST_FRONTDAC, 18, 0x00|0x30|0x80); /*unmute, 24LSB, ATLD */
                    i2c_write(devc, XONAR_ST_FRONTDAC, 16, mix_scale(75,8));
                    i2c_write(devc, XONAR_ST_FRONTDAC, 17, mix_scale(75,8));
                    i2c_write(devc, XONAR_ST_FRONTDAC, 19, 0); /*ATS1/FLT_SHARP*/

                    /* initialize the codec 0 */
                    ac97_hwinit(devc);

		    break;

	   case SUBID_XONAR_DS:
			/* GPIO 8 = 1 output enabled 0 mute */
			/* GPIO 7 = 1 lineout enabled 0 mute */
			/* GPIO 6 = 1 mic select 0 line-in select */
			/* GPIO 4 = 1 FP Headphone plugged in */
			/* GPIO 3 = 1 FP Mic plugged in */

                    /* setup for spi communication mode */
                    OUTB(devc->osdev, (INB (devc->osdev, FUNCTION) & ~0x40)|0x32, FUNCTION);
                    /* setup the GPIO direction */
                    OUTW(devc->osdev, INW(devc->osdev, GPIO_CONTROL) | 0x1D0, GPIO_CONTROL);
	            /* setup GPIO Pins */
	            OUTW(devc->osdev, INW(devc->osdev, GPIO_DATA) | 0x1D0, GPIO_DATA);

		    spi_write(devc, XONAR_DS_FRONTDAC, 0x17, 0x1); /* reset */
	 	    spi_write(devc, XONAR_DS_FRONTDAC, 0x7, 0x90); /* dac control */
	 	    spi_write(devc, XONAR_DS_FRONTDAC, 0x8, 0); /* unmute */
	 	    spi_write(devc, XONAR_DS_FRONTDAC, 0xC, 0x22 ); /* powerdown hp */
	 	    spi_write(devc, XONAR_DS_FRONTDAC, 0xD, 0x8); /* powerdown hp */
	 	    spi_write(devc, XONAR_DS_FRONTDAC, 0xA, 0x1); /* LJust/16bit*/
	 	    spi_write(devc, XONAR_DS_FRONTDAC, 0xB, 0x1); /* LJust/16bit*/

	 	    spi_write(devc, XONAR_DS_SURRDAC, 0x1f, 1); /* reset */
	 	    spi_write(devc, XONAR_DS_SURRDAC, 0x3, 0x1|0x20); /* LJust/24bit*/
		   break;

	   default:
		   /* SPI default for anything else, including the */
		   OUTB(devc->osdev, (INB (devc->osdev, FUNCTION) & ~0x40) | 0x80, FUNCTION);
		   break;
  }

  /* check if MPU401 is enabled in MISC register */
  if (INB (devc->osdev, MISC_REG) & 0x40)
    attach_mpu (devc);

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      cmi8788_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE;
      int fmt = AFMT_S16_LE;

      switch (i)
	{
	case 0:
	  sprintf (tmp_name, "%s (MultiChannel)", devc->chip_name);
	  caps |= ADEV_DUPLEX;
	  fmt |= AFMT_S32_LE;
	  portc->dac_type = ADEV_MULTICH;
	  switch(devc->model)
  	  {
		case SUBID_XONAR_D1:
		case SUBID_XONAR_DX:
		case SUBID_XONAR_D2:
		case SUBID_XONAR_D2X:
		case SUBID_XONAR_STX:
		case SUBID_XONAR_ST:
			portc->adc_type = ADEV_I2SADC2;
      			break;
		case SUBID_XONAR_DS:
			portc->adc_type = ADEV_I2SADC1;
			break;
		default: 
			portc->adc_type = ADEV_I2SADC1;
      			OUTB (devc->osdev, INB (devc->osdev, REC_ROUTING) | 0x18, REC_ROUTING);
			break;
  	  }

	  portc->min_rate = 32000;
	  portc->max_rate = 192000;
	  portc->min_chan = 2;
	  portc->max_chan = 8;
	  break;

	case 1:
	  sprintf (tmp_name, "%s (Multichannel)", devc->chip_name);
	  caps |= ADEV_DUPLEX | ADEV_SHADOW;
	  fmt |= AFMT_S32_LE;
	  portc->dac_type = ADEV_MULTICH;
          switch(devc->model)
          {
                case SUBID_XONAR_D1:
                case SUBID_XONAR_DX:
                case SUBID_XONAR_D2:
                case SUBID_XONAR_D2X:
                case SUBID_XONAR_STX:
                case SUBID_XONAR_ST:
                        portc->adc_type = ADEV_I2SADC2;
                        break;
                case SUBID_XONAR_DS:
                        portc->adc_type = ADEV_I2SADC1;
                        break;
                default:
                        portc->adc_type = ADEV_I2SADC1;
      			OUTB (devc->osdev, INB (devc->osdev, REC_ROUTING) | 0x18, REC_ROUTING);
                        break;
          }

	  portc->min_rate = 32000;
	  portc->max_rate = 192000;
	  portc->min_chan = 2;
	  portc->max_chan = 8;
	  break;

	case 2:
	  /* if there is no front panel AC97, then skip the device */
	  if (devc->fp_mixer_dev == -1)
	    continue;

	  sprintf (tmp_name, "%s (Front Panel)", devc->chip_name);
	  caps |=
	    ADEV_DUPLEX | ADEV_16BITONLY | ADEV_STEREOONLY | ADEV_SPECIAL;
	  fmt |= AFMT_AC3;
	  portc->dac_type = ADEV_FRONTPANEL;
	  portc->adc_type = ADEV_I2SADC2;
      	  OUTB (devc->osdev, INB (devc->osdev, REC_ROUTING) | 0x18, REC_ROUTING);
	  portc->min_rate = 8000;
	  portc->max_rate = 48000;
	  portc->min_chan = 2;
	  portc->max_chan = 2;
	  break;

	case 3:
	  sprintf (tmp_name, "%s (SPDIF)", devc->chip_name);
	  caps |= ADEV_DUPLEX | ADEV_STEREOONLY | ADEV_SPECIAL;
	  fmt |= AFMT_AC3 | AFMT_S32_LE;
	  portc->dac_type = ADEV_SPDIF;
	  portc->adc_type = ADEV_I2SADC3;
	  portc->min_rate = 32000;
	  portc->max_rate = 192000;
	  portc->min_chan = 2;
	  portc->max_chan = 2;
	  break;
	}

      if ((portc->audiodev =
	   oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION, devc->osdev,
				 devc->osdev, tmp_name,
				 &cmi8788_audio_driver,
				 sizeof (audiodrv_t), caps, fmt,
				 devc, -1)) < 0)
	{
	  return 0;
	}
      else
	{
	  if (first_dev == -1)
	    first_dev = portc->audiodev;
	  audio_engines[portc->audiodev]->portc = portc;
	  audio_engines[portc->audiodev]->rate_source = first_dev;
	  if (caps & ADEV_FIXEDRATE)
	    {
	      audio_engines[portc->audiodev]->min_rate = 48000;
	      audio_engines[portc->audiodev]->max_rate = 48000;
	      audio_engines[portc->audiodev]->fixed_rate = 48000;
	    }
	  audio_engines[portc->audiodev]->min_rate = portc->min_rate;
	  audio_engines[portc->audiodev]->max_rate = portc->max_rate;

	  audio_engines[portc->audiodev]->caps |= DSP_CAP_FREERATE;
	  audio_engines[portc->audiodev]->min_channels = portc->min_chan;
	  audio_engines[portc->audiodev]->max_channels = portc->max_chan;
	  portc->open_mode = 0;
	  portc->audio_enabled = 0;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, portc->audiodev, -1, 0);
#endif
	}
    }

  /*
   * Setup the default volumes to 90%
   */
  
  devc->mute = 0;

  default_vol = mix_scale(90,8)<<8|mix_scale(90,8);

  devc->playvol[0] =
    cmi8788_mixer_ioctl (devc->cmi_mixer_dev, first_dev,
			 MIXER_WRITE (SOUND_MIXER_PCM), &default_vol);
  devc->playvol[1] =
    cmi8788_mixer_ioctl (devc->cmi_mixer_dev, first_dev,
			 MIXER_WRITE (SOUND_MIXER_REARVOL), &default_vol);
  devc->playvol[2] =
    cmi8788_mixer_ioctl (devc->cmi_mixer_dev, first_dev,
			 MIXER_WRITE (SOUND_MIXER_CENTERVOL), &default_vol);
  devc->playvol[3] =
    cmi8788_mixer_ioctl (devc->cmi_mixer_dev, first_dev,
			 MIXER_WRITE (SOUND_MIXER_SIDEVOL), &default_vol);

  return 1;
}

int
oss_cmi878x_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  unsigned short sub_vendor, sub_id;
  int err;
  cmi8788_devc *devc;

  DDB (cmn_err (CE_CONT, "Entered CMEDIA CMI8788 attach routine\n"));
  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);
  pci_read_config_word (osdev, PCI_SUBSYSTEM_VENDOR_ID, &sub_vendor);
  pci_read_config_word (osdev, PCI_SUBSYSTEM_ID, &sub_id);

  if (vendor != CMEDIA_VENDOR_ID || device != CMEDIA_CMI8788)
    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  DDB (cmn_err (CE_WARN, "CMI8788 I/O base %04x\n", pci_ioaddr));

  if (pci_ioaddr == 0)
    {
      cmn_err (CE_WARN, "I/O address not assigned by BIOS.\n");
      return 0;
    }

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS (%d).\n", pci_irq_line);
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->irq = pci_irq_line;

  /* Map the IO Base address */
  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);

  /* Remove I/O space marker in bit 0. */
  devc->base &= ~3;

  /* set the PCI_COMMAND register to master mode */
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  if (device == CMEDIA_CMI8788)
    {
      /* Detect Xonar device */
      if(sub_vendor == ASUS_VENDOR_ID)
      {
        switch(sub_id)
        {
          case SUBID_XONAR_D1:
            devc->chip_name = "Asus Xonar D1 (AV100)";
            break;
          case SUBID_XONAR_DX:
            devc->chip_name = "Asus Xonar DX (AV100)";
            break;
          case SUBID_XONAR_D2:
            devc->chip_name = "Asus Xonar D2 (AV200)";
            break;
          case SUBID_XONAR_D2X:
            devc->chip_name = "Asus Xonar D2X (AV200)";
            break;
          case SUBID_XONAR_STX:
            devc->chip_name = "Asus Xonar Essence STX (AV100)";
            break;
          case SUBID_XONAR_ST:
            devc->chip_name = "Asus Xonar Essence ST (AV100)";
            break;
	  case SUBID_XONAR_DS:
	    devc->chip_name = "Asus Xonar DS (AV66)";
	    break;
          default:
            devc->chip_name = "Asus Xonar (unknown)";
            sub_id = SUBID_GENERIC;
            break;
        }
        devc->model = sub_id;
      }
      else
      {
        /* If not one of the above, regular. */
        devc->model = MDL_CMI8788;
        devc->chip_name = "CMedia CMI8788";
      }
    }
  else
    {
      cmn_err (CE_WARN, "Unknown CMI8788 model\n");
      return 0;
    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);
  MUTEX_INIT (devc->osdev, devc->dac_mutex, MH_DRV + 2);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, cmi8788intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d, err=%d\n", pci_irq_line, err);
      return 0;
    }

  return init_cmi8788 (devc);	/* Detected */
}

int
oss_cmi878x_detach (oss_device_t * osdev)
{
  cmi8788_devc *devc = (cmi8788_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  if (devc->mpu_attached)
    unload_cmi8788uart (devc);

  oss_unregister_interrupts (devc->osdev);

  UNMAP_PCI_IOADDR (devc->osdev, 0);
  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  MUTEX_CLEANUP (devc->dac_mutex);

  oss_unregister_device (osdev);
  return 1;
}
