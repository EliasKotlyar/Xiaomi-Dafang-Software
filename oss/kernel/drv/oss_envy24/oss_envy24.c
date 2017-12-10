/*
 * Purpose: Driver for IC Ensemble ENVY24 based audio cards.
 *
 * The audio input and output devices implemented by this driver use additional
 * layer of buffering for channel re-interleaving. The device itself uses
 * 10/12 channel interleaved 32 bit format in hardware level. The
 * re-interleaving engine splits these multi channel devices to several
 * "stereo" devices.
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

#include "oss_envy24_cfg.h"
#include <ac97.h>
#include <oss_pci.h>

#include "envy24.h"

extern int envy24_skipdevs;
extern int envy24_force_mono;
extern int envy24_gain_sliders;
int envy24_virtualout = 0;	/* This used to be an config option */

extern int envy24_devmask;
#define DMASK_ANALOGOUT		1
#define DMASK_ANALOGIN		2
#define DMASK_SPDIFOUT		4
#define DMASK_SPDIFIN		8
#define DMASK_MONITORIN	       16
#define DMASK_RAWDEVS	       32

extern envy24_auxdrv_t default_auxdrv;
extern envy24_auxdrv_t ap2496_auxdrv;
extern envy24_auxdrv_t d410_auxdrv;
extern envy24_auxdrv_t d1010lt_auxdrv;
extern envy24_auxdrv_t tdif_auxdrv;
extern envy24_auxdrv_t ewx2496_auxdrv;
extern envy24_auxdrv_t ews88d_auxdrv;
extern envy24_auxdrv_t dmx6fire_auxdrv;

static card_spec models[] = {
  {0xd6301412, "M Audio Delta 1010", 8, 8,
   MF_MAUDIO | MF_MIDI1 | MF_SPDIF | MF_WCLOCK | MF_MEEPROM},
  {0xd6311412, "M Audio Delta DiO 2496", 2, 0,
   MF_MAUDIO | MF_SPDIF | MF_SPDSELECT | MF_MEEPROM},
  {0xd6321412, "M Audio Delta 66", 4, 4,
   MF_MAUDIO | MF_SPDIF | MF_AKMCODEC | MF_MEEPROM},
  {0xd6331412, "M Audio Delta 44", 4, 4,
   MF_MAUDIO | MF_AKMCODEC | MF_MEEPROM},
  {0xd6341412, "M Audio Audiophile 2496", 2, 2,
   MF_AP | MF_SPDIF | MF_MIDI1 | MF_MEEPROM, &ap2496_auxdrv},
  {0xd6381412, "M Audio Delta 410", 8, 2, MF_D410 | MF_SPDIF | MF_MEEPROM,
   &d410_auxdrv},

  /* Delta 1010 rev E is based on 1010LT instead of the original 1010 design */
  {0xd63014ff, "M Audio Delta 1010 rev E", 8, 8,
   MF_MIDI1 | MF_SPDIF | MF_MEEPROM | MF_WCLOCK, &d1010lt_auxdrv},
  
  {0xd63b1412, "M Audio Delta 1010LT", 8, 8,
   MF_MIDI1 | MF_SPDIF | MF_MEEPROM | MF_WCLOCK, &d1010lt_auxdrv},
  {0xd6351412, "M Audio Delta TDIF", 8, 8, MF_SPDIF | MF_MEEPROM | MF_WCLOCK,
   &tdif_auxdrv},
  {0x1115153b, "Terratec EWS88MT", 8, 8,
   MF_MIDI1 | MF_SPDIF | MF_EWS88 | MF_AC97},
  {0x112b153b, "Terratec EWS88D", 8, 8,
   MF_MIDI1 | MF_MIDI2 | MF_SPDIF | MF_AC97 | MF_WCLOCK, &ews88d_auxdrv},
  {0x1130153b, "Terratec EWX 24/96", 2, 2, MF_SPDIF | MF_EWX2496,
   &ewx2496_auxdrv},
  {0x1138153b, "Terratec DMX6fire 24/96", 6, 6,
   MF_MIDI1 | MF_MIDI2 | MF_SPDIF, &dmx6fire_auxdrv},
  {0x17121412, "Generic Envy24 based card", 8, 8,
   MF_SPDIF | MF_MIDI1 | MF_CONSUMER | MF_HOONTECH},
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
  /* {64000, 0x0f}, doesn't work */
  {
   88200, 0x0b}
  ,
  {
   96000, 0x07}
  ,
  {
   -1, 0x10}
  ,
};

int
envy24_read_cci (envy24_devc * devc, int pos)
{
  OUTB (devc->osdev, pos, devc->ccs_base + 0x03);
  return INB (devc->osdev, devc->ccs_base + 0x04);
}

void
envy24_write_cci (envy24_devc * devc, int pos, int data)
{
  OUTB (devc->osdev, pos, devc->ccs_base + 0x03);
  OUTB (devc->osdev, data, devc->ccs_base + 0x04);
}

static int
eeprom_read (envy24_devc * devc, int pos)
{
  int i, status;

  for (i = 0; i < 0x10000; i++)
    {
      status = INB (devc->osdev, devc->ccs_base + 0x13);
      if (!(status & 1))
	break;

    }

  OUTB (devc->osdev, 0xa0, devc->ccs_base + 0x10);	/* EEPROM read */
  OUTB (devc->osdev, pos, devc->ccs_base + 0x11);	/* Offset */

  for (i = 0; i < 2000; i++)
    {
      status = INB (devc->osdev, devc->ccs_base + 0x13);
      if (!(status & 1))
	break;

    }

  oss_udelay (1);
  return INB (devc->osdev, devc->ccs_base + 0x12);
}

static int
load_eeprom (envy24_devc * devc, int subid)
{
  int status, i, check;

  status = INB (devc->osdev, devc->ccs_base + 0x13);

  if (!(status & 0x80))
    return 0;			/* No EEPROM */

  for (i = 0; i < 32; i++)
    {
      devc->eeprom[i] = eeprom_read (devc, i);
      devc->eeprom[i] = eeprom_read (devc, i);
    }
  DDB (cmn_err (CE_CONT, "EEPROM="));
  for (i = 0; i < 10; i++)
    DDB (cmn_err (CE_CONT, "0x%02x, ", devc->eeprom[i]));
  DDB (cmn_err (CE_CONT, "\n"));

  check = 0;
  for (i = 0; i < 4; i++)
    {
      check <<= 8;
      check |= devc->eeprom[i];
    }

  if (check != subid)
    cmn_err (CE_CONT,
	     "Envy24 WARNING: Possible EEPROM read error %08x != %08x\n",
	     check, subid);

  return 1;
}

static void
handle_playdev (envy24_devc * devc, envy24_portc * portc, int this_frag)
{
  int sample, nsamples, nbytes, ch;
  dmap_t *dmap = audio_engines[portc->dev]->dmap_out;

  if (!(portc->trigger_bits & PCM_ENABLE_OUTPUT) && devc->playback_started)
    return;

  nsamples = devc->hw_fragsamples;	/* Number of 32 bit samples */

  nbytes = nsamples * portc->channels;

  if (audio_engines[portc->dev]->dmap_out->flags & DMAP_POST)
    {
      if (portc->pcm_qlen > 0)
	portc->pcm_qlen--;
    }
  else
    {
      if (portc->pcm_qlen < devc->writeahead)
	portc->pcm_qlen++;
    }

  if (portc->bits & (AFMT_S16_LE | AFMT_S16_BE | AFMT_AC3))
    nbytes *= 2;
  else if (portc->
	   bits & (AFMT_S32_LE | AFMT_S32_BE | AFMT_S24_LE | AFMT_S24_BE))
    nbytes *= 4;

  if (nbytes != dmap->fragment_size)
    return;			/* Fragment size mismatch */

  switch (portc->bits)
    {
    case AFMT_U8:
      {
	unsigned char *ip;
	int *op;

	ip = audio_engines[portc->dev]->dmap_out->dmabuf;
	ip += (dmap_get_qhead (dmap) * dmap->fragment_size);
	op = (int *) (devc->playbuf + devc->hw_pfragsize * this_frag);
	VMEM_CHECK (ip, nsamples * sizeof (*ip));
	VMEM_CHECK (op, nsamples * sizeof (*op));

	for (sample = 0; sample < nsamples; sample++)
	  {
	    int *p = &op[sample * 10 + portc->chnum];

	    for (ch = 0; ch < portc->channels; ch++)
	      {
		*p++ = (*ip++ ^ 0x80) << 24;
	      }
	  }
      }
      break;

    case AFMT_AC3:
    case AFMT_S16_LE:
      {
	short *ip;
	int *op;
#ifdef DO_TIMINGS
	oss_timing_printf ("Envy24: Copy out %d, %d",
		   dmap_get_qhead (dmap) * dmap->fragment_size, nbytes);
#endif

	ip = (short *) (dmap->dmabuf +
			(dmap_get_qhead (dmap) * dmap->fragment_size));
	op = (int *) (devc->playbuf + devc->hw_pfragsize * this_frag);
	VMEM_CHECK (ip, nsamples * sizeof (*ip));
	VMEM_CHECK (op, nsamples * sizeof (*op));

	for (sample = 0; sample < nsamples; sample++)
	  {
	    int *p = &op[sample * 10 + portc->chnum];

	    for (ch = 0; ch < portc->channels; ch++)
	      {
		*p++ = *ip++ << 16;
	      }
	  }
      }
      break;

    case AFMT_S16_BE:
      {
	short *ip;
	int *op;

	ip = (short *) (audio_engines[portc->dev]->dmap_out->dmabuf +
			(dmap_get_qhead (dmap) * dmap->fragment_size));
	op = (int *) (devc->playbuf + devc->hw_pfragsize * this_frag);
	VMEM_CHECK (ip, nsamples * sizeof (*ip));
	VMEM_CHECK (op, nsamples * sizeof (*op));

	for (sample = 0; sample < nsamples; sample++)
	  {
	    int *p = &op[sample * 10 + portc->chnum];

	    for (ch = 0; ch < portc->channels; ch++)
	      {
		short s = (short) (((*(unsigned short *) ip & 0xff) << 8) |
				   ((*(unsigned short *) ip & 0xff00) >>
				    8));
		ip++;
		*p++ = s << 16;
	      }
	  }
      }
      break;

    case AFMT_S24_LE:
      {
	int *ip;
	int *op;

	ip = (int *) (audio_engines[portc->dev]->dmap_out->dmabuf +
		      (dmap_get_qhead (dmap) * dmap->fragment_size));
	op = (int *) (devc->playbuf + devc->hw_pfragsize * this_frag);
	VMEM_CHECK (ip, nsamples * sizeof (*ip));
	VMEM_CHECK (op, nsamples * sizeof (*op));

	for (sample = 0; sample < nsamples; sample++)
	  {
	    int *p = &op[sample * 10 + portc->chnum];

	    for (ch = 0; ch < portc->channels; ch++)
	      {
		*p++ = *ip++ << 8;
	      }
	  }
      }
      break;

    case AFMT_S32_LE:
      {
	int *ip;
	int *op;

	ip = (int *) (audio_engines[portc->dev]->dmap_out->dmabuf +
		      (dmap_get_qhead (dmap) * dmap->fragment_size));
	op = (int *) (devc->playbuf + devc->hw_pfragsize * this_frag);
	VMEM_CHECK (ip, nsamples * sizeof (*ip));
	VMEM_CHECK (op, nsamples * sizeof (*op));

	for (sample = 0; sample < nsamples; sample++)
	  {
	    int *p = &op[sample * 10 + portc->chnum];

	    for (ch = 0; ch < portc->channels; ch++)
	      {
		*p++ = *ip++;
	      }
	  }
      }
      break;
    }

  oss_audio_outputintr (portc->dev, 1);
}

#ifdef DO_RIAA
static __inline__ int32_t
_riaa_sat31 (register int32_t a, register int32_t b)
{
  register int64_t v = (((int64_t) a) * b) + (1 << 30);
  return (int32_t) (v >> 31);
}
#endif

