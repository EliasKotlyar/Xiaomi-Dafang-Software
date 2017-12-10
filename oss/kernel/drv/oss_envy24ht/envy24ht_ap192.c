/*
 * Purpose: Low level routines for M Audio Audiophile 192
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
  "main",
  "unused1",
  "unused2",
  "unused3"
};

#define BIT(x) (1<<(x))
#define BIT3 BIT(3)

/*----- SPI bus for CODEC communication. */
/* */
#define SPI_CLK 		1	/* Clock output to CODEC's, rising edge clocks data. */
#define SPI_DIN 		2	/* Data input from the CODEC. */
#define SPI_DOUT 		3	/* Data output to the CODEC. */
#define SPI_CS0n 		(1<<4)	/* Selects first chip. */
#define SPI_CS1n 		(1<<5)	/* Selects second chip. */

#define SPI_CC_AK4358 		0x02	/* C1:C0 for ak4358. */
#define SPI_CC_AK4114 		0x02	/* C1:C0 for ak4114. */
#define WRITEMASK 		0xffff
/*----- Revolution defines. */
/* */
#define ap192_AK4114 (1)	/* iDevice value for AK4114 DIR. */
#define ap192_AK4358 (2)	/* iDevice value for AK4358 D/A. */

static unsigned int
GpioReadAll (envy24ht_devc * devc)
{
  return INW (devc->osdev, devc->ccs_base + 0x14);
}

static void
GPIOWriteAll (envy24ht_devc * devc, unsigned int data)
{
  OUTW (devc->osdev, 0xffff, devc->ccs_base + 0x18);	/* GPIO direction */
  OUTW (devc->osdev, 0x0000, devc->ccs_base + 0x16);	/* GPIO write mask */

  OUTW (devc->osdev, data, devc->ccs_base + 0x14);
}

static void
GPIOWrite (envy24ht_devc * devc, int pos, int bit)
{
  int data = GpioReadAll (devc);

  bit = (bit != 0);

  data &= ~(1 << pos);
  data |= (bit << pos);

  GPIOWriteAll (devc, data);
}

void
ap192_Assert_CS (envy24ht_devc * devc, int iDevice)
/*
*****************************************************************************
* Assert chip select to specified GPIO-connected device.
* iDevice: ap192_AK4114=DIG, ap192_AK4358=DAC.
****************************************************************************/
{
  unsigned int dwGPIO;		/* Current GPIO's. */
  dwGPIO = GpioReadAll (devc);	/* Read current GPIO's. */
  dwGPIO |= (SPI_CS0n | SPI_CS1n);	/* Reset CS bits. */
  switch (iDevice)		/* Select CS#. */
    {
    case ap192_AK4358:
      dwGPIO &= ~SPI_CS0n;
      break;			/* DAC */
    case ap192_AK4114:
      dwGPIO &= ~SPI_CS1n;
      break;			/* DIG */
    default:
      break;
    }
  GPIOWriteAll (devc, dwGPIO);	/* Write hardware. */
}

void
ap192_DeAssert_CS (envy24ht_devc * devc)
/*
*****************************************************************************
* De-Assert all chip selects.
****************************************************************************/
{
  unsigned int dwGPIO = GpioReadAll (devc);	/* Current GPIO's. */
  dwGPIO |= (SPI_CS0n | SPI_CS1n);	/* Clear CS bits. */
  GPIOWriteAll (devc, dwGPIO);	/* Write back to hardware. */
}

/*#define _delay()	oss_udelay(1) */
#define _delay()	{}

