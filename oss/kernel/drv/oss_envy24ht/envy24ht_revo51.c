/*
 * Purpose: Low level routines for M Audio Revolution 5.1
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

#include "oss_envy24ht_cfg.h"

#include "spdif.h"
#include "envy24ht.h"

static char channel_names[4][10] = {
  "front",
  "c/l",
  "surround",
  "headph"
};

#define BIT(x) (1<<(x))
#define BIT3 BIT(3)

/*----- SPI bus for CODEC communication. */
/* */
#define SPI_CLK 1		/* Clock output to CODEC's, rising edge clocks data. */
#define SPI_DOUT 3		/* Data output to the CODEC. */
#define SPI_CS0n (1<<4)		/* Selects first chip. */
#define SPI_CS1n (1<<5)		/* Selects second chip. */
#define SPI_CC_AK4358 0x02	/* C1:C0 for ak4358. */
#define SPI_CC_AK5365 0x02	/* C1:C0 for ak5365. */
#define WRITEMASK 	0xffff
/*----- Revolution defines. */
/* */
#define REVO51_AK5365 (1)	/* iDevice value for AK5365 A/D. */
#define REVO51_AK4358 (2)	/* iDevice value for AK4358 D/A. */

static unsigned int
GpioReadAll (envy24ht_devc * devc)
{
  return INW (devc->osdev, devc->ccs_base + 0x14);
}

static void
GpioWriteAll (envy24ht_devc * devc, unsigned int data)
{
  OUTW (devc->osdev, 0xffff, devc->ccs_base + 0x18);	/* GPIO direction */
  OUTW (devc->osdev, 0x0000, devc->ccs_base + 0x16);	/* GPIO write mask */

  OUTW (devc->osdev, data, devc->ccs_base + 0x14);
}

static void
GpioWrite (envy24ht_devc * devc, int pos, int bit)
{
  int data = GpioReadAll (devc);

  bit = (bit != 0);

  data &= ~(1 << pos);
  data |= (bit << pos);

  GpioWriteAll (devc, data);
}

void
REVO51_Assert_CS (envy24ht_devc * devc, int iDevice)
/*
*****************************************************************************
* Assert chip select to specified GPIO-connected device.
* iDevice: REVO51_AK5365=ADC, REVO51_AK4358=DAC.
****************************************************************************/
{
  unsigned int dwGPIO;		/* Current GPIO's. */
  dwGPIO = GpioReadAll (devc);	/* Read current GPIO's. */
  dwGPIO |= (SPI_CS0n | SPI_CS1n);	/* Reset CS bits. */
  switch (iDevice)		/* Select CS#. */
    {
    case REVO51_AK4358:
      dwGPIO &= ~SPI_CS0n;
      break;			/* DAC */
    case REVO51_AK5365:
      dwGPIO &= ~SPI_CS1n;
      break;			/* ADC */
    default:
      break;
    }
  GpioWriteAll (devc, dwGPIO);	/* Write hardware. */
}

void
REVO51_DeAssert_CS (envy24ht_devc * devc)
/*
*****************************************************************************
* De-Assert all chip selects.
****************************************************************************/
{
  unsigned int dwGPIO = GpioReadAll (devc);	/* Current GPIO's. */
  dwGPIO |= (SPI_CS0n | SPI_CS1n);	/* Clear CS bits. */
  GpioWriteAll (devc, dwGPIO);	/* Write back to hardware. */
}

/*#define _delay()	oss_udelay(1) */
#define _delay()	{}

