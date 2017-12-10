/*
 * Purpose: Card specific routines for M Audio Delta 1010LT
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

#include "oss_config.h"
#include <ac97.h>
#include "envy24.h"

#define CH_STEREO	0x80

/* SPI chip select codes */
#define CS8_CS 0x4
#define CDC_CS 0x0
#define NONE_CS 0x7

#define SPI_CLK 1
#define SPI_DIN 2
#define SPI_DOUT 3
#define WCLOCK_ENABLE 7

#define CS8_ADDR 0x20		/* Chip SPI/I2C address */
#define CS8_RD 0x01
#define CS8_WR 0x00

#define BIT0		0x01
#define BIT1		0x02
#define BIT2		0x04
#define BIT3		0x08
#define BIT4		0x10
#define BIT5		0x20
#define BIT6		0x40
#define BIT7		0x80

#if 0
# define PRT_STATUS(v) outb(v&0xff, 0x378)
#else
# define PRT_STATUS(v)
#endif

/* -----	DEFINITION OF GPIOs     ----- */

#define	FPGA_PROGRAM_L		7	/* FPGA program control select line (active low) */
#define FPGA_CLK		1	/* FPGA program clock */
#define EXT_REG_CLK		0	/* Was GPIO Fast mode (debug) */
#define FPGA_D0			3	/* FPGA program data */

#define LATCH_EN_L		4	/* Strobe for external latch (active low) */
#define FPGA_MUTE		5

#define FPGA_INIT_STATE	0xFC	/* Init state for extern register to allow */
				/* FPGA initialization. */

#define FPGA_MASTER		5

extern int envy24_gain_sliders;
extern int envy24_virtualout;
extern int envy24_zerolatency;	/* Testing in progress */

static void
SPI_select (envy24_devc * devc, int sel)
{
  int tmp;

  tmp = envy24_read_cci (devc, 0x20);
  tmp &= ~0x70;
  tmp |= (sel & 0x7) << 4;
  envy24_write_cci (devc, 0x20, tmp);
}

static void
write_d1010lt_codec (envy24_devc * devc, int sel, int bRegister,
		     unsigned char bData)
{
  int bMask;

  WriteGPIObit (devc, SPI_DOUT, 0);
  WriteGPIObit (devc, SPI_CLK, 1);

  SPI_select (devc, sel);
  oss_udelay (1);
  bRegister = (bRegister & 0x0F) | 0xA0;	/* Add I2C address field. */

  /* Assert the CODEC chip select and wait at least 150 nS. */
  /* */

  /* Write the register address byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Write current data bit. */
      if (bMask & bRegister)
	WriteGPIObit (devc, SPI_DOUT, 1);
      else
	WriteGPIObit (devc, SPI_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, SPI_CLK, 1);
    }


  /* Write the data byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Write current data bit. */
      if (bMask & bData)
	WriteGPIObit (devc, SPI_DOUT, 1);
      else
	WriteGPIObit (devc, SPI_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, SPI_CLK, 1);
    }

  /* De-assert chip select. */
  /* */

  SPI_select (devc, NONE_CS);
}

static void
d1010lt_card_init (envy24_devc * devc)
{
  int i;

  SPI_select (devc, NONE_CS);

  for (i = 0; i < 4; i++)
    {
      write_d1010lt_codec (devc, i, 0, 0x07);
      write_d1010lt_codec (devc, i, 1, 0x03);
      write_d1010lt_codec (devc, i, 2, 0x60);
      write_d1010lt_codec (devc, i, 3, 0x19);
      write_d1010lt_codec (devc, i, 4, 0x7f);
      write_d1010lt_codec (devc, i, 5, 0x7f);
      write_d1010lt_codec (devc, i, 6, 0x7f);
      write_d1010lt_codec (devc, i, 7, 0x7f);
    }

}