void
ap192_WriteSpiAddr (envy24ht_devc * devc, int iDevice, unsigned char bReg)
/*
*****************************************************************************
* Write the address byte part of the SPI serial stream.
* iDevice: ap192_AK4358=DAC, ap192_AK4114=DIG, etc.
****************************************************************************/
{
  unsigned char bHdr;
  unsigned char bNum;
/* Built 8-bit packet header: C1,C0,R/W,A4,A3,A2,A1,A0. */
/* */
  switch (iDevice)
    {
    case ap192_AK4358:
      bHdr = SPI_CC_AK4358 << 6;
      break;
    case ap192_AK4114:
      bHdr = SPI_CC_AK4114 << 6;
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
      GPIOWrite (devc, SPI_CLK, 0);	/* Drop clock low. */
      _delay ();
      GPIOWrite (devc, SPI_DOUT, 0x080 & bHdr);	/* Write data bit. */
      _delay ();
      GPIOWrite (devc, SPI_CLK, 1);	/* Raise clock. */
      _delay ();
      bHdr <<= 1;		/* Next bit. */
    }
}

void
ap192_WriteSpiReg (envy24ht_devc * devc, int iDevice, unsigned char bReg,
		   unsigned char bData)
/*
*****************************************************************************
* Writes one register in specified CHIP.
* devc = PCI slot code of specific board.
* iDevice: ap192_AK4358=DAC, ap192_AK4114=DIG, etc.
****************************************************************************/
{
  unsigned char bNum;
  GPIOWrite (devc, SPI_DOUT, 0);	/* Init SPI signals. */
  GPIOWrite (devc, SPI_CLK, 1);	/* */
/* Drop the chip select low. */
/* Wait at least 150 nS. */
/* */
  ap192_Assert_CS (devc, iDevice);
  _delay ();
/* Write the address byte. */
/* */
  ap192_WriteSpiAddr (devc, iDevice, bReg);
/* Write the data byte. */
/* */
  for (bNum = 0; bNum < 8; bNum++)
    {
      GPIOWrite (devc, SPI_CLK, 0);	/* Drop clock low. */
      _delay ();
      GPIOWrite (devc, SPI_DOUT, 0x080 & bData);	/* Write data bit. */
      _delay ();
      GPIOWrite (devc, SPI_CLK, 1);	/* Raise clock. */
      _delay ();
      bData <<= 1;		/* Next bit. */
    }
/* De-assert chip selects. */
/* */
  ap192_DeAssert_CS (devc);
  _delay ();
}


#define GPIO_MUTEn 22		/* Converter mute signal. */
void
ap192_Mute (envy24ht_devc * devc, int bMute)
/*
*****************************************************************************
* Mutes all outputs if bMute=TRUE.
****************************************************************************/
{
  if (bMute)
    {
/* Soft-mute. Delay currently unspecified, try ½ second. */
      ap192_WriteSpiReg (devc, ap192_AK4358, 1, 0x03);
      _delay ();
/* Switch mute transistors on. */
      GPIOWrite (devc, GPIO_MUTEn, 0);
    }
  else
    {
/* Switch mute transistors off. Delay currently unspecified, try ½ second. */
      GPIOWrite (devc, GPIO_MUTEn, 1);
      _delay ();
/* Release Soft-mute. */
      ap192_WriteSpiReg (devc, ap192_AK4358, 1, 0x01);
    }

  devc->mute = bMute;
}


void
ap192_Set_OutAttn (envy24ht_devc * devc, unsigned char bChan, int iAttn)
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
      cmn_err (CE_CONT, "Dnvalid data! %d=bChan, %d=iAttn", bChan, iAttn);
      return;
    }
  if (bChan < 6)
    bIndex = 0x04 + bChan;	/* for registers 0x04..0x09 */
  else
    bIndex = 0x05 + bChan;	/* for registers 0x0B..0x0C */
  bAttn = (0x80 + (unsigned char) (iAttn + 127));	/* 7F is max volume. */
/* MSB enables attenuation. */
  ap192_WriteSpiReg (devc, ap192_AK4358, bIndex, bAttn);
}

static void
ap192_Set_48K_Mode (envy24ht_devc * devc)
/*
*****************************************************************************
* Sets Chip and Envy24 for 8kHz-48kHz sample rates.
****************************************************************************/
{
/* ICE MCLK = 256x. */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 2) & ~BIT3,
	devc->mt_base + 2);