void
REVO51_WriteSpiAddr (envy24ht_devc * devc, int iDevice, unsigned char bReg)
/*
*****************************************************************************
* Write the address byte part of the SPI serial stream.
* iDevice: REVO51_AK4358=DAC, REVO51_AK5365=ADC, etc.
****************************************************************************/
{
  unsigned char bHdr;
  unsigned char bNum;
/* Built 8-bit packet header: C1,C0,R/W,A4,A3,A2,A1,A0. */
/* */
  switch (iDevice)
    {
    case REVO51_AK4358:
      bHdr = SPI_CC_AK4358 << 6;
      break;
    case REVO51_AK5365:
      bHdr = SPI_CC_AK5365 << 6;
      break;
    default:
      bHdr = 0;
      break;
    }
  bHdr = bHdr | 0x20 | (bReg & 0x1F);	/* "write" + address. */
/* Write header to SPI. */
/* */
  for (bNum = 0; bNum < 8; bNum++)
    {
      GpioWrite (devc, SPI_CLK, 0);	/* Drop clock low. */
      _delay ();
      GpioWrite (devc, SPI_DOUT, 0x080 & bHdr);	/* Write data bit. */
      _delay ();
      GpioWrite (devc, SPI_CLK, 1);	/* Raise clock. */
      _delay ();
      bHdr <<= 1;		/* Next bit. */
    }
}

void
REVO51_WriteSpiReg (envy24ht_devc * devc, int iDevice, unsigned char bReg,
		    unsigned char bData)
/*
*****************************************************************************
* Writes one register in specified CHIP.
* devc = PCI slot code of specific board.
* iDevice: REVO51_AK4358=DAC, REVO51_AK5365=ADC, etc.
****************************************************************************/
{
  unsigned char bNum;
  GpioWrite (devc, SPI_DOUT, 0);	/* Init SPI signals. */
  GpioWrite (devc, SPI_CLK, 1);	/* */
/* Drop the chip select low. */
/* Wait at least 150 nS. */
/* */
  REVO51_Assert_CS (devc, iDevice);
  _delay ();
/* Write the address byte. */
/* */
  REVO51_WriteSpiAddr (devc, iDevice, bReg);
/* Write the data byte. */
/* */
  for (bNum = 0; bNum < 8; bNum++)
    {
      GpioWrite (devc, SPI_CLK, 0);	/* Drop clock low. */
      _delay ();
      GpioWrite (devc, SPI_DOUT, 0x080 & bData);	/* Write data bit. */
      _delay ();
      GpioWrite (devc, SPI_CLK, 1);	/* Raise clock. */
      _delay ();
      bData <<= 1;		/* Next bit. */
    }
/* De-assert chip selects. */
/* */
  REVO51_DeAssert_CS (devc);
  _delay ();
}


#define GPIO_MUTEn 22		/* Converter mute signal. */
void
REVO51_Mute (envy24ht_devc * devc, int bMute)
/*
*****************************************************************************
* Mutes all outputs if bMute=1.
****************************************************************************/
{
  if (bMute)
    {
/* Soft-mute. Delay currently unspecified, try ½ second. */
      REVO51_WriteSpiReg (devc, REVO51_AK4358, 1, 0x03);
      _delay ();
/* Switch mute transistors on. */
      GpioWrite (devc, GPIO_MUTEn, 0);
    }
  else
    {
/* Switch mute transistors off. Delay currently unspecified, try ½ second. */
      GpioWrite (devc, GPIO_MUTEn, 1);
      _delay ();
/* Release Soft-mute. */
      REVO51_WriteSpiReg (devc, REVO51_AK4358, 1, 0x01);
    }

  devc->mute = bMute;
}


