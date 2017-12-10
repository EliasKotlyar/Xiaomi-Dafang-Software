/*
 * Purpose: Card specific routines for M Audio Delta TDIF
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
#include "ac97.h"
#include "envy24.h"
#include "envy24_tdif.h"

#define CDC_CLK 1		/* Clock input to the CODEC's, rising edge clocks data. */
#define CDC_DIN 2		/* Data input to Envy from the CODEC. */
#define CDC_DOUT 3		/* Data output from Envy to the CODEC. */
#define CS8_CS 2		/* Chip select (0=select) for the SPDIF tx/rx. */
#define CDC_CS 3		/* Chip select (0=select) for the CODEC. */
#define CS_ASSERT 0		/* Asserted chip select (selects are inverted). */
#define CS_RELEASE 1		/* Idle chip select (selects are inverted). */


#define BIT0		0x01
#define BIT1		0x02
#define BIT2		0x04
#define BIT3		0x08
#define BIT4		0x10
#define BIT5		0x20
#define BIT6		0x40
#define BIT7		0x80

#define CS8_CLK CDC_CLK
#define CS8_DIN CDC_DIN
#define CS8_DOUT CDC_DOUT
#define CS_TRUE CS_ASSERT
#define CS_FALSE CS_RELEASE
#define CS8_ADDR 0x20		/* Chip SPI/I2C address */
#define CS8_RD 0x01
#define CS8_WR 0x00

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
WriteExternReg (envy24_devc * devc, unsigned char bData)
/* Initialize external register by serially shifting the data out */
/* and then strobe the external latch. */
{
  int i;

  devc->gpio_tmp = bData;

  WriteGPIObit (devc, LATCH_EN_L, 0);	/* DISABLE External Latch */

  /* FPGA clock to low */
  WriteGPIObit (devc, EXT_REG_CLK, 0);

  for (i = 0; i < 8; i++)
    {
      /* Present data */
      WriteGPIObit (devc, FPGA_D0, !!(bData & 0x80));
      /* clock data in */
      WriteGPIObit (devc, EXT_REG_CLK, 1);
      WriteGPIObit (devc, EXT_REG_CLK, 0);
      /* next bit */
      bData = bData << 1;
    }

  WriteGPIObit (devc, LATCH_EN_L, 1);	/* STROBE External Latch */
  WriteGPIObit (devc, LATCH_EN_L, 0);	/* DISABLE External Latch */
}

static void
WriteExternRegBit (envy24_devc * devc, int bit, int value)
{
  devc->gpio_tmp &= ~(1 << bit);
  if (value)
    devc->gpio_tmp |= (1 << bit);
  WriteExternReg (devc, devc->gpio_tmp);
}

static void
tdif_pause (int n)
/*
*****************************************************************************
*  Programmable pause.
****************************************************************************/
{
  static int d;
  int i;

  for (i = 0; i < n; i++)
    for (d = 1000L; d--;);
}

/****************************************************************************/


