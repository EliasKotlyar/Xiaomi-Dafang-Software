/*
 * Purpose: VIA ENVY24HT chipset driver.
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
#include <oss_pci.h>

#define SUPPORTED_FORMAT AFMT_S32_LE

#include <spdif.h>
#include "envy24ht.h"

/* extern envy24ht_auxdrv_t envy24ht_viaref_auxdrv; */
extern envy24ht_auxdrv_t envy24ht_ac97_auxdrv;
extern envy24ht_auxdrv_t envy24ht_revo51_auxdrv;
extern envy24ht_auxdrv_t envy24ht_revo71_auxdrv;
extern envy24ht_auxdrv_t envy24ht_aureon_auxdrv;
extern envy24ht_auxdrv_t envy24ht_julia_auxdrv;
extern envy24ht_auxdrv_t envy24ht_ap192_auxdrv;

#define OUTCH_NAMES "front c/l side rear"
static char channel_names[4][10] = {
  "front",
  "c/l",
  "side",
  "rear"
};

static card_spec models[] = {
  {0x17241412, "Generic Envy24PT motherboard audio", 6, 2,
   MF_SPDIFOUT | MF_ENVY24PT, &envy24ht_ac97_auxdrv},
  {0xf641270f, "Chaintech ZNF3-150", 6, 2,
   MF_SPDIFOUT | MF_ENVY24PT, &envy24ht_ac97_auxdrv},
  {0x2723270f, "Chaintech 9CJS", 6, 2,
   MF_SPDIFOUT | MF_ENVY24PT, &envy24ht_ac97_auxdrv},
  {0x50361297, "Shuttle SN25P", 6, 2,
   MF_SPDIFOUT | MF_ENVY24PT, &envy24ht_ac97_auxdrv},
  {0x020010b0, "Gainward Hollywood Envy24HT-S", 6, 2,
   MF_SPDIFOUT | MF_ENVY24PT, &envy24ht_ac97_auxdrv},
  {0x36311412, "M Audio Revolution 5.1", 6, 2,
   MF_SPDIFOUT | MF_192K, &envy24ht_revo51_auxdrv},
  {0x36301412, "M Audio Revolution 7.1", 8, 2,
   MF_SPDIFOUT | MF_192K, &envy24ht_revo71_auxdrv},
  {SSID_AUREON_SPACE, "Terratec Aureon 7.1 Space", 8, 2,
   MF_SPDIFOUT | MF_192K | MF_NOAC97, &envy24ht_aureon_auxdrv},
  {SSID_AUREON_UNIVERSE, "Terratec Aureon 7.1 Universe", 8, 2,
   MF_SPDIFOUT | MF_192K | MF_NOAC97, &envy24ht_aureon_auxdrv},
  {SSID_AUREON_SKY, "Terratec Aureon 7.1 Sky", 8, 2,
   MF_SPDIFOUT | MF_192K | MF_NOAC97, &envy24ht_aureon_auxdrv},
  {SSID_PRODIGY71, "Audiotrak Prodigy 7.1", 8, 2,
   MF_SPDIFOUT | MF_192K | MF_NOAC97, &envy24ht_aureon_auxdrv},
  {SSID_JULIA, "Ego Systems Juli@", 2, 2,
   MF_SPDIFOUT | MF_SPDIFIN | MF_192K | MF_NOAC97 | MF_MIDI,
   &envy24ht_julia_auxdrv},
  {SSID_PHASE28, "Terratec PHASE 28", 8, 2,
   MF_SPDIFOUT | MF_192K | MF_NOAC97 | MF_MIDI, &envy24ht_aureon_auxdrv},
  {SSID_AP192, "M-Audio Audiophile 192", 2, 2,
   MF_SPDIFOUT | MF_SPDIFIN | MF_192K | MF_NOAC97 | MF_MIDI,
   &envy24ht_ap192_auxdrv},
  {0x24031412, "VIA Vinyl Tremor Audio", 6, 2,
   MF_SPDIFOUT | MF_ENVY24PT, &envy24ht_ac97_auxdrv},
  /* XXX Do a separate auxdrv, to adjust for Envy24HT-S and other differences. */                
  {SSID_PRODIGYHD2, "Audiotrak Prodigy HD2", 2, 2,                                               
   MF_SPDIFOUT | MF_192K | MF_NOAC97, &envy24ht_ap192_auxdrv},                                   
  {SSID_PRODIGYHD2_ADE, "Audiotrak Prodigy HD2 Advance DE", 2, 2,                                
   MF_SPDIFOUT | MF_192K | MF_NOAC97, &envy24ht_ap192_auxdrv},         
  /* {0x17241412, "VIA Envy24HT reference design", 6, 2, */
  /* MF_SPDIFOUT | MF_MIDI, &envy24ht_viaref_auxdrv}, */
  {0}
};

static struct speed_sel speed_tab[] = {
  {
   8000, 0x06}
  ,
  {
   9600, 0x03}
  ,
  {
   11025, 0x0a}
  ,
  {
   12000, 0x02}
  ,
  {
   16000, 0x05}
  ,
  {
   22050, 0x09}
  ,
  {
   24000, 0x01}
  ,
  {
   32000, 0x04}
  ,
  {
   44100, 0x08}
  ,
  {
   48000, 0x00}
  ,
  {
   64000, 0x0f}
  ,
  {
   88200, 0x0b}
  ,
  {
   96000, 0x07}
  ,
  {
   176400, 0x0c}
  ,
  {
   192000, 0x0e}
  ,
  {-1, 0x10}
  ,
};

static const envy24ht_auxdrv_t dummy_auxdrv = { NULL };

static int
ac97_read (void *devc_, int wAddr)
{
  envy24ht_devc *devc = devc_;
  oss_native_word flags;
  int n = 0, dat;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  while (n++ < 10000 && INB (devc->osdev, devc->mt_base + 0x05) & 0x30);
  OUTB (devc->osdev, wAddr, devc->mt_base + 0x04);
  OUTB (devc->osdev, 0x10, devc->mt_base + 0x05);	/* Codec read */

  n = 0;
  while (n++ < 10000 && INB (devc->osdev, devc->mt_base + 0x05) & 0x10);
  dat = INW (devc->osdev, devc->mt_base + 0x06);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return dat;
}

static int
ac97_write (void *devc_, int wAddr, int wData)
{
  envy24ht_devc *devc = devc_;
  oss_native_word flags;
  int n = 0;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  while (n++ < 10000 && INB (devc->osdev, devc->mt_base + 0x05) & 0x30);

  OUTB (devc->osdev, wAddr, devc->mt_base + 0x04);
  OUTW (devc->osdev, wData, devc->mt_base + 0x06);
  OUTB (devc->osdev, 0x20, devc->mt_base + 0x05);	/* Codec write */

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return 0;
}

static __inline__ int
input_avail (envy24ht_devc * devc)
{
  unsigned char status;

  status = INB (devc->osdev, devc->ccs_base + 0x0b);
  return status & 0x1f;		/* Number of bytes in RX queue */
}

static __inline__ int
output_ready (envy24ht_devc * devc)
{
  unsigned char status;

  status = INB (devc->osdev, devc->ccs_base + 0x0a);
  return (31 - (status & 0x1f)) > 0;	/* Number of free bytes in TX queue */
}

static __inline__ void
midi_cmd (envy24ht_devc * devc, unsigned char cmd)
{
  OUTB (devc->osdev, cmd, devc->ccs_base + 0x0d);
}

static __inline__ int
midi_read (envy24ht_devc * devc)
{
  return INB (devc->osdev, devc->ccs_base + 0x0c);
}

static __inline__ void
midi_write (envy24ht_devc * devc, unsigned char byte)
{
  OUTB (devc->osdev, byte, devc->ccs_base + 0x0c);
}

static void reset_midi (envy24ht_devc * devc);
static void enter_uart_mode (envy24ht_devc * devc);