void
REVO51_Set_OutAttn (envy24ht_devc * devc, unsigned char bChan, int iAttn)
/*
*****************************************************************************
* Sets the attenuation on one output channel.
* bChan = Channel number (0..7).
* Channel 0:1 = front, 2:3 = center/sub, 4:5 = rear, 6:7 = headphones.
* Registers are 0x04, 05, 06, 07, 08, 09, 0B, 0C respectively
* iAttn = Number of steps to attenuate CODEC.
* Each step equals .5dB (-127..0)
****************************************************************************/
{
  unsigned char bIndex;
  unsigned char bAttn;
  if (bChan > 7 || iAttn > 0 || iAttn < -127)	/* parameter test */
    {
      cmn_err (CE_CONT, "\ninvalid data! %d=bChan, %d=iAttn", bChan, iAttn);
      return;
    }
  if (bChan < 6)
    bIndex = 0x04 + bChan;	/* for registers 0x04..0x09 */
  else
    bIndex = 0x05 + bChan;	/* for registers 0x0B..0x0C */
  bAttn = (0x80 + (unsigned char) (iAttn + 127));	/* 7F is max volume. */
/* MSB enables attenuation. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, bIndex, bAttn);
}

static void
ADC_Set_Chan (envy24ht_devc * devc, int iChan)
/***************************************************************************
* Makes input selection for ADC.
* 0=mic, 1=line, 2=aux
****************************************************************************/
{
/*Check param */
  if (2 < iChan)
#define GPIO_SDA 6		/* SDA pin */
    cmn_err (CE_CONT, "\nInvalid Input channel parameter");
  else
    REVO51_WriteSpiReg (devc, REVO51_AK5365, 1, (unsigned char) iChan);
}

#define GPIO_SCL 7		/* SCL pin */
static void
start_bit (envy24ht_devc * devc)
/*
*****************************************************************************
* Send I2C Start Bit.
****************************************************************************/
{
  GpioWrite (devc, GPIO_SDA, 1);	/* Make sure DATA high. */
  _delay ();
  GpioWrite (devc, GPIO_SCL, 1);	/* Set clock high. */
  _delay ();
  GpioWrite (devc, GPIO_SDA, 0);	/* Falling edge indicates start bit. */
  _delay ();
  GpioWrite (devc, GPIO_SCL, 0);	/* Start clock train. */
}

static void
byte_out (envy24ht_devc * devc, unsigned char b8)
/*
*****************************************************************************
* Send current 8 input bits in B8. Msb is first written.
****************************************************************************/
{
  int bnum;			/* bit number */
  for (bnum = 7; bnum >= 0; bnum--)
    {
      GpioWrite (devc, GPIO_SDA, b8 & 0x80);	/* Set bit. */
      _delay ();		/* */
      GpioWrite (devc, GPIO_SCL, 1);	/* Set clock high. */
      _delay ();		/* */
      GpioWrite (devc, GPIO_SCL, 0);	/* Return clock low. */
      b8 = b8 << 1;		/* Next position. */
    }
/* No ACK, but we need to clock a false "9th" bit. */
  _delay ();			/* */
  GpioWrite (devc, GPIO_SCL, 1);	/* Set clock high. */
  _delay ();			/* */
  GpioWrite (devc, GPIO_SCL, 0);	/* Return clock low. */
}

static void
stop_bit (envy24ht_devc * devc)
/*
*****************************************************************************
* Send I2C Stop Bit.
****************************************************************************/
{
  GpioWrite (devc, GPIO_SDA, 0);	/* Make sure data is low. */
  _delay ();
  GpioWrite (devc, GPIO_SCL, 1);	/* Set clock high. */
  _delay ();
  GpioWrite (devc, GPIO_SDA, 1);	/* Rising edge indicates stop bit. */
}
static void
PT2258S_Write_Data (envy24ht_devc * devc, int cnt, unsigned char *aData)
/*
************************************************************************ *****
* Write an array of bytes (aData) to address VADDR in PT2258S.
* Assumes I2C Bus is inactive, i.e., SCL=1,SDA=1 and both are outputs.
* This routine waits for the byte to complete writing.
************************************************************************ ****/
{
  int i;
  start_bit (devc);		/* Send start bit. */
  byte_out (devc, (unsigned char) 0x80);	/* Set address to write data to. */
  for (i = 0; i < cnt; i++)
    byte_out (devc, aData[i]);	/* Write data to address. */
  stop_bit (devc);		/* Stop bit ends operation. */
}
static void
PT2258S_Write_Byte (envy24ht_devc * devc, unsigned char vdata)
/*
*****************************************************************************
* Write a byte VDATA to address VADDR in PT2258S.
* Assumes I2C Bus is inactive, i.e., SCL=1,SDA=1 and both are outputs.
* This routine waits for the byte to complete writing.
****************************************************************************/
{
  start_bit (devc);		/* Send start bit. */
  byte_out (devc, (unsigned char) 0x80);	/* Set address to write data to. */
  byte_out (devc, vdata);	/* Write data to address. */
  stop_bit (devc);		/* Stop bit ends operation. */
}

static void
PT2258S_Set_Attn (envy24ht_devc * devc, unsigned char bChan, int iAttn)
/*
************************************************************************ *****
* Set the attenuation of a specific channel.
* bChan = 0..5.
* iAttn = 0..-79.
************************************************************************ ****/
{
  static unsigned char bXlat[] = { 0x90, 0x50, 0x10, 0x30, 0x70, 0xB0 };
  unsigned char aAttn[2];
  aAttn[1] = (-iAttn) % 10;	/* 1's digit */
  aAttn[0] = (-iAttn) / 10;	/* 10's digit */
/* Check parameters. */
  if ((bChan > 5) || (iAttn < (-79) || iAttn > 0))
    {
      cmn_err (CE_CONT, "\nPT2258S_Set_Attn() parameter out of range!");
      return;
    }
/* Always set 10's digit, then 1's. */
  aAttn[0] += bXlat[bChan] - 0x10;
  aAttn[1] += bXlat[bChan];
  PT2258S_Write_Data (devc, 2, aAttn);
}
static void
PT2258S_Mute (envy24ht_devc * devc, int bMute)
/*
*****************************************************************************
* Mute all 6 outputs of the monitoring volume control.
* Unmuting returns volume to previous levels.
****************************************************************************/
{
  if (bMute)
    PT2258S_Write_Byte (devc, (unsigned char) 0xF9);
  else
    PT2258S_Write_Byte (devc, (unsigned char) 0xF8);
}


static void
REVO51_Set_48K_Mode (envy24ht_devc * devc)
/*
*****************************************************************************
* Sets Chip and Envy24 for 8kHz-48kHz sample rates.
****************************************************************************/
{
/* ICE MCLK = 256x. */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 2) & ~BIT3,
	devc->mt_base + 2);