static void
handle_recdev (envy24_devc * devc, envy24_portc * portc)
{
  int sample, nsamples, nbytes, ch;
  dmap_t *dmap = audio_engines[portc->dev]->dmap_in;

  if (portc->trigger_bits == 0 && devc->recording_started)
    return;

  nsamples = devc->hw_fragsamples;	/* Number of 32 bit samples */

  nbytes = nsamples * portc->channels;

  if (portc->bits & (AFMT_S16_LE | AFMT_S16_BE | AFMT_AC3))
    nbytes *= 2;
  else if (portc->bits & (AFMT_S32_LE | AFMT_S24_LE))
    nbytes *= 4;

  if (nbytes != dmap->fragment_size)
    {
      return;			/* Fragment size mismatch */
    }

  switch (portc->bits)
    {
    case AFMT_U8:
      {
	unsigned char *ip;
	int *op;

	ip = audio_engines[portc->dev]->dmap_in->dmabuf;
	ip += (dmap_get_qtail (dmap) * dmap->fragment_size);
	op = (int *) (devc->recbuf + devc->hw_rfragsize * devc->hw_recfrag);
	VMEM_CHECK (ip, nsamples * sizeof (*ip));
	VMEM_CHECK (op, nsamples * sizeof (*op));

	for (sample = 0; sample < nsamples; sample++)
	  {
	    int *p = &op[sample * 12 + portc->chnum];

	    for (ch = 0; ch < portc->channels; ch++)
	      {
		*ip++ = ((*p++) >> 24) ^ 0x80;
	      }
	  }
      }
      break;

    case AFMT_S16_LE:
#ifdef DO_RIAA
      if (portc->riaa_filter)
	{
	  /* RIAA filtered version */
	  short *ip;
	  int *op;

	  ip = (short *) (audio_engines[portc->dev]->dmap_in->dmabuf +
			  (dmap_get_qtail (dmap) * dmap->fragment_size));
	  op = (int *) (devc->recbuf + devc->hw_rfragsize * devc->hw_recfrag);

	  VMEM_CHECK (ip, nsamples * sizeof (*ip));
	  VMEM_CHECK (op, nsamples * sizeof (*op));

	  for (ch = 0; ch < portc->channels; ch++)
	    {
	      int *p = &op[portc->chnum + ch];
	      short *p2 = &ip[ch];
	      riaa_t *ff = &portc->riaa_parms[ch];

	      int32_t x1 = ff->x1, x2 = ff->x2, x3 = ff->x3,
		y1 = ff->y1, y2 = ff->y2, y3 = ff->y3, x0, y0;

	      for (sample = 0; sample < nsamples; sample++)
		{
		  int tmp = *p;
		  p += 12;

		  x0 = _riaa_sat31 (tmp, 0x4C30C30C);

		  y0 = _riaa_sat31 (x0, 0xF38FB92F) +
		    _riaa_sat31 (x1, 0xF2492994) +
		    _riaa_sat31 (x2, 0x1AB82385) +
		    _riaa_sat31 (x3, 0x023FB0F8) +
		    (_riaa_sat31 (y1, 0x574DB88C) << 1) +
		    _riaa_sat31 (y2, 0xF650F27D) +
		    _riaa_sat31 (y3, 0xDACB84B9);

		  x3 = x2;
		  x2 = x1;
		  x1 = x0;
		  y3 = y2;
		  y2 = y1;
		  y1 = y0;

		  tmp = -y0;

		  *p2 = tmp >> 16;
		  p2 += portc->channels;
		}

	      ff->x1 = x1;
	      ff->x2 = x2;
	      ff->x3 = x3;
	      ff->y1 = y1;
	      ff->y2 = y2;
	      ff->y3 = y3;
	    }
	  /* RIAA filtered version */
	}
      else
#endif
	{
	  short *ip;
	  int *op;

	  ip = (short *) (audio_engines[portc->dev]->dmap_in->dmabuf +
			  (dmap_get_qtail (dmap) * dmap->fragment_size));
	  op = (int *) (devc->recbuf + devc->hw_rfragsize * devc->hw_recfrag);

	  VMEM_CHECK (ip, nsamples * sizeof (*ip));
	  VMEM_CHECK (op, nsamples * sizeof (*op));
	  for (sample = 0; sample < nsamples; sample++)
	    {
	      int *p = &op[sample * 12 + portc->chnum];

	      for (ch = 0; ch < portc->channels; ch++)
		{
		  *ip++ = (*p++) >> 16;
		}
	    }
	}
      break;

    case AFMT_S32_LE:
      {
	int *ip;
	int *op;

	ip = (int *) (audio_engines[portc->dev]->dmap_in->dmabuf +
		      (dmap_get_qtail (dmap) * dmap->fragment_size));
	op = (int *) (devc->recbuf + devc->hw_rfragsize * devc->hw_recfrag);

	VMEM_CHECK (ip, nsamples * sizeof (*ip));
	VMEM_CHECK (op, nsamples * sizeof (*op));
	for (sample = 0; sample < nsamples; sample++)
	  {
	    int *p = &op[sample * 12 + portc->chnum];

	    for (ch = 0; ch < portc->channels; ch++)
	      {
		*ip++ = *p++;
	      }
	  }
      }
      break;

    case AFMT_S24_LE:
      {
	int *ip;
	int *op;

	ip = (int *) (audio_engines[portc->dev]->dmap_in->dmabuf +
		      (dmap_get_qtail (dmap) * dmap->fragment_size));
	op = (int *) (devc->recbuf + devc->hw_rfragsize * devc->hw_recfrag);

	VMEM_CHECK (ip, nsamples * sizeof (*ip));
	VMEM_CHECK (op, nsamples * sizeof (*op));
	for (sample = 0; sample < nsamples; sample++)
	  {
	    int *p = &op[sample * 12 + portc->chnum];

	    for (ch = 0; ch < portc->channels; ch++)
	      {
		*ip++ = *p++ >> 8;
	      }
	  }
      }
      break;
    }

  oss_audio_inputintr (portc->dev, 0);
}

static void
tank_playback_data (envy24_devc * devc)
{
  int i, nc = devc->nr_outdevs;
  envy24_portc *portc;
  unsigned char *p;

  p = devc->playbuf + devc->hw_playfrag * devc->hw_pfragsize;
  VMEM_CHECK (p, devc->hw_pfragsize);
  memset (p, 0, devc->hw_pfragsize);	/* Cleanup the fragment */

  for (i = 0; i < nc; i++)
    {
      portc = &devc->play_portc[i];

      if (!portc->open_mode)	/* Not opened */
	continue;
      handle_playdev (devc, portc, devc->hw_playfrag);
    }

  devc->hw_playfrag = (devc->hw_playfrag + 1) % devc->hw_nfrags;
}

static void
handle_recording (envy24_devc * devc)
{
  int i;
  envy24_portc *portc;
  /* oss_native_word flags; */

  /*
   * TODO: Fix mutexes and move the inputintr/outputintr calls outside the
   * mutex block.
   */
  /* MUTEX_ENTER_IRQDISABLE (devc->mutex, flags); */
  for (i = 0; i < devc->nr_indevs; i++)
    {
      portc = &devc->rec_portc[i];

      if (!portc->open_mode)	/* Not opened */
	continue;
      handle_recdev (devc, portc);
    }

  devc->hw_recfrag = (devc->hw_recfrag + 1) % devc->hw_nfrags;
  /* MUTEX_EXIT_IRQRESTORE (devc->mutex, flags); */
}

extern int envy24d_get_buffer_pointer (int dev, dmap_t * dmap, int direction);

static void
mt_audio_intr (envy24_devc * devc)
{
  int status;

#ifdef DO_TIMINGS
  oss_timing_enter (DF_INTERRUPT);
  oss_do_timing2 (DFLAG_PROFILE, "Envy24_audio_intr");
#endif
  status = INB (devc->osdev, devc->mt_base + 0x00);
  if (devc->playback_started && (status & 0x01))	/* Playback interrupt */
    {
/* cmn_err(CE_CONT, "%d\n", GET_JIFFIES()); */
      if (devc->direct_audio_opened & OPEN_WRITE)
	{
	  envy24d_playintr (devc);
	}
      else
	{
	  int ptr, qlen, i;

	  ptr = INW (devc->osdev, devc->mt_base + 0x14);
	  ptr = (devc->playbuffsize - ((ptr + 1) * 4)) / devc->hw_pfragsize;

	  /* Find the number of current fragments in the hardware level buffer */
	  qlen = 0;
	  i = devc->hw_playfrag;

	  while (qlen < 15 && i != ptr)
	    {
	      qlen++;
	      i = (i + 1) % devc->hw_nfrags;
	    }

	  if (qlen != devc->writeahead)
	    {
	      tank_playback_data (devc);
	    }

	  if (devc->hw_playfrag == ptr)	/* Out of sync */
	    {
	      tank_playback_data (devc);	/* Try to catch the hardware pointer */
	    }


	  tank_playback_data (devc);
	}
    }

  if (devc->recording_started && (status & 0x02))	/* Record interrupt */
    {
      if (devc->direct_audio_opened & OPEN_READ)
	envy24d_recintr (devc);
      else
	handle_recording (devc);
    }

  OUTB (devc->osdev, status, devc->mt_base + 0x00);
#ifdef DO_TIMINGS
  oss_timing_leave (DF_INTERRUPT);
  oss_do_timing2 (DFLAG_PROFILE, "Envy24_audio_intr done");
#endif
}

static int
envy24intr (oss_device_t * osdev)
{
  int status;
  envy24_devc *devc;

  devc = osdev->devc;

  status = INB (devc->osdev, devc->ccs_base + 0x02);
  if (status == 0)
    return 0;

  if (status & 0x80)		/* MIDI UART 1 */
    if (devc->model_data->flags & MF_MIDI1)
      uart401_irq (&devc->uart401devc1);

  if (status & 0x20)		/* MIDI UART 2 */
    if (devc->model_data->flags & MF_MIDI2)
      uart401_irq (&devc->uart401devc2);

  if (status & 0x10)
    {
/*cmn_err(CE_CONT, "%d/%d.", GET_JIFFIES(), envy24d_get_buffer_pointer(11, audio_engines[11]->dmap_out, DMODE_OUTPUT)); */
      mt_audio_intr (devc);
    }

  OUTB (devc->osdev, status, devc->ccs_base + 0x02);	/* ACK */

  return 1;
}

static void envy24_setup_pro_speed (envy24_devc * devc);
static void envy24_setup_consumer_speed (envy24_devc * devc);

void
envy24_prepare_play_engine (envy24_devc * devc)
{
  int tmp, fragsize, buffsize;

  if (devc->playback_prepared)
    return;

  /* Set S/PDIF sample rate indication */

  if (devc->spdif_cbits[0] & 0x01)
    envy24_setup_pro_speed (devc);
  else
    envy24_setup_consumer_speed (devc);

  if (devc->model_data->flags & MF_SPDIF)
    {
      tmp = 0x80;

      if (devc->ac3_mode)
	tmp |= 0x40;		/* Audio mode off */

      switch (devc->speed)
	{
	case 48000:
	  tmp |= 0x01;
	  break;
	case 44100:
	  tmp |= 0x02;
	  break;
	case 32000:
	  tmp |= 0x03;
	  break;
	}

      if (devc->model_data->auxdrv->spdif_set)
	devc->model_data->auxdrv->spdif_set (devc, tmp);

    }

  if (devc->model_data->auxdrv->set_rate)
    devc->model_data->auxdrv->set_rate (devc);
  else
    {
      tmp = devc->speedbits;
      if (devc->syncsource != SYNC_INTERNAL)
	{
	  tmp |= 0x10;		/* S/PDIF input clock select */
	  if (devc->model_data->flags & MF_WCLOCK)	/* Has world clock too */
	    {
	      int cmd = envy24_read_cci (devc, 0x20);
	      cmd |= 0x10;	/* S/PDIF */
	      if (devc->syncsource == SYNC_WCLOCK)
		cmd &= ~0x10;	/* World clock */
	      envy24_write_cci (devc, 0x20, cmd);
	    }
	}
      OUTB (devc->osdev, tmp, devc->mt_base + 0x01);
    }

  fragsize = devc->hw_pfragsize;
  buffsize = devc->playbuffsize / 4 - 1;

  PMEM_CHECK (devc->playbuf_phys, devc->playbuffsize);

  OUTL (devc->osdev, devc->playbuf_phys, devc->mt_base + 0x10);	/* Base */
  OUTW (devc->osdev, buffsize, devc->mt_base + 0x14);	/* Count */
  OUTL (devc->osdev, devc->playbuf_phys, devc->mt_base + 0x10);	/* Base */
  OUTW (devc->osdev, buffsize, devc->mt_base + 0x14);	/* Count */
  OUTL (devc->osdev, devc->playbuf_phys, devc->mt_base + 0x10);	/* Base */
  OUTW (devc->osdev, fragsize / 4 - 1, devc->mt_base + 0x16);	/* Interrupt rate */
  OUTL (devc->osdev, devc->playbuf_phys, devc->mt_base + 0x10);	/* Base */

  devc->playback_prepared = 1;
  mixer_devs[devc->mixer_dev]->modify_counter++;
}

void
envy24_launch_play_engine (envy24_devc * devc)
{
  /* Unmask playback interrupts */
  OUTB (devc->osdev,
	INB (devc->osdev, devc->mt_base + 0x00) & ~0x40,
	devc->mt_base + 0x00);
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x00) & ~0x40,
	devc->mt_base + 0x00);
  /* Kick it */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) | 0x01,
	devc->mt_base + 0x18);
  devc->playback_started = 1;

  if (devc->model_data->auxdrv->set_rate)
    devc->model_data->auxdrv->set_rate (devc);
}

static void
start_playback (envy24_devc * devc)
{
  devc->hw_playfrag = 0;

#ifdef DO_TIMINGS
  oss_do_timing ("Envy24: Start playback");
#endif
  tank_playback_data (devc);
  tank_playback_data (devc);
  if (devc->writeahead == 2)
    tank_playback_data (devc);

  envy24_prepare_play_engine (devc);
  envy24_launch_play_engine (devc);
}

void
envy24_stop_playback (envy24_devc * devc)
{
#ifdef DO_TIMINGS
  oss_do_timing ("Envy24: Stop playback");
#endif
  memset (devc->playbuf, 0, devc->playbuffsize);
  /*
   * Give the engine time to eat some silent samples 
   * This makes the corresponding digital mixer inputs to drop to 0
   * which decreases noise in the monitor outputs.
   */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x01,
	devc->mt_base + 0x18);
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x01,
	devc->mt_base + 0x18);

  /* Mask playback interrupts */
  OUTB (devc->osdev,
	INB (devc->osdev, devc->mt_base + 0x00) | 0x40, devc->mt_base + 0x00);
  devc->playback_started = 0;
  devc->playback_prepared = 0;
}