static void
TDIF_InitFPGA (envy24_devc * devc)
/*
*****************************************************************************
*  Initialize FPGA
*  wInst = PCI slot code of specific board.
*  Hardware = Spartan II FPGA (XC2S50-5TQ144)
****************************************************************************/
{
  int bReadByte;
  int i, j;

#if 0
  if (envy24_read_cci (devc, 0x20) & 0x40)
    {
      DDB (cmn_err (CE_CONT, "envy24: FPGA is already up and running\n"));
      return;
    }
#endif

  cmn_err (CE_CONT, "Envy24: Loading FPGA - please wait\n");

  /*Init external register */
  WriteGPIObit (devc, FPGA_PROGRAM_L, 1);	/*DISABLE FPGA Programming */
  WriteGPIObit (devc, LATCH_EN_L, 0);	/*DISABLE External Latch */

  bReadByte = FPGA_INIT_STATE;

  WriteExternReg (devc, bReadByte);

  /*Pull PROGRAM# pin LOW to start loading configuration data */
  WriteGPIObit (devc, FPGA_PROGRAM_L, 0);
  tdif_pause (10);
  WriteGPIObit (devc, FPGA_PROGRAM_L, 1);

  for (j = 0; j < sizeof (fpga_code); j++)
    {
      bReadByte = fpga_code[j];
      for (i = 0; i < 8; i++)
	{
	  /*FPGA clock to low */
	  WriteGPIObit (devc, FPGA_CLK, 0);
	  /*Present data */
	  WriteGPIObit (devc, FPGA_D0, !!(bReadByte & 0x80));
	  /*clock data in */
	  WriteGPIObit (devc, FPGA_CLK, 1);

	  /*next bit */
	  bReadByte = bReadByte << 1;
	}

    }

  /*Some clocks to start FPGA */
  for (i = 0; i < 64; i++)
    {
      /*FPGA clock to low */
      WriteGPIObit (devc, FPGA_CLK, 0);
      /*clock data in */
      WriteGPIObit (devc, FPGA_CLK, 1);
    }

  if (!(envy24_read_cci (devc, 0x20) & 0x40))
    cmn_err (CE_CONT, "Envy24: FPGA failed to initialize\n");
  else
    DDB (cmn_err (CE_CONT, "FPGA is up and running\n"));

  oss_udelay (10);
  WriteExternRegBit (devc, FPGA_MUTE, 1);	/* Keep it still muted */

}

static void
write_tdif_codec (envy24_devc * devc, int bRegister, unsigned char bData)
/*

*****************************************************************************
*  Writes a byte to a specific register of the Delta-AP CODEC.
*  Register must be (0..15).
****************************************************************************/
{
  unsigned char bMask;

  bRegister = (bRegister & 0x0F) | 0xA0;	/* Add I2C address field. */

  /* Assert the CODEC chip select and wait at least 150 nS. */
  /* */
  WriteExternRegBit (devc, CDC_CS, CS_TRUE);

  /* Write the register address byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bRegister)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CDC_CLK, 1);
    }


  /* Write the data byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bData)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CDC_CLK, 1);
    }

  /* De-assert chip select. */
  /* */
  WriteExternRegBit (devc, CDC_CS, CS_FALSE);
}

static void write_tdif_spdif_reg (envy24_devc * devc, int bRegister,
				  int bData);
static int read_tdif_spdif_reg (envy24_devc * devc, int bRegister);

static void
tdif_card_init (envy24_devc * devc)
{
  WriteExternReg (devc, 0xfc);
  TDIF_InitFPGA (devc);
  write_tdif_spdif_reg (devc, 4, read_tdif_spdif_reg (devc, 4) & (~BIT0));
  WriteGPIObit (devc, FPGA_MASTER, 1);
  WriteExternRegBit (devc, 4, 1);	/* Disable TDIF master PLL */
  WriteExternRegBit (devc, 6, 0);	/* Don't route PLL master clock to Envy24. */

  write_tdif_codec (devc, 0, 0x07);
  write_tdif_codec (devc, 1, 0x00);
  write_tdif_codec (devc, 2, 0x60);
  write_tdif_codec (devc, 3, 0x0d);
  write_tdif_codec (devc, 4, 0x7f);
  write_tdif_codec (devc, 1, 0x03);
  write_tdif_codec (devc, 5, 0x7f);
}

static void
tdif_card_uninit (envy24_devc * devc)
{
  WriteGPIObit (devc, FPGA_PROGRAM_L, 1);	/*DISABLE FPGA Programming */
  WriteGPIObit (devc, LATCH_EN_L, 0);	/*DISABLE External Latch */
  WriteGPIObit (devc, FPGA_PROGRAM_L, 0);
}