/* DFS=normal, RESET. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x4E);
/* DFS=normal, NORMAL OPERATION. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x4F);

  /* Set ADC modes */
  GPIOWrite (devc, 8, 0);	/* CKS0 = 0. MCLK = 256x */
  GPIOWrite (devc, 9, 0);	/* DFS0 = 0. */
  GPIOWrite (devc, 10, 0);	/* DFS1 = 0. Single speed mode. */

  /* Reset ADC timing */
  GPIOWrite (devc, 11, 0);
  _delay ();
  GPIOWrite (devc, 11, 1);
}

static void
ap192_Set_96K_Mode (envy24ht_devc * devc)
/*
*****************************************************************************
* Sets CODEC and Envy24 for 60kHz-96kHz sample rates.
****************************************************************************/
{
/* ICE MCLK = 256x. */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 2) & ~BIT3,
	devc->mt_base + 2);
/* DFS=double-speed, RESET. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x5E);
/* DFS=double-speed, NORMAL OPERATION. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x5F);

  /* Set ADC modes */
  GPIOWrite (devc, 8, 0);	/* CKS0 = 0. MCLK = 256x */
  GPIOWrite (devc, 9, 1);	/* DFS0 = 0. */
  GPIOWrite (devc, 10, 0);	/* DFS1 = 0. Single speed mode. */

  /* Reset ADC timing */
  GPIOWrite (devc, 11, 0);
  _delay ();
  GPIOWrite (devc, 11, 1);
}

static void
ap192_Set_192K_Mode (envy24ht_devc * devc)
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
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x6E);
  _delay ();
/* DFS=quad-speed, NORMAL OPERATION. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x6F);

  /* SPDIF */
  ap192_WriteSpiReg (devc, ap192_AK4114, 0x00, 0x0d);
  ap192_WriteSpiReg (devc, ap192_AK4114, 0x00, 0x0f);

  /* Set ADC modes */
  GPIOWrite (devc, 8, 1);	/* CKS0 = 0. MCLK = 256x */
  GPIOWrite (devc, 9, 0);	/* DFS0 = 0. */
  GPIOWrite (devc, 10, 1);	/* DFS1 = 0. Single speed mode. */

  /* Reset ADC timing */
  GPIOWrite (devc, 11, 0);
  _delay ();
  GPIOWrite (devc, 11, 1);
}

static int
set_dac (envy24ht_devc * devc, int reg, int level)
{
  if (level < 0)
    level = 0;
  if (level > 0x7f)
    level = 0x7f;

  ap192_WriteSpiReg (devc, ap192_AK4358, reg, level | 0x80);

  return level;
}

static void
AK4358_Init (envy24ht_devc * devc)
{
/*===== AK4358 D/A initialization. Leave soft-muted. */
/* */
/* Power down, reset, normal mode. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x00);
/* Power up, reset, normal mode */
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x4E);
/* Reset timing, Mode 3(I2S), disable auto clock detect, sharp roll off. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 0, 0x06);
/* Soft mute, reset timing. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 1, 0x02);
/* De-emphasis off. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 3, 0x01);
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
  ap192_WriteSpiReg (devc, ap192_AK4358, 0xA, 0x00);
/* DZF control disabled. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 0xD, 0x00);
  ap192_WriteSpiReg (devc, ap192_AK4358, 0xE, 0x00);
  ap192_WriteSpiReg (devc, ap192_AK4358, 0xF, 0x00);
/* Power up, normal operation. */
  ap192_WriteSpiReg (devc, ap192_AK4358, 2, 0x4F);
}

static void
ap192_set_rate (envy24ht_devc * devc)
{
  int tmp;

  tmp = INB (devc->osdev, devc->mt_base + 0x02);
  if (devc->speed <= 48000)
    {
      ap192_Set_48K_Mode (devc);
      OUTB (devc->osdev, tmp & ~BIT (3), devc->mt_base + 0x02);
      return;
    }

  if (devc->speed <= 96000)
    {
      ap192_Set_96K_Mode (devc);

      return;
    }

  ap192_Set_192K_Mode (devc);
  OUTB (devc->osdev, tmp | BIT (3), devc->mt_base + 0x02);
}