void
envy24_start_recording (envy24_devc * devc)
{
  int tmp;

  devc->hw_recfrag = 0;
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x04,
	devc->mt_base + 0x18);
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x04,
	devc->mt_base + 0x18);
  oss_udelay (20);

  if (devc->model_data->flags & MF_SPDIF)
    {
      tmp = 0x80;

      switch (devc->speed)
	{
	case 48000:
	  tmp |= 0x01;
	  break;
	case 44100:
	  tmp |= 0x02;
	  break;
	case 32000:
	  tmp |= 0x03;
	  break;
	}

      if (devc->model_data->auxdrv->spdif_set)
	devc->model_data->auxdrv->spdif_set (devc, tmp);

    }

  tmp = devc->speedbits;
  if (devc->syncsource != SYNC_INTERNAL)
    {
      tmp |= 0x10;		/* S/PDIF input clock select */
      if (devc->model_data->flags & MF_WCLOCK)	/* Has world clock too */
	{
	  int cmd = envy24_read_cci (devc, 0x20);
	  cmd |= 0x10;		/* S/PDIF */
	  if (devc->syncsource == SYNC_WCLOCK)
	    cmd &= ~0x10;	/* World clock */
	  envy24_write_cci (devc, 0x20, cmd);
	}
    }

  OUTB (devc->osdev, tmp, devc->mt_base + 0x01);

  if (devc->model_data->auxdrv->set_rate)
    devc->model_data->auxdrv->set_rate (devc);

  PMEM_CHECK (devc->recbuf_phys, devc->recbuffsize);

  OUTL (devc->osdev, devc->recbuf_phys, devc->mt_base + 0x20);	/* Base */
  oss_udelay (20);
  OUTL (devc->osdev, devc->recbuf_phys, devc->mt_base + 0x20);	/* Base */
  oss_udelay (20);
  OUTW (devc->osdev, devc->recbuffsize / 4 - 1, devc->mt_base + 0x24);	/* Count */
  OUTL (devc->osdev, devc->recbuf_phys, devc->mt_base + 0x20);	/* Base */
  oss_udelay (60);
  OUTW (devc->osdev, devc->hw_rfragsize / 4 - 1, devc->mt_base + 0x26);	/* Interrupt rate */

  oss_udelay (60);
}

void
envy24_launch_recording (envy24_devc * devc)
{

#if 1
  /* Unmask recording interrupts */
  OUTB (devc->osdev,
	INB (devc->osdev, devc->mt_base + 0x00) & ~0x80,
	devc->mt_base + 0x00);

#endif
  /* Kick it */
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) | 0x04,
	devc->mt_base + 0x18);
  devc->recording_started = 1;
  mixer_devs[devc->mixer_dev]->modify_counter++;

}

void
envy24_stop_recording (envy24_devc * devc)
{
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x04,
	devc->mt_base + 0x18);
  OUTB (devc->osdev, INB (devc->osdev, devc->mt_base + 0x18) & ~0x04,
	devc->mt_base + 0x18);

  /* Mask recording interrupts */
  OUTB (devc->osdev,
	INB (devc->osdev, devc->mt_base + 0x00) | 0x80, devc->mt_base + 0x00);
  devc->recording_started = 0;
  memset (devc->recbuf, 0, devc->recbuffsize);
}

/*
 * Audio entrypoint routines
 */

int
envy24_audio_set_rate (int dev, int arg)
{
  envy24_devc *devc = audio_engines[dev]->devc;
#if 1
  int i = 0, ix = -1, df, best = 0x7fffffff;
  oss_native_word flags;

  if (arg <= 0)
    return devc->speed;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (devc->recording_started || devc->playback_started)
    {
      DDB (cmn_err (CE_CONT,
		    "Requested sampling rate(1) on device %d was %d, got %d\n",
		    dev, arg, devc->speed));
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return devc->speed;
    }

  if ((devc->open_inputs + devc->open_outputs) > 1)
    {
      DDB (cmn_err (CE_CONT,
		    "Requested sampling rate(2) on device %d was %d, got %d\n",
		    dev, arg, devc->speed));
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return devc->speed;
    }

  if (devc->ratelock)
    {
      DDB (cmn_err (CE_CONT,
		    "Requested sampling rate(3) on device %d was %d, got %d\n",
		    dev, arg, devc->speed));
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return devc->speed;
    }

/* This is the only open device file so change the speed */

  i = 0;

  while (speed_tab[i].speed != -1)
    {
      df = arg - speed_tab[i].speed;
      if (df < 0)
	df = -df;

      if (df < best)
	{
	  best = df;
	  ix = i;
	  if (df == 0)
	    break;
	}

      i++;
    }

  if (ix == -1)			/* No matching rate */
    {
      DDB (cmn_err (CE_CONT,
		    "Requested sampling rate(4) on device %d was %d, got %d\n",
		    dev, arg, devc->speed));
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return devc->speed;
    }

  devc->speed = speed_tab[ix].speed;
  devc->speedbits = speed_tab[ix].speedbits;
#endif
  if (devc->speed != arg)
    {
      DDB (cmn_err (CE_CONT,
		    "Requested sampling rate(5) on device %d was %d, got %d\n",
		    dev, arg, devc->speed));
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return devc->speed;
    }
  DDB (cmn_err (CE_CONT, "Sampling rate set to %d\n", devc->speed));
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return devc->speed;
}

static void
update_fragments (envy24_portc * portc)
{
  int nsamples, nbytes, dev = portc->dev;
  envy24_devc *devc = audio_engines[dev]->devc;

  nsamples = devc->hw_fragsamples;	/* Number of 32 bit samples */

  nbytes = nsamples * portc->channels;

  if (portc->bits & (AFMT_S16_LE | AFMT_S16_BE | AFMT_AC3))
    {
      nbytes *= 2;
    }
  else if (portc->bits & (AFMT_S32_LE | AFMT_S24_LE))
    nbytes *= 4;

  audio_engines[dev]->min_block = nbytes;
  audio_engines[dev]->max_block = nbytes;
}

static short
envy24_audio_set_channels (int dev, short arg)
{
  envy24_portc *portc = audio_engines[dev]->portc;
  envy24_devc *devc = audio_engines[dev]->devc;
  int i, nc = devc->nr_play_channels;
  oss_native_word flags;

  if (envy24_virtualout)
    nc = 10;

  if (arg <= portc->channels)
    return portc->channels;

  /* Force mono->stereo conversion if in skip=2 mode */
  if (devc->skipdevs == 2 && arg < 2)
    arg = 2;

  if (envy24_force_mono)
    arg = 1;

  if (portc->direction == DIR_INPUT)
    {
      if ((portc->chnum + arg) > devc->nr_rec_channels)
	return portc->channels;
      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
      for (i = portc->channels; i < arg; i++)
	if (devc->rec_channel_mask & (1 << (portc->chnum + i)))
	  {
	    MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	    return portc->channels;
	  }
      for (i = portc->channels; i < arg; i++)
	devc->rec_channel_mask |= (1 << (portc->chnum + i));
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
    }
  else
    {
      if ((portc->chnum + arg) > nc)
	return portc->channels;
      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
      for (i = portc->channels; i < arg; i++)
	if (devc->play_channel_mask & (1 << (portc->chnum + i)))
	  {
	    MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	    return portc->channels;
	  }
      for (i = portc->channels; i < arg; i++)
	devc->play_channel_mask |= (1 << (portc->chnum + i));
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
    }

  portc->channels = arg;
  update_fragments (portc);

  return portc->channels;
}

static unsigned int
envy24_audio_set_format (int dev, unsigned int arg)
{
  envy24_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & audio_engines[dev]->oformat_mask))
    return portc->bits = AFMT_S16_LE;

  portc->bits = arg;

  if (arg == AFMT_AC3)
    {
      envy24_audio_set_channels (dev, 2);
    }

  update_fragments (portc);

  return portc->bits;
}

static int
envy24_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  envy24_devc *devc = audio_engines[dev]->devc;
  envy24_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  int rt;

  if (arg == NULL)
    return OSS_EINVAL;

  switch (cmd)
    {
    case SNDCTL_DSP_GET_RECSRC:
    case SNDCTL_DSP_SET_RECSRC:
    case SNDCTL_DSP_GET_PLAYTGT:
    case SNDCTL_DSP_SET_PLAYTGT:
      return *arg = 0;
      break;

    case SNDCTL_DSP_GET_RECSRC_NAMES:
      return oss_encode_enum ((oss_mixer_enuminfo *) arg, portc->name, 0);
      break;

    case SNDCTL_DSP_GET_PLAYTGT_NAMES:
      return oss_encode_enum ((oss_mixer_enuminfo *) arg, portc->name, 0);
      break;

    case SNDCTL_DSP_GET_CHNORDER:
      *(oss_uint64_t *) arg = CHNORDER_UNDEF;
      return 0;
    }

  if (devc->model_data->auxdrv->spdif_ioctl == NULL)
    return OSS_EINVAL;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  rt = devc->model_data->auxdrv->spdif_ioctl (devc, dev, cmd, arg);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return rt;
}

static void envy24_audio_trigger (int dev, int state);

static void
envy24_audio_reset (int dev)
{
#ifdef DO_TIMINGS
  oss_do_timing ("Envy24: Reset audio");
#endif
  envy24_audio_trigger (dev, 0);
}

#define WriteCsByte(devc, b, v) (devc)->spdif_cbits[b]=(v)
#define ReadCsByte(devc, b) (devc)->spdif_cbits[b]

static __inline__ void
WriteCsField (envy24_devc * devc, unsigned char bByteNum,
	      unsigned short bMask, unsigned short bBits)
{
  /* Get current reg value. */
  unsigned char bTemp = ReadCsByte (devc, bByteNum);

  /* Clear field to be written. */
  bTemp &= ~(bMask);

  /* Set new values. */
  WriteCsByte (devc, bByteNum, (unsigned char) (bTemp | (bBits & bMask)));
}

static void
envy24_setup_pro_speed (envy24_devc * devc)
{

  switch (devc->speed)
    {
    case 32000:
      WriteCsField (devc, 0, 0xc0, 0xc0);
      break;

    case 44100:
      WriteCsField (devc, 0, 0xc0, 0x40);
      break;

    case 48000:
      WriteCsField (devc, 0, 0xc0, 0x80);
      break;

    default:
      WriteCsField (devc, 0, 0xc0, 0x00);
      break;
    }
}

static void
setup_pro_mode (envy24_devc * devc)
{
  devc->spdif_cbits[0] |= 0x01;	/* Pro mode */
  devc->spdif_cbits[2] |= 0x2c;	/* 24-bit data word */

  envy24_setup_pro_speed (devc);
}

static void
envy24_setup_consumer_speed (envy24_devc * devc)
{

  /*
   * Set the sampling rate indication
   */
  if (devc->ac3_mode)
    WriteCsField (devc, 0, 0x02, 0x02);	/* 1:1 = 1 */
  else
    WriteCsField (devc, 0, 0x02, 0x00);	/* 1:1 = 0 */

  switch (devc->speed)
    {
    case 22050L:
      WriteCsField (devc, 0, 0xC0, 0x00);	/* 7:6 = 00 */
      WriteCsField (devc, 3, 0x0F, 0x00);	/* 3:0 = 0000 */
      WriteCsField (devc, 4, 0x0F, 0x09);	/* 3:0 = 1001 */
      break;
    case 32000L:
      WriteCsField (devc, 0, 0xC0, 0xC0);	/* 7:6 = 11 */
      WriteCsField (devc, 3, 0x0F, 0x03);	/* 3:0 = 0011 */
      WriteCsField (devc, 4, 0x0F, 0x00);	/* 3:0 = 0000 */
      break;
    case 44100L:
      WriteCsField (devc, 0, 0xC0, 0x40);	/* 7:6 = 01 */
      WriteCsField (devc, 3, 0x0F, 0x00);	/* 3:0 = 0000 */
      WriteCsField (devc, 4, 0x0F, 0x00);	/* 3:0 = 0000 */
      break;
    case 48000L:
      WriteCsField (devc, 0, 0xC0, 0x80);	/* 7:6 = 10 */
      WriteCsField (devc, 3, 0x0F, 0x02);	/* 3:0 = 0010 */
      WriteCsField (devc, 4, 0x0F, 0x00);	/* 3:0 = 0000 */
      break;
    case 88200L:
      WriteCsField (devc, 0, 0xC0, 0x00);	/* 7:6 = 00 */
      WriteCsField (devc, 3, 0x0F, 0x00);	/* 3:0 = 0000 */
      WriteCsField (devc, 4, 0x0F, 0x05);	/* 3:0 = 0101 */
      break;
    case 96000L:
      WriteCsField (devc, 0, 0xC0, 0x00);	/* 7:6 = 00 */
      WriteCsField (devc, 3, 0x0F, 0x00);	/* 3:0 = 0000 */
      WriteCsField (devc, 4, 0x0F, 0x04);	/* 3:0 = 0100 */
      break;
    default:
      WriteCsField (devc, 0, 0xC0, 0x00);	/* 7:6 = 00 */
      WriteCsField (devc, 3, 0x0F, 0x00);	/* 3:0 = 0000 */
      WriteCsField (devc, 4, 0x0F, 0x00);	/* 3:0 = 0000 */
      break;
    }
}

static void
setup_consumer_mode (envy24_devc * devc)
{
  WriteCsByte (devc, 0, ReadCsByte (devc, 0) & ~(0x02));	/* Set audio mode */
  WriteCsByte (devc, 0, ReadCsByte (devc, 0) & ~(0x38));	/* Set no emphasis */

  WriteCsByte (devc, 0, ReadCsByte (devc, 0) & ~(0x04));	/* Set "original" */
  WriteCsByte (devc, 1, ReadCsByte (devc, 1) | (0x80));	/* Set "original" */

  envy24_setup_consumer_speed (devc);
}

static void
setup_spdif_control (envy24_devc * devc)
{
/*  unsigned char *cbits; */

  memset (devc->spdif_cbits, 0, sizeof (devc->spdif_cbits));

/*  cbits = devc->spdif_cbits; */

  if (devc->spdif_pro_mode)
    {
      setup_pro_mode (devc);
    }
  else
    {
      setup_consumer_mode (devc);
    }
}