/* DFS=normal, RESET. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x4E);
/* DFS=normal, NORMAL OPERATION. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x4F);
}

static void
REVO51_Set_96K_Mode (envy24ht_devc * devc)
/*
*****************************************************************************
* Sets CODEC and Envy24 for 60kHz-96kHz sample rates.
****************************************************************************/
{
/* ICE MCLK = 256x. */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 2) & ~BIT3,
	devc->mt_base + 2);
/* DFS=double-speed, RESET. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x5E);
/* DFS=double-speed, NORMAL OPERATION. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x5F);
}

static void
REVO51_Set_192K_Mode (envy24ht_devc * devc)
/*
*****************************************************************************
* Sets CODEC and Envy24 for 120kHz-192kHz sample rate.
****************************************************************************/
{
/* ICE MCLK = 128x. */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 2) | BIT3,
	devc->mt_base + 2);
  _delay ();
/*----- SET THE D/A. */
/* DFS=quad-speed, RESET. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x6E);
  _delay ();
/* DFS=quad-speed, NORMAL OPERATION. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x6F);
/*------ POWER DOWN THE A/D -- doesn't support 192K. */
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 0, 0x00);
}

static int
set_dac (envy24ht_devc * devc, int reg, int level)
{
  if (level < 0)
    level = 0;
  if (level > 0x7f)
    level = 0x7f;

  REVO51_WriteSpiReg (devc, REVO51_AK4358, reg, level | 0x80);

  return level;
}