static int
envy24_set_d1010lt (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int codec, level;
  static unsigned char levels[] = { 0x7f, 0x8c, 0x98 };

  if (ctrl >= 0xff)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      return devc->akm_gains[ctrl];
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {
      codec = (ctrl & 0x7) / 2;
      if (envy24_gain_sliders)
	{
	  level = value & 0xff;
	  if (level < 0)
	    level = 0;
	  if (level > 144)
	    level = 144;
	}
      else
	{
	  if (value < 0 || value > 2)
	    value = 0;
	  level = levels[value];
	}

      switch (ctrl & 0x89)	/* IN/OUT, LEFT/RIGHT, Gang switches */
	{
	case 0x00:		/* Left output channel only */
	  write_d1010lt_codec (devc, codec, 6, level);
	  break;

	case 0x01:		/* Right output channel only */
	  write_d1010lt_codec (devc, codec, 7, level);
	  break;

	case 0x80:		/* Both output channels */
	  write_d1010lt_codec (devc, codec, 6, level);
	  write_d1010lt_codec (devc, codec, 7, level);
	  break;

	case 0x08:		/* Left input channel only */
	  write_d1010lt_codec (devc, codec, 4, level);
	  break;

	case 0x09:		/* Right input channel only */
	  write_d1010lt_codec (devc, codec, 5, level);
	  break;

	case 0x88:		/* Both input channels */
	  write_d1010lt_codec (devc, codec, 4, level);
	  write_d1010lt_codec (devc, codec, 5, level);
	  break;
	}

      return devc->akm_gains[ctrl] = value;
    }
  return OSS_EINVAL;
}


