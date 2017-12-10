/*
 * Purpose: Low level routines for AC97 based Envy24HT boards (mainly Envy24-PT).
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

#define AKM_ADDRESS 0x10
#if 0
# define PRT_STATUS(v) outb(v&0xff, 0x378)
#else
# define PRT_STATUS(v)
#endif

#if 0
static unsigned char
i2c_read (envy24ht_devc * devc, unsigned char addr, unsigned char pos)
{
  int i;
  unsigned char data;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, pos, devc->ccs_base + 0x11);	/* Offset */
  OUTB (devc->osdev, addr << 1, devc->ccs_base + 0x10);	/* Read address  */

  for (i = 0; i < 2000; i++)
    {
      unsigned char status = INB (devc->osdev, devc->ccs_base + 0x13);
      if (!(status & 1))
	break;

    }

  oss_udelay (1);
  data = INB (devc->osdev, devc->ccs_base + 0x12);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return data;
}

static void
i2c_write (envy24ht_devc * devc, unsigned char addr, unsigned char pos,
	   unsigned char data)
{
  int i;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, pos, devc->ccs_base + 0x11);	/* Offset */
  OUTB (devc->osdev, data, devc->ccs_base + 0x12);	/* Data */
  OUTB (devc->osdev, (addr << 1) | 1, devc->ccs_base + 0x10);	/* Write address  */

  for (i = 0; i < 2000; i++)
    {
      unsigned char status = INB (devc->osdev, devc->ccs_base + 0x13);
      if (!(status & 1))
	break;

    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}
#endif

/*ARGSUSED*/ 
static void
init_cs8415a (envy24ht_devc * devc)
{
}

/*ARGSUSED*/ 
static void
init_wm8728 (envy24ht_devc * devc)
{

#if 0
  printk ("Regs=");
  for (i = 0; i < 0x18; i++)
    {
      PRT_STATUS (2);
      printk ("%02x ", i2c_read (devc, addr, i));
      PRT_STATUS (0);
    }
  printk ("\n");
#endif
}

static void
ac97_card_init (envy24ht_devc * devc)
{

  PRT_STATUS (0);
  PRT_STATUS (0x01);
  PRT_STATUS (0);

  OUTW (devc->osdev, 0x000f, devc->ccs_base + 0x14);	/* GPIO */

  oss_udelay (1000);

  devc->recsrc = 0;
  init_cs8415a (devc);
  init_wm8728 (devc);
}

/*ARGSUSED*/ 
static int
ac97_mixer_init (envy24ht_devc * devc, int dev, int group)
{
  return 0;
}

#if 0
static int
ac97_private1 (envy24ht_devc * devc, int value)
{
  i2c_write (devc, AKM_ADDRESS, (value >> 8) & 0xff, value & 0xff);
  return 0;
}
#endif

envy24ht_auxdrv_t envy24ht_ac97_auxdrv = {
  ac97_card_init,
  ac97_mixer_init
};