static void
AK4358_Init (envy24ht_devc * devc)
{
/*===== AK4358 D/A initialization. Leave soft-muted. */
/* */
/* Power down, reset, normal mode. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x00);
/* Power up, reset, normal mode */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x4E);
/* Reset timing, Mode 3(I2S), disable auto clock detect, sharp roll off. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 0, 0x06);
/* Soft mute, reset timing. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 1, 0x02);
/* De-emphasis off. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 3, 0x01);
/* Max volume on all 8 channels. */
  set_dac (devc, 0x04, 0x7f);
  set_dac (devc, 0x05, 0x7f);
  set_dac (devc, 0x06, 0x7f);
  set_dac (devc, 0x07, 0x7f);
  set_dac (devc, 0x08, 0x7f);
  set_dac (devc, 0x09, 0x7f);
  set_dac (devc, 0x0b, 0x7f);
  set_dac (devc, 0x0c, 0x7f);

/* Datt mode 0, DZF non-invert, DCLK polarity 0, PCM mode, DCKS 512fs, TDM normal. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 0xA, 0x00);
/* DZF control disabled. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 0xD, 0x00);
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 0xE, 0x00);
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 0xF, 0x00);
/* Power up, normal operation. */
  REVO51_WriteSpiReg (devc, REVO51_AK4358, 2, 0x4F);
}

static void
PT2258_init (envy24ht_devc * devc)
{
/*===== PT2258 initialization. */
/* */
/* Initializes and clears register . */
  PT2258S_Write_Byte (devc, (unsigned char) 0xC0);
/*Example of 2-line command controlling all channels. */
/*This sets all channels to max volume. */
/*PT2258S_Write_Byte(devc, (unsigned char)0xE0); */
/*PT2258S_Write_Byte(devc, (unsigned char)0xD0); */
/* set volumes */
  PT2258S_Set_Attn (devc, 0, 0);
  PT2258S_Set_Attn (devc, 1, 0);
  PT2258S_Set_Attn (devc, 2, 0);
  PT2258S_Set_Attn (devc, 3, 0);
  PT2258S_Set_Attn (devc, 4, 0);
  PT2258S_Set_Attn (devc, 5, 0);
/*mute, true or false */
  PT2258S_Mute (devc, 0);
}

static void
REVO51_set_recsrc (envy24ht_devc * devc, int src)
{
  devc->recsrc = src;

  REVO51_WriteSpiReg (devc, REVO51_AK5365, 1, src);
}

static void
AK5365_Init (envy24ht_devc * devc)
{
/*===== AK5365 2-ch A/D initialization. Leave soft-muted. */
/* */
/* Power down. */
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 0, 0x00);
/* Set input to (LINE IN). */
  REVO51_set_recsrc (devc, 0);
/* Clock freq 256fs, I2S mode, mute off. */
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 2, 0x08);
/* ALC settings (ALC is disabled, so rewrite default values). */
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 3, 0x2B);
/* Neutral analog and digital gain. */
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 4, 0x7F);
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 5, 0x7F);
/* ALC settings (ALC is disabled, so rewrite default values). */
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 6, 0x28);
/* ALC settings (ALC is disabled, so rewrite default values). */
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 7, 0x89);
/* Power up. */
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 0, 0x01);
}

static void
revo51_set_rate (envy24ht_devc * devc)
{
  int tmp;

  tmp = INB (devc->osdev, devc->mt_base + 0x02);
  if (devc->speed <= 48000)
    {
      REVO51_Set_48K_Mode (devc);
      OUTB (devc->osdev, tmp & ~BIT (3), devc->mt_base + 0x02);
      return;
    }

  if (devc->speed <= 96000)
    {
      REVO51_Set_96K_Mode (devc);

      return;
    }

  REVO51_Set_192K_Mode (devc);
  OUTB (devc->osdev, tmp | BIT (3), devc->mt_base + 0x02);
}