static int
d1010lt_mix_init (envy24_devc * devc, int dev, int group)
{
  int i, mask = devc->outportmask, err, skip;
  int typ = MIXT_ENUM, range = 3;
  char tmp[64];

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_GAIN")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  if (envy24_gain_sliders)
    {
      typ = MIXT_MONOSLIDER;
      range = 144;

      for (i = 0; i < 0xff; i++)
	devc->akm_gains[i] = 0x7f;
    }
  else
    for (i = 0; i < 0xff; i++)
      devc->akm_gains[i] = 0;

  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUT");
	  else
	    sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);

	  if ((err = mixer_ext_create_control (dev, group,
					       i | CH_STEREO,
					       envy24_set_d1010lt, typ, tmp,
					       range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUTL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDOUTR");
	  else
	    sprintf (tmp, "ENVY24_OUT%d", i + 1);

	  if ((err = mixer_ext_create_control (dev, group,
					       i, envy24_set_d1010lt,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  mask = devc->inportmask;
  for (i = 0; i < 8; i += skip)
    {

      if (!(mask & (1 << i)))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDIN");
	  else
	    sprintf (tmp, "ENVY24_IN%d/%d", i + 1, i + 2);

	  if ((err = mixer_ext_create_control (dev, group,
					       (8 + i) | CH_STEREO,
					       envy24_set_d1010lt, typ, tmp,
					       range,
					       MIXF_RECVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
      else
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDINL");
	  else if (i == 9)
	    strcpy (tmp, "ENVY24_SPDINR");
	  else
	    sprintf (tmp, "ENVY24_IN%d", i + 1);

	  if ((err = mixer_ext_create_control (dev, group,
					       8 + i, envy24_set_d1010lt,
					       typ,
					       tmp, range,
					       MIXF_RECVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  return 0;
}

static void
write_d1010lt_spdif_reg (envy24_devc * devc, int bRegister, int bData)
{
  unsigned char bMask;
  unsigned char bSPI;

  /* Assert the CODEC chip select and wait at least 150 nS. */
  /* */
  SPI_select (devc, CS8_CS);

  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR | CS8_WR;
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, SPI_DOUT, 1);
      else
	WriteGPIObit (devc, SPI_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, SPI_CLK, 1);
    }

  /* Write the address (MAP) byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Write current data bit. */
      if (bMask & bRegister)
	WriteGPIObit (devc, SPI_DOUT, 1);
      else
	WriteGPIObit (devc, SPI_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, SPI_CLK, 1);
    }


  /* Write the data byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Write current data bit. */
      if (bMask & bData)
	WriteGPIObit (devc, SPI_DOUT, 1);
      else
	WriteGPIObit (devc, SPI_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, SPI_CLK, 1);
    }

  /* De-assert chip select. */
  /* */
  SPI_select (devc, NONE_CS);
}

static int
read_d1010lt_spdif_reg (envy24_devc * devc, int reg)
{
  unsigned char bMask;
  unsigned char bRet = 0;
  unsigned char bSPI;


	/****** WRITE MAP ADDRESS FIRST ******/

  /* Drop the chip select low. */
  /* Wait at least 150 nS. */
  /* */
  SPI_select (devc, CS8_CS);

  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR + CS8_WR;	/* SPI address field plus WRITE operation. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, SPI_DOUT, 1);
      else
	WriteGPIObit (devc, SPI_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, SPI_CLK, 1);
    }


  /* Write the address (MAP) byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Write current data bit. */
      if (bMask & reg)
	WriteGPIObit (devc, SPI_DOUT, 1);
      else
	WriteGPIObit (devc, SPI_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, SPI_CLK, 1);
    }

  /* De-assert chip select(s). */
  /* */
  SPI_select (devc, NONE_CS);


	/****** NOW READ THE DATA ******/

  /* Drop the chip select low. */
  /* Wait at least 150 nS. */
  /* */
  SPI_select (devc, CS8_CS);


  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR + CS8_RD;	/* SPI address field plus READ operation. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, SPI_DOUT, 1);
      else
	WriteGPIObit (devc, SPI_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, SPI_CLK, 1);
    }


  /* Read the data byte. */
  /* */
  bRet = 0;
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock low. */
      WriteGPIObit (devc, SPI_CLK, 0);

      /* Read current data bit. */
      if (ReadGPIObit (devc, SPI_DIN))
	bRet |= bMask;

      /* Raise clock. */
      WriteGPIObit (devc, SPI_CLK, 1);
    }


  /* De-assert chip selects. */
  /* */
  SPI_select (devc, NONE_CS);

  /* Return value. */

  return bRet;
}

static void
set_d1010lt_speed (envy24_devc * devc)
{
  int tmp;

  tmp = devc->speedbits;

  switch (devc->syncsource)
    {
    case SYNC_INTERNAL:
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      WriteGPIObit (devc, WCLOCK_ENABLE, 0);
      write_d1010lt_spdif_reg (devc, 4,
			       read_d1010lt_spdif_reg (devc, 4) & (~BIT0));
      break;

    case SYNC_SPDIF:
      tmp |= 0x10;
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      WriteGPIObit (devc, WCLOCK_ENABLE, 0);
      write_d1010lt_spdif_reg (devc, 4,
			       read_d1010lt_spdif_reg (devc, 4) | BIT0);
      break;

    case SYNC_WCLOCK:
      tmp |= 0x10;
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      WriteGPIObit (devc, WCLOCK_ENABLE, 1);
      write_d1010lt_spdif_reg (devc, 4,
			       read_d1010lt_spdif_reg (devc, 4) & (~BIT0));
      if (devc->model_data->svid == 0xd63014ff)
        {
	  /*
	   * 1010 rev E only
	   * don't aks me why, but it seems to work
	   */
	  WriteGPIObit (devc, 6, 0);
	  WriteGPIObit (devc, WCLOCK_ENABLE, 1);
        }
      break;
    }
}

static int
d1010lt_get_locked_status (envy24_devc * devc)
{
  /* TODO */
  return 1;
}

envy24_auxdrv_t d1010lt_auxdrv = {
  d1010lt_card_init,
  d1010lt_mix_init,
  init_cs8427_spdif,
  write_cs8427_spdif,
  cs8427_spdif_ioctl,
  read_d1010lt_spdif_reg,
  write_d1010lt_spdif_reg,
  set_d1010lt_speed,
  d1010lt_get_locked_status,
  cs8427_spdif_mixer_init
};