static int
envy24_set_tdif (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int level;
  static unsigned char levels[] = { 0x60, 0x6f, 0x7f };

  if (ctrl >= 0xff)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (envy24_gain_sliders)
	return devc->akm_gains[ctrl];
      return devc->akm_gains[ctrl];
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {

      if (envy24_gain_sliders)
	level = value & 0xff;
      else
	{
	  if (value > 2)
	    return OSS_EINVAL;
	  level = levels[value];
	}

      write_tdif_codec (devc, 4, level);
      write_tdif_codec (devc, 5, level);
      return devc->akm_gains[ctrl] = value;
    }
  return OSS_EINVAL;
}


static int
tdif_mix_init (envy24_devc * devc, int dev, int group)
{
  int i, mask = devc->outportmask, err, skip, codec, ports;
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
      range = 164;

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

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = 0x0c;		/* Both output ports */
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports, envy24_set_tdif,
					       typ,
					       tmp, range,
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

	  codec = (i > 1) ? AKM_B : AKM_A;
	  ports = (i & 1) ? 0x08 : 0x04;
	  if ((err = mixer_ext_create_control (dev, group,
					       codec | ports, envy24_set_tdif,
					       typ,
					       tmp, range,
					       MIXF_MAINVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	}
    }

  return 0;
}

static void
set_tdif_speed (envy24_devc * devc)
{
  int tmp;

  WriteExternRegBit (devc, FPGA_MUTE, 1);	/* Mute FPGA */
  /* Set the TDIF sampling rate */
  devc->gpio_tmp &= ~0x03;
  switch (devc->speed)
    {
    case 32000:
      devc->gpio_tmp |= 0x01;
      break;
    case 44100:
      devc->gpio_tmp |= 0x02;
      break;
    case 48000:
      devc->gpio_tmp |= 0x01;
      break;
    default:
      devc->gpio_tmp |= 0x01;
      break;
    }
  WriteExternReg (devc, devc->gpio_tmp);

  tmp = devc->speedbits;

  switch (devc->syncsource)
    {
    case SYNC_INTERNAL:
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      WriteGPIObit (devc, FPGA_MASTER, 1);
      write_tdif_spdif_reg (devc, 4, read_tdif_spdif_reg (devc, 4) & (~BIT0));
      WriteExternRegBit (devc, 4, 1);	/* Disable TDIF master PLL */
      WriteExternRegBit (devc, 6, 0);	/* Don't route PLL master clock to Envy24. */
      break;

    case SYNC_SPDIF:
      tmp |= 0x10;
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      WriteGPIObit (devc, FPGA_MASTER, 1);
      write_tdif_spdif_reg (devc, 4, read_tdif_spdif_reg (devc, 4) | BIT0);
      WriteExternRegBit (devc, 6, 1);	/* Do route PLL master clock to Envy24. */
      WriteExternRegBit (devc, 4, 1);	/* Disable TDIF master PLL */
      break;

    case SYNC_WCLOCK:
      tmp |= 0x10;
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
      write_tdif_spdif_reg (devc, 4, read_tdif_spdif_reg (devc, 4) & (~BIT0));
      WriteExternRegBit (devc, 4, 0);	/* Enable TDIF master PLL */
      WriteExternRegBit (devc, 6, 1);	/* Do route PLL master clock to Envy24. */
      WriteGPIObit (devc, FPGA_MASTER, 0);
      break;
    }

  if (devc->speed > 48000)
    {
      write_tdif_codec (devc, 0x01, 0x00);
      write_tdif_codec (devc, 0x02, 0x65);
      write_tdif_codec (devc, 0x01, 0x03);
    }
  else
    {
      write_tdif_codec (devc, 0x01, 0x00);
      write_tdif_codec (devc, 0x02, 0x60);
      write_tdif_codec (devc, 0x01, 0x03);
    }
  WriteExternRegBit (devc, FPGA_MUTE, 0);	/* Unmute */
}

static void
write_tdif_spdif_reg (envy24_devc * devc, int bRegister, int bData)
{
  unsigned char bMask;
  unsigned char bSPI;

  /* Assert the CODEC chip select and wait at least 150 nS. */
  /* */
  WriteExternRegBit (devc, CS8_CS, CS_TRUE);

  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR | CS8_WR;
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CS8_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, CS8_DOUT, 1);
      else
	WriteGPIObit (devc, CS8_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CS8_CLK, 1);
    }

  /* Write the address (MAP) byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CS8_CLK, 0);

      /* Write current data bit. */
      if (bMask & bRegister)
	WriteGPIObit (devc, CS8_DOUT, 1);
      else
	WriteGPIObit (devc, CS8_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CS8_CLK, 1);
    }


  /* Write the data byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop SPI clock low. */
      WriteGPIObit (devc, CS8_CLK, 0);

      /* Write current data bit. */
      if (bMask & bData)
	WriteGPIObit (devc, CS8_DOUT, 1);
      else
	WriteGPIObit (devc, CS8_DOUT, 0);

      /* Raise SPI clock to "clock data in". */
      WriteGPIObit (devc, CS8_CLK, 1);
    }

  /* De-assert chip select. */
  /* */
  WriteExternRegBit (devc, CS8_CS, CS_FALSE);
}