static int
revo51_audio_ioctl (envy24ht_devc * devc, envy24ht_portc * portc, unsigned int cmd,
		    ioctl_arg arg)
{
  int value;

  switch (cmd)
    {
    case SNDCTL_DSP_GET_RECSRC_NAMES:
      return oss_encode_enum ((oss_mixer_enuminfo *) arg, "mic line aux", 0);
      break;

    case SNDCTL_DSP_GET_RECSRC:
      return *arg = devc->recsrc;
      break;

    case SNDCTL_DSP_SET_RECSRC:
      value = *arg;
      if (value < 0 || value > 2)
	return OSS_EINVAL;
      REVO51_set_recsrc (devc, value);
      return *arg = devc->recsrc;
      break;

    case SNDCTL_DSP_GET_PLAYTGT:
    case SNDCTL_DSP_SET_PLAYTGT:
      return *arg = 0;
      break;

    case SNDCTL_DSP_GET_PLAYTGT_NAMES:
      return oss_encode_enum ((oss_mixer_enuminfo *) arg, portc->name, 0);
      break;


    default:
      return OSS_EINVAL;
    }
}

static int
set_reclevel (envy24ht_devc * devc, int value)
{
  int left, right;

  left = value & 0xff;
  right = (value >> 8) & 0xff;

  if (left > 0x98)
    left = 0x98;
  if (right > 0x98)
    right = 0x98;

  value = left | (right << 8);

  devc->reclevel = value;

  REVO51_WriteSpiReg (devc, REVO51_AK5365, 4, left);
  REVO51_WriteSpiReg (devc, REVO51_AK5365, 5, right);

  return value;
}

static int
revo51_set_control (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->hw_devc;

  if (cmd == SNDCTL_MIX_READ)
    switch (ctrl)
      {
      case 0:
	return devc->mute;

      case 1:
	return devc->recsrc;

      case 2:
	return devc->reclevel;

      default:
	return OSS_EINVAL;
      }

  if (cmd == SNDCTL_MIX_WRITE)
    switch (ctrl)
      {
      case 0:
	value = !!value;
	REVO51_Mute (devc, value);
	return devc->mute;

      case 1:
	if (value < 0 || value > 2)
	  return devc->recsrc;
	REVO51_set_recsrc (devc, value);
	return devc->recsrc;

      case 2:
	return set_reclevel (devc, value);

      default:
	return OSS_EINVAL;
      }

  return OSS_EINVAL;
}

static int
revo51_set_ak4358 (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->hw_devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (ctrl < 0 || ctrl > 4)
	return OSS_EIO;

      return devc->gains[ctrl];
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      int left, right;

      left = value & 0xff;
      right = (value >> 8) & 0xff;

      switch (ctrl)
	{
	case 0:		/* Front */
	  left = set_dac (devc, 0x04, left);
	  right = set_dac (devc, 0x05, right);
	  break;

	case 1:		/* Center */
	  left = set_dac (devc, 0x06, left);
	  right = left;
	  break;

	case 2:		/* LFE */
	  left = set_dac (devc, 0x07, left);
	  right = left;
	  break;

	case 3:		/* Surround */
	  left = set_dac (devc, 0x08, left);
	  left = set_dac (devc, 0x09, right);
	  break;

	case 4:		/* Headphones */
	  left = set_dac (devc, 0x0b, left);
	  left = set_dac (devc, 0x0c, right);
	  break;

	default:
	  return OSS_EINVAL;
	}

      value = left | (right << 8);
      return devc->gains[ctrl] = value;
    }

  return OSS_EINVAL;
}

static int
set_mongain (envy24ht_devc * devc, int reg, int value)
{
  if (value < 0)
    value = 0;
  if (value > 79)
    value = 79;

  PT2258S_Set_Attn (devc, reg, value - 79);
  return value;
}

static int
revo51_set_monitor (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->hw_devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (ctrl < 0 || ctrl > 3)
	return OSS_EIO;

      return devc->monitor[ctrl];
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      int left, right;

      left = value & 0xff;
      right = (value >> 8) & 0xff;

      switch (ctrl)
	{
	case 0:		/* Mic */
	  left = set_mongain (devc, 0, left);
	  right = set_mongain (devc, 1, right);
	  break;

	case 1:		/* Line */
	  left = set_mongain (devc, 2, left);
	  right = set_mongain (devc, 3, right);
	  break;

	case 2:		/* Aux */
	  left = set_mongain (devc, 4, left);
	  right = set_mongain (devc, 5, right);
	  break;

	case 3:		/* Mute */
	  value = !!value;
	  PT2258S_Mute (devc, value);
	  return devc->monitor[3] = value;
	  break;

	default:
	  return OSS_EINVAL;
	}

      value = left | (right << 8);
      return devc->monitor[ctrl] = value;
    }

  return OSS_EINVAL;
}

 /*ARGSUSED*/ static int
