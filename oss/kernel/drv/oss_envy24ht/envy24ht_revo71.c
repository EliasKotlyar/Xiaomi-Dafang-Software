/*
 * Purpose: Low level routines for M Audio Revolution 7.1
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

#define BIT(x) (1<<(x))

#define CCLK	(1<<1)
#define CDIN	(1<<2)		/* Currently not connected */
#define CDTI	(1<<3)
#define CSDIG	(1<<4)
#define CSN1	(1<<5)
#define CSN2	(1<<6)

#define WRITEMASK 	(CSN1|CSN2|CDTI|CCLK)

#define DAC1 0x01		/* 2ch Front DAC (AK4381) */
#define DAC2 0x03		/* 6ch Surround DAC (AK4355) */


static void
writereg (envy24ht_devc * devc, int csn, int chip, unsigned char reg,
	  unsigned char val)
{
  int i;
  unsigned short v, tmp;

  tmp = INW (devc->osdev, devc->ccs_base + 0x14) & ~(csn | CCLK | CDTI);
  OUTW (devc->osdev, tmp, devc->ccs_base + 0x14);

  reg = (reg & 0x1f) | 0x20 | (chip << 6);	/* Chip address (variable), write */
  /* Address bits */

  for (i = 7; i >= 0; i--)
    {
      v = (reg & (1 << i)) ? CDTI : 0;
      OUTW (devc->osdev, v | tmp, devc->ccs_base + 0x14);	/* Tack */
      OUTW (devc->osdev, v | CCLK | tmp, devc->ccs_base + 0x14);	/* Tick */
    }

  /* Data bits */

  for (i = 7; i >= 0; i--)
    {
      v = (val & (1 << i)) ? CDTI : 0;
      OUTW (devc->osdev, v | tmp, devc->ccs_base + 0x14);	/* Tack */
      OUTW (devc->osdev, v | CCLK | tmp, devc->ccs_base + 0x14);	/* Tick */
    }

  OUTW (devc->osdev, v | tmp, devc->ccs_base + 0x14);	/* Tack */
  OUTW (devc->osdev, tmp | csn | CDTI | CCLK, devc->ccs_base + 0x14);	/* Release */

}

static void
writedac1 (envy24ht_devc * devc, unsigned char reg, unsigned char data)
{
  writereg (devc, CSN1, DAC1, reg, data);
  devc->dac1val[reg] = data;
}

static void
writedac2 (envy24ht_devc * devc, unsigned char reg, unsigned char data)
{
  writereg (devc, CSN2, DAC2, reg, data);
  devc->dac2val[reg] = data;
}

static void
revo71_mute (envy24ht_devc * devc, int mute)
{
  if (mute)
    {
      writedac1 (devc, 1, devc->dac1val[1] | BIT (0));
      writedac2 (devc, 1, devc->dac2val[1] | BIT (1));
      oss_udelay (1000);
/*      OUTB (devc->osdev, INB (devc->osdev, devc->ccs_base + 0x1e) & ~BIT(22-16), */
/*          devc->ccs_base + 0x1e); */
    }
  else
    {
/*      OUTB (devc->osdev, INB (devc->osdev, devc->ccs_base + 0x1e) | ~BIT(22-16), */
/*          devc->ccs_base + 0x1e); */
      oss_udelay (1000);
      writedac1 (devc, 1, devc->dac1val[1] & ~BIT (0));
      writedac2 (devc, 1, devc->dac2val[1] & ~BIT (1));
    }
}