/*ARGSUSED*/ 
static int
envy24_audio_open (int dev, int mode, int open_flags)
{
  envy24_portc *portc = audio_engines[dev]->portc;
  envy24_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  mode |= ADEV_NOVIRTUAL;

  if (devc->playbuf == NULL || devc->recbuf == NULL)
    {
      cmn_err (CE_WARN, "No DMA buffer\n");
      return OSS_ENOSPC;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode != 0 || devc->direct_audio_opened != 0)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (portc->direction == DIR_INPUT)
    {
      if (devc->rec_channel_mask & (1 << portc->chnum))
	{
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EBUSY;
	}
      devc->rec_channel_mask |= (1 << portc->chnum);
    }
  else
    {
      if (devc->play_channel_mask & (1 << portc->chnum))
	{
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EBUSY;
	}
      devc->play_channel_mask |= (1 << portc->chnum);
    }

  portc->open_mode = mode;
  portc->channels = 1;
  if (devc->skipdevs == 2)
    portc->channels = 2;
  portc->pcm_qlen = 0;
  if (portc->direction == DIR_INPUT)
    {
      if (devc->open_inputs++ == 0 && devc->open_outputs == 0)
	{
	  devc->speed = speed_tab[devc->pending_speed_sel].speed;
	  devc->speedbits = speed_tab[devc->pending_speed_sel].speedbits;
	}
    }
  else
    {
      if (devc->open_inputs == 0 && devc->open_outputs++ == 0)
	{
	  if (portc->flags & PORTC_SPDOUT)
	    {
	      setup_spdif_control (devc);
	    }

	  devc->speed = speed_tab[devc->pending_speed_sel].speed;
	  devc->speedbits = speed_tab[devc->pending_speed_sel].speedbits;
	}
    }
#if 1
  if (devc->use_src)
    {
      /* SRC stuff */
      audio_engines[dev]->flags |= ADEV_FIXEDRATE;
      audio_engines[dev]->fixed_rate = devc->speed;
      audio_engines[dev]->min_rate = devc->speed;
      audio_engines[dev]->max_rate = devc->speed;
    }
  else
    {
      audio_engines[dev]->flags &= ~ADEV_FIXEDRATE;
      audio_engines[dev]->fixed_rate = 0;
      audio_engines[dev]->min_rate = 8000;
      audio_engines[dev]->max_rate = 96000;
    }
#endif
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/ 
static void
envy24_audio_close (int dev, int mode)
{
  envy24_devc *devc = audio_engines[dev]->devc;
  envy24_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  int i;

  envy24_audio_reset (dev);

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->open_mode = 0;
  if (portc->flags & PORTC_SPDOUT)
    devc->ac3_mode = 0;
  if (portc->direction == DIR_INPUT)
    {
      devc->open_inputs--;
      for (i = 0; i < portc->channels; i++)
	devc->rec_channel_mask &= ~(1 << (portc->chnum + i));
    }
  else
    {
      devc->open_outputs--;
      for (i = 0; i < portc->channels; i++)
	devc->play_channel_mask &= ~(1 << (portc->chnum + i));
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/ 
static void
envy24_audio_output_block (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
}

/*ARGSUSED*/ 
static void
envy24_audio_start_input (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
}

static int
envy24_sync_control (int dev, int event, int mode)
{
  envy24_devc *devc = audio_engines[dev]->devc;
  envy24_portc *portc = audio_engines[dev]->portc;

  if (event == SYNC_PREPARE)
    {
      if (mode & PCM_ENABLE_OUTPUT)
	{
	  if (!devc->playback_prepared)
	    devc->hw_playfrag = 0;
	  handle_playdev (devc, portc, devc->hw_playfrag);
	  handle_playdev (devc, portc, devc->hw_playfrag + 1);
	  if (devc->writeahead == 2)
	    handle_playdev (devc, portc, devc->hw_playfrag + 2);
	  envy24_prepare_play_engine (devc);
	  portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	}

      if (mode & PCM_ENABLE_INPUT)
	{
	  if (devc->active_inputs == 0)
	    {
	      envy24_start_recording (devc);
	    }
	  portc->trigger_bits |= PCM_ENABLE_INPUT;
	}
      return 0;
    }

  if (event == SYNC_TRIGGER)
    {
      if (mode & PCM_ENABLE_OUTPUT)
	{
	  envy24_prepare_play_engine (devc);	/* Just to make sure */
	  devc->hw_playfrag = 1 + devc->writeahead;
	  if (devc->active_outputs++ == 0)
	    envy24_launch_play_engine (devc);
	}

      if (mode & PCM_ENABLE_INPUT)
	{
	  if (devc->active_inputs++ == 0)
	    {
	      devc->hw_recfrag = 0;
	      envy24_launch_recording (devc);
	    }
	}
      return 0;
    }

  return OSS_EIO;
}

static void
envy24_audio_trigger (int dev, int state)
{
  int changed;
  oss_native_word flags;

  envy24_portc *portc = audio_engines[dev]->portc;
  envy24_devc *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  changed = state ^ portc->trigger_bits;

  if (portc->direction == DIR_OUTPUT && (changed & PCM_ENABLE_OUTPUT))
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Envy24: Trigger start output");
#endif
	  portc->trigger_bits = state;
	  if (devc->active_outputs++ == 0)
	    start_playback (devc);
	}
      else
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Envy24: Trigger stop output");
#endif
	  portc->trigger_bits = state;
	  if (--devc->active_outputs == 0)
	    envy24_stop_playback (devc);
	}
    }

  if (portc->direction == DIR_INPUT && (changed & PCM_ENABLE_INPUT))
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  portc->trigger_bits = state;
	  if (devc->active_inputs++ == 0)
	    {
	      envy24_start_recording (devc);
	      envy24_launch_recording (devc);
	    }
	}
      else
	{
	  if (--devc->active_inputs == 0)
	    envy24_stop_recording (devc);
	  portc->trigger_bits = state;
	}
    }

  portc->trigger_bits = state;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/ 
static int
envy24_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  int nsamples, nbytes;

  envy24_portc *portc = audio_engines[dev]->portc;
  envy24_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  if (audio_engines[dev]->flags & ADEV_NOINPUT)
    return OSS_EACCES;

  nsamples = devc->hw_fragsamples;	/* Number of 32 bit samples */

  nbytes = nsamples * portc->channels;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
#ifdef DO_RIAA
  memset (portc->riaa_parms, 0, sizeof (portc->riaa_parms));
  if (portc->riaa_filter)
    cmn_err (CE_CONT, "oss: RIAA filter activated for /dev/dsp%d\n", dev);
#endif

  if (portc->bits & (AFMT_S16_LE | AFMT_S16_BE | AFMT_AC3))
    {
      nbytes *= 2;
    }
  else if (portc->bits & (AFMT_S32_LE | AFMT_S24_LE))
    nbytes *= 4;

  if (nbytes != bsize)
    {
      dmap_p dmap = audio_engines[dev]->dmap_in;
      dmap->fragment_size = bsize = nbytes;
      dmap->bytes_in_use = dmap->fragment_size * dmap->nfrags;
      if (dmap->bytes_in_use > dmap->buffsize)
	{
	  dmap->nfrags = dmap->buffsize / dmap->fragment_size;
	  dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/ 
static int
envy24_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  int nsamples, nbytes;
  oss_native_word flags;

  envy24_portc *portc = audio_engines[dev]->portc;
  envy24_devc *devc = audio_engines[dev]->devc;

  if (audio_engines[dev]->flags & ADEV_NOOUTPUT)
    return OSS_EACCES;

  nsamples = devc->hw_fragsamples;	/* Number of 32 bit samples */

  nbytes = nsamples * portc->channels;

  if (portc->flags & PORTC_SPDOUT)
    if (portc->bits == AFMT_AC3)
      devc->ac3_mode = 1;

  if (portc->bits & (AFMT_S16_LE | AFMT_S16_BE | AFMT_AC3))
    {
      nbytes *= 2;
    }
  else if (portc->bits & (AFMT_S32_LE | AFMT_S32_BE))
    nbytes *= 4;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (nbytes != bsize)
    {
      dmap_p dmap = audio_engines[dev]->dmap_out;
      cmn_err (CE_CONT, "Fragment size mismatch: hw=%d, sw=%d\n",
	       nbytes, bsize);
      cmn_err (CE_NOTE,
	       "Application bug detected. Fix ioctl() calling order\n");

      oss_audio_set_error (dev, E_PLAY,
			   OSSERR (1012, "Wrong ioctl call order"), 0);
      /*
       * Errordesc: The envy24 driver requires that number of channels, sample format and
       * sampling rate are set before calling any ioctl call that may lock
       * the fragment size prematurely. In such case the driver cannot change the
       * fragment size to value that is suitable for the device.
       *
       * Please use the recommended ioctl call order defined in
       * http://manuals.opensound.com/developer/callorder.html.
       */
      dmap->fragment_size = bsize = nbytes;
      dmap->bytes_in_use = dmap->fragment_size * dmap->nfrags;
      if (dmap->bytes_in_use > dmap->buffsize)
	{
	  dmap->nfrags = dmap->buffsize / dmap->fragment_size;
	  dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;
	}

      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EIO;
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/ 
static int
envy24_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  envy24_devc *devc = audio_engines[dev]->devc;

  if (dmap->dmabuf != NULL)
    return 0;

#if 0
/*
 * Ignore the direction parameter since it's missleading. Instead use the
 * ADEV_NOINPUT/ADEV_NOOUTPUT flag.
 */

  if (audio_engines[dev]->flags & ADEV_NOINPUT)
    direction = OPEN_WRITE;
  else
    direction = OPEN_READ;
#endif

  dmap->buffsize = devc->skipdevs * DEV_BUFSIZE;
  dmap->dmabuf_phys = 0;
  dmap->dmabuf = KERNEL_MALLOC (dmap->buffsize);

  if (dmap->dmabuf == NULL)
    {
      cmn_err (CE_WARN, "Failed to allocate a DMA buffer\n");
      return OSS_ENOSPC;
    }
  memset (dmap->dmabuf, 0, dmap->buffsize);
  return 0;
}

/*ARGSUSED*/ 
static int
envy24_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;
#if 1
  KERNEL_FREE (dmap->dmabuf);
#endif
  dmap->dmabuf = NULL;
  return 0;
}

static int
envy24_check_input (int dev)
{
  envy24_devc *devc = audio_engines[dev]->devc;

  if (!devc->recording_started)
    return 0;

  cmn_err (CE_NOTE, "Input timed out.\n");
  return OSS_EIO;
}

static int
envy24_check_output (int dev)
{
  envy24_devc *devc = audio_engines[dev]->devc;

  if (!devc->playback_started)
    return 0;

  cmn_err (CE_NOTE, "Output timed out\n");
  return OSS_EIO;
}

static int
envy24_local_qlen (int dev)
{
  envy24_portc *portc = audio_engines[dev]->portc;

  return portc->pcm_qlen * audio_engines[dev]->dmap_out->fragment_size;
}

static const audiodrv_t envy24_audio_driver = {
  envy24_audio_open,
  envy24_audio_close,
  envy24_audio_output_block,
  envy24_audio_start_input,
  envy24_audio_ioctl,
  envy24_audio_prepare_for_input,
  envy24_audio_prepare_for_output,
  envy24_audio_reset,
  envy24_local_qlen,
  NULL,
  NULL,
  NULL,
  envy24_audio_trigger,
  envy24_audio_set_rate,
  envy24_audio_set_format,
  envy24_audio_set_channels,
  NULL,
  NULL,
  envy24_check_input,
  envy24_check_output,
  envy24_alloc_buffer,
  envy24_free_buffer,
  NULL,
  NULL,
  NULL,				/* envy24_get_buffer_pointer */
  NULL,				/* calibrate_speed */
  envy24_sync_control
};

/*ARGSUSED*/ 
static int
envy24_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  extern int envy24_realencoder_hack;

  if (!envy24_realencoder_hack)
    {
      if (cmd == SOUND_MIXER_READ_DEVMASK ||
	  cmd == SOUND_MIXER_READ_RECMASK || cmd == SOUND_MIXER_READ_RECSRC ||
	  cmd == SOUND_MIXER_READ_STEREODEVS)
	return *arg = 0;
    }

  if (cmd == SOUND_MIXER_READ_DEVMASK ||
      cmd == SOUND_MIXER_READ_RECMASK || cmd == SOUND_MIXER_READ_RECSRC ||
      cmd == SOUND_MIXER_READ_STEREODEVS)
    return *arg =
      SOUND_MASK_LINE | SOUND_MASK_PCM | SOUND_MASK_MIC |
      SOUND_MASK_VOLUME | SOUND_MASK_CD;

  if (cmd == SOUND_MIXER_READ_VOLUME || cmd == SOUND_MIXER_READ_PCM ||
      cmd == SOUND_MIXER_READ_LINE || cmd == SOUND_MIXER_READ_MIC ||
      cmd == SOUND_MIXER_READ_CD || cmd == MIXER_READ (SOUND_MIXER_DIGITAL1))
    return *arg = 100 | (100 << 8);
  if (cmd == SOUND_MIXER_WRITE_VOLUME || cmd == SOUND_MIXER_WRITE_PCM ||
      cmd == SOUND_MIXER_WRITE_LINE || cmd == SOUND_MIXER_READ_MIC ||
      cmd == SOUND_MIXER_WRITE_CD ||
      cmd == MIXER_WRITE (SOUND_MIXER_DIGITAL1))
    return *arg = 100 | (100 << 8);
  if (cmd == SOUND_MIXER_READ_CAPS)
    return *arg = SOUND_CAP_EXCL_INPUT;
  if (cmd == SOUND_MIXER_PRIVATE1)
    return *arg = 0;
  return OSS_EINVAL;
}