revo51_mixer_init (envy24ht_devc * devc, int dev, int g)
{
  int group = 0;
  int err;

  if ((err = mixer_ext_create_control (dev, group,
				       0, revo51_set_control,
				       MIXT_ONOFF,
				       "ENVY24_MUTE", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       0, revo51_set_ak4358,
				       MIXT_STEREOSLIDER,
				       "ENVY24_FRONT", 0x7f,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       1, revo51_set_ak4358,
				       MIXT_SLIDER,
				       "ENVY24_CENTER", 0x7f,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       2, revo51_set_ak4358,
				       MIXT_SLIDER,
				       "ENVY24_LFE", 0x7f,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       3, revo51_set_ak4358,
				       MIXT_STEREOSLIDER,
				       "ENVY24_SURROUND", 0x7f,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       4, revo51_set_ak4358,
				       MIXT_STEREOSLIDER,
				       "ENVY24_HEADPH", 0x7f,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       2, revo51_set_control,
				       MIXT_STEREOSLIDER,
				       "ENVY24_REC", 0x98,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       1, revo51_set_control,
				       MIXT_ENUM,
				       "ENVY24_RECSRC", 3,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  mixer_ext_set_strings (dev, err, "mic line aux", 0);

  if ((group = mixer_ext_create_group (dev, g, "MONITOR")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group,
				       3, revo51_set_monitor,
				       MIXT_ONOFF,
				       "MON_MUTE", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       0, revo51_set_monitor,
				       MIXT_STEREOSLIDER,
				       "ENVY24_MIC", 79,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       1, revo51_set_monitor,
				       MIXT_STEREOSLIDER,
				       "ENVY24_LINE", 79,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       2, revo51_set_monitor,
				       MIXT_STEREOSLIDER,
				       "ENVY24_AUX", 79,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

static void
revo51_card_init (envy24ht_devc * devc)
{

  int i;

#if 1
  OUTW (devc->osdev, 0xffff, devc->ccs_base + 0x18);	/* GPIO direction */
  OUTW (devc->osdev, 0x0000, devc->ccs_base + 0x16);	/* GPIO write mask */
  OUTW (devc->osdev, 0xffff, devc->ccs_base + 0x14);	/* Initial bit state */

  OUTB (devc->osdev, 0xff, devc->ccs_base + 0x1a);	/* GPIO direction for bits 16:22 */
  OUTB (devc->osdev, 0x00, devc->ccs_base + 0x1f);	/* GPIO mask for bits 16:22 */
  OUTB (devc->osdev, 0xff, devc->ccs_base + 0x1e);	/* GPIO data for bits 16:22 */
#endif

  memcpy (devc->channel_names, channel_names, sizeof (channel_names));
  AK4358_Init (devc);
  AK5365_Init (devc);
  PT2258_init (devc);
  ADC_Set_Chan (devc, 0);	/* TODO */
  REVO51_Set_48K_Mode (devc);

  for (i = 0; i < 5; i++)
    devc->gains[i] = 0x7f7f;
  for (i = 0; i < 3; i++)
    devc->monitor[i] = 79 | (79 << 8);
  devc->monitor[3] = 0;		/* Unmuted */

  set_reclevel (devc, 0x7f7f);	/* +0 dB */

  REVO51_Mute (devc, 0);
}

envy24ht_auxdrv_t envy24ht_revo51_auxdrv = {
  revo51_card_init,
  revo51_mixer_init,
  revo51_set_rate,
  NULL,
  NULL,
  NULL,				/* revo51_private1 */
  revo51_audio_ioctl
};