static int
ap192_audio_ioctl (envy24ht_devc * devc, envy24ht_portc * portc, unsigned int cmd,
		   ioctl_arg arg)
{
  int left, right, value;

  switch (cmd)
    {
    case SNDCTL_DSP_GETPLAYVOL:
      if (portc != &devc->play_portc[0])
	return OSS_EINVAL;
      left = (devc->gains[0] & 0xff) * 100 / 0x7f;
      right = ((devc->gains[0] >> 8) & 0xff) * 100 / 0x7f;
      return *arg = (left | (right << 8));
      break;

    case SNDCTL_DSP_SETPLAYVOL:
      if (portc != &devc->play_portc[0])
	return OSS_EINVAL;
      value = *arg;
      left = value & 0xff;
      right = (value >> 8) & 0xff;

      left = (left * 0x7f) / 100;
      right = (right * 0x7f) / 100;
      left = set_dac (devc, 0x04, left);
      right = set_dac (devc, 0x05, right);
      devc->gains[0] = left | (right << 8);
      mixer_devs[devc->mixer_dev]->modify_counter++;
      return 0;
      break;
    }
  return OSS_EINVAL;
}

static int
ap192_set_control (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->hw_devc;

  if (cmd == SNDCTL_MIX_READ)
    switch (ctrl)
      {
      case 0:
	return devc->mute;

      default:
	return OSS_EINVAL;
      }

  if (cmd == SNDCTL_MIX_WRITE)
    switch (ctrl)
      {
      case 0:
	value = !!value;
	ap192_Mute (devc, value);
	return devc->mute;

      default:
	return OSS_EINVAL;
      }

  return OSS_EINVAL;
}

static int
ap192_set_ak4358 (int dev, int ctrl, unsigned int cmd, int value)
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
	case 0:		/* PCM */
	  left = set_dac (devc, 0x04, left);
	  right = set_dac (devc, 0x05, right);
	  break;

	case 1:		/* Line IN */
	  /* Line IN monitor permits panning but we don't support it */
	  left = set_dac (devc, 0x06, left);
	  set_dac (devc, 0x07, 0);
	  set_dac (devc, 0x08, 0);
	  right = set_dac (devc, 0x09, right);
	  break;

	case 2:		/* S/PDIF */
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

#if 0
/*ARGSUSED*/ 
static int
set_mongain (envy24ht_devc * devc, int reg, int value)
{
  if (value < 0)
    value = 0;
  if (value > 79)
    value = 79;

  return value;
}
#endif

/*ARGSUSED*/ 
static int
ap192_mixer_init (envy24ht_devc * devc, int dev, int g)
{
  int group = g;
  int err;

  if ((group = mixer_ext_create_group (dev, g, "VOL")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group,
				       0, ap192_set_control,
				       MIXT_ONOFF,
				       "ENVY24_MUTE", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       0, ap192_set_ak4358,
				       MIXT_STEREOSLIDER,
				       "ENVY24_PCM", 0x7f,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       1, ap192_set_ak4358,
				       MIXT_STEREOSLIDER,
				       "ENVY24_IN", 0x7f,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       2, ap192_set_ak4358,
				       MIXT_STEREOSLIDER,
				       "ENVY24_SPDIF", 0x7f,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

static void
ap192_card_init (envy24ht_devc * devc)
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
  ap192_Set_48K_Mode (devc);

  for (i = 0; i < 5; i++)
    devc->gains[i] = 0x7f7f;

  ap192_Mute (devc, 0);

  GPIOWrite (devc, 5, 0);	/* Select S/PDIF output mux */
  GPIOWrite (devc, 5, 0);	/* Select S/PDIF output mux */

}

envy24ht_auxdrv_t envy24ht_ap192_auxdrv = {
  ap192_card_init,
  ap192_mixer_init,
  ap192_set_rate,
  NULL,
  NULL,
  NULL,				/* ap192_private1 */
  ap192_audio_ioctl
};