static int
envy24_set_control (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;

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

      case 4:
	{
	  int tmp = envy24_read_cci (devc, 0x20);
	  return !!(tmp & 0x10);
	}
	break;

      case 5:
	return devc->ratelock;
	break;

      case 6:
	return devc->speed;
	break;

      case 7:
	return devc->sync_locked =
	  devc->model_data->auxdrv->get_locked_status (devc);

      default:
	return OSS_EIO;
      }

  if (cmd == SNDCTL_MIX_WRITE)
    switch (ctrl)
      {
      case 1:
	if (value < 0 || value > 12)
	  return OSS_EIO;

	if (value != devc->pending_speed_sel)
	  {
	    if (devc->open_inputs == 0 && devc->open_outputs == 0)	/* IDDLE */
	      OUTB (devc->osdev, value, devc->mt_base + 0x01);	/* Make the change now */
	  }

	return devc->pending_speed_sel = value;
	break;

      case 2:
	if (value < 0 || value > 2)
	  return OSS_EIO;
	return devc->syncsource = value;
	break;

      case 3:
	return devc->use_src = value;
	break;

      case 4:
	{
	  int tmp = envy24_read_cci (devc, 0x20) & ~0x10;
	  if (value)
	    tmp |= 0x10;	/* Optical */
	  envy24_write_cci (devc, 0x20, tmp);
	  return !!(tmp & 0x10);
	}
	break;

      case 5:
	return devc->ratelock = value;
	break;

      case 6:
	return devc->speed;
	break;

      case 7:
	return devc->sync_locked =
	  devc->model_data->auxdrv->get_locked_status (devc);
	break;

      default:
	return OSS_EIO;
      }

  return OSS_EINVAL;
}

static int
read_mon (envy24_devc * devc, int ch, int is_right)
{
  int tmp;

  if (ch >= 20)
    return 0;

  OUTB (devc->osdev, ch, devc->mt_base + 0x3a);
  tmp = INW (devc->osdev, devc->mt_base + 0x38);

  if (is_right)
    tmp >>= 8;
  tmp &= 0x7f;
  if (tmp > 0x60)		/* Mute? */
    return 0;

  tmp = (tmp * 15) / 10;
  return 144 - tmp;
}

static int
mon_scale (int v)
{
  if (v == 0)
    return 0x7f;		/* Mute */

  v = 144 - v;

  v = (10 * v) / 15;
  if (v > 0x60)
    v = 0x7f;
  return v;
}

static void
mon_set (envy24_devc * devc, int ch, int left, int right)
{

  left = mon_scale (left);
  right = mon_scale (right);

  OUTB (devc->osdev, 1, devc->mt_base + 0x3b);	/* Volume change rate */
  OUTB (devc->osdev, ch, devc->mt_base + 0x3a);
  OUTW (devc->osdev, left | (right << 8), devc->mt_base + 0x38);
}

static int
read_peak (envy24_devc * devc, int ch)
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
envy24_get_peak (int dev, int ctrl, unsigned int cmd, int value)
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

  envy24_devc *devc = mixer_devs[dev]->devc;

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

static int
envy24_set_mon (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;

  int i, orign, n = -1, left = 0, right = 0;

  for (i = 0; i < 10 && n == -1; i++)
    if (ctrl & (1 << i))
      n = i;

  if (n == -1)
    return OSS_EINVAL;

  orign = n;
  if (ctrl & 0x80000000)
    n += 10;			/* Recording stream */

  if (cmd == SNDCTL_MIX_READ)
    {
      left = read_mon (devc, n, 0) * 10;
      if (ctrl & (1 << (orign + 1)))	/* Stereo mode? */
	right = read_mon (devc, n + 1, 1) * 10;
      else
	right = read_mon (devc, n, 1) * 10;

      return left | (right << 16);
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {
      left = value & 0xffff;
      right = (value >> 16) & 0xffff;

      if (right > 1440)
	right = 1440;
      if (left > 1440)
	left = 1440;

      if (ctrl & (1 << (orign + 1)))	/* Stereo mode? */
	{
	  mon_set (devc, n, left / 10, 0);
	  mon_set (devc, n + 1, 0, right / 10);
	}
      else
	{
	  mon_set (devc, n, left / 10, right / 10);
	}
      return left | (right << 16);
    }
  return OSS_EINVAL;
}

static int
get_loopback (envy24_devc * devc, int ch)
{
  int tmp;

  tmp = INL (devc->osdev, devc->mt_base + 0x34);
  return (tmp >> (4 * ch)) & 0x07;
}

static int
get_spdif_loopback (envy24_devc * devc, int ch)
{
  int tmp;

  tmp = INL (devc->osdev, devc->mt_base + 0x34);
  return (tmp >> ((4 * ch) + 3)) & 0x01;
}

static void
set_loopback (envy24_devc * devc, int ch, int val)
{
  int tmp = INL (devc->osdev, devc->mt_base + 0x34);
  tmp &= ~(0x07 << (4 * ch));
  tmp |= (val & 0x07) << (4 * ch);
  OUTL (devc->osdev, tmp, devc->mt_base + 0x34);
}

static void
set_spdif_loopback (envy24_devc * devc, int ch, int val)
{
  int tmp = INL (devc->osdev, devc->mt_base + 0x34);
  tmp &= ~(0x08 << (4 * ch));
  tmp |= (val & 0x01) << ((4 * ch) + 3);
  OUTL (devc->osdev, tmp, devc->mt_base + 0x34);
}

static int
envy24_set_outrout (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int tmp, i;

  if (cmd == SNDCTL_MIX_READ)
    {
      tmp = INW (devc->osdev, devc->mt_base + 0x30);

      for (i = 0; i < 8; i++)
	if (ctrl & (1 << i))
	  {
	    tmp = (tmp >> (2 * i)) & 0x03;
	    switch (tmp)
	      {
	      case 0:		/* DMA */
		return 0;
		break;

	      case 1:		/* Monitor */
		return 1;
		break;

	      case 2:		/* Analog input loopback */
		return 2 + get_loopback (devc, i);
		break;

	      case 3:		/* S/PDIF input loopback */
		return 10 + get_spdif_loopback (devc, i);
		break;
	      }
	  }

      return OSS_EINVAL;
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {
      tmp = INW (devc->osdev, devc->mt_base + 0x30);
      for (i = 0; i < 8; i++)
	if (ctrl & (1 << i))
	  {
	    int ch;
	    ch = i / 2;
	    if (i & 1)
	      ch += 4;

	    tmp &= ~(0x03 << (ch * 2));	/* Cleanup */

	    if (value == 0)	/* DMA */
	      continue;

	    if (value == 1)	/* Monitor */
	      {
		tmp |= 1 << (ch * 2);
		continue;
	      }

	    if (value < 10)	/* Analog inputs */
	      {
		tmp |= 2 << (ch * 2);
		set_loopback (devc, i, value - 2);
		continue;
	      }

	    tmp |= 3 << (ch * 2);
	    set_spdif_loopback (devc, i, value - 10);
	  }

      OUTW (devc->osdev, tmp, devc->mt_base + 0x30);
      return value;
    }
  return OSS_EINVAL;
}

static int
envy24_set_stereo_outrout (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int tmp, i;

  if (cmd == SNDCTL_MIX_READ)
    {
      tmp = INW (devc->osdev, devc->mt_base + 0x30);

      for (i = 0; i < 8; i++)
	if (ctrl & (1 << i))
	  {
	    int ch;
	    ch = i / 2;
	    if (i & 1)
	      ch += 4;
	    tmp = (tmp >> (2 * ch)) & 0x03;
	    switch (tmp)
	      {
	      case 0:		/* DMA */
		return 0;
		break;

	      case 1:		/* Monitor */
		return 1;
		break;

	      case 2:		/* Analog input loopback */
		return 2 + get_loopback (devc, i) / 2;
		break;

	      case 3:		/* S/PDIF input loopback */
		return 6;
		break;
	      }
	  }

      return OSS_EINVAL;
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {
      tmp = INW (devc->osdev, devc->mt_base + 0x30);
      for (i = 0; i < 8; i++)
	if (ctrl & (1 << i))
	  {
	    int ch;
	    ch = i / 2;
	    if (i & 1)
	      ch += 4;

	    tmp &= ~(0x03 << (ch * 2));	/* Cleanup */

	    if (value == 0)	/* DMA */
	      {
		continue;
	      }

	    if (value == 1)	/* Monitor */
	      {
		tmp |= 1 << (ch * 2);
		continue;
	      }

	    if (value < 6)	/* Analog inputs */
	      {
		tmp |= 2 << (ch * 2);
		set_loopback (devc, i, (value - 2) * 2 + (i & 1));
		continue;
	      }

	    tmp |= 3 << (ch * 2);	/* S/PDIF */
	    set_spdif_loopback (devc, i, (value - 10) + (i & 1));
	    continue;
	  }

      OUTW (devc->osdev, tmp, devc->mt_base + 0x30);
      return value;
    }
  return OSS_EINVAL;
}

static int
read_spdif_stereo (envy24_devc * devc)
{
  int tmp;
  tmp = INL (devc->osdev, devc->mt_base + 0x32);

/*
 * Look only at the left channel. Assume the same settings on right.
 */

  switch (tmp & 0x03)
    {
    case 0:			/* From DMA */
      return 0;
      break;

    case 1:			/* From digital mixer */
      return 1;
      break;

    case 2:			/* Analog input # loopback */
      return 2 + ((tmp >> 9) & 0x03);
      break;

    case 3:			/* S/PDIF input loopback */
      return 6;
      break;
    }

  return 0;
}

static int
read_spdif_mono (envy24_devc * devc, int ch)
{
  int tmp, v;
  tmp = INL (devc->osdev, devc->mt_base + 0x32);

  if (ch == 0)			/* Left channel ? */
    v = (tmp) & 0x03;
  else
    v = (tmp >> 2) & 0x03;

  switch (v)
    {
    case 0:			/* DMA */
      return 0;
      break;

    case 1:			/* Monitor */
      return 1;
      break;

    case 2:			/* Analog input */
      if (ch == 0)		/* Left or right */
	v = (tmp >> 8) & 0x07;
      else
	v = (tmp >> 12) & 0x07;

      return 2 + v;
      break;

    case 3:
      if (ch == 0)		/* Left or right */
	v = (tmp >> 11) & 0x01;
      else
	v = (tmp >> 15) & 0x01;
      return 10 + v;
      break;

    }

  return 0;
}

static int
write_spdif_mono (envy24_devc * devc, int ch, int val)
{
  int tmp = 0, v;
  tmp = INW (devc->osdev, devc->mt_base + 0x32);

  if (val == 0)			/* DMA */
    {
      if (ch == 0)		/* Left */
	tmp &= ~0x0003;
      else
	tmp &= ~0x000c;
      goto do_ne;
    }

  if (val == 1)			/* Monitor */
    {
      if (ch == 0)		/* Left */
	{
	  tmp &= ~0x0003;
	  tmp |= 0x0001;
	}
      else
	{
	  tmp &= ~0x000c;
	  tmp |= 0x0004;
	}
      goto do_ne;
    }

  if (val < 10)			/* Analog inputs */
    {
      v = (val - 2) & 0x07;

      if (ch == 0)		/* Left */
	{
	  tmp &= ~(0x0003 | (0x07 << 8));
	  tmp |= 0x02 | (v << 8);
	}
      else
	{
	  tmp &= ~(0x000c | (0x07 << 12));
	  tmp |= 0x08 | (v << 12);
	}
      goto do_ne;
    }

  /* Else S/PDIF */

  if (ch == 0)			/* Left */
    {
      tmp &= ~(1 << 11);
      tmp |= 0x0003;

      if (val == 11)
	tmp |= 1 << 11;
    }
  else
    {
      tmp &= ~(1 << 15);
      tmp |= 0x000c;

      if (val == 11)
	tmp |= 1 << 15;
    }

do_ne:
  OUTW (devc->osdev, tmp, devc->mt_base + 0x32);
  return val;
}

static int
write_spdif_stereo (envy24_devc * devc, int val)
{
  int tmp = 0, v;

  if (val == 0)			/* DMA */
    {
      tmp = 0x0000;
      goto do_ne;
    }

  if (val == 1)			/* Monitor */
    {
      tmp = 0x0005;
      goto do_ne;
    }

  if (val < 6)			/* Analog inputs */
    {
      tmp = 0x000a;

      v = (val - 2) * 2;
      tmp |= (v << 8);
      tmp |= ((v + 1) << 12);
      goto do_ne;
    }

  /* Else S/PDIF */

  tmp = 0x800f;

do_ne:
  OUTW (devc->osdev, tmp, devc->mt_base + 0x32);
  return val;
}

static int
envy24_set_spdifrout (int dev, int ctrl, unsigned int cmd, int value)
{
  envy24_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      if (ctrl == 3)
	return read_spdif_stereo (devc);
      else
	return read_spdif_mono (devc, ctrl - 1);
    }
  else if (cmd == SNDCTL_MIX_WRITE)
    {
      if (ctrl == 3)
	return write_spdif_stereo (devc, value);
      else
	return write_spdif_mono (devc, ctrl - 1, value);
    }
  return OSS_EINVAL;
}

/*ARGSUSED*/ 
static int
create_output_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, num, skip;
  char tmp[64];

  int nc = devc->nr_play_channels;

  if (envy24_virtualout)
    {
      mask = 0;
      nc = 10;
      for (i = 0; i < nc; i++)
	mask |= (1 << i);
    }

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_OUTPUT")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  for (i = 0; i < nc; i += skip)
    {

      num = 1 << i;
      if (!(mask & num))
	continue;		/* Not present */

      sprintf (tmp, "@pcm%d", devc->play_portc[i / 2].dev);

      num |= 1 << (i + 1);
      if ((err = mixer_ext_create_control (dev, group,
					   num, envy24_set_mon,
					   MIXT_STEREOSLIDER16,
					   tmp, 1440,
					   MIXF_MONVOL |
					   MIXF_READABLE |
					   MIXF_WRITEABLE | MIXF_CENTIBEL)) <
	  0)
	return err;
      if ((err = mixer_ext_create_control (dev, group,
					   num, envy24_get_peak,
					   MIXT_STEREOPEAK,
					   "-", 144,
					   MIXF_READABLE | MIXF_DECIBEL)) < 0)
	return err;
    }

  return 0;
}

/*ARGSUSED*/ 
static int
create_input_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->inportmask, group, err, num, skip;
  char tmp[64];

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_INPUT")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  for (i = 0; i < devc->nr_rec_channels && i < 10; i += skip)
    {

      num = (1 << i);
      if (!(mask & num))
	continue;		/* Not present */

      num |= 0x80000000;	/* Input flag */

      if (i == 8)
	strcpy (tmp, "ENVY24_SPDIN");
      else
	sprintf (tmp, "ENVY24_IN%d/%d", i + 1, i + 2);

      num |= (1 << (i + 1));

      if ((err = mixer_ext_create_control (dev, group,
					   num, envy24_set_mon,
					   MIXT_STEREOSLIDER16,
					   tmp, 1440,
					   MIXF_MONVOL |
					   MIXF_READABLE |
					   MIXF_WRITEABLE | MIXF_CENTIBEL)) <
	  0)
	return err;

      if ((err = mixer_ext_create_control (dev, group,
					   num, envy24_get_peak,
					   MIXT_STEREOPEAK,
					   "-", 144,
					   MIXF_READABLE | MIXF_DECIBEL)) < 0)
	return err;
    }

  num = (1 << 11);

  if ((err = mixer_ext_create_control (dev, group,
				       num, envy24_get_peak,
				       MIXT_STEREOPEAK,
				       "MONITOR", 144,
				       MIXF_READABLE | MIXF_DECIBEL)) < 0)
    return err;
  return 0;
}