static int
read_tdif_spdif_reg (envy24_devc * devc, int reg)
{
  unsigned char bMask;
  unsigned char bRet = 0;
  unsigned char bSPI;


	/****** WRITE MAP ADDRESS FIRST ******/

  /* Drop the chip select low. */
  /* Wait at least 150 nS. */
  /* */
  WriteExternRegBit (devc, CS8_CS, CS_TRUE);

  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR + CS8_WR;	/* SPI address field plus WRITE operation. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, CDC_CLK, 1);
    }


  /* Write the address (MAP) byte. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & reg)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, CDC_CLK, 1);
    }

  /* De-assert chip select(s). */
  /* */
  WriteExternRegBit (devc, CS8_CS, CS_FALSE);


	/****** NOW READ THE DATA ******/

  /* Drop the chip select low. */
  /* Wait at least 150 nS. */
  /* */
  WriteExternRegBit (devc, CS8_CS, CS_TRUE);


  /* Write the SPI address/cmd byte. */
  /* */
  bSPI = CS8_ADDR + CS8_RD;	/* SPI address field plus READ operation. */
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock (GPIO5) low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Write current data bit. */
      if (bMask & bSPI)
	WriteGPIObit (devc, CDC_DOUT, 1);
      else
	WriteGPIObit (devc, CDC_DOUT, 0);

      /* Raise clock (GPIO5). */
      WriteGPIObit (devc, CDC_CLK, 1);
    }


  /* Read the data byte. */
  /* */
  bRet = 0;
  /* */
  for (bMask = 0x80; bMask; bMask = (bMask >> 1) & 0x7F)
    {
      /* Drop clock low. */
      WriteGPIObit (devc, CDC_CLK, 0);

      /* Read current data bit. */
      if (ReadGPIObit (devc, CDC_DIN))
	bRet |= bMask;

      /* Raise clock. */
      WriteGPIObit (devc, CDC_CLK, 1);
    }


  /* De-assert chip selects. */
  /* */
  WriteExternRegBit (devc, CS8_CS, CS_FALSE);

  /* Return value. */

  return bRet;
}

static int
tdif_get_locked_status (envy24_devc * devc)
{
  switch (devc->syncsource)
    {
    case SYNC_INTERNAL:
      return 1;
      break;

    case SYNC_SPDIF:
      return !(read_tdif_spdif_reg (devc, 16) & 0x17);
      break;

    case SYNC_WCLOCK:
      /* The SPI input pin shows the TDIF locked status */
      if (envy24_read_cci (devc, 0x20) & (1 << 2))
	return 1;
      return 0;
      break;
    }

  return 1;
}

envy24_auxdrv_t tdif_auxdrv = {
  tdif_card_init,
  tdif_mix_init,
  init_cs8427_spdif,
  write_cs8427_spdif,
  cs8427_spdif_ioctl,
  read_tdif_spdif_reg,
  write_tdif_spdif_reg,
  set_tdif_speed,
  tdif_get_locked_status,
  cs8427_spdif_mixer_init,
  tdif_card_uninit
};