static void
revo71_card_init (envy24ht_devc * devc)
{
  int i;

  OUTL (devc->osdev, WRITEMASK | 0x400000, devc->ccs_base + 0x18);	/* GPIO direction */
  OUTW (devc->osdev, ~WRITEMASK, devc->ccs_base + 0x16);	/* GPIO write mask */
  OUTB (devc->osdev, 0x40, devc->ccs_base + 0x1f);
  OUTW (devc->osdev, WRITEMASK, devc->ccs_base + 0x14);	/* Initial bit state */

  OUTB (devc->osdev, 0xff, devc->ccs_base + 0x1a);	/* GPIO direction for bits 16:22 */
  OUTB (devc->osdev, 0x00, devc->ccs_base + 0x1f);	/* GPIO mask for bits 16:22 */
  OUTB (devc->osdev, 0xff, devc->ccs_base + 0x1e);	/* GPIO data for bits 16:22 */

#if 0
  for (i = 0; i < 7; i++)
    {
      OUTW (devc->osdev, 1 << i, devc->ccs_base + 0x14);	/* Test bit */
      OUTW (devc->osdev, 0, devc->ccs_base + 0x14);	/* Test bit */
    }
#endif

  OUTW (devc->osdev, WRITEMASK, devc->ccs_base + 0x14);	/* Initial bit state */
  oss_udelay (10);

  revo71_mute (devc, 1);
/*
 * Init front DAC (AK4381)
 */
  writedac1 (devc, 0x00, 0x0c);
  writedac1 (devc, 0x01, 0x03);
  writedac1 (devc, 0x02, 0x00);

  writedac1 (devc, 0x03, 0xff);	/* Initial volume */
  writedac1 (devc, 0x04, 0xff);	/* Initial volume */

  writedac1 (devc, 0x00, 0x0f);
/*
 * Init surround DAC (AK4355)
 */
  writedac2 (devc, 0x01, 0x02);
  writedac2 (devc, 0x00, 0x06);
  oss_udelay (10);
  writedac2 (devc, 0x02, 0x0e);
  writedac2 (devc, 0x03, 0x01);

  for (i = 4; i < 10; i++)
    writedac2 (devc, i, 0xff);	/* Initial volumes */
  writedac2 (devc, 0x0a, 0x00);

  writedac2 (devc, 0x01, 0x01);
  revo71_mute (devc, 0);
}

static void
revo71_set_rate (envy24ht_devc * devc)
{
  int rate = devc->speed, i;
  unsigned char tmp;

  if (devc->speed == devc->prev_speed)
    return;
  devc->prev_speed = devc->speed;

  revo71_mute (devc, 1);

  /* Pulse the PRST# signal to reset converters */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x05) | 0x80,
	devc->mt_base + 0x05);
  oss_udelay (5000);
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x05) & ~0x80,
	devc->mt_base + 0x05);
  oss_udelay (5000);

  tmp = 0x03;
  if (rate <= 48000)
    tmp |= 0x00;
  else
    {
      if (rate <= 96000)
	tmp |= 0x04;
      else
	tmp |= 0x08;
    }

  /* Front DAC */

  writedac1 (devc, 0, 0x0c);
  writedac1 (devc, 1, tmp);
  writedac1 (devc, 2, 0x00);
  writedac1 (devc, 3, devc->dac1val[3]);
  writedac1 (devc, 4, devc->dac1val[4]);
  writedac1 (devc, 0, 0x0f);

  /* Surround DAC */
  writedac2 (devc, 1, 0x02);
  writedac2 (devc, 0, 0x06);

  tmp = 0x0e;

  if (devc->speed > 60000)
    {
      if (devc->speed > 120000)
	tmp |= 0x20;
      else
	tmp |= 0x10;
    }
  writedac2 (devc, 2, tmp);
  writedac2 (devc, 3, 0x01);

  for (i = 4; i < 10; i++)
    writedac2 (devc, i, devc->dac2val[i]);

  writedac2 (devc, 0xa, 0x00);
  writedac2 (devc, 1, 0x03);

  revo71_mute (devc, 0);
}

static int
revo71_set_akm (int dev, int ctrl, unsigned int cmd, int value)
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
	  writedac1 (devc, 0x03, left);
	  writedac1 (devc, 0x04, right);
	  break;

	case 1:		/* Rear */
	  writedac2 (devc, 0x06, left);
	  writedac2 (devc, 0x07, right);
	  break;

	case 2:		/* Center */
	  writedac2 (devc, 0x04, left);
	  right = left;
	  break;

	case 3:		/* LFE */
	  writedac2 (devc, 0x05, left);
	  right = left;
	  break;

	case 4:		/* Surround */
	  writedac2 (devc, 0x08, left);
	  writedac2 (devc, 0x09, right);
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
revo71_mixer_init (envy24ht_devc * devc, int dev, int group)
{
  int i, err;

  for (i = 0; i < 6; i++)
    devc->gains[i] = 0xffff;

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24HT_GAIN")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group,
				       0, revo71_set_akm,
				       MIXT_STEREOSLIDER,
				       "GAIN_FRONT", 255,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       1, revo71_set_akm,
				       MIXT_STEREOSLIDER,
				       "GAIN_REAR", 255,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       2, revo71_set_akm,
				       MIXT_MONOSLIDER,
				       "GAIN_CENTER", 255,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       3, revo71_set_akm,
				       MIXT_MONOSLIDER,
				       "GAIN_LFE", 255,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       4, revo71_set_akm,
				       MIXT_STEREOSLIDER,
				       "GAIN_surround", 255,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

envy24ht_auxdrv_t envy24ht_revo71_auxdrv = {
  revo71_card_init,
  revo71_mixer_init,
  revo71_set_rate,
  NULL,
  NULL,
  NULL,				/* revo71_private1 */
};