/*ARGSUSED*/ 
static int
create_mon_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, num, skip;
  char tmp[64];

  int nc = devc->nr_play_channels;

  if (envy24_virtualout)
    {
      mask = 0;
      nc = 10;
      for (i = 0; i < nc; i++)
	mask |= (1 << i);
    }

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24_MON")) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  for (i = 0; i < nc; i += skip)
    {

      num = 1 << i;
      if (!(mask & num))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDOUT");
	  else
	    sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);

	  num |= 1 << (i + 1);
	  if ((err = mixer_ext_create_control (dev, group,
					       num, envy24_set_mon,
					       MIXT_STEREOSLIDER16,
					       tmp, 1440,
					       MIXF_MONVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE |
					       MIXF_CENTIBEL)) < 0)
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
					       num, envy24_set_mon,
					       MIXT_STEREOSLIDER16,
					       tmp, 1440,
					       MIXF_MONVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE |
					       MIXF_CENTIBEL)) < 0)
	    return err;
	}
    }

  mask = devc->inportmask;
  for (i = 0; i < devc->nr_rec_channels && i < 10; i += skip)
    {

      num = 1 << i;
      if (!(mask & num))
	continue;		/* Not present */

      num |= 0x80000000;	/* Input flag */

      if (devc->skipdevs == 2)
	{
	  if (i == 8)
	    strcpy (tmp, "ENVY24_SPDIN");
	  else
	    sprintf (tmp, "ENVY24_IN%d/%d", i + 1, i + 2);

	  num |= 1 << (i + 1);
	  if ((err = mixer_ext_create_control (dev, group,
					       num, envy24_set_mon,
					       MIXT_STEREOSLIDER16,
					       tmp, 1440,
					       MIXF_MONVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE |
					       MIXF_CENTIBEL)) < 0)
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
					       num, envy24_set_mon,
					       MIXT_STEREOSLIDER16,
					       tmp, 1440,
					       MIXF_MONVOL |
					       MIXF_READABLE |
					       MIXF_WRITEABLE |
					       MIXF_CENTIBEL)) < 0)
	    return err;
	}
    }

  return 0;
}

/*ARGSUSED*/ 
static int
create_peak_mixer (int dev, envy24_devc * devc, int root)
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
	  strcpy (tmp, "ENVY24_SPDOUT");
	else
	  sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);

	num |= 1 << (i + 1);
	if ((err = mixer_ext_create_control (dev, group,
					     num, envy24_get_peak,
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
	  strcpy (tmp, "ENVY24_SPDIN");
	else if (i == 10)
	  strcpy (tmp, "ENVY24_MAIN");
	else
	  sprintf (tmp, "ENVY24_IN%d/%d", i + 1, i + 2);

	num |= 1 << (i + 1);
	if ((err = mixer_ext_create_control (dev, group,
					     num, envy24_get_peak,
					     MIXT_STEREOPEAK,
					     tmp, 144,
					     MIXF_READABLE | MIXF_DECIBEL)) <
	    0)
	  return err;
      }
    }

  return 0;
}

void
envy24_set_enum_mask (int dev, int ctl, oss_native_word mask)
{
  oss_mixext *ext;
  int i;

  ext = mixer_find_ext (dev, ctl);

  if (ext == NULL)
    {
      cmn_err (CE_WARN, "Cannot locate the mixer extension\n");
      return;
    }

  memset (ext->enum_present, 0, sizeof (ext->enum_present));

  for (i = 0; i < 32; i++)
    if (mask & (1 << i))
      ext->enum_present[i / 8] |= (1 << (i % 8));
}