static void
midi_input_loop (envy24ht_devc * devc)
{
  int n = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  while (input_avail (devc) && n++ < 33)
    {
      unsigned char c = midi_read (devc);

      devc->input_byte = c;
      if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	devc->midi_input_intr (devc->midi_dev, c);
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
midiintr (envy24ht_devc * devc)
{
  int status;

  status = INB (devc->osdev, devc->ccs_base + 0x02);
  if (status & 0x80)
    midi_input_loop (devc);
  if ((status & 0x20))
    {
      OUTB (devc->osdev, INB (devc->osdev, devc->ccs_base + 0x01) | 0x20,
	    devc->ccs_base + 0x01);

#if 0
      if (devc->midi_output_intr)
	devc->midi_output_intr (devc->midi_dev);
#endif
    }
}

/*ARGSUSED*/
static int
midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
	   oss_midi_inputbuf_t inputbuf, oss_midi_outputintr_t outputintr)
{
  envy24ht_devc *devc = (envy24ht_devc *) midi_devs[dev]->devc;
  int n = 0, tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (devc->midi_opened & mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }
  devc->midi_opened |= mode;

  if (mode & OPEN_READ)
    {
      while (n++ < 33 && input_avail (devc))
	midi_read (devc);

      devc->midi_input_intr = inputbyte;
      devc->midi_output_intr = outputintr;
    }
  enter_uart_mode (devc);
  tmp = INB (devc->osdev, devc->ccs_base + 0x01);
  if (mode & OPEN_READ)
    tmp &= ~0x80;
  OUTB (devc->osdev, tmp, devc->ccs_base + 0x01);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
midi_close (int dev, int mode)
{
  envy24ht_devc *devc = (envy24ht_devc *) midi_devs[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  reset_midi (devc);
  enter_uart_mode (devc);
  reset_midi (devc);
  OUTB (devc->osdev, INB (devc->osdev, devc->ccs_base + 0x01) | 0xa0,
	devc->ccs_base + 0x01);
  devc->midi_opened &= ~mode;
  devc->midi_input_intr = NULL;
  devc->midi_output_intr = NULL;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static int
midi_out (int dev, unsigned char midi_byte)
{
  envy24ht_devc *devc = (envy24ht_devc *) midi_devs[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (!output_ready (devc))
    {
      OUTB (devc->osdev, INB (devc->osdev, devc->ccs_base + 0x01) & ~0x20,
	    devc->ccs_base + 0x01);
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
    }

  midi_write (devc, midi_byte);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 1;
}

/*ARGSUSED*/ 
static int
midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t envy24ht_midi_driver = {
  midi_open,
  midi_close,
  midi_ioctl,
  midi_out
};

static void
enter_uart_mode (envy24ht_devc * devc)
{
  devc->input_byte = 0;
  midi_cmd (devc, 1);
}

void
attach_midi (envy24ht_devc * devc)
{
  char name[128];
  enter_uart_mode (devc);

  sprintf (name, "%s input", devc->model_data->product);
  devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "ENVY24HT", name, &envy24ht_midi_driver, sizeof (midi_driver_t),
					MFLAG_INPUT, devc, devc->osdev);
  sprintf (name, "%s output", devc->model_data->product);
  devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "ENVY24HT", name, &envy24ht_midi_driver, sizeof (midi_driver_t),
					MFLAG_OUTPUT, devc, devc->osdev);
  devc->midi_opened = 0;
  devc->midi_attached = 1;
}

static void
reset_midi (envy24ht_devc * devc)
{
  /*
   * Send the RESET command. Try again if no success at the first time.
   */

  midi_cmd (devc, 0);
}

void
unload_midi (envy24ht_devc * devc)
{
  if (devc->midi_attached)
    reset_midi (devc);
  devc->midi_attached = 0;
}

static int
envy24htintr (oss_device_t * osdev)
{
  envy24ht_devc *devc = osdev->devc;
  int port, status, mt_status, serviced = 0;

  status = INB (devc->osdev, devc->ccs_base + 0x02);
  if (status != 0)
    serviced = 1;

  if (status & 0xa0)
    midiintr (devc);

/*
 * Handle audio interrupts 
 */
  mt_status = INB (devc->osdev, devc->mt_base + 0x00);

  if (mt_status & 0x08)		/* FIFO underrun/overrun */
    {
      OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x1A),
	    devc->mt_base + 0x1A);
      serviced = 1;
    }

  if ((status & 0x10) || mt_status != 0)
    {

      for (port = 0; port < devc->nr_outdevs; port++)
	{
	  envy24ht_portc *portc = &devc->play_portc[port];

	  if (mt_status & portc->mask)
	    {
	      oss_audio_outputintr (portc->dev, 1);
	    }
	}

      for (port = 0; port < devc->nr_indevs; port++)
	{
	  envy24ht_portc *portc = &devc->rec_portc[port];

	  if (mt_status & portc->mask)
	    {
	      oss_audio_inputintr (portc->dev, 0);
	    }
	}
    }
  OUTB (devc->osdev, mt_status, devc->mt_base + 0x00);
  OUTB (devc->osdev, status, devc->ccs_base + 0x02);

  return serviced;
}

/*ARGSUSED*/ 
static int
envy24ht_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static mixer_driver_t envy24ht_mixer_driver = {
  envy24ht_mixer_ioctl
};

static int
envy24ht_set_route (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->hw_devc;
  unsigned int tmp;
  oss_native_word flags;

  if (ctrl < 0 || ctrl > 8)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      tmp = INL (devc->osdev, devc->mt_base + 0x2c);

      switch (ctrl)
	{
	case 0:
	  tmp >>= 8;
	  break;
	case 2:
	  tmp >>= 11;
	  break;
	case 4:
	  tmp >>= 14;
	  break;
	case 6:
	  tmp >>= 17;
	  break;
	}

      tmp = (tmp & 0x03) >> 1;

      if (tmp == 3)
	return 2;
      return tmp;
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {
      int left_shift = 0, right_shift = 0;

      static const unsigned char cnv_tab[3] = { 0x00, 0x02, 0x06 };
      if (value < 0 || value > 2)
	return OSS_EINVAL;

      MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
      tmp = INL (devc->osdev, devc->mt_base + 0x2c);

      switch (ctrl)
	{
	case 0:
	  left_shift = 8;
	  right_shift = 20;
	  break;
	case 2:
	  left_shift = 11;
	  right_shift = 23;
	  break;
	case 4:
	  left_shift = 14;
	  right_shift = 26;
	  break;
	case 6:
	  left_shift = 17;
	  right_shift = 29;
	  break;
	case 8:
	  left_shift = 0;
	  right_shift = 3;
	  break;
	}

      tmp &= ~(0x7 << left_shift);
      tmp &= ~(0x7 << right_shift);
      tmp |= cnv_tab[value] << left_shift;
      if (ctrl != 8)
	tmp |= (cnv_tab[value] + 1) << right_shift;
      else
	tmp |= cnv_tab[value] << right_shift;

      OUTL (devc->osdev, tmp, devc->mt_base + 0x2c);
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

      return value;
    }
  return OSS_EINVAL;
}

static int
read_peak (envy24ht_devc * devc, int ch)
{
  int tmp;

  if (ch >= 22)
    return 0;

  OUTB (devc->osdev, ch, devc->mt_base + 0x3e);
  tmp = INB (devc->osdev, devc->mt_base + 0x3f);

  return tmp;
}

/*ARGSUSED*/ 
static int
envy24ht_get_peak (int dev, int ctrl, unsigned int cmd, int value)
{
  static const unsigned char peak_cnv[256] = {
    0, 18, 29, 36, 42, 47, 51, 54, 57, 60, 62, 65, 67, 69, 71, 72,
    74, 75, 77, 78, 79, 81, 82, 83, 84, 85, 86, 87, 88, 89, 89, 90,
    91, 92, 93, 93, 94, 95, 95, 96, 97, 97, 98, 99, 99, 100, 100, 101,
    101, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 107, 107, 108, 108,
    108,
    109, 109, 110, 110, 110, 111, 111, 111, 112, 112, 113, 113, 113, 114, 114,
    114,
    115, 115, 115, 115, 116, 116, 116, 117, 117, 117, 118, 118, 118, 118, 119,
    119,
    119, 119, 120, 120, 120, 121, 121, 121, 121, 122, 122, 122, 122, 122, 123,
    123,
    123, 123, 124, 124, 124, 124, 125, 125, 125, 125, 125, 126, 126, 126, 126,
    126,
    127, 127, 127, 127, 127, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129,
    130,
    130, 130, 130, 130, 130, 131, 131, 131, 131, 131, 131, 132, 132, 132, 132,
    132,
    132, 133, 133, 133, 133, 133, 133, 134, 134, 134, 134, 134, 134, 134, 135,
    135,
    135, 135, 135, 135, 135, 136, 136, 136, 136, 136, 136, 136, 137, 137, 137,
    137,
    137, 137, 137, 138, 138, 138, 138, 138, 138, 138, 138, 139, 139, 139, 139,
    139,
    139, 139, 139, 140, 140, 140, 140, 140, 140, 140, 140, 141, 141, 141, 141,
    141,
    141, 141, 141, 141, 142, 142, 142, 142, 142, 142, 142, 142, 142, 143, 143,
    143,
    143, 143, 143, 143, 143, 143, 144, 144, 144, 144, 144, 144, 144, 144, 144,
    144,
  };

  envy24ht_devc *devc = mixer_devs[dev]->hw_devc;

  int i, orign, n = -1, left = 0, right = 0;

  for (i = 0; i < 12 && n == -1; i++)
    if (ctrl & (1 << i))
      n = i;

  if (n == -1)
    return OSS_EINVAL;

  orign = n;
  if (ctrl & 0x80000000)
    n += 10;			/* Recording stream */

  if (cmd == SNDCTL_MIX_READ)
    {
      left = read_peak (devc, n);
      if (ctrl & (1 << (orign + 1)))	/* Stereo mode? */
	right = read_peak (devc, n + 1);
      else
	right = left;

      left = peak_cnv[left];
      right = peak_cnv[right];
      return left | (right << 8);
    }

  return OSS_EINVAL;
}

/*ARGSUSED*/ 
static int
create_peak_mixer (int dev, envy24ht_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, num, skip;
  int nc = devc->nr_play_channels;
  char tmp[64];

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_PEAK")) < 0)
    return group;

  skip = 2;

  for (i = 0; i < nc; i += skip)
    {

      num = 1 << i;
      if (!(mask & num))
	continue;		/* Not present */

      {
	if (i == 8)
	  strcpy (tmp, "ENVY24_SPDIFOUT");
	else
	  sprintf (tmp, devc->channel_names[i / 2], i + 1, i + 2);

	num |= 1 << (i + 1);
	if ((err = mixer_ext_create_control (dev, group,
					     num, envy24ht_get_peak,
					     MIXT_STEREOPEAK,
					     tmp, 144,
					     MIXF_READABLE | MIXF_DECIBEL)) <
	    0)
	  return err;
      }
    }

  mask = devc->inportmask;
  for (i = 0; i < devc->nr_rec_channels; i += skip)
    {

      num = 1 << i;
      if (!(mask & num))
	continue;		/* Not present */

      num |= 0x80000000;	/* Input flag */

      {
	if (i == 8)
	  strcpy (tmp, "ENVY24_SPDIFIN");
	else
	  strcpy (tmp, "ENVY24_IN");

	num |= 1 << (i + 1);
	if ((err = mixer_ext_create_control (dev, group,
					     num, envy24ht_get_peak,
					     MIXT_STEREOPEAK,
					     tmp, 144,
					     MIXF_READABLE | MIXF_DECIBEL)) <
	    0)
	  return err;
      }
    }

  return 0;
}

/*ARGSUSED*/ 
static int
create_rout_mixer (int dev, envy24ht_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, ret, num;
  char *name;

  if ((group =
       mixer_ext_create_group_flags (dev, 0, "ENVY24_ROUTE", MIXF_FLAT)) < 0)
    return group;

  for (i = 0; i < 8; i += 2)
    {

      num = 1 << i;
      if (!(mask & num))
	continue;		/* Not present */

      name = devc->channel_names[i / 2];

      if ((ret = mixer_ext_create_control (dev, group,
					   i,
					   envy24ht_set_route,
					   MIXT_ENUM, name, 3,
					   MIXF_READABLE |
					   MIXF_WRITEABLE)) < 0)
	return ret;
      mixer_ext_set_strings (dev, ret, "DMA ANALOGIN DIGITALIN", 0);
    }

  if (devc->model_data == NULL)
    {
      cmn_err (CE_CONT, "Internal error: No model data\n");
      return 0;
    }

  mask = devc->inportmask;

  if (devc->model_data->flags & MF_SPDIFOUT)
    {
      if ((ret = mixer_ext_create_control (dev, group,
					   8, envy24ht_set_route,
					   MIXT_ENUM,
					   "ENVY24_SPDIFOUT", 3,
					   MIXF_READABLE |
					   MIXF_WRITEABLE)) < 0)
	return ret;
    }

  return 0;
}

/*
 * S/PDIF lowlevel driver
 */
/*ARGSUSED*/ 
static int
default_reprogram_device (void *_devc, void *_portc,
			  oss_digital_control * ctl, unsigned int mask)
{
  unsigned char c;
  unsigned short cbits = 0;
  envy24ht_devc *devc = _devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  cbits |= (ctl->rate_bits & 0x0f) << 12;	/* Sample rate */

  if (ctl->out_vbit == VBIT_ON)
    cbits |= 0x8000;		/* Turn on the V bit */

  cbits |= ctl->cbitout[0] & 0x07;	/* Consumer/pro, audio/data, copyright */
  cbits |= (!!(ctl->cbitout[1] & 0x80)) << 11;	/* Generation level */
  cbits |= (ctl->cbitout[1] & 0x7f) << 4;	/* Category code */
  cbits |= (ctl->emphasis_type & 1) << 3;	/* Pre-emphasis */

  if (cbits != INW (devc->osdev, devc->mt_base + 0x3c))
    {
      c = INB (devc->osdev, devc->ccs_base + 0x07);
      OUTB (devc->osdev, c & ~0x80, devc->ccs_base + 0x07);	/* Disable S/PDIF transmitter */
      OUTW (devc->osdev, cbits, devc->mt_base + 0x3c);
      OUTB (devc->osdev, c | 0x80, devc->ccs_base + 0x07);	/* (Re)enable S/PDIF transmitter */
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

spdif_driver_t default_spdif_driver = {
/* reprogram_device: */ default_reprogram_device,
};


static void
setup_sample_rate (envy24ht_devc * devc)
{
  unsigned char bits;
  int change = 0;

  devc->speedbits = bits = speed_tab[devc->pending_speed_sel].speedbits;
  if (devc->speed != speed_tab[devc->pending_speed_sel].speed)
    change = 1;
  devc->speed = devc->pending_speed =
    speed_tab[devc->pending_speed_sel].speed;
  mixer_devs[devc->mixer_dev]->modify_counter++;

  if (change)
    {
      oss_spdif_setrate (&devc->spdc, devc->speed);

      if (devc->model_data->svid == SSID_JULIA)
	goto JULIA;

      if (devc->syncsource != 0)	/* External sync */
	bits |= 0x10;

      if (devc->speed > 120000)
	{
	  OUTB (devc->osdev, 0x08, devc->mt_base + 0x02);	/* 128x I2S setup */
	}
      else
	{
	  OUTB (devc->osdev, 0x00, devc->mt_base + 0x02);	/* 256x I2S setup */
	}

      OUTB (devc->osdev, bits & 0x0f, devc->mt_base + 0x01);	/* Sampling rate */
JULIA:
      if (devc->auxdrv->set_rate)
	devc->auxdrv->set_rate (devc);
    }
}

static int
envy24ht_set_control (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24ht_devc *devc = mixer_devs[dev]->hw_devc;
  oss_native_word flags;

  if (cmd == SNDCTL_MIX_READ)
    switch (ctrl)
      {
      case 1:
	return devc->pending_speed_sel;
	break;

      case 2:
	return devc->syncsource;
	break;

      case 3:
	return devc->use_src;
	break;

      case 5:
	return devc->ratelock;
	break;

      case 6:
	return devc->speed;
	break;

      case 7:
	return 1;

      default:
	return OSS_EIO;
      }

  if (cmd == SNDCTL_MIX_WRITE)
    switch (ctrl)
      {
      case 1:
	if (value < 0 || value > devc->max_ratesel)
	  return OSS_EIO;
	MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
	if (devc->configured_rate_sel != value)
	  {
	    devc->configured_rate_sel = value;
	    if (devc->open_count < 1)
	      {
		devc->pending_speed_sel = devc->configured_rate_sel;
		setup_sample_rate (devc);
	      }
	  }
	MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	return devc->configured_rate_sel;
	break;

      case 2:
	if (value < 0 || value > 2)
	  return OSS_EIO;
	devc->syncsource = value;
	if (devc->model_data->svid == SSID_JULIA)
	    devc->auxdrv->private1 (devc, value);
	return devc->syncsource;
	break;

      case 3:
	devc->use_src = !!value;
	return devc->use_src;
	break;

      case 5:
	return devc->ratelock = !!value;
	break;

      case 6:
	return devc->speed;
	break;

      case 7:
	return 1;
	break;

      default:
	return OSS_EIO;
      }

  return OSS_EINVAL;
}

static int
envy24ht_mix_init (int dev)
{
  envy24ht_devc *devc = mixer_devs[dev]->hw_devc;
  int group, err, n;

  if ((group =
       mixer_ext_create_group_flags (dev, 0, "ENVY24", MIXF_FLAT)) < 0)
    return group;

  if ((err = create_peak_mixer (dev, devc, group)) < 0)
    return err;

  if ((err = create_rout_mixer (dev, devc, group)) < 0)
    return err;

  n = devc->max_ratesel + 1;
  if ((err = mixer_ext_create_control (dev, group,
				       1, envy24ht_set_control,
				       MIXT_ENUM,
				       "ENVY24_RATE", n,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  mixer_ext_set_strings (dev, err, 
   "8000 9600 11025 12000 16000 22050 24000 32000 44100 48000 64000 88200 96000 176400 192000", 0);

  if ((err = mixer_ext_create_control (dev, group,
				       2, envy24ht_set_control,
				       MIXT_ENUM,
				       "ENVY24_SYNC", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       3, envy24ht_set_control,
				       MIXT_ONOFF,
				       "ENVY24_SRC", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       5, envy24ht_set_control,
				       MIXT_ONOFF,
				       "ENVY24_RATELOCK", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       6, envy24ht_set_control,
				       MIXT_VALUE,
				       "ENVY24_ACTRATE", 192000,
				       MIXF_READABLE)) < 0)
    return err;

  if (devc->auxdrv->mixer_init)
    if ((err = devc->auxdrv->mixer_init (devc, dev, 0)) < 0)
      return err;

  if ((err = oss_spdif_mix_init (&devc->spdc)) < 0)
    return err;

  return 0;
}

static int
eeprom_read (envy24ht_devc * devc, int pos)
{
  int i, status;

  for (i = 0; i < 0x10000; i++)
    {
      status = INB (devc->osdev, devc->ccs_base + 0x13);
      if (!(status & 1))
	break;

    }

  OUTB (devc->osdev, pos, devc->ccs_base + 0x11);	/* Offset */
  OUTB (devc->osdev, 0xa0, devc->ccs_base + 0x10);	/* EEPROM read */

  for (i = 0; i < 0x10000; i++)
    {
      status = INB (devc->osdev, devc->ccs_base + 0x13);
      if (!(status & 1))
	break;

    }

  oss_udelay (1);
  return INB (devc->osdev, devc->ccs_base + 0x12);
}

static void
envy24pt_init (envy24ht_devc * devc)
{
  int gpio;

  gpio = INW (devc->osdev, devc->ccs_base + 0x14);
  gpio |= INB (devc->osdev, devc->ccs_base + 0x1e) << 16;

#if 0
#define GPBIT(nn) !!(1<<nn)
  cmn_err (CE_CONT, "GPIO=%06x\n", gpio);
  cmn_err (CE_CONT, "With SPDIF_IN 'optical' connector: %d\n", GPBIT (1));
  cmn_err (CE_CONT, "With SPDIF_IN 'coaxial' connector: %d\n", GPBIT (2));

  cmn_err (CE_CONT, "AC97 with stereo DAC for 7.1: %d\n", !GPBIT (8));
  cmn_err (CE_CONT, "Extra ADC/DAC connected to S/PDIF pins: %d\n",
	   GPBIT (9));
  cmn_err (CE_CONT, "S/PDIF connected to RDMA1: %d\n", !GPBIT (10));
  cmn_err (CE_CONT, "Smart 5.1 function supported: %d\n", GPBIT (11));
  cmn_err (CE_CONT, "De-POP function supported: %d\n", !GPBIT (12));
  cmn_err (CE_CONT, "PDMA4 DAC: 0=cs4321 1=WM8728: %d\n", GPBIT (13));
#endif

  OUTB (devc->osdev, 0x00, devc->ccs_base + 0x04);	/* System configuration */
  OUTB (devc->osdev, 0x02, devc->ccs_base + 0x05);	/* AC-link configuration */
  OUTB (devc->osdev, 0x00, devc->ccs_base + 0x06);	/* I2S configuration */
  OUTB (devc->osdev, 0x83, devc->ccs_base + 0x07);	/* S/PDIF configuration */

  /* TODO: GPIO initialization */
}

static void
julia_eeprom_init (envy24ht_devc * devc)
{
  OUTB (devc->osdev, 0x39, devc->ccs_base + 0x04);	/* System configuration */
  OUTB (devc->osdev, 0x80, devc->ccs_base + 0x05);	/* AC-link configuration */
  OUTB (devc->osdev, 0x78, devc->ccs_base + 0x06);	/* I2S configuration */
  OUTB (devc->osdev, 0xc3, devc->ccs_base + 0x07);	/* S/PDIF configuration */

  OUTW (devc->osdev, 0xffff, devc->ccs_base + 0x18);	/* GPIO direction */
  OUTW (devc->osdev, 0x0000, devc->ccs_base + 0x16);	/* GPIO write mask */
  OUTW (devc->osdev, 0x801A, devc->ccs_base + 0x14);	/* Initital bit state */
}

static int
init_eeprom_v2 (envy24ht_devc * devc)
{
  unsigned char *eeprom = (unsigned char *) &devc->eeprom;

  OUTB (devc->osdev, eeprom[6], devc->ccs_base + 0x04);
  OUTB (devc->osdev, eeprom[7], devc->ccs_base + 0x05);
  OUTB (devc->osdev, eeprom[8], devc->ccs_base + 0x06);
  OUTB (devc->osdev, eeprom[9], devc->ccs_base + 0x07);

  OUTB (devc->osdev, eeprom[10], devc->ccs_base + 0x18);
  OUTB (devc->osdev, eeprom[11], devc->ccs_base + 0x19);
  OUTB (devc->osdev, eeprom[12], devc->ccs_base + 0x1a);

  OUTB (devc->osdev, eeprom[13], devc->ccs_base + 0x16);
  OUTB (devc->osdev, eeprom[14], devc->ccs_base + 0x17);
  OUTB (devc->osdev, eeprom[15], devc->ccs_base + 0x1f);

  OUTB (devc->osdev, eeprom[16], devc->ccs_base + 0x14);
  OUTB (devc->osdev, eeprom[17], devc->ccs_base + 0x15);
  OUTB (devc->osdev, eeprom[18], devc->ccs_base + 0x1e);
  return 1;
}

static int
load_eeprom (envy24ht_devc * devc)
{
  int status, i, check;
  unsigned char *eeprom = (unsigned char *) &devc->eeprom, c;
  static const char resolutions[4] = { 16, 18, 20, 24 };

  c = 0;

  status = INB (devc->osdev, devc->ccs_base + 0x13);

  if (!(status & 0x80))
    return 0;			/* No EEPROM */

  devc->eeprom.bSize = sizeof (devc->eeprom);	/* Guess the size */

  for (i = 0; i < devc->eeprom.bSize; i++)
    {
      eeprom[i] = eeprom_read (devc, i);
      eeprom[i] = eeprom_read (devc, i);

      if (devc->eeprom.bSize > sizeof (devc->eeprom))
	devc->eeprom.bSize = sizeof (devc->eeprom);
    }
#if 1
  DDB (cmn_err (CE_CONT, "EEPROM="));
  for (i = 0; i < devc->eeprom.bSize; i++)
    DDB (cmn_err (CE_CONT, "0x%02x, ", eeprom[i]));
  DDB (cmn_err (CE_CONT, "\n"));
#endif

  check = 0;
  for (i = 0; i < 4; i++)
    {
      check <<= 8;
      check |= eeprom[i];
    }

  if (check != devc->model_data->svid)
    cmn_err (CE_CONT,
	     "Envy24 WARNING: Possible EEPROM read error %08x != %08x\n",
	     check, devc->model_data->svid);

  DDB (cmn_err (CE_CONT, "EEPROM version %d\n", devc->eeprom.bVersion));

  if (devc->eeprom.bVersion == 2)
    {
      return init_eeprom_v2 (devc);
    }

  /* Init the controller registers based on the EEPROM data */

  OUTB (devc->osdev, devc->eeprom.bCodecConfig, devc->ccs_base + 0x04);
  OUTB (devc->osdev, devc->eeprom.bACLinkConfig, devc->ccs_base + 0x05);
  OUTB (devc->osdev, devc->eeprom.bI2SID, devc->ccs_base + 0x06);
  OUTB (devc->osdev, devc->eeprom.bSpdifConfig, devc->ccs_base + 0x07);

  /* GPIO ports */

  OUTB (devc->osdev, devc->eeprom.bGPIODirection2, devc->ccs_base + 0x18);
  OUTB (devc->osdev, devc->eeprom.bGPIODirection1, devc->ccs_base + 0x19);
  OUTB (devc->osdev, devc->eeprom.bGPIODirection0, devc->ccs_base + 0x1a);

  OUTB (devc->osdev, devc->eeprom.bGPIOInitMask2, devc->ccs_base + 0x14);
  OUTB (devc->osdev, devc->eeprom.bGPIOInitMask1, devc->ccs_base + 0x15);
  OUTB (devc->osdev, devc->eeprom.bGPIOInitMask0, devc->ccs_base + 0x1f);

  OUTB (devc->osdev, devc->eeprom.bGPIOInitState2, devc->ccs_base + 0x14);
  OUTB (devc->osdev, devc->eeprom.bGPIOInitState1, devc->ccs_base + 0x15);
  OUTB (devc->osdev, devc->eeprom.bGPIOInitState0, devc->ccs_base + 0x1e);

#if 1
  DDB (cmn_err (CE_CONT, "GPIO=%02x%02x%02x (%02x%02x%02x, %02x%02x%02x)\n",
		devc->eeprom.bGPIOInitState2,
		devc->eeprom.bGPIOInitState1,
		devc->eeprom.bGPIOInitState0,
		devc->eeprom.bGPIODirection2,
		devc->eeprom.bGPIODirection1,
		devc->eeprom.bGPIODirection0,
		devc->eeprom.bGPIOInitMask2,
		devc->eeprom.bGPIOInitMask1, devc->eeprom.bGPIOInitMask0));

  c = devc->eeprom.bCodecConfig;
  switch ((c >> 6) % 0x03)
    {
    case 0:
      DDB (cmn_err (CE_CONT, "24.576MHz crystal\n"));
      break;
    case 1:
      DDB (cmn_err (CE_CONT, "49.152MHz crystal\n"));
      break;
    default:
      DDB (cmn_err (CE_CONT, "Unknown crystal frequency\n"));
    }

  if (c & 0x20)
    {
      DDB (cmn_err (CE_CONT, "Has MPU401 UART\n"));
    }
  else
    {
      DDB (cmn_err (CE_CONT, "No MPU401 UART\n"));
    }
  DDB (cmn_err
       (CE_CONT, "%d stereo ADC pairs connected\n", ((c >> 2) & 0x03) + 1));
  DDB (cmn_err (CE_CONT, "%d stereo DAC pairs connected\n", (c & 0x03) + 1));

  c = devc->eeprom.bACLinkConfig;
  DDB (cmn_err
       (CE_CONT, "Converter type: %s\n", (c & 0x80) ? "I2S" : "AC97"));
  if (!(c & 0x80))
    {
      if (!(devc->model_data->flags & MF_NOAC97))
	devc->codec_type = CODEC_AC97;

      DDB (cmn_err (CE_NOTE,
		    "AC link connection mode type: %s\n",
		    (c & 0x02) ? "packed" : "split"));
    }
  else
    {
      c = devc->eeprom.bI2SID;

      DDB (cmn_err (CE_CONT, "I2C codec has volume control/mute: %s\n",
		    (c % 0x80) ? "YES" : "NO"));
      DDB (cmn_err (CE_CONT, "I2C codec has 96 KHz S/R support: %s\n",
		    (c % 0x40) ? "YES" : "NO"));
      DDB (cmn_err (CE_CONT, "I2C codec has 192 KHz S/R support: %s\n",
		    (c % 0x08) ? "YES" : "NO"));

      DDB (cmn_err (CE_CONT,
		    "Converter resolution %d bits\n",
		    resolutions[(c >> 4) & 0x03]));
    }

  c = INB (devc->osdev, devc->ccs_base + 0x07);
  DDB (cmn_err (CE_CONT,
		"Internal S/PDIF out implemented: %s\n",
		(c & 0x40) ? "YES" : "NO"));
  DDB (cmn_err
       (CE_CONT, "Internal S/PDIF out enabled: %s\n",
	(c & 0x80) ? "YES" : "NO"));
  DDB (cmn_err
       (CE_CONT, "External S/PDIF out implemented: %s\n",
	(c & 0x01) ? "YES" : "NO"));
  DDB (cmn_err
       (CE_CONT, "S/PDIF input present: %s\n", (c & 0x02) ? "YES" : "NO"));
  DDB (cmn_err (CE_CONT, "S/PDIF chip IDs %x\n", (c >> 2) & 0x0f));
#endif

  return 1;
}

/*ARGSUSED*/ 
static void
dump_regs (envy24ht_devc * devc, char *lbl)
{
#if 0
  int i;

  cmn_err (CE_CONT, "\nDump registers: %s\n", lbl);

  for (i = 0; i < 0x20; i += 4)
    {
      if (!(i % (8 * 4)))
	cmn_err (CE_CONT, "\nCCS%02x: ", i);
      cmn_err (CE_CONT, "%08x ", INL (devc->osdev, devc->ccs_base + i));
    }
  cmn_err (CE_CONT, "\n");

  for (i = 0; i < 0x80; i += 4)
    {
      if (!(i % (8 * 4)))
	cmn_err (CE_CONT, "\nMT%02x: ", i);
      cmn_err (CE_CONT, "%08x ", INL (devc->osdev, devc->mt_base + i));
    }
  cmn_err (CE_CONT, "\n");
#endif
}

static int
verify_rate (envy24ht_devc * devc, int arg)
{
  if (devc->codec_type == CODEC_AC97 && arg > 48000)
    arg = 48000;
  if (arg > 96000 && !(devc->model_data->flags & MF_192K))
    arg = 96000;

  return arg;
}

static int
envy24ht_set_rate (int dev, int arg)
{
  envy24ht_devc *devc = audio_engines[dev]->devc;
  envy24ht_portc *portc = audio_engines[dev]->portc;
  int i, ix, diff, bestdiff;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (arg == 0)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return devc->pending_speed;
    }

  arg = verify_rate (devc, arg);
/*
 * Don't permit changing the sampling rate if we have multiple clients.
 */
  if (devc->open_count != 1 || devc->ratelock)
    {
      DDB (cmn_err (CE_CONT, "Can't set speed: open_count %d, ratelock %d\n",
		    devc->open_count, devc->ratelock));
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      if (arg != devc->pending_speed)
	{
	  audio_engines[dev]->fixed_rate = devc->speed;
	  audio_engines[dev]->min_rate = devc->speed;
	  audio_engines[dev]->max_rate = devc->speed;
	  audio_engines[dev]->flags |= ADEV_FIXEDRATE;
	}
      else
	{
	  audio_engines[dev]->min_rate = 8000;
	  audio_engines[dev]->max_rate = 192000;
	  audio_engines[dev]->flags &= ~ADEV_FIXEDRATE;
	}
      return devc->pending_speed;
    }

  if (portc->dev_flags & DF_SPDIF)
    {
      /* Allow only supported S/PDIF rates */
      if (arg < 32000)
	arg = 32000;
      if (arg > 96000)
	arg = 96000;
    }

  ix = 9;
  bestdiff = 1000000;
  i = 0;
  audio_engines[dev]->flags &= ~ADEV_FIXEDRATE;

  while (speed_tab[i].speed != -1)
    {
      diff = speed_tab[i].speed - arg;
      if (diff < 0)
	diff = -diff;
      if (diff < bestdiff)
	{
	  ix = i;
	  bestdiff = diff;
	}
      i++;
    }

  devc->pending_speed = speed_tab[ix].speed;
  devc->pending_speed_sel = ix;
  /*cmn_err(CE_CONT, "Requested sampling rate %d, got %d\n", arg, devc->pending_speed); */

  //setup_sample_rate (devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return devc->pending_speed;
}

static short
envy24ht_set_channels (int dev, short arg)
{
  envy24ht_portc *portc = audio_engines[dev]->portc;
  envy24ht_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  if (arg == 0)
    return portc->channels;

  if (portc->dev_flags & DF_MULTICH)
    {
      int n = 2, ch, i, mask;

      if (arg < 2)
	arg = 2;

      arg = ((arg + 1) / 2) * 2;	/* Round to even number of channels */

      if (arg > devc->model_data->nr_outs)
	arg = devc->model_data->nr_outs;

      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

      devc->busy_play_channels &= ~portc->used_chmask;

      for (ch = 2; ch <= arg; ch += 2)
	{
	  mask = 0;

	  for (i = 0; i < ch; i++)
	    mask |= (1 << i);

	  if (devc->busy_play_channels & mask)
	    break;
	  n = ch;
	}

      portc->channels = n;
      portc->used_chmask = 0;
      for (i = 0; i < n; i++)
	portc->used_chmask |= (1 << i);

      devc->busy_play_channels |= portc->used_chmask;
      /* MT19: Channel allocation */
      OUTB (devc->osdev, 4 - n / 2, devc->mt_base + 0x19);
      /* cmn_err(CE_CONT, "%d channels: MT19=%02x\n", n, INB(devc->osdev, devc->mt_base+0x19)); */

      if (portc->channels == 6)
	{
	  /* The fragment size must be a multiple of 6 */
	  audio_engines[dev]->min_block = 4 * 288;
	  audio_engines[dev]->max_block = 4 * 288;

	}
      else
	{
	  audio_engines[dev]->min_block = 0;
	  audio_engines[dev]->max_block = 0;
	}

      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return portc->channels;
    }

  return portc->channels = 2;
}

/*ARGSUSED*/ 
static int
ac3_write (adev_t * adev,
	   dmap_t * dmap,
	   void *frombuf, void *tobuf, int maxspace, int *fromlen, int *tolen)
{
/*
 * This routine takes AC3 input 16 bits at time and packs them to
 * 32 bit words.
 */
  int i, l;
  unsigned short *ip;
  unsigned int *op;

  l = *fromlen * 2;
  if (l > maxspace)
    {
      l = maxspace;
    }

  *tolen = l;
  *fromlen = l / 2;
  l /= 4;

  ip = frombuf;
  op = tobuf;

  for (i = 0; i < l; i++)
    {
      *op++ = (*ip++) << 16;
    }

  return 0;
}

static unsigned int
envy24ht_set_format (int dev, unsigned int arg)
{
  envy24ht_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->fmt;

  if (arg == AFMT_AC3 && (portc->dev_flags & DF_AC3))
    {
      audio_engines[dev]->dmap_out->device_write = ac3_write;
      return portc->fmt = AFMT_AC3;
    }
  audio_engines[dev]->dmap_out->device_write = NULL;
  return portc->fmt = SUPPORTED_FORMAT;
}

static int
envy24ht_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  envy24ht_portc *portc = audio_engines[dev]->portc;
  envy24ht_devc *devc = audio_engines[dev]->devc;

  switch (cmd)
    {
    case SNDCTL_DSP_GET_CHNORDER:
      *(oss_uint64_t *) arg = CHNORDER_NORMAL;
      return 0;
    }

  if (portc->dev_flags & DF_SPDIF)
    {
      int ret;
      ret = oss_spdif_ioctl (&devc->spdc, portc->open_mode, cmd, arg);
      if (ret != SPDIF_NOIOCTL)
	return ret;
    }

  if (devc->auxdrv->audio_ioctl)
    return devc->auxdrv->audio_ioctl (devc, portc, cmd, arg);
  return OSS_EINVAL;
}

static void envy24ht_trigger (int dev, int state);

static void
envy24ht_reset (int dev)
{
  envy24ht_trigger (dev, 0);
}

/*ARGSUSED*/ 
static int
envy24ht_open_input (int dev, int mode, int open_flags)
{
  envy24ht_portc *portc = audio_engines[dev]->portc;
  envy24ht_devc *devc = audio_engines[dev]->devc;
  adev_p adev = audio_engines[dev];
  oss_native_word flags;

  if (mode & OPEN_WRITE)
    {
      cmn_err (CE_CONT, "Playback is not possible with %s\n", adev->devnode);
      return OSS_ENOTSUP;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode || (devc->busy_rec_channels & portc->chmask))
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }
  portc->open_mode = mode;
  devc->open_count++;
  if (devc->open_count == 1)
    {
      devc->pending_speed_sel = devc->configured_rate_sel;
    }

  if (portc->dev_flags & DF_SPDIF)
    oss_spdif_open (&devc->spdc, mode);

  portc->used_chmask = portc->chmask;
  devc->busy_rec_channels |= portc->chmask;

  if (!devc->use_src)
    adev->flags |= ADEV_NOSRC;
  else
    adev->flags &= ~ADEV_NOSRC;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/ 
static int
envy24ht_open_output (int dev, int mode, int open_flags)
{
  envy24ht_portc *portc = audio_engines[dev]->portc;
  envy24ht_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  adev_p adev = audio_engines[dev];

  if (mode & OPEN_READ)
    {
      cmn_err (CE_CONT, "Recording is not possible with %s\n", adev->devnode);
      return OSS_ENOTSUP;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode || (devc->busy_play_channels & portc->chmask))
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  portc->used_chmask = portc->chmask;
  devc->busy_play_channels |= portc->chmask;
  audio_engines[dev]->dmap_out->device_write = NULL;

  devc->open_count++;
  if (devc->open_count == 1)
    {
      devc->pending_speed_sel = devc->configured_rate_sel;
    }

  if (portc->dev_flags & DF_SPDIF)
    oss_spdif_open (&devc->spdc, mode);

  if (!devc->use_src)
    adev->flags |= ADEV_NOSRC;
  else
    adev->flags &= ~ADEV_NOSRC;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
envy24ht_close (int dev, int mode)
{
  envy24ht_portc *portc = audio_engines[dev]->portc;
  envy24ht_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->open_count--;

  if (portc->open_mode & OPEN_READ)
    devc->busy_rec_channels &= ~portc->used_chmask;
  if (portc->open_mode & OPEN_WRITE)
    devc->busy_play_channels &= ~portc->used_chmask;
  portc->open_mode = 0;

  if (portc->dev_flags & DF_MULTICH)
    {
      OUTB (devc->osdev, 0x03, devc->mt_base + 0x19);	/* Channel allocation */
      portc->chmask = 0x003;	/* Just the front channels */
    }

  if (portc->dev_flags & DF_SPDIF)
    oss_spdif_close (&devc->spdc, mode);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/ 
static void
envy24ht_output_block (int dev, oss_native_word buf, int count, int fragsize,
		       int intrflag)
{
}

/*ARGSUSED*/ 
static void
envy24ht_start_input (int dev, oss_native_word buf, int count, int fragsize,
		      int intrflag)
{
}

static void
envy24ht_trigger (int dev, int state)
{
  envy24ht_devc *devc = audio_engines[dev]->devc;
  envy24ht_portc *portc = audio_engines[dev]->portc;
  unsigned char enable, intrmask;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  enable = INB (devc->osdev, devc->mt_base + 0x18);
  intrmask = INB (devc->osdev, devc->mt_base + 0x03);

  if (portc->state_bits == state)	/* No change */
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return;
    }
  portc->state_bits = state;

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  enable |= portc->mask;
	  intrmask &= ~portc->mask;
	}
      else
	{
	  enable &= ~portc->mask;
	  intrmask |= portc->mask;
	}
    }

  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  enable |= portc->mask;
	  intrmask &= ~portc->mask;
	}
      else
	{
	  enable &= ~portc->mask;
	  intrmask |= portc->mask;
	}
    }
  OUTB (devc->osdev, enable, devc->mt_base + 0x18);
  OUTB (devc->osdev, intrmask, devc->mt_base + 0x03);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (state)
    dump_regs (devc, "trigger");
}

/*ARGSUSED*/ 
static int
envy24ht_prepare_for_input (int dev, int bsize, int bcount)
{
  envy24ht_devc *devc = audio_engines[dev]->devc;
  envy24ht_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_in;
  int buffsize, fragsize;
  oss_native_word flags;

  if (audio_engines[dev]->flags & ADEV_NOINPUT)
    return OSS_EACCES;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  setup_sample_rate (devc);
  buffsize = dmap->bytes_in_use / 4 - 1;
  fragsize = dmap->fragment_size / 4 - 1;

  OUTL (devc->osdev, dmap->dmabuf_phys, portc->base + 0x00);
  OUTW (devc->osdev, buffsize, portc->base + 0x04);
  OUTW (devc->osdev, fragsize, portc->base + 0x06);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/ 
static int
envy24ht_prepare_for_output (int dev, int bsize, int bcount)
{
  envy24ht_devc *devc = audio_engines[dev]->devc;
  envy24ht_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_out;
  int buffsize, fragsize;
  oss_native_word flags;

  if (audio_engines[dev]->flags & ADEV_NOOUTPUT)
    return OSS_EACCES;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  setup_sample_rate (devc);
  buffsize = dmap->bytes_in_use / 4 - 1;
  fragsize = dmap->fragment_size / 4 - 1;

  if (portc->dev_flags & DF_MULTICH)
    {
      /* Multi ch device */
      OUTL (devc->osdev, dmap->dmabuf_phys, devc->mt_base + 0x10);
      OUTL (devc->osdev, buffsize, devc->mt_base + 0x14);
      OUTL (devc->osdev, fragsize, devc->mt_base + 0x1c);
    }
  else
    {
      OUTL (devc->osdev, dmap->dmabuf_phys, portc->base + 0x00);
      OUTW (devc->osdev, buffsize, portc->base + 0x04);
      OUTW (devc->osdev, fragsize, portc->base + 0x06);
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/ 
static int
envy24ht_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  envy24ht_portc *portc = audio_engines[dev]->portc;
  envy24ht_devc *devc;
  int pos;

  devc = audio_engines[dev]->devc;
  pos = (INW (devc->osdev, portc->base + 0x04) + 1) * 4;
  return dmap->bytes_in_use - pos;
}

static int
envy24ht_sync_control(int dev, int event, int mode)
{
  envy24ht_devc *devc = audio_engines[dev]->devc;
  envy24ht_portc *portc = audio_engines[dev]->portc;
  unsigned char enable, intrmask;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE(devc->mutex, flags);
  if(event == SYNC_PREPARE)
  {
    devc->syncstart_mask |= portc->mask;
    portc->state_bits = mode;
  }
  else if(event == SYNC_TRIGGER)
  {
    if(devc->syncstart_mask)
    {
      enable = INB (devc->osdev, devc->mt_base + 0x18);
      intrmask = INB (devc->osdev, devc->mt_base + 0x03);
      enable |= devc->syncstart_mask;
      intrmask &= ~devc->syncstart_mask;
      OUTB (devc->osdev, enable, devc->mt_base + 0x18);
      OUTB (devc->osdev, intrmask, devc->mt_base + 0x03);
      devc->syncstart_mask = 0;
    }
  }
  MUTEX_EXIT_IRQRESTORE(devc->mutex, flags);
  return 0;
}

#if 0
static int
envy24ht_check_output (int dev)
{
  int pos;
  envy24ht_devc *devc = audio_engines[dev]->devc;

  pos = envy24ht_get_buffer_pointer (dev, audio_engines[dev]->dmap_out, 0);

  cmn_err (CE_CONT,
	   "Envy24ht: Output timeout on device %d (%d, %02x, %02x)\n", dev,
	   pos, INB (devc->osdev, devc->ccs_base + 0x02), INB (devc->osdev,
							       devc->
							       ccs_base +
							       0x00));
  return OSS_EIO;
}
#endif

static audiodrv_t envy24ht_output_driver = {
  envy24ht_open_output,
  envy24ht_close,
  envy24ht_output_block,
  envy24ht_start_input,
  envy24ht_ioctl,
  envy24ht_prepare_for_input,
  envy24ht_prepare_for_output,
  envy24ht_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  envy24ht_trigger,
  envy24ht_set_rate,
  envy24ht_set_format,
  envy24ht_set_channels,
  NULL,
  NULL,
  NULL,				/* check input */
  NULL,				/* envy24ht_check_output */
  NULL,				/* envy24ht_alloc_buffer */
  NULL,				/* envy24ht_free_buffer */
  NULL,
  NULL,
  envy24ht_get_buffer_pointer,
  NULL,
  envy24ht_sync_control
};


static audiodrv_t envy24ht_input_driver = {
  envy24ht_open_input,
  envy24ht_close,
  envy24ht_output_block,
  envy24ht_start_input,
  envy24ht_ioctl,
  envy24ht_prepare_for_input,
  envy24ht_prepare_for_output,
  envy24ht_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  envy24ht_trigger,
  envy24ht_set_rate,
  envy24ht_set_format,
  envy24ht_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* envy24ht_alloc_buffer */
  NULL,				/* envy24ht_free_buffer */
  NULL,
  NULL,
  envy24ht_get_buffer_pointer,
  NULL,
  envy24ht_sync_control
};

static const int bindings[MAX_ODEV] = {
  DSP_BIND_FRONT,
  DSP_BIND_CENTER_LFE,
  DSP_BIND_SURR,
  DSP_BIND_REAR
};

static int
init_play_device (envy24ht_devc * devc, int chmask, int offset,
		  unsigned char mask, char *name, int dev_flags,
		  char *port_id, char *devfile_name)
{
  int opts, dev, formats;
  char tmp[80];
  envy24ht_portc *portc = NULL;
  int i;
  adev_p adev;

  sprintf (tmp, "%s %s out", devc->model_data->product, name);

  if (devc->nr_outdevs >= MAX_ODEV)
    {
      cmn_err (CE_CONT, "Envy24ht: Too many audio devices\n");
      return OSS_ENXIO;
    }

  opts = ADEV_AUTOMODE | ADEV_NOINPUT;

  if (dev_flags & DF_SPDIF)
    opts |= ADEV_SPECIAL;

  formats = SUPPORTED_FORMAT;
  if (dev_flags & DF_AC3)
    formats |= AFMT_AC3;

  if (dev_flags & DF_MULTICH)
    opts |= ADEV_COLD;
  else
    opts |= ADEV_SPECIAL;

  if ((dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
				   devc->osdev,
				   devc->osdev,
				   tmp,
				   &envy24ht_output_driver,
				   sizeof (audiodrv_t),
				   opts, formats, devc, -1,
				   devfile_name)) < 0)
    {
      return dev;
    }

  if (devc->first_dev == -1)
    {
      devc->first_dev = dev;
      audio_engines[dev]->outch_names = OUTCH_NAMES;
    }
  adev = audio_engines[dev];

  portc = &devc->play_portc[devc->nr_outdevs];
  for (i = 0; speed_tab[i].speed != -1; i++)
    adev->rates[adev->nrates++] = speed_tab[i].speed;

  portc->name = port_id;
  audio_engines[dev]->portc = portc;
  audio_engines[dev]->mixer_dev = devc->mixer_dev;
  audio_engines[dev]->rate_source = devc->first_dev;
  audio_engines[dev]->min_rate = 8000;
  audio_engines[dev]->max_rate = 192000;
  audio_engines[dev]->binding = bindings[devc->nr_outdevs];
  if (dev_flags & DF_SPDIF)
    audio_engines[dev]->binding = DSP_BIND_SPDIF;

  portc->dev = dev;
  portc->open_mode = 0;
  portc->fmt = SUPPORTED_FORMAT;
  portc->base = devc->mt_base + offset;
  portc->mask = mask;
  portc->dev_flags = dev_flags;
  portc->chmask = chmask;
  portc->state_bits = 0;
  portc->direction = PCM_ENABLE_OUTPUT;

  audio_engines[dev]->min_channels = 2;
  audio_engines[dev]->max_channels = 2;

  if (dev_flags & DF_SPDIF)
    audio_engines[dev]->caps |= PCM_CAP_DIGITALOUT | DSP_CH_STEREO;
  else
    {
      if (dev_flags & DF_MULTICH)
	{
	  audio_engines[dev]->caps |= PCM_CAP_ANALOGOUT;
	  audio_engines[dev]->caps |= DSP_CH_STEREO;
	  audio_engines[dev]->min_channels = 2;
	  audio_engines[dev]->max_channels = devc->model_data->nr_outs;
	}
      else
	audio_engines[dev]->caps |= PCM_CAP_ANALOGOUT | DSP_CH_STEREO;
    }
  devc->nr_outdevs++;

  return dev;
}

static int
init_rec_device (envy24ht_devc * devc, int chmask, int offset,
		 unsigned char mask, char *name, int dev_flags, char *devfile_name)
{
  int opts, dev, formats;
  adev_p adev;
  int i;
  envy24ht_portc *portc = NULL;
  char tmp[80];
  sprintf (tmp, "%s %s in", devc->model_data->product, name);

  if (devc->nr_indevs >= MAX_IDEV)
    {
      cmn_err (CE_CONT, "Envy24ht: Too many audio devices\n");
      return OSS_ENXIO;
    }

  opts = ADEV_AUTOMODE | ADEV_NOOUTPUT | ADEV_COLD;

  if (dev_flags & DF_SPDIF)
    opts |= ADEV_SPECIAL;

  formats = SUPPORTED_FORMAT;
  if (dev_flags & DF_AC3)
    formats |= AFMT_AC3;

  if ((dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
				   devc->osdev,
				   devc->osdev,
				   tmp,
				   &envy24ht_input_driver,
				   sizeof (audiodrv_t),
				   opts, formats, devc, -1,
				   devfile_name)) < 0)
    {
      return dev;
    }

  if (devc->first_dev == -1)
    devc->first_dev = dev;
  portc = &devc->rec_portc[devc->nr_indevs];
  adev = audio_engines[dev];

  for (i = 0; speed_tab[i].speed != -1; i++)
    adev->rates[adev->nrates++] = speed_tab[i].speed;

  audio_engines[dev]->portc = portc;
  audio_engines[dev]->mixer_dev = devc->mixer_dev;
  audio_engines[dev]->rate_source = devc->first_dev;
  audio_engines[dev]->min_rate = 8000;
  audio_engines[dev]->max_rate = 192000;

  portc->dev = dev;
  portc->name = "rec";
  portc->open_mode = 0;
  portc->base = devc->mt_base + offset;
  portc->mask = mask;
  portc->state_bits = 0;
  portc->fmt = SUPPORTED_FORMAT;
  portc->dev_flags = dev_flags;
  portc->chmask = chmask;
  portc->direction = PCM_ENABLE_INPUT;
  if (dev_flags & DF_SPDIF)
    audio_engines[dev]->caps |= PCM_CAP_DIGITALIN | DSP_CH_STEREO;
  else
    audio_engines[dev]->caps |= PCM_CAP_ANALOGIN | DSP_CH_STEREO;
  devc->nr_indevs++;

  return dev;
}

static void
init_devices (envy24ht_devc * devc)
{
  int front_engine, rec_engine;

  OUTB (devc->osdev, 0x03, devc->mt_base + 0x19);	/* Channel allocation */
  OUTB (devc->osdev, 0x00, devc->mt_base + 0x1b);	/* Unpause ALL channels */

  devc->first_dev = -1;

  front_engine=init_play_device (devc, 0x003, 0x10, 0x01, devc->channel_names[0],
		    DF_MULTICH, "front", "");

  if (devc->model_data->nr_outs > 2)
    init_play_device (devc, 0x00c, 0x70, 0x10, devc->channel_names[1], 0,
		      "C/LFE", "");

  if (devc->model_data->nr_outs > 4)
    init_play_device (devc, 0x030, 0x60, 0x20, devc->channel_names[2], 0,
		      "side", "");

  if (devc->model_data->nr_outs > 6)
    init_play_device (devc, 0x0c0, 0x50, 0x40, devc->channel_names[3], 0,
		      "rear", "");

  if (devc->model_data->flags & MF_SPDIFOUT)
    {
      init_play_device (devc, 0x300, 0x40, 0x80, "digital",
			DF_SPDIF | DF_AC3, "spdif", "spdout");
    }

  rec_engine = init_rec_device (devc, 0x003, 0x20, 0x02, "analog", 0, "");

  if (devc->model_data->flags & MF_SPDIFIN)
    {
      init_rec_device (devc, 0x00c, 0x30, 0x04, "digital", DF_SPDIF, "spdin");
    }

#ifdef CONFIG_OSS_VMIX
    if (rec_engine < 0)
       rec_engine = -1; /* Not available */

    if (front_engine >= 0)
       vmix_attach_audiodev(devc->osdev, front_engine, rec_engine, 0);
#endif
}

static void
install_ac97_mixer (envy24ht_devc * devc)
{
  int tmp;
  tmp = 0;

  DDB (cmn_err (CE_CONT, "Installing AC97 mixer\n"));

  devc->mixer_dev =
    ac97_install (&devc->ac97devc, devc->model_data->product, ac97_read,
		  ac97_write, devc, devc->osdev);
  if (devc->mixer_dev < 0)
    {
      cmn_err (CE_CONT, "Envy24ht: Mixer install failed\n");
      return;
    }
  ac97_init_ext (devc->mixer_dev, &devc->ac97devc, envy24ht_mix_init, 50);

#if 1
  /* AD1616 specific stuff. Check this if there is some other AC97 chip */
  /* Maybe this should be moved to ac97.c in a way or another */

  /* Turn surround dacs ON */
  tmp = ac97_read (devc, 0x2a);
  tmp &= ~0x3800;
  ac97_write (devc, 0x2a, tmp);

  tmp = ac97_read (devc, 0x5a);
  tmp &= ~0x8000;
  tmp |= 0x1800;
  ac97_write (devc, 0x5a, tmp);
#endif

#if 0
  for (tmp = 0; tmp < 0x3f; tmp += 2)
    cmn_err (CE_CONT, "%02x: %04x\n", tmp, ac97_read (devc, tmp));
  for (tmp = 0x5a; tmp < 0x5d; tmp += 2)
    cmn_err (CE_CONT, "%02x: %04x\n", tmp, ac97_read (devc, tmp));
  for (tmp = 0x7a; tmp < 0x7f; tmp += 2)
    cmn_err (CE_CONT, "%02x: %04x\n", tmp, ac97_read (devc, tmp));
#endif
}

int
oss_envy24ht_attach (oss_device_t * osdev)
{
  envy24ht_devc *devc;
  extern int envy24ht_model;
  unsigned char pci_irq_line;
  unsigned short pci_command, vendor, device;
  unsigned int subvendor;
  unsigned int pci_ioaddr, pci_ioaddr1;
  int i, err;

  char *name = "Generic ENVY24HT";

  DDB (cmn_err (CE_CONT, "Entered Envy24HT probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != ICENSEMBLE_VENDOR_ID || device != ICENSEMBLE_ENVY24HT_ID)
    return 0;

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  MUTEX_INIT (osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (osdev, devc->low_mutex, MH_DRV + 1);

  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_1, &pci_ioaddr1);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, 0x2c, &subvendor);

  DDB (cmn_err (CE_CONT,
		"Device found at I/O %x, %x\n", pci_ioaddr & ~3,
		pci_ioaddr1 & ~3));

  devc->subvendor = subvendor;

  devc->ccs_base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr) & ~0x3;
  DDB (cmn_err (CE_CONT, "CCS base %x/%lx\n", pci_ioaddr, devc->ccs_base));

  devc->mt_base = MAP_PCI_IOADDR (devc->osdev, 1, pci_ioaddr1) & ~0x3;
  DDB (cmn_err (CE_CONT, "MT base %x/%lx\n", pci_ioaddr1, devc->mt_base));

  /* Reset the chip */
  OUTB (devc->osdev, 0x81, devc->ccs_base + 0x00);
  oss_udelay (1000);

  /* Release reset */
  OUTB (devc->osdev, 0x00, devc->ccs_base + 0x00);
  oss_udelay (1000);

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  devc->nr_outdevs = devc->nr_indevs = 0;
  i = 0;

  if ((envy24ht_model > -1)
      && (envy24ht_model < (sizeof (models) / sizeof (card_spec)) - 1))
    i = envy24ht_model;
  else
    while (models[i].svid != 0)
      {
	if (models[i].svid == subvendor)
	  {
	    name = models[i].product;
	    devc->model_data = &models[i];
	    DDB (cmn_err (CE_CONT, "Card id '%s'\n", name));

	    break;
	  }

	i++;
      }

  if (models[i].svid == 0)
    {
      cmn_err (CE_CONT, "Unknown device ID (%08x).\n", subvendor);
      cmn_err (CE_CONT, "This card may not be supported (yet).\n");
      i = 0; /* Assume AC97 based Envy23PT */
    }

  oss_register_device (osdev, name);

  if (devc->model_data == NULL)
    {
      cmn_err (CE_CONT, "Envy24ht: This card was not recognized: %08x\n",
	       subvendor);
      return 0;
    }

  /* Disable all interrupts */
  OUTB (devc->osdev, 0xff, devc->ccs_base + 0x01);
  OUTB (devc->osdev, 0xff, devc->mt_base + 0x03);

  if (devc->model_data->flags & MF_ENVY24PT)
    {
      devc->codec_type = CODEC_AC97;
      envy24pt_init (devc);
    }
  else if (devc->model_data->svid == SSID_JULIA)
    {
      julia_eeprom_init (devc);
    }
  else
    load_eeprom (devc);

  devc->irq = pci_irq_line;
  if ((err =
       oss_register_interrupts (devc->osdev, 0, envy24htintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  i = 0;
  devc->max_ratesel = 0;

  while (speed_tab[i].speed != -1)
    {
      int rate = speed_tab[i].speed;

      if (verify_rate (devc, rate) == rate)
	devc->max_ratesel = i;

      i++;
    }

  OUTB (devc->osdev, ~0x10, devc->ccs_base + 0x01);	/* Enable audio interrupts */

  if (devc->model_data->flags & MF_MIDI)
    {
      attach_midi (devc);
    }
  i = 0;
  devc->max_ratesel = 0;

  while (speed_tab[i].speed != -1)
    {
      int rate = speed_tab[i].speed;

      if (verify_rate (devc, rate) == rate)
	devc->max_ratesel = i;

      i++;
    }

  devc->syncstart_mask = 0;
  devc->speedbits = 0;
  devc->speed = 0;
  devc->pending_speed = 0;
  devc->prev_speed = 0;
  devc->pending_speed_sel = 9;
  devc->configured_rate_sel = devc->pending_speed_sel;
  devc->open_count = 0;
  memcpy (devc->channel_names, channel_names, sizeof (channel_names));

  devc->nr_play_channels = 10;
  devc->nr_rec_channels = 10;
#define setmask(m, b) m|=(1<<(b))

  devc->inportmask = 0;
  devc->outportmask = 0;
  devc->busy_play_channels = 0;
  devc->busy_rec_channels = 0;

  for (i = 0; i < devc->model_data->nr_outs; i++)
    setmask (devc->outportmask, i);
  if (devc->model_data->flags & MF_SPDIFOUT)
    {
      setmask (devc->outportmask, 8);	/* SPDIF */
      setmask (devc->outportmask, 9);	/* SPDIF */
    }
  for (i = 0; i < devc->model_data->nr_ins; i++)
    setmask (devc->inportmask, i);
  if (devc->model_data->flags & MF_SPDIFIN)
    {
      setmask (devc->inportmask, 8);	/* SPDIF */
      setmask (devc->inportmask, 9);	/* SPDIF */
    }

  if (devc->model_data->auxdrv == NULL)
    {
      devc->auxdrv = &dummy_auxdrv;
    }
  else
    {
      devc->auxdrv = devc->model_data->auxdrv;
      if (devc->auxdrv->card_init)
	devc->auxdrv->card_init (devc);
    }

  if (devc->codec_type == CODEC_AC97)
    install_ac97_mixer (devc);
  else
    {
      if ((devc->mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
						devc->osdev,
						devc->osdev,
						devc->model_data->
						product,
						&envy24ht_mixer_driver,
						sizeof (mixer_driver_t),
						devc)) >= 0)
	{
	  int n = 50;

	  mixer_devs[devc->mixer_dev]->hw_devc = devc;
	  mixer_ext_set_init_fn (devc->mixer_dev, envy24ht_mix_init, n);
	  mixer_devs[devc->mixer_dev]->priority = 1;	/* Possible default mixer candidate */
	}
    }

  if (devc->model_data->flags & (MF_SPDIFOUT | MF_SPDIFIN))
    {
      int err;

      if ((err = oss_spdif_install (&devc->spdc, devc->osdev,
				    &default_spdif_driver,
				    sizeof (spdif_driver_t), devc, NULL,
				    devc->mixer_dev, SPDF_OUT,
				    DIG_PASSTHROUGH | DIG_EXACT |
				    DIG_CBITOUT_LIMITED | DIG_VBITOUT |
				    DIG_PRO | DIG_CONSUMER)) != 0)
	{
	  cmn_err (CE_CONT,
		   "S/PDIF driver install failed. error %d\n", err);
	  return 0;
	}
    }
  OUTB (devc->osdev, ~0x10, devc->ccs_base + 0x01);	/* Enable audio interrupts */
  init_devices (devc);
  setup_sample_rate (devc);


  return 1;
}

int
oss_envy24ht_detach (oss_device_t * osdev)
{
  envy24ht_devc *devc = osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  /* Disable all interrupts */
  OUTB (devc->osdev, 0xff, devc->ccs_base + 0x01);
  /* disable DMA interrupt mask */
  OUTB (devc->osdev, 0xff, devc->mt_base + 0x00);

  /* Stop playback */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x01,
	devc->mt_base + 0x18);
  oss_udelay (100);
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x01,
	devc->mt_base + 0x18);

  /* Stop recording */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x04,
	devc->mt_base + 0x18);
  oss_udelay (100);
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x04,
	devc->mt_base + 0x18);

  unload_midi (devc);

  if (devc->auxdrv->card_uninit)
     devc->auxdrv->card_uninit(devc);

  oss_unregister_interrupts (devc->osdev);

  if (devc->model_data->flags & (MF_SPDIFOUT | MF_SPDIFIN))
    {
      oss_spdif_uninstall (&devc->spdc);
    }

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);
  UNMAP_PCI_IOADDR (devc->osdev, 1);

  oss_unregister_device (osdev);
  return 1;
}