/*ARGSUSED*/ 
static int
create_rout_mixer (int dev, envy24_devc * devc, int root)
{
  int i, mask = devc->outportmask, group, err, skip, num, chnum;
  char tmp[64];

  if ((group =
       mixer_ext_create_group_flags (dev, 0, "ENVY24_ROUTE", MIXF_FLAT)) < 0)
    return group;

  skip = devc->skipdevs;
  if (skip != 2)
    skip = 1;

  for (i = 0; i < 8; i += skip)
    {

      num = 1 << i;
      if (!(mask & num))
	continue;		/* Not present */

      if (devc->skipdevs == 2)
	{
	  oss_native_word tmpmask = 0x00000001;
	  int j;

	  if (i < 2)
	    tmpmask |= 0x00000002;
	  for (j = 0; j < 8; j++)
	    if (mask & (1 << j))
	      tmpmask |= 1 << ((j / 2) + 2);
	  if (devc->model_data->flags & MF_SPDIF)
	    tmpmask |= 0x00000040;

	  sprintf (tmp, "ENVY24_OUT%d/%d", i + 1, i + 2);
	  chnum = i;
	  num = (1 << chnum) | (1 << (chnum + 1));
	  if ((err = mixer_ext_create_control (dev, group,
					       num, envy24_set_stereo_outrout,
					       MIXT_ENUM,
					       tmp, 7,
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	  envy24_set_enum_mask (dev, err, tmpmask);
	}
      else
	{
	  oss_native_word tmpmask = 0x00000001;
	  int j;

	  sprintf (tmp, "ENVY24_OUT%d", i + 1);
	  chnum = i;
	  num = 1 << chnum;

	  if (i < 2)
	    tmpmask |= (1 << 1);
	  for (j = 0; j < 8; j++)
	    if (mask & (1 << j))
	      tmpmask |= 1 << (j + 2);
	  if (devc->model_data->flags & MF_SPDIF)
	    tmpmask |= (3 << 10);

	  if ((err = mixer_ext_create_control (dev, group,
					       num, envy24_set_outrout,
					       MIXT_ENUM,
					       tmp, 12,
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	  envy24_set_enum_mask (dev, err, tmpmask);
	}
    }

  mask = devc->inportmask;

  if (devc->model_data->flags & MF_SPDIF)
    {
      if (devc->skipdevs == 2)
	{
	  oss_native_word tmpmask = 0x00000043;
	  int j;
	  for (j = 0; j < 8; j++)
	    if (mask & (1 << j))
	      tmpmask |= (1 << ((j / 2) + 2));

	  if ((err = mixer_ext_create_control (dev, group,
					       3, envy24_set_spdifrout,
					       MIXT_ENUM,
					       "ENVY24_SPDIF", 7,
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	  envy24_set_enum_mask (dev, err, tmpmask);
	}
      else
	{
	  oss_native_word tmpmask = 0x00000c03;
	  int j;
	  for (j = 0; j < 8; j++)
	    if (mask & (1 << j))
	      tmpmask |= (1 << (j + 2));

	  if ((err = mixer_ext_create_control (dev, group,
					       1, envy24_set_spdifrout,
					       MIXT_ENUM,
					       "ENVY24_SPDIFL", 12,
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	  envy24_set_enum_mask (dev, err, tmpmask);

	  if ((err = mixer_ext_create_control (dev, group,
					       2, envy24_set_spdifrout,
					       MIXT_ENUM,
					       "ENVY24_SPDIFR", 12,
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return err;
	  envy24_set_enum_mask (dev, err, tmpmask);
	}
    }

#if 0
  for (i = 0; i < devc->nr_rec_channels && i < 10; i += skip)
    {

      num = 1 << i;
      if (!(mask & num))
	continue;		/* Not present */

      num |= 0x80000000;	/* Input flag */

      if (devc->skipdevs == 2)
	{
	  sprintf (tmp, "ENVY24_IN%d/%d", i + 1, i + 2);
	  num |= 1 << (i + 1);
	  if ((err = mixer_ext_create_control (dev, group,
					       num, envy24_set_mon,
					       MIXT_STEREOSLIDER16,
					       tmp, 1440,
					       MIXF_READABLE |
					       MIXF_WRITEABLE |
					       MIXF_CENTIBEL)) < 0)
	    return err;
	}
      else
	{
	  sprintf (tmp, "ENVY24_IN%d", i + 1);
	  if ((err = mixer_ext_create_control (dev, group,
					       num, envy24_set_mon,
					       MIXT_STEREOSLIDER16,
					       tmp, 1440,
					       MIXF_READABLE |
					       MIXF_WRITEABLE |
					       MIXF_CENTIBEL)) < 0)
	    return err;
	}
    }
#endif

  return 0;
}

static int
envy24_mix_init (int dev)
{
  envy24_devc *devc = mixer_devs[dev]->devc;
  int group, err, ctl;
  int n;

  extern int envy24_mixerstyle;

  if ((group = mixer_ext_create_group (dev, 0, "ENVY24")) < 0)
    return group;

  if (envy24_skipdevs == 2)
    switch (envy24_mixerstyle)
      {
      case 2:
	/* New style input and output mixer sections */
	if ((err = create_output_mixer (dev, devc, group)) < 0)
	  return err;
	if ((err = create_input_mixer (dev, devc, group)) < 0)
	  return err;
	break;

      default:
	/* Traditional mixer (peak meters and montor gains separated) */
	if ((err = create_peak_mixer (dev, devc, group)) < 0)
	  return err;
	if ((err = create_mon_mixer (dev, devc, group)) < 0)
	  return err;
	break;
      }

  if ((err = create_rout_mixer (dev, devc, group)) < 0)
    return err;

  if (devc->model_data->auxdrv->mixer_init)
    if ((err = devc->model_data->auxdrv->mixer_init (devc, dev, group)) < 0)
      return err;

  if ((err = mixer_ext_create_control (dev, group,
				       1, envy24_set_control,
				       MIXT_ENUM,
				       "ENVY24_RATE", 12,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  mixer_ext_set_strings (dev, err, 
   "8000 9600 11025 12000 16000 22050 24000 32000 44100 48000 88200 96000", 0);

  if (devc->model_data->flags & (MF_SPDIF | MF_WCLOCK))
    {
      n = 2;
      if (devc->model_data->flags & MF_WCLOCK)
	n = 3;
      if ((err = mixer_ext_create_control (dev, group,
					   2, envy24_set_control,
					   MIXT_ENUM,
					   "ENVY24_SYNC", n,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;
    }

  if (devc->model_data->flags & MF_SPDSELECT)
    {
      if ((err = mixer_ext_create_control (dev, group,
					   4, envy24_set_control,
					   MIXT_ENUM,
					   "ENVY24_SPDIN", 2,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;
    }

#if 0
/* Always on */
  if ((err = mixer_ext_create_control (dev, group,
				       3, envy24_set_control,
				       MIXT_ONOFF,
				       "ENVY24_SRC", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
#endif

  if ((err = mixer_ext_create_control (dev, group,
				       5, envy24_set_control,
				       MIXT_ONOFF,
				       "ENVY24_RATELOCK", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((ctl = mixer_ext_create_control (dev, group,
				       6, envy24_set_control,
				       MIXT_VALUE,
				       "ENVY24_ACTRATE", 96000,
				       MIXF_READABLE)) < 0)
    return ctl;
  mixer_ext_set_description(dev, ctl, "Sample rate currently used by the device");

#if 1
  if (devc->model_data->auxdrv->get_locked_status)
    {
      devc->sync_locked = devc->model_data->auxdrv->get_locked_status (devc);

      if ((err = mixer_ext_create_control (dev, group,
					   7, envy24_set_control,
					   MIXT_ONOFF,
					   "ENVY24_LOCKED", 1,
					   MIXF_READABLE)) < 0)
	return err;
    }
#endif

  if (devc->model_data->auxdrv->spdif_mixer_init)
    if ((err =
	 devc->model_data->auxdrv->spdif_mixer_init (devc, dev, group)) < 0)
      return err;
  return 0;
}

static mixer_driver_t envy24_mixer_driver = {
  envy24_mixer_ioctl
};

static int
install_adev (envy24_devc * devc, char *name, int flags, int skip,
	      int portc_flags, char *port_id, char *devfile_name)
{
  int dev, i;
  adev_p adev;
  int fmts = 0, last;
  extern int envy24_realencoder_hack;

  if (portc_flags & PORTC_SPDOUT)
    fmts |= AFMT_AC3;

  if ((dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
				   devc->osdev,
				   devc->osdev,
				   name,
				   &envy24_audio_driver,
				   sizeof (audiodrv_t),
				   ADEV_AUTOMODE | ADEV_NOMMAP |
				   flags | ADEV_NOVIRTUAL,
				   fmts | AFMT_S16_LE | AFMT_S32_LE |
				   AFMT_S24_LE, devc, -1,
				   devfile_name)) < 0)
    {
      dev = -1;
      return 0;
    }
  else
    {
      envy24_portc *portc;
      adev = audio_engines[dev];

      if (devc->first_dev == -1)
	devc->first_dev = dev;
      for (i = 0; speed_tab[i].speed != -1; i++)
	adev->rates[adev->nrates++] = speed_tab[i].speed;

      adev->vmix_flags = 0;

      if (flags == DIR_OUTPUT)
	{
	  last = 10;
	  audio_engines[dev]->port_number = devc->curr_outch;
	  audio_engines[dev]->min_rate = 8000;
	  audio_engines[dev]->max_rate = 96000;
	  portc = &devc->play_portc[devc->nr_outdevs++];
	  portc->chnum = devc->curr_outch;
	  strncpy (portc->name, port_id, sizeof (portc->name) - 1);
	  portc->name[sizeof (portc->name) - 1] = 0;
	  devc->curr_outch += skip;
	  if (portc_flags & PORTC_SPDOUT)
	    audio_engines[dev]->caps |= PCM_CAP_DIGITALOUT;
	  if (portc_flags & PORTC_SPDIN)
	    audio_engines[dev]->caps |= PCM_CAP_DIGITALIN;

	}
      else
	{
	  last = 12;
	  portc = &devc->rec_portc[devc->nr_indevs++];
	  audio_engines[dev]->port_number = devc->curr_inch + 10;
	  portc->chnum = devc->curr_inch;
	  strncpy (portc->name, port_id, sizeof (portc->name) - 1);
	  portc->name[sizeof (portc->name) - 1] = 0;
	  devc->curr_inch += skip;
#ifdef DO_RIAA
	  portc->riaa_filter = 0;
#endif
	}

      portc->flags = portc_flags;
      audio_engines[dev]->devc = devc;
      audio_engines[dev]->portc = portc;
      audio_engines[dev]->rate_source = devc->first_dev;

      switch (skip)
	{
	case 1:
	  audio_engines[dev]->caps |= DSP_CH_MONO;
	  audio_engines[dev]->min_channels = 1;
	  audio_engines[dev]->max_channels =
	    (envy24_force_mono) ? 1 : last - portc->chnum;
	  break;

	case 2:
	  audio_engines[dev]->caps |= DSP_CH_STEREO;
	  audio_engines[dev]->min_channels = 1;
	  audio_engines[dev]->max_channels = last - portc->chnum;
	  break;

	default:
	  audio_engines[dev]->caps |= DSP_CH_MULTI;
	  audio_engines[dev]->min_channels = 1;
	  audio_engines[dev]->max_channels = last - portc->chnum;
	}

      audio_engines[dev]->mixer_dev = devc->mixer_dev;
      portc->dev = dev;
      portc->open_mode = 0;
      portc->is_active = 0;
      portc->direction = flags;
      portc->trigger_bits = 0;

      if (envy24_realencoder_hack && flags == DIR_INPUT
	  && devc->nr_indevs > 1)
	if (oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
			       devc->osdev,
			       devc->osdev,
			       "Dummy mixer",
			       &envy24_mixer_driver,
			       sizeof (mixer_driver_t), devc) >= 0)
	  {
	    mixer_devs[devc->mixer_dev]->priority = -1;	/* Don't use as the default mixer */
	  }
    }

  return 1;
}

static int
install_output_devices (envy24_devc * devc, int mask)
{
  char tmp[512], id[32];
  int i, nc, portc_flags = 0;

  char *lr = "", *kind;

  nc = devc->nr_play_channels = MAX_ODEV;
  devc->nr_rec_channels = MAX_IDEV;

  if (devc->skipdevs < 1)
    devc->skipdevs = 1;

  for (i = 0; i < nc; i += devc->skipdevs)
    {
      char *devfile_name = "";

      if (devc->skipdevs != 2)
	lr = (i & 1) ? "R" : "L";
      kind = "";

      if (!(mask & (1 << devc->curr_outch)))
	kind = "(virtual) ";

      switch (devc->curr_outch)
	{
	case 8:
	case 9:
	  portc_flags = PORTC_SPDOUT;
	  sprintf (tmp, "%s %sS/PDIF out %s", devc->model_data->product, kind,
		   lr);
	  sprintf (id, "SPDIF%s", lr);
	  devfile_name = "spdout";
	  break;

	default:
	  if (devc->skipdevs > 1)
	    {
	      sprintf (tmp, "%s %sout%d/%d", devc->model_data->product, kind,
		       i + 1, i + devc->skipdevs);
	      sprintf (id, "OUT%d/%d", i + 1, i + devc->skipdevs);
	    }
	  else
	    {
	      sprintf (tmp, "%s %sout%d", devc->model_data->product, kind,
		       i + 1);
	      sprintf (id, "OUT%d", i + 1);
	    }
	}

      if (mask & (1 << devc->curr_outch))
	install_adev (devc, tmp, ADEV_NOINPUT, devc->skipdevs, portc_flags,
		      id, devfile_name);
      else
	devc->curr_outch += devc->skipdevs;
    }

  return 1;
}

/*ARGSUSED*/ 
static int
install_virtual_output_devices (envy24_devc * devc, int mask)
{
#if 0
  char tmp[512];
  int i, nc;

  char *lr = "";
  nc = devc->nr_play_channels = MAX_ODEV;
  devc->nr_rec_channels = MAX_IDEV;

  if (envy24_virtualout)
    {
      nc = 10;
    }

  if (devc->skipdevs < 1)
    devc->skipdevs = 1;

  for (i = 0; i < nc; i += devc->skipdevs)
    {

      if (devc->skipdevs != 2)
	lr = (i & 1) ? "R" : "L";

      switch (devc->curr_outch)
	{
	case 8:
	case 9:
	  sprintf (tmp, "%s virtual out %s", devc->model_data->product, lr);
	  break;

	default:
	  if (devc->skipdevs > 1)
	    sprintf (tmp, "%s virtual out%d/%d", devc->model_data->product,
		     i + 1, i + devc->skipdevs);
	  else
	    sprintf (tmp, "%s virtual out%d", devc->model_data->product,
		     i + 1);
	}

      if (!(mask & (1 << devc->curr_outch)))	/* Not done yet */
	install_adev (devc, tmp, ADEV_NOINPUT, devc->skipdevs, 0, "virtual", ""); // TODO: Find better device file name
      else
	devc->curr_outch += devc->skipdevs;
    }
#endif
  return 1;
}

static int
install_input_devices (envy24_devc * devc, int mask)
{
  char tmp[512], id[32];
  int i, portc_flags = 0;

  char *lr = "";

  devc->nr_play_channels = MAX_ODEV;
  devc->nr_rec_channels = MAX_IDEV;

  if (devc->skipdevs < 1)
    devc->skipdevs = 1;

  for (i = 0; i < devc->nr_rec_channels; i += devc->skipdevs)
    {
      char *devfile_name = "";

      if (devc->skipdevs != 2)
	lr = (i & 1) ? "R" : "L";

      switch (devc->curr_inch)
	{
	case 8:
	case 9:
	  portc_flags = PORTC_SPDIN;
	  sprintf (tmp, "%s S/PDIF in %s", devc->model_data->product, lr);
	  sprintf (id, "SPDIF%s", lr);
	  devfile_name = "spdin";
	  break;

	case 10:
	case 11:
	  sprintf (tmp, "%s input from mon. mixer %s",
		   devc->model_data->product, lr);
	  sprintf (id, "MON%s", lr);
	  devfile_name = "mon";
	  break;

	default:
	  if (devc->skipdevs > 1)
	    {
	      sprintf (tmp, "%s in%d/%d", devc->model_data->product, i + 1,
		       i + devc->skipdevs);
	      sprintf (id, "IN%d/%d", i + 1, i + devc->skipdevs);
	    }
	  else
	    {
	      sprintf (tmp, "%s in%d", devc->model_data->product, i + 1);
	      sprintf (id, "IN%d", i + 1);
	    }
	}

      if (mask & (1 << devc->curr_inch))
	install_adev (devc, tmp, ADEV_NOOUTPUT, devc->skipdevs, portc_flags,
		      id, devfile_name);
      else
	devc->curr_inch += devc->skipdevs;
    }

  OUTL (devc->osdev, 0x00224466, devc->mt_base + 0x34);	/* 1 to 1 input routing */

  return 1;
}

static int
install_audio_devices (envy24_devc * devc)
{
  extern int envy24_swapdevs;
  int maskout, maskin, i;
#define setmask(m, b) m|=(1<<(b))

  maskout = maskin = 0;

  if (envy24_devmask == 0)
    envy24_devmask = 65535;

  if (envy24_devmask & DMASK_MONITORIN)
    {
      setmask (maskin, 10);	/* Monitor input left */
      setmask (maskin, 11);	/* Monitor input right */
    }

  if (devc->model_data->flags & MF_SPDIF)
    {
      if (envy24_devmask & DMASK_SPDIFIN)
	{
	  setmask (maskin, 8);	/* S/PDIF left */
	  setmask (maskin, 9);	/* S/PDIF right */
	}

      if (envy24_devmask & DMASK_SPDIFOUT)
	{
	  setmask (maskout, 8);	/* S/PDIF left */
	  setmask (maskout, 9);	/* S/PDIF right */
	}
      if (devc->model_data->auxdrv->spdif_set)
	devc->model_data->auxdrv->spdif_set (devc, 0x20);
    }

  if (envy24_devmask & DMASK_ANALOGOUT)
    for (i = 0; i < devc->model_data->nr_outs; i++)
      setmask (maskout, i);

  if (envy24_devmask & DMASK_ANALOGIN)
    for (i = 0; i < devc->model_data->nr_ins; i++)
      setmask (maskin, i);

  devc->inportmask = maskin;
  devc->outportmask = maskout;

  if (envy24_swapdevs)
    {
      install_input_devices (devc, maskin);
      install_output_devices (devc, maskout);
      install_virtual_output_devices (devc, maskout);
    }
  else
    {
      install_output_devices (devc, maskout);
      install_input_devices (devc, maskin);
      install_virtual_output_devices (devc, maskout);
    }

  for (i = 0; i < 10; i += devc->skipdevs)
    {
      int num = 1 << i;
      if (devc->skipdevs == 2)
	num |= 1 << (i + 1);

      if (maskout & num)
	envy24_set_mon (devc->mixer_dev, num, SNDCTL_MIX_WRITE,
			1340 | (1340 << 16));
      if (maskin & num)
	envy24_set_mon (devc->mixer_dev, 0x80000000 | num, SNDCTL_MIX_WRITE,
			1340 | (1340 << 16));
    }

  if (envy24_devmask & DMASK_RAWDEVS)
    envy24d_install (devc);
  return 1;
}

static int
ac97_read (void *devc_, int reg)
{
  envy24_devc *devc = devc_;
  int i, status;

  OUTB (devc->osdev, reg, devc->ccs_base + 0x08);
  OUTB (devc->osdev, reg, devc->ccs_base + 0x08);
  OUTB (devc->osdev, reg, devc->ccs_base + 0x08);
  OUTB (devc->osdev, 0x10, devc->ccs_base + 0x09);

  for (i = 0; i < 1000; i++)
    {
      status = INB (devc->osdev, devc->ccs_base + 0x09);
      if (!(status & 0x10))
	{
	  status = INW (devc->osdev, devc->ccs_base + 0x0a);
	  return status;
	}
    }

  return 0xffff;
}

static int
ac97_writereg (void *devc_, int reg, int data)
{
  envy24_devc *devc = devc_;
  int i, status;

  OUTB (devc->osdev, reg, devc->ccs_base + 0x08);
  OUTB (devc->osdev, reg, devc->ccs_base + 0x08);
  OUTB (devc->osdev, reg, devc->ccs_base + 0x08);
  OUTB (devc->osdev, 0x20, devc->ccs_base + 0x09);

  for (i = 0; i < 1000; i++)
    {
      status = INB (devc->osdev, devc->ccs_base + 0x09);
      if (!(status & 0x20))
	{
	  OUTW (devc->osdev, data & 0xffff, devc->ccs_base + 0x0a);
	  return 1;
	}
    }

  return 0;
}

static int
ac97_write (void *devc, int reg, int data)
{
  int ret;

  ac97_writereg (devc, reg, data);
  ac97_writereg (devc, reg, data);
  ret = ac97_writereg (devc, reg, data);
  return ret;
}

static void
install_consumer_devices (envy24_devc * devc)
{
#if 1
  int i, status;

  OUTB (devc->osdev, 0x80, devc->ccs_base + 0x09);	/* Cold reset mixer */
  oss_udelay (200);
  OUTB (devc->osdev, 0x00, devc->ccs_base + 0x09);	/* Release reset */
  oss_udelay (200);

  for (i = 0; i < 1000; i++)
    {
      status = INB (devc->osdev, devc->ccs_base + 0x09);

      if (status & 0x80)
	break;

      oss_udelay (1000);
    }

  if (i >= 1000)
    {
    }
#endif

  devc->consumer_mixer_dev =
    ac97_install (&devc->ac97devc, "Envy24 consumer mixer", ac97_read,
		  ac97_write, devc, devc->osdev);

  /* Route monitor output to consumer AC97 */
  OUTB (devc->osdev, 0x01, devc->mt_base + 0x3c);

  /* Set consumer volumes to full */
  envy24_write_cci (devc, 3, 0);
  envy24_write_cci (devc, 4, 0);
}

static int
maudio_load_eeprom (envy24_devc * devc)
{
  int status;

  status = INB (devc->osdev, devc->ccs_base + 0x13);

  if (!(status & 0x80))
    return 0;			/* No EEPROM */

  envy24_write_cci (devc, 0x22, devc->eeprom[0xc]);	/* GPIO direction */
  envy24_write_cci (devc, 0x21, devc->eeprom[0xa]);	/* GPIO write mask */
  envy24_write_cci (devc, 0x20, devc->eeprom[0xb]);	/* GPIO data */

  return 1;
}


static int
envy24_init (envy24_devc * devc)
{
  extern int envy24_nfrags;
  oss_native_word phaddr;
  int err;

  /* Disable all interrupts */
  OUTB (devc->osdev, 0xff, devc->ccs_base + 0x01);
  OUTB (devc->osdev, 0xff, devc->mt_base + 0x00);

  if ((err = oss_register_interrupts (devc->osdev, 0, envy24intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  if (envy24_skipdevs < 1)
    envy24_skipdevs = 1;
  if (envy24_skipdevs > 2)
    envy24_skipdevs = 2;

  devc->skipdevs = envy24_skipdevs;

  if (envy24_skipdevs != 1)
    envy24_force_mono = 0;

  if (devc->model_data->flags & MF_MIDI1)
    {
      char name[128];
      oss_native_word flags;
      sprintf (name, "%s #1", devc->model_data->product);

      MUTEX_ENTER (devc->mutex, flags);
      uart401_init (&devc->uart401devc1, devc->osdev, devc->ccs_base + 0x0c,
		    name);
      MUTEX_EXIT (devc->mutex, flags);

      /* Enable UART1 interrupts */
      OUTB (devc->osdev,
	    INB (devc->osdev, devc->ccs_base + 0x01) & ~0x80,
	    devc->ccs_base + 0x01);
    }

  if (devc->model_data->flags & MF_MIDI2)
    {
      char name[128];
      oss_native_word flags;
      sprintf (name, "%s #2", devc->model_data->product);

      MUTEX_ENTER (devc->mutex, flags);
      uart401_init (&devc->uart401devc2, devc->osdev, devc->ccs_base + 0x1c,
		    name);
      MUTEX_EXIT (devc->mutex, flags);
      /* Enable UART2 interrupts */
      OUTB (devc->osdev,
	    INB (devc->osdev, devc->ccs_base + 0x01) & ~0x20,
	    devc->ccs_base + 0x01);
    }

  devc->speedbits = 0;
  devc->speed = 48000;
  devc->pending_speed_sel = 9;

  if (devc->model_data->flags & (MF_MEEPROM))
    maudio_load_eeprom (devc);

  if (devc->model_data->auxdrv->card_init)
    devc->model_data->auxdrv->card_init (devc);
  if (devc->model_data->auxdrv->spdif_init)
    devc->model_data->auxdrv->spdif_init (devc);

  if ((devc->mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
					    devc->osdev,
					    devc->osdev,
					    devc->model_data->product,
					    &envy24_mixer_driver,
					    sizeof (mixer_driver_t),
					    devc)) >= 0)
    {
      int n = 50;

      if (devc->skipdevs == 1)
	n += 30;
      mixer_ext_set_init_fn (devc->mixer_dev, envy24_mix_init, n);
    }

  if (envy24_nfrags != 2 && envy24_nfrags != 4 && envy24_nfrags != 8 &&
      envy24_nfrags != 16 && envy24_nfrags != 32 && envy24_nfrags != 64)
    envy24_nfrags = 16;

  devc->playbuffsize = HW_PLAYBUFFSIZE;
  devc->recbuffsize = HW_RECBUFFSIZE;

  devc->hw_nfrags = envy24_nfrags;
  devc->hw_pfragsize = devc->playbuffsize / devc->hw_nfrags;
  devc->hw_rfragsize = devc->recbuffsize / devc->hw_nfrags;
  devc->hw_fragsamples = devc->hw_pfragsize / 40;	/* # of 32 bit samples/fragment/channel */

  if (devc->hw_pfragsize % 40)
    cmn_err (CE_WARN, "Error! Bad per channel fragment size\n");
  devc->hw_playfrag = 0;
  devc->hw_recfrag = 0;

  devc->playbuf =
    CONTIG_MALLOC (devc->osdev, HW_ALLOCSIZE, MEMLIMIT_28BITS, &phaddr, devc->playbuf_dma_handle);
  if (devc->playbuf == NULL)
    {
      cmn_err (CE_WARN, "Failed to allocate %d bytes of DMA buffer\n",
	       HW_ALLOCSIZE);
      return 0;
    }

  devc->playbuf_phys = phaddr;
  if ((devc->playbuf_phys + HW_ALLOCSIZE) >= (256 * 1024 * 1024))
    {
      cmn_err (CE_WARN, "Got DMA buffer beyond address 256M.\n");
      cmn_err (CE_CONT, "Reboot and try again\n");
      return 1;
    }

  OUTL (devc->osdev, devc->playbuf_phys, devc->mt_base + 0x10);	/* Play base */

  devc->recbuf =
    CONTIG_MALLOC (devc->osdev, HW_ALLOCSIZE, MEMLIMIT_28BITS, &phaddr, devc->recbuf_dma_handle);
  if (devc->recbuf == NULL)
    {
      cmn_err (CE_WARN, "Failed to allocate %d bytes of DMA buffer\n",
	       HW_ALLOCSIZE);
      return 0;
    }

  devc->recbuf_phys = phaddr;
  if ((devc->recbuf_phys + HW_ALLOCSIZE) >= (256 * 1024 * 1024))
    {
      cmn_err (CE_WARN, "Got DMA buffer beyond address 256M.\n");
      cmn_err (CE_CONT, "Reboot and try again\n");
      return 1;
    }

  OUTL (devc->osdev, devc->recbuf_phys, devc->mt_base + 0x20);	/* Rec base */

  devc->playback_started = 0;
  devc->recording_started = 0;
  devc->playback_prepared = 0;
  devc->recording_prepared = 0;
  devc->writeahead = 1;
#ifdef __VXWORKS__
  devc->ratelock = 0;
#else
  devc->ratelock = 1;
#endif
  memset (devc->playbuf, 0, HW_ALLOCSIZE);
  memset (devc->recbuf, 0, HW_ALLOCSIZE);

  install_audio_devices (devc);

  if (devc->consumer_ac97_present || (devc->model_data->flags & MF_CONSUMER))
    install_consumer_devices (devc);
  /* Enable professional rec/play interrupts */
  OUTB (devc->osdev,
	INB (devc->osdev, devc->ccs_base + 0x01) & ~0x10,
	devc->ccs_base + 0x01);

  setup_spdif_control (devc);

  if (devc->model_data->auxdrv->spdif_set)
    devc->model_data->auxdrv->spdif_set (devc, 0x20);

#if 1
  /* Make sure everything is initialized */
  envy24_prepare_play_engine (devc);
  envy24_launch_play_engine (devc);
  oss_udelay (10000);
  envy24_stop_playback (devc);
#endif

#if 0
  {
    char line[200], *s = line;
    *line = 0;

    for (i = 0; i < 0x20; i++)
      {
	if (!(i % 16))
	  {
	    if (*line != 0)
	      cmn_err (CE_CONT, "%s\n", line);

	    s = line;
	    sprintf (s, "CCS%02x: ", i);
	    s = line + strlen (line);
	  }
	sprintf (s, "%02x ", INB (devc->osdev, devc->ccs_base + i));
	s = line + strlen (line);
      }
    *line = 0;

    for (i = 0; i < 0x40; i++)
      {
	if (!(i % 16))
	  {
	    if (*line != 0)
	      cmn_err (CE_CONT, "%s\n", line);

	    s = line;
	    sprintf (s, "MT%02x: ", i);
	    s = line + strlen (line);
	  }
	sprintf (s, "%02x ", INB (devc->osdev, devc->mt_base + i));
	s = line + strlen (line);
      }

    cmn_err (CE_CONT, "%s\n", line);
  }
#endif

  return 1;
}

int
oss_envy24_attach (oss_device_t * osdev)
{
  envy24_devc *devc;
  static int status;
  unsigned char pci_irq_line;
  unsigned short pci_command, vendor, device;
  unsigned int subvendor;
  unsigned int pci_ioaddr, pci_ioaddr3;
  int i;

  char *name = "Generic ENVY24";

  DDB (cmn_err (CE_CONT, "Entered Envy24 probe routine\n"));

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != ICENSEMBLE_VENDOR_ID || device != ICENSEMBLE_ENVY24_ID)
    return 0;

  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);
  DDB (cmn_err (CE_CONT, "Device found at I/O %x\n", pci_ioaddr));
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_3, &pci_ioaddr3);

  devc->active_inputs = 0;
  devc->active_outputs = 0;
  devc->sync_locked = 1;
  devc->first_dev = -1;

  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, 0x2c, &subvendor);
  devc->ccs_base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr) & ~0x3;
  DDB (cmn_err (CE_CONT, "CCS base %x/%lx\n", pci_ioaddr, devc->ccs_base));

  devc->mt_base = MAP_PCI_IOADDR (devc->osdev, 3, pci_ioaddr3) & ~0x3;
  DDB (cmn_err (CE_CONT, "MT base %x/%lx\n", pci_ioaddr3, devc->mt_base));

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);


  /* Reset the chip */
  OUTB (devc->osdev, 0x81, devc->ccs_base + 0x00);
  oss_udelay (200);
  /* Release reset */
  OUTB (devc->osdev, 0x01, devc->ccs_base + 0x00);
  oss_udelay (200);

  devc->nr_outdevs = devc->nr_indevs = 0;
  devc->curr_outch = devc->curr_inch = 0;
  devc->playbuffsize = 0;
  devc->recbuffsize = 0;
  devc->playbuf = devc->recbuf = NULL;

  status = INB (devc->osdev, devc->ccs_base + 0x13);
  if (status & 0x80)		/* EEPROM present */
    {
      static char resol_tab[] = { 16, 18, 20, 24 };
      unsigned char tmpbyte;

      load_eeprom (devc, subvendor);

      /* Fix bit 0x80 of EEPROM location 0x07. */
      pci_read_config_byte (osdev, 0x61, &tmpbyte);
      tmpbyte &= 0x80;
      devc->eeprom[0x07] &= ~0x80;
      devc->eeprom[0x07] |= tmpbyte;

#if 1
      devc->eeprom[0x07] |= 0x80;
#endif

      pci_write_config_byte (osdev, 0x60, devc->eeprom[0x06]);
      pci_write_config_byte (osdev, 0x61, devc->eeprom[0x07]);
      pci_write_config_byte (osdev, 0x62, devc->eeprom[0x08]);
      pci_write_config_byte (osdev, 0x63, devc->eeprom[0x09]);

#if 1
      if (devc->eeprom[0x06] & 0x20)
	DDB (cmn_err (CE_CONT, "Two MPU401 UARTs present.\n"));
      if (devc->eeprom[0x06] & 0x10)
	{
	  DDB (cmn_err (CE_CONT, "Consumer AC97 not present.\n"));
	}
      else
	{
	  DDB (cmn_err (CE_CONT, "Consumer AC97 present.\n"));
	}
      DDB (cmn_err (CE_CONT, "%d stereo ADC(s) available\n",
		    ((devc->eeprom[0x06] >> 2) & 0x03) + 1));
      DDB (cmn_err (CE_CONT, "%d stereo DAC(s) available\n",
		    ((devc->eeprom[0x06] >> 0) & 0x03) + 1));

      DDB (cmn_err (CE_CONT, "MT converter type %s\n",
		    (devc->eeprom[0x07] & 0x80) ? "I2S" : "AC97"));

      if (devc->eeprom[0x08] & 0x80)
	{
	  DDB (cmn_err (CE_CONT, "Has I2S volume control and mute\n"));
	}
      else
	{
	  DDB (cmn_err (CE_CONT, "No I2S volume control and mute\n"));
	}
      if (devc->eeprom[0x08] & 0x20)
	DDB (cmn_err (CE_CONT, "Has 96kHz support\n"));

      DDB (cmn_err (CE_CONT, "Converter resolution %d bits\n",
		    resol_tab[(devc->eeprom[0x08] >> 4) & 0x03]));

      if (devc->eeprom[0x09] & 0x02)
	DDB (cmn_err (CE_CONT, "Has S/PDIF in support\n"));
      if (devc->eeprom[0x09] & 0x01)
	DDB (cmn_err (CE_CONT, "Has S/PDIF out support\n"));

#endif

      if (subvendor == 0xd6301412)	/* Delta 1010 */
      if (devc->eeprom[0xc] == 0x7b)	/* Looks like Delta 1010 rev E */
	 subvendor = 0xd63014ff;	/* Delta 1010E */


#if 1
      if (!(devc->eeprom[0x07] & 0x80))
	{
	  cmn_err (CE_WARN, "Cards with AC97 codecs are not supported\n");
	  return 0;
	}
#endif
    }

  i = 0;
  while (models[i].svid != 0)
    {
      if (models[i].svid == subvendor)
	{
	  name = models[i].product;
	  devc->model_data = &models[i];
	  if (devc->model_data->auxdrv == NULL)
	    devc->model_data->auxdrv = &default_auxdrv;
	  DDB (cmn_err (CE_CONT, "Card id '%s'\n", name));

	  break;
	}

      i++;
    }

  if (devc->model_data->flags & MF_AC97)
    devc->consumer_ac97_present = 1;

  if (models[i].svid == 0)
    {
      cmn_err (CE_NOTE, "Unknown device ID (%08x).\n", subvendor);
      cmn_err (CE_NOTE, "This card is not supported (yet).\n");
      return 0;
    }

  MUTEX_INIT (osdev, devc->mutex, MH_DRV);
  oss_register_device (osdev, name);
  devc->irq = pci_irq_line;
  return envy24_init (devc);	/* Detected */
}


int
oss_envy24_detach (oss_device_t * osdev)
{
  envy24_devc *devc;

  devc = osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  /* Disable all interrupts */
  OUTB (devc->osdev, 0xff, devc->ccs_base + 0x01);

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

  if (devc->model_data->flags & MF_MIDI1)
    uart401_disable (&devc->uart401devc1);
  if (devc->model_data->flags & MF_MIDI2)
    uart401_disable (&devc->uart401devc2);

  oss_unregister_interrupts (osdev);
  MUTEX_CLEANUP (devc->mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);
  UNMAP_PCI_IOADDR (devc->osdev, 3);

  if (devc->playbuf != NULL)
    CONTIG_FREE (devc->osdev, devc->playbuf, HW_ALLOCSIZE, devc->playbuf_dma_handle);

  if (devc->recbuf != NULL)
    CONTIG_FREE (devc->osdev, devc->recbuf, HW_ALLOCSIZE, devc->recbuf_dma_handle);
  devc->playbuf = devc->recbuf = NULL;

  oss_unregister_device (osdev);
  return 1;
}
