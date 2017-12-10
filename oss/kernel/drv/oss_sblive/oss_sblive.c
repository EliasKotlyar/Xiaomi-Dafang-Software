/*
 * Purpose: Driver for Creative SB Live/Audigy/2/4. Audio, MIDI and mixer services.
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

#include "oss_sblive_cfg.h"
#include "midi_core.h"
#include "oss_pci.h"
#include "remux.h"

/*
 * Include the DSP files for emu10k1 (Live!) and emu10k2 (Audigy)
 * These headers have been generated using the emu10k tools (by 4Front).
 * The _be.h files have been generated in a big endian system and the others
 * in a litle endian (intel) system.
 */
#ifdef OSS_BIG_ENDIAN
#   include "emu10k1_dsp_be.h"
#   include "emu10k2_dsp_be.h"
#else
#   include "emu10k1_dsp.h"
#   include "emu10k2_dsp.h"
#endif

#define NO_EMU10K1_SYNTH
#undef  TEST_3D

#define BOGUS_MIXER_CONTROLS ( \
	SOUND_MASK_SPEAKER | \
	SOUND_MASK_ALTPCM | \
	SOUND_MASK_VIDEO | \
	SOUND_MASK_DEPTH | \
	SOUND_MASK_MONO \
	)

#ifndef linux
#define NO_EMU10K1_SYNTH
#endif

#include "ac97.h"
#include "sblive.h"
#include "eq1.h"

#define MAX_SENDS	4

#define SEND_L			0
#define SEND_R			1
#define SEND_SL			2
#define SEND_SR			3
#define SEND_C			4
#define SEND_W			5
#define SEND_RL			6
#define SEND_RR			7

#define SPDIF_L			20
#define SPDIF_R			21

static unsigned char default_routing[MAX_SENDS] =
  { SEND_L, SEND_R, SEND_SL, SEND_SR };
static unsigned char front_routing[MAX_SENDS] =
  { SEND_L, SEND_R, 0x3f, 0x3f };
static unsigned char surr_routing[MAX_SENDS] =
  { SEND_SL, SEND_SR, 0x3f, 0x3f };
static unsigned char center_lfe_routing[MAX_SENDS] =
  { SEND_C, SEND_W, 0x3f, 0x3f };
static unsigned char rear_routing[MAX_SENDS] =
  { SEND_RL, SEND_RR, 0x3f, 0x3f };
static unsigned char spdif_routing[MAX_SENDS] =
  { SPDIF_L, SPDIF_R, 0x3f, 0x3f };

typedef struct
{
  int speed;
  int pitch;
  int recbits;
  int audigy_recbits;
  int rom;
}
speed_ent;

/* Note! with audigy speedsel=7 means 12 kHz */

static speed_ent speed_tab[] = {
  {8000, 0xb6a41b, 7, 8, ROM7},
  {11025, 0xbe0b64, 6, 6, ROM6},
  {16000, 0xc6a41b, 5, 5, ROM5},
  {22050, 0xce0b64, 4, 3, ROM4},
  {24000, 0xd00000, 3, 3, ROM3},
  {32000, 0xd6a41b, 2, 2, ROM2},
  {44100, 0xde0b64, 1, 1, ROM1},
  {48000, 0xe00000, 0, 0, ROM0},
  {0}
};

#define PCI_VENDOR_ID_CREATIVE 		0x1102
#define PCI_DEVICE_ID_SBLIVE		0x0002
#define PCI_DEVICE_ID_AUDIGY		0x0004
#define PCI_DEVICE_ID_AUDIGYVALUE	0x0008
#define PCI_DEVICE_ID_AUDIGY_CARDBUS	0x2001

#define LEFT_CH		0
#define RIGHT_CH	1

void sblive_init_voice (sblive_devc * devc, int chn);
static void audigyuartintr (sblive_devc * devc);

static int
ac97_read (void *devc_, int wAddr)
{
  sblive_devc *devc = devc_;
  int dtemp = 0, i;

  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, wAddr, devc->base + 0x1e);
  for (i = 0; i < 10000; i++)
    if (INB (devc->osdev, devc->base + 0x1e) & 0x80)
      break;
  if (i == 1000)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;		/* Timeout */
    }
  dtemp = INW (devc->osdev, devc->base + 0x1c);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return dtemp & 0xffff;
}

static int
ac97_write (void *devc_, int wAddr, int wData)
{
  sblive_devc *devc = devc_;
  oss_native_word flags;
  int i;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, wAddr, devc->base + 0x1e);
  for (i = 0; i < 10000; i++)
    if (INB (devc->osdev, devc->base + 0x1e) & 0x80)
      break;
  OUTW (devc->osdev, wData, devc->base + 0x1c);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return 0;
}

unsigned int
sblive_read_reg (sblive_devc * devc, int reg, int chn)
{
  oss_native_word flags;
  unsigned int ptr, ptr_addr_mask, val, mask, size, offset;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  ptr_addr_mask = (devc->feature_mask & SB_AUDIGY) ? 0x0fff0000 : 0x07ff0000;
  ptr = ((reg << 16) & ptr_addr_mask) | (chn & 0x3f);
  OUTL (devc->osdev, ptr, devc->base + 0x00);	/* Pointer */
  val = INL (devc->osdev, devc->base + 0x04);	/* Data */
  if (reg & 0xff000000)
    {
      size = (reg >> 24) & 0x3f;
      offset = (reg >> 16) & 0x1f;
      mask = ((1 << size) - 1) << offset;
      val &= mask;
      val >>= offset;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return val;
}

void
sblive_write_reg (sblive_devc * devc, int reg, int chn, unsigned int value)
{
  oss_native_word flags;
  unsigned int ptr, ptr_addr_mask, mask, size, offset;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  ptr_addr_mask = (devc->feature_mask & SB_AUDIGY) ? 0x0fff0000 : 0x07ff0000;
  ptr = ((reg << 16) & ptr_addr_mask) | (chn & 0x3f);
  OUTL (devc->osdev, ptr, devc->base + 0x00);	/* Pointer */
  if (reg & 0xff000000)
    {
      size = (reg >> 24) & 0x3f;
      offset = (reg >> 16) & 0x1f;
      mask = ((1 << size) - 1) << offset;
      value <<= offset;
      value &= mask;
      value |= INL (devc->osdev, devc->base + 0x04) & ~mask;	/* data */
    }
  OUTL (devc->osdev, value, devc->base + 0x04);	/* Data */
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

static void
write_efx (sblive_devc * devc, int reg, unsigned int value)
{
  sblive_write_reg (devc, reg, 0, value);
}

static void
update_vu (sblive_devc * devc, sblive_portc * portc, dmap_p dmap, int frag)
{
  int left = 0, right = 0;
  int i, l;

  frag %= dmap->nfrags;
  l = dmap->fragment_size / 2;

  if (portc->format == AFMT_AC3)	/* Raw S/PDIF mode */
    {
      if (devc->vu_tmp2 == 0)
	devc->vu_tmp2 = 2;

      portc->vu_left = devc->vu_tmp;
      portc->vu_right = 144 - devc->vu_tmp;
      devc->vu_tmp = devc->vu_tmp + devc->vu_tmp2;

      if (devc->vu_tmp >= 144)
	{
	  devc->vu_tmp2 = -2;
	  devc->vu_tmp = 144;
	}

      if (devc->vu_tmp <= 0)
	{
	  devc->vu_tmp2 = 2;
	  devc->vu_tmp = 0;
	}

      return;
    }

  if (portc->format == AFMT_U8)
    {
      unsigned char *p;
      int v;

      p = dmap->dmabuf + (frag * dmap->fragment_size);

      if (dmap->dmabuf != NULL)
	for (i = 0; i < l; i++)
	  {
	    v = *p;
	    p++;
	    v -= 128;
	    if (v < 0)
	      v = -v;
	    if (v > left)
	      left = v;

	    v = *p;
	    p++;
	    v -= 128;
	    if (v < 0)
	      v = -v;
	    if (v > right)
	      right = v;
	  }
    }
  else
    {
      short *p;
      int v;

      l /= 2;
      p = (short *) (dmap->dmabuf + (frag * dmap->fragment_size));

      if (dmap->dmabuf != NULL)
	for (i = 0; i < l; i++)
	  {
	    v = SSWAP (*p++) >> 8;
	    if (v < 0)
	      v = -v;
	    if (v > left)
	      left = v;

	    v = *p++ >> 8;
	    if (v < 0)
	      v = -v;
	    if (v > right)
	      right = v;
	  }
    }

  if (portc->channels == 1)	/* Mono */
    {
      if (right > left)
	left = right;
      right = left;
    }

  if (left > portc->vu_left)
    portc->vu_left = left;
  if (right > portc->vu_right)
    portc->vu_right = right;
}

static int
sbliveintr (oss_device_t * osdev)
{
  int p;
  unsigned int status;
#ifndef NO_EMU10K1_SYNTH
  extern int sblive_synth_enable;
#endif

  sblive_devc *devc = osdev->devc;

  /*
   * TODO: Fix mutexes and move the inputintr/outputintr calls outside the
   * mutex block.
   */
  /* oss_native_word flags; */
  /* MUTEX_ENTER (devc->mutex, flags); */

  status = INL (devc->osdev, devc->base + 0x08);

#if !defined(sparc)
  if (status == 0)
    {
      /* MUTEX_EXIT (devc->mutex, flags); */
      return 0;
    }
#endif

  if (status & 0x00000080)	/* MIDI RX interrupt */
    {
      if (devc->feature_mask & SB_AUDIGY)
	audigyuartintr (devc);
      else
	uart401_irq (&devc->uart401devc);
    }

  if (status & 0x00008000)	/* ADC buffer full intr */
    {
      /* Force the starting position to match this moment */
      unsigned int pos;
      pos = (INL (devc->osdev, devc->base + 0x10) >> 6) & 0xfffff;
      devc->portc[0].rec_starttime = pos;
    }

  if (status & 0x00000240)	/* Interval timer or channel loop interrupt */
    {
      for (p = 0; p < devc->n_audiodevs; p++)
	{
	  sblive_portc *portc = &devc->portc[p];

	  if (portc->audiodev >= 0 && portc->audiodev < num_audio_engines
	      && portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    {
	      int pos, n;
	      dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;

	      pos = sblive_read_reg (devc, QKBCA, portc->voice_chn) & 0x00ffffff;	/* Curr pos */
	      pos <<= portc->out_sz;
	      pos -= dmap->driver_use_value;

	      if (dmap->fragment_size == 0)
		{
		  cmn_err (CE_WARN, "dmap->fragment_size == 0\n");
		  continue;
		}

	      pos /= dmap->fragment_size;
	      if (pos < 0 || pos >= dmap->nfrags)
		pos = 0;

	      /* 
	       * If this was a full/half loop interrupt then use forced pointer
	       */
	      if (sblive_get_voice_loopintr (devc, portc->voice_chn))
		pos = 0;	/* Full loop boundary */
	      else if (sblive_get_voice_halfloopintr (devc, portc->voice_chn))
		pos = dmap->nfrags / 2;	/* Half loop boundary */

	      n = 0;
	      while (dmap_get_qhead (dmap) != pos && n++ < dmap->nfrags)
		{
		  update_vu (devc, portc, dmap, dmap_get_qhead (dmap));
		  oss_audio_outputintr (portc->audiodev, 0);
		}
	    }

	  if (num_audio_engines > 0 && portc->audiodev < num_audio_engines
	      && portc->trigger_bits & PCM_ENABLE_INPUT)
	    {
	      int n;
	      unsigned int pos;

	      dmap_t *dmap = audio_engines[portc->audiodev]->dmap_in;

	      /* Compute current pos based on the wall clock register */
	      pos = (INL (devc->osdev, devc->base + 0x10) >> 6) & 0xfffff;
	      if (pos > portc->rec_starttime)
		pos = pos - portc->rec_starttime;
	      else
		pos = 0xfffff - (portc->rec_starttime - pos);
	      pos = (pos * (portc->speed / 25)) / (48000 / 25);
	      pos *= 2;		/* 16 bit->bytes */
	      pos *= portc->channels;
	      pos = pos % dmap->bytes_in_use;
	      if (dmap->fragment_size == 0)
		cmn_err (CE_WARN, "dmap->fragment_size==0\n");
	      else
		{
		  pos /= dmap->fragment_size;
		  if (pos >= dmap->nfrags)
		    pos = 0;
		  n = 0;
		  while (dmap_get_qtail (dmap) != pos && n++ < dmap->nfrags)
		    {
		      oss_audio_inputintr (devc->recording_dev, 0);
		    }
		}
	    }
	}

#ifndef NO_EMU10K1_SYNTH
      if (sblive_synth_enable)
	sblive_synth_interrupt (devc);
#endif
    }

  OUTL (devc->osdev, status, devc->base + 0x08);	/* Acknowledge them */

  /* MUTEX_EXIT (devc->mutex, flags); */

  return 1;
}

static int
setup_passthrough (sblive_devc * devc, sblive_portc * portc, int pass)
{
  int ctrl = devc->passthrough_gpr;

  if (ctrl < 0)
    return 0;

  if (pass == portc->uses_spdif)
    return 1;

  if (pass && devc->spdif_busy)
    return 0;

  portc->uses_spdif = pass;

  sblive_write_reg (devc, ctrl + GPR0, 0, pass);
  sblive_write_reg (devc, ctrl + GPR0 + 1, 0, !pass);
  if (pass)
    {
      devc->spdif_busy = 1;
    }
  else
    {
      devc->spdif_busy = 0;
    }

  return 1;
}

static int
sblive_audio_set_rate (int dev, int arg)
{
  sblive_portc *portc = audio_engines[dev]->portc;
  int i, n = -1, dif, best = -1, bestdif = 0x7fffffff;

  if (arg == 0)
    return portc->speed;

  for (i = 0; speed_tab[i].speed != 0 && n == -1; i++)
    {
      if (speed_tab[i].speed == arg)	/* Exact match */
	{
	  n = i;
	  break;
	}

      dif = arg - speed_tab[i].speed;
      if (dif < 0)
	dif = -dif;

      if (dif < bestdif)
	{
	  best = i;
	  bestdif = dif;
	}
    }

  if (n == -1)
    n = best;

  if (n == -1)
    n = 0;

  portc->speed = speed_tab[n].speed;
  portc->speedsel = n;

  return portc->speed;
}

static short
sblive_audio_set_channels (int dev, short arg)
{
  sblive_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
sblive_audio_set_format (int dev, unsigned int arg)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->format;

  if (portc->mode & OPEN_READ)
    arg = AFMT_S16_LE;

  if (arg != AFMT_U8 && arg != AFMT_S16_LE && arg != AFMT_AC3)
    return portc->format;

  /* Enforce stereo mode with AC3 */
  if (arg == AFMT_AC3)
    {
      if (!setup_passthrough (devc, portc, 1))
	return portc->format;
      portc->channels = 2;
      portc->speed = 48000;
    }
  else
    if (portc->input_type != ITYPE_SPDIF && portc->uses_spdif
	&& arg != AFMT_AC3)
    {
      setup_passthrough (devc, portc, 0);
    }

  portc->format = arg;

  return portc->format;
}

static int mixer_ext_init (int dev);
static int create_efx_mixer (int dev);
static int sblive_set_gpr (int dev, int ctrl, unsigned int cmd, int value);

static int
is_special_gpr (int gpr)
{
  if (gpr >= NEXT_FREE_GPR)
    return 0;

  if (SPECIAL_GPRS & (1 << gpr))
    {
      return 1;
    }

  if (gpr > 0)
    if (SPECIAL_GPRS & (1 << (gpr - 1)))
      {
	return 1;
      }
  return 0;
}

static void update_output_device (sblive_devc * devc, sblive_portc * portc);

static int
sblive_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;
  sblive_reg *reg = (sblive_reg *) arg;
  gpr_info *gpr = (gpr_info *) arg;
  const_info *consts = (const_info *) arg;
  unsigned int *code = (unsigned int *) arg;
  int i;

  switch (cmd)
    {
    case SNDCTL_DSP_GETCHANNELMASK:
      return *arg =
	DSP_BIND_FRONT | DSP_BIND_REAR | DSP_BIND_SURR | DSP_BIND_CENTER_LFE;
      break;

    case SNDCTL_DSP_BIND_CHANNEL:
      {
	int val;

	val = *arg;
	portc->speaker_mode = SMODE_BIND;
	portc->binding = val;
	return *arg = val;
      }
      break;

    case SNDCTL_DSP_SETPLAYVOL:
      {
	int left, right, val;

	val = *arg;

	left = val & 0xff;
	right = (val >> 8) & 0xff;
	if (left < 0)
	  left = 0;
	if (right < 0)
	  right = 0;
	if (left > 100)
	  left = 100;
	if (right > 100)
	  right = 100;

#if 0
	if (right > left)
	  left = right;
#endif
	portc->playvol = left;
	update_output_device (devc, portc);

	return *arg = left | (left << 8);
      }
      break;

    case SNDCTL_DSP_GETPLAYVOL:
      {
	int vol;
	vol = (portc->playvol << 8) | portc->playvol;
	return *arg = vol;
      }
      break;

    case SBLIVE_READREG:
#ifdef GET_PROCESS_UID
      if (GET_PROCESS_UID () != 0)	/* Not root */
	return OSS_EINVAL;
#endif
      reg->value = sblive_read_reg (devc, reg->reg, reg->chn);
      return 0;
      break;

    case SBLIVE_WRITEREG:
#ifdef GET_PROCESS_UID
      if (GET_PROCESS_UID () != 0)	/* Not root */
	return OSS_EINVAL;
#endif
      sblive_write_reg (devc, reg->reg, reg->chn, reg->value);
      return 0;
      break;

    case SBLIVE_READGPIO:
#ifdef GET_PROCESS_UID
      if (GET_PROCESS_UID () != 0)	/* Not root */
	return OSS_EINVAL;
#endif
      return *arg = INW (devc->osdev, devc->base + 0x18);
      break;

    case SBLIVE_WRITEGPIO:
      {
	int val;
	val = *arg;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	OUTW (devc->osdev, val, devc->base + 0x18);

      }
      return 0;

    case SBLIVE_WRITEPARMS:
      {

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	if (gpr->ngpr >= MAX_GPR_PARMS)
	  return OSS_EIO;

	for (i = 0; i < gpr->ngpr; i++)
	  {
	    gpr->gpr[i].name[GPR_NAME_SIZE - 1] = 0;	/* Overflow protection */
	    if (strlen (gpr->gpr[i].name) >= 32)	/* Name may be bad */
	      {
		return OSS_EIO;
	      }

	    if (gpr->gpr[i].num >= MAX_GPR)
	      {
		return OSS_EIO;
	      }

/* cmn_err(CE_CONT, "Gpr %d = %s (vol %x) type=%x\n", gpr->gpr[i].num, gpr->gpr[i].name, gpr->gpr[i].def, gpr->gpr[i].type); */
	    if (gpr->gpr[i].type != MIXT_GROUP)
	      {
		if (is_special_gpr (gpr->gpr[i].num))
		  sblive_set_gpr (devc->mixer_dev, gpr->gpr[i].num,
				  SNDCTL_MIX_WRITE,
				  devc->gpr_values[gpr->gpr[i].num]);
		else
		  sblive_set_gpr (devc->mixer_dev, gpr->gpr[i].num,
				  SNDCTL_MIX_WRITE, gpr->gpr[i].def);
	      }
	  }


	if (devc->gpr == NULL)
	  {
	    devc->gpr = PMALLOC (devc->osdev, sizeof (gpr_info));
	    if (devc->gpr == NULL)
	      {
		cmn_err (CE_WARN, "Out of memory (gpr)\n");
		return OSS_ENOSPC;
	      }
	    memset (devc->gpr, 0, sizeof (gpr_info));
	  }
	memcpy (devc->gpr, gpr, sizeof (gpr_info));
	create_efx_mixer (devc->mixer_dev);
      }
      return 0;
      break;

    case SBLIVE_WRITECONST:
      {

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	if (consts->nconst >= MAX_CONST_PARMS)
	  return OSS_EIO;

	for (i = 0; i < consts->nconst; i++)
	  {
	    if (consts->consts[i].gpr >= MAX_GPR)
	      {
		return OSS_EIO;
	      }

	    sblive_write_reg (devc, consts->consts[i].gpr + GPR0, 0,
			      consts->consts[i].value);
	  }

      }
      return 0;
      break;

    case SBLIVE_WRITECODE1:
      {
	int pc;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	for (pc = 0; pc < 512; pc++)
	  {
	    write_efx (devc, UC0 + pc, code[pc]);
	  }

      }
      sblive_write_reg (devc, DBG, 0, 0);
      return 0;
      break;

    case SBLIVE_WRITECODE2:
      {
	int pc;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	for (pc = 0; pc < 512; pc++)
	  {
	    write_efx (devc, UC0 + 512 + pc, code[pc]);
	  }
      }
      sblive_write_reg (devc, DBG, 0, 0);
      return 0;
      break;

    case SBLIVE_GETCHIPTYPE:
#ifdef GET_PROCESS_UID
      if (GET_PROCESS_UID () != 0)	/* Not root */
	return OSS_EINVAL;
#endif
      return *arg = devc->feature_mask & ~SB_AUDIGY2;
      break;

    }
  return OSS_EINVAL;
}

static void sblive_audio_trigger (int dev, int state);

static void
sblive_audio_reset (int dev)
{
  sblive_audio_trigger (dev, 0);
}

static void
sblive_audio_reset_input (int dev)
{
  sblive_portc *portc = audio_engines[dev]->portc;
  sblive_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
sblive_audio_reset_output (int dev)
{
  sblive_portc *portc = audio_engines[dev]->portc;
  sblive_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

static int sblive_set_vol (int dev, int ctrl, unsigned int cmd, int value);

static void
reset_portc_volume (sblive_devc * devc, sblive_portc * portc)
{
  int v;
#ifdef TEST_3D
  v = 100 | (50 << 8) | (0 << 16);	/* vol=100, dist=50, angle=0 */
#else
  v = 100 | (100 < 8);
#endif
  sblive_set_vol (devc->mixer_dev, portc->port_number, SNDCTL_MIX_WRITE, v);
}

#if MAX_ADEV == 2
static const unsigned int binding_map[MAX_ADEV] =
  { DSP_BIND_FRONT, DSP_BIND_SURR };
#else
static const unsigned int binding_map[MAX_ADEV] = {
  DSP_BIND_FRONT,
  DSP_BIND_FRONT,
  DSP_BIND_SURR,
  DSP_BIND_CENTER_LFE,
  DSP_BIND_REAR
};
#endif

/*ARGSUSED*/
static int
sblive_audio_open (int dev, int mode, int open_flags)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->mode = mode;
  portc->audio_active &= ~mode;

  portc->resetvol = 0;

  portc->speaker_mode = devc->speaker_mode;
  portc->binding = binding_map[portc->port_number];
  if (portc->binding == 0)
    portc->binding = DSP_BIND_FRONT;
  mixer_devs[devc->mixer_dev]->modify_counter++;	/* Force update of mixer */

  audio_engines[dev]->flags &= ~ADEV_FIXEDRATE;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (devc->autoreset || portc->resetvol)
    reset_portc_volume (devc, portc);

  if (portc->input_type == ITYPE_SPDIF)
    {
      if (!setup_passthrough (devc, portc, 1))
	{
	  portc->mode = 0;
	  return OSS_EBUSY;
	}
    }
  else
    {
      /* Enable AC3 format if possible */
      if (!devc->spdif_busy)
	audio_engines[dev]->oformat_mask |= AFMT_AC3;
      else
	audio_engines[dev]->oformat_mask &= ~AFMT_AC3;
    }
  return 0;
}

static void
sblive_audio_close (int dev, int mode)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;

  sblive_audio_reset (dev);
  portc->mode = 0;
  portc->audio_active &= ~mode;
  mixer_devs[devc->mixer_dev]->modify_counter++;
  setup_passthrough (devc, portc, 0);
  audio_engines[dev]->oformat_mask |= AFMT_AC3;
}

#ifdef TEST_3D
/* Sin table for Q1 (*10000) */
static const int sincos[] = {
  0, 174, 348, 523, 697, 871, 1045, 1218, 1391, 1564,
  1736, 1908, 2079, 2249, 2419, 2588, 2756, 2923, 3090, 3255,
  3420, 3583, 3746, 3907, 4067, 4226, 4383, 4539, 4694, 4848,
  5000, 5150, 5299, 5446, 5591, 5735, 5877, 6018, 6156, 6293,
  6427, 6560, 6691, 6819, 6946, 7071, 7193, 7313, 7431, 7547,
  7660, 7771, 7880, 7986, 8090, 8191, 8290, 8386, 8480, 8571,
  8660, 8746, 8829, 8910, 8987, 9063, 9135, 9205, 9271, 9335,
  9396, 9455, 9510, 9563, 9612, 9659, 9702, 9743, 9781, 9816,
  9848, 9876, 9902, 9925, 9945, 9961, 9975, 9986, 9993, 9998,
  10000,
};

static __inline__ int
oss_sin (int angle)
{
  int a;
  int f;

  a = angle % 90;

  if ((angle / 90) & 1)
    a = 90 - a;

  f = sincos[a];
  if (angle >= 180)
    f = -f;
  return f;
}

static __inline__ int
oss_cos (int angle)
{
  int a, q;
  int f;

  a = angle % 90;
  q = angle / 90;

  if (!(q & 1))
    a = 90 - a;

  f = sincos[a];
  if (angle >= 90 && angle < 270)
    f = -f;
  return f;
}

static void
compute_3d (sblive_devc * devc, sblive_portc * portc, int voice, int chn,
	    int *send)
{
  int angle = portc->playangle;
  int dist, opening = 45;
  int i;

  /* left, right, rear_right, rear_left */
  static int spk_angles[4] = { 315, 45, 135, 225 };
  int gain = 50, leak = 0;
  int v[4];

  dist = portc->playdist;

  if (dist < 0)
    dist = 0;
  if (dist > 100)
    dist = 100;
  portc->playdist = dist;

  dist = 100 - dist;		/* Invert distance */
  opening = (90 * dist) / 100;

  if (dist < 50)
    {				/* Attenuate distant sounds */
      gain = dist;
    }
  else
    {
      /* "Expand" close sounds by leaking signal to silent channels */
      leak = dist - 50;
    }

  if (portc->channels == 2)
    {
      if (chn == LEFT_CH)
	angle -= opening;
      else
	angle += opening;
    }

  if (angle < 0)
    angle += 360;

  angle %= 360;

  for (i = 0; i < 4; i++)
    v[i] = (gain * portc->playvol * 255 + 25) / 50;

  for (i = 0; i < 4; i++)
    {
      int a = spk_angles[i] - angle;

      if (a < 0)
	a = -a;			/* ABS */
      if (a > 180)
	a = 360 - a;

      if (a >= 90)		/* Too far */
	{
	  v[i] = 0;		/* Muted speaker */
	  continue;
	}
      else
	v[i] = ((v[i] * oss_cos (a) + 5000) / 10000) / 100;
    }

  if (leak > 0)
    {
      leak = (255 * portc->playvol * leak + 2500) / 5000;

      for (i = 0; i < 4; i++)
	if (v[i] < leak)
	  v[i] = leak;
    }

  send[0] = v[0];		/* Left */
  send[1] = v[1];		/* Right */
  send[2] = v[3];		/* Rear left */
  send[3] = v[2];		/* Rear right */
}
#endif

static void
write_routing (sblive_devc * devc, int voice, unsigned char *routing)
{
  int i;

  if (routing == NULL)
    routing = default_routing;

  if (devc->feature_mask & SB_AUDIGY)
    {
      unsigned int srda = 0;

      for (i = 0; i < 4; i++)
	srda |= routing[i] << (i * 8);

      sblive_write_reg (devc, SRDA, voice, srda);
    }
  else
    {
      int fxrt = 0;

      for (i = 0; i < 4; i++)
	fxrt |= routing[i] << ((i * 4) + 16);
      sblive_write_reg (devc, FXRT, voice, fxrt);
    }
}

/*ARGSUSED*/
static void
compute_bind (sblive_devc * devc, sblive_portc * portc, unsigned char *send,
	      int chn)
{
  memset (send, 0, MAX_SENDS);

  if (chn == LEFT_CH)
    send[0] = (0xff * portc->playvol + 50) / 100;
  else
    send[1] = (0xff * portc->playvol + 50) / 100;

  switch (portc->binding)
    {
    case DSP_BIND_FRONT:
      portc->routing = front_routing;
      break;
    case DSP_BIND_SURR:
      portc->routing = surr_routing;
      break;
    case DSP_BIND_CENTER_LFE:
      portc->routing = center_lfe_routing;
      break;
    case DSP_BIND_REAR:
      portc->routing = rear_routing;
      break;
    default:
      portc->routing = default_routing;
    }
}

static void
update_output_volume (int dev, int voice, int chn)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  int left, right;
  unsigned int loop_start, loop_end, tmp;
  unsigned char send[MAX_SENDS];

  send[0] = 0xff;		/* Max */
  send[1] = 0xff;		/* Max */
  send[2] = 0xff;		/* Max */
  send[3] = 0xff;		/* Max */

  if (portc->input_type == ITYPE_SPDIF || portc->format == AFMT_AC3)
    {
      /* Digital voice */
      send[2] = 0;		/* Muted */
      send[3] = 0;		/* Muted */

      /* sends are revered between Audigy2 and Audigy */
      left = (devc->feature_mask & SB_AUDIGY2) ? 1 : 0;
      right = !left;

      if (portc->channels > 1)
	{
	  if (chn == LEFT_CH)
	    {
	      send[left] = 0;
	    }
	  else
	    {
	      send[right] = 0;
	    }
	}
    }
  else
    {
      /* Analog voice */

      if (portc->channels > 1)
	{
	  if (chn == LEFT_CH)
	    {
	      send[1] = 0;
	    }
	  else
	    {
	      send[0] = 0;
	    }
	}
      send[2] = send[0];
      send[3] = send[1];

#ifdef TEST_3D
      if (portc->speaker_mode == SMODE_3D)
	compute_3d (devc, portc, voice, chn, send);
      else
#endif
	{
	  send[0] = (send[0] * portc->playvol + 50) / 100;
	  send[1] = (send[1] * portc->playvol + 50) / 100;
	  send[2] = (send[2] * portc->playvol + 50) / 100;
	  send[3] = (send[3] * portc->playvol + 50) / 100;

	  switch (portc->speaker_mode)
	    {
	    case SMODE_FRONT:
	      send[2] = send[3] = 0;
	      break;

	    case SMODE_SURR:
	      send[0] = send[1] = 0;
	      break;

	    case SMODE_FRONTREAR:
	      break;

	    case SMODE_BIND:
	      compute_bind (devc, portc, send, chn);
	      break;
	    }
	}

      /* Analog voice */
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  loop_end = sblive_read_reg (devc, SDL, voice) & 0xffffff;
  sblive_write_reg (devc, SDL, voice, loop_end | (send[3] << 24));
  loop_start = sblive_read_reg (devc, SCSA, voice) & 0xffffff;
  sblive_write_reg (devc, SCSA, voice, loop_start | (send[2] << 24));
  tmp = sblive_read_reg (devc, PTAB, voice) & 0xffff0000;
  sblive_write_reg (devc, PTAB, voice, tmp | (send[0] << 8) | send[1]);
  write_routing (devc, voice, portc->routing);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
setup_audio_voice (int dev, int voice, int chn)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  unsigned int nCRA = 0;

  unsigned int loop_start, loop_end;

  int sz = 1;
  int start_pos;

  sblive_write_reg (devc, VEDS, voice, 0x0);	/* OFF */
  sblive_write_reg (devc, VTFT, voice, 0xffff);
  sblive_write_reg (devc, CVCF, voice, 0xffff);

  if (devc->feature_mask & SB_AUDIGY)
    sblive_write_reg (devc, SRDA, voice, 0x03020100);
  else
    sblive_write_reg (devc, FXRT, voice, 0x32100000);


  sz =
    (((portc->format == AFMT_S16_LE
       || portc->format == AFMT_AC3)) ? 1 : 0) + ((portc->channels ==
						   2) ? 1 : 0);

  loop_start = dmap->driver_use_value >> sz;
  loop_end = (dmap->driver_use_value + dmap->bytes_in_use) >> sz;

  /* set mono/stereo */
  sblive_write_reg (devc, CPF, voice, (portc->channels > 1) ? 0x8000 : 0);

  nCRA = (portc->channels > 1) ? 28 : 30;
  nCRA *= (portc->format == AFMT_S16_LE || portc->format == AFMT_AC3) ? 1 : 2;
  start_pos = loop_start + nCRA;

  /* SDL, ST, CA */
  portc->out_sz = sz;

  sblive_write_reg (devc, SDL, voice, loop_end);
  sblive_write_reg (devc, SCSA, voice, loop_start);
  sblive_write_reg (devc, PTAB, voice, 0);

  update_output_volume (dev, voice, chn);	/* Set volume */

  if (portc->format == AFMT_S16_LE || portc->format == AFMT_AC3)
    sblive_write_reg (devc, QKBCA, voice, start_pos);
  else
    sblive_write_reg (devc, QKBCA, voice, start_pos | BYTESIZE);

  sblive_write_reg (devc, Z1, voice, 0);
  sblive_write_reg (devc, Z2, voice, 0);

  sblive_write_reg (devc, MAPA, voice, 0x1fff | (devc->silent_page_phys << 1));	/* This is really a physical address */
  sblive_write_reg (devc, MAPB, voice, 0x1fff | (devc->silent_page_phys << 1));	/* This is really a physical address */

  sblive_write_reg (devc, VTFT, voice, 0x0000ffff);
  sblive_write_reg (devc, CVCF, voice, 0x0000ffff);
  sblive_write_reg (devc, MEHA, voice, 0);
  sblive_write_reg (devc, MEDS, voice, 0x7f);
  sblive_write_reg (devc, MLV, voice, 0x8000);
  sblive_write_reg (devc, VLV, voice, 0x8000);
  sblive_write_reg (devc, VFM, voice, 0);
  sblive_write_reg (devc, TMFQ, voice, 0);
  sblive_write_reg (devc, VVFQ, voice, 0);
  sblive_write_reg (devc, MEV, voice, 0x8000);
  sblive_write_reg (devc, VEHA, voice, 0x7f7f);	/* OK */
  sblive_write_reg (devc, VEV, voice, 0x8000);	/* No volume envelope delay (OK) */
  sblive_write_reg (devc, PEFE_FILTERAMOUNT, voice, 0x7f);
  sblive_write_reg (devc, PEFE_PITCHAMOUNT, voice, 0x00);
}

/*ARGSUSED*/
static void
update_output_device (sblive_devc * devc, sblive_portc * portc)
{
  int voiceL = portc->voice_chn, voiceR = portc->voice_chn + 1;

  if (!(portc->audio_active & PCM_ENABLE_OUTPUT))
    return;

  update_output_volume (portc->audiodev, voiceL, LEFT_CH);
  update_output_volume (portc->audiodev, voiceR, RIGHT_CH);
}


/*ARGSUSED*/
static void
sblive_audio_output_block (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  sblive_portc *portc = audio_engines[dev]->portc;

  portc->audio_active |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
sblive_audio_start_input (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  sblive_portc *portc = audio_engines[dev]->portc;

  portc->audio_active |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

void
sblive_set_loop_stop (sblive_devc * devc, int voice, int s)
{
  unsigned int tmp;
  int offs, bit;

  offs = voice / 32;
  bit = voice % 32;
  s = !!s;

  tmp = sblive_read_reg (devc, SOLL + offs, 0);
  tmp &= ~(1 << bit);

  if (s)
    tmp |= (1 << bit);
  sblive_write_reg (devc, SOLL + offs, 0, tmp);
}

int
sblive_get_voice_loopintr (sblive_devc * devc, int voice)
{
  unsigned int tmp;
  int offs, bit;

  offs = voice / 32;
  bit = voice % 32;

  tmp = sblive_read_reg (devc, CLIPL + offs, 0);
  tmp &= 1 << bit;
  sblive_write_reg (devc, CLIPL + offs, 0, tmp);	/* Ack the interrupt */

  return !!tmp;
}

int
sblive_get_voice_halfloopintr (sblive_devc * devc, int voice)
{
  unsigned int tmp;
  int offs, bit;

  offs = voice / 32;
  bit = voice % 32;

  tmp = sblive_read_reg (devc, HLIPL + offs, 0);
  tmp &= 1 << bit;
  sblive_write_reg (devc, HLIPL + offs, 0, tmp);	/* Ack the interrupt */

  return !!tmp;
}

void
sblive_set_voice_intr (sblive_devc * devc, int voice, int s)
{
  unsigned int tmp;
  int offs, bit;

  offs = voice / 32;
  bit = voice % 32;
  s = !!s;

  tmp = sblive_read_reg (devc, CLIEL + offs, 0);
  tmp &= ~(1 << bit);
  if (s)
    tmp |= (1 << bit);
  sblive_write_reg (devc, CLIEL + offs, 0, tmp);
  sblive_write_reg (devc, HLIEL + offs, 0, tmp);
}

static unsigned int
emu_rate_to_pitch (unsigned int rate)
{
  static unsigned int logMagTable[128] = {
    0x00000, 0x02dfc, 0x05b9e, 0x088e6, 0x0b5d6, 0x0e26f, 0x10eb3, 0x13aa2,
    0x1663f, 0x1918a, 0x1bc84, 0x1e72e, 0x2118b, 0x23b9a, 0x2655d, 0x28ed5,
    0x2b803, 0x2e0e8, 0x30985, 0x331db, 0x359eb, 0x381b6, 0x3a93d, 0x3d081,
    0x3f782, 0x41e42, 0x444c1, 0x46b01, 0x49101, 0x4b6c4, 0x4dc49, 0x50191,
    0x5269e, 0x54b6f, 0x57006, 0x59463, 0x5b888, 0x5dc74, 0x60029, 0x623a7,
    0x646ee, 0x66a00, 0x68cdd, 0x6af86, 0x6d1fa, 0x6f43c, 0x7164b, 0x73829,
    0x759d4, 0x77b4f, 0x79c9a, 0x7bdb5, 0x7dea1, 0x7ff5e, 0x81fed, 0x8404e,
    0x86082, 0x88089, 0x8a064, 0x8c014, 0x8df98, 0x8fef1, 0x91e20, 0x93d26,
    0x95c01, 0x97ab4, 0x9993e, 0x9b79f, 0x9d5d9, 0x9f3ec, 0xa11d8, 0xa2f9d,
    0xa4d3c, 0xa6ab5, 0xa8808, 0xaa537, 0xac241, 0xadf26, 0xafbe7, 0xb1885,
    0xb3500, 0xb5157, 0xb6d8c, 0xb899f, 0xba58f, 0xbc15e, 0xbdd0c, 0xbf899,
    0xc1404, 0xc2f50, 0xc4a7b, 0xc6587, 0xc8073, 0xc9b3f, 0xcb5ed, 0xcd07c,
    0xceaec, 0xd053f, 0xd1f73, 0xd398a, 0xd5384, 0xd6d60, 0xd8720, 0xda0c3,
    0xdba4a, 0xdd3b4, 0xded03, 0xe0636, 0xe1f4e, 0xe384a, 0xe512c, 0xe69f3,
    0xe829f, 0xe9b31, 0xeb3a9, 0xecc08, 0xee44c, 0xefc78, 0xf148a, 0xf2c83,
    0xf4463, 0xf5c2a, 0xf73da, 0xf8b71, 0xfa2f0, 0xfba57, 0xfd1a7, 0xfe8df
  };
  static char logSlopeTable[128] = {
    0x5c, 0x5c, 0x5b, 0x5a, 0x5a, 0x59, 0x58, 0x58,
    0x57, 0x56, 0x56, 0x55, 0x55, 0x54, 0x53, 0x53,
    0x52, 0x52, 0x51, 0x51, 0x50, 0x50, 0x4f, 0x4f,
    0x4e, 0x4d, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4b,
    0x4a, 0x4a, 0x49, 0x49, 0x48, 0x48, 0x47, 0x47,
    0x47, 0x46, 0x46, 0x45, 0x45, 0x45, 0x44, 0x44,
    0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x41, 0x41,
    0x41, 0x40, 0x40, 0x40, 0x3f, 0x3f, 0x3f, 0x3e,
    0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c,
    0x3b, 0x3b, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x39,
    0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x38, 0x37,
    0x37, 0x37, 0x37, 0x36, 0x36, 0x36, 0x36, 0x35,
    0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x34, 0x34,
    0x33, 0x33, 0x33, 0x33, 0x32, 0x32, 0x32, 0x32,
    0x32, 0x31, 0x31, 0x31, 0x31, 0x31, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x2f, 0x2f, 0x2f, 0x2f, 0x2f
  };
  int i;

  if (rate == 0)
    return 0;			/* Bail out if no leading "1" */
  rate *= 11185;		/* Scale 48000 to 0x20002380 */
  for (i = 31; i > 0; i--)
    {
      if (rate & 0x80000000)
	{			/* Detect leading "1" */
	  return (((unsigned int) (i - 15) << 20) +
		  logMagTable[0x7f & (rate >> 24)] +
		  (0x7f & (rate >> 17)) * logSlopeTable[0x7f & (rate >> 24)]);
	}
      rate <<= 1;
    }

  return 0;			/* Should never reach this point */
}

static unsigned int
emu_rate_to_linearpitch (unsigned int rate)
{
  rate = (rate << 8) / 375;
  return (rate >> 1) + (rate & 1);
}

static void
start_audio_voice (int dev, int voice, int chn)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;
  unsigned int sample, initial_pitch, pitch_target;
  unsigned int cra, cs, ccis, i;

  /* setup CCR regs */
  cra = 64;
  cs = (portc->channels > 1) ? 4 : 2;
  ccis = (portc->channels > 1) ? 28 : 30;
  ccis *= (portc->format == AFMT_S16_LE || portc->format == AFMT_AC3) ? 1 : 2;
  sample = (portc->format == AFMT_S16_LE
	    || portc->format == AFMT_AC3) ? 0x00000000 : 0x80808080;

  for (i = 0; i < cs; i++)
    sblive_write_reg (devc, CD0 + i, voice, sample);

  sblive_write_reg (devc, CCR_CACHEINVALIDSIZE, voice, 0);
  sblive_write_reg (devc, CCR_READADDRESS, voice, cra);
  sblive_write_reg (devc, CCR_CACHEINVALIDSIZE, voice, ccis);

  /* Set current pitch */
  sblive_write_reg (devc, IFA, voice, 0xff00);
  sblive_write_reg (devc, VTFT, voice, 0xffffffff);
  sblive_write_reg (devc, CVCF, voice, 0xffffffff);
  sblive_set_loop_stop (devc, voice, 0);

  pitch_target = emu_rate_to_linearpitch (portc->speed);
  initial_pitch = emu_rate_to_pitch (portc->speed) >> 8;
  sblive_write_reg (devc, PTRX_PITCHTARGET, voice, pitch_target);
  sblive_write_reg (devc, CPF_CURRENTPITCH, voice, pitch_target);
  sblive_write_reg (devc, IP, voice, initial_pitch);

  if (chn == LEFT_CH)
    {
      sblive_get_voice_loopintr (devc, voice);
      sblive_get_voice_halfloopintr (devc, voice);
      sblive_set_voice_intr (devc, voice, 1);
    }
  sblive_write_reg (devc, VEDS, voice, /*0x80 | */ 0x7f7f);	/* Trigger (OK) */
}

/*ARGSUSED*/
static void
stop_audio_voice (int dev, int voice, int chn)
{
  sblive_devc *devc = audio_engines[dev]->devc;

  sblive_write_reg (devc, IFA, voice, 0xffff);
  sblive_write_reg (devc, VTFT, voice, 0xffff);
  sblive_write_reg (devc, PTRX_PITCHTARGET, voice, 0);
  sblive_write_reg (devc, CPF_CURRENTPITCH, voice, 0);
  sblive_write_reg (devc, IP, voice, 0);
  sblive_set_loop_stop (devc, voice, 1);
  sblive_set_voice_intr (devc, voice, 0);
}

static void
sblive_audio_trigger (int dev, int state)
{
  sblive_portc *portc = audio_engines[dev]->portc;
  sblive_devc *devc = audio_engines[dev]->devc;
  int voiceL = portc->voice_chn, voiceR = portc->voice_chn + 1;

  if (portc->mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_active & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      start_audio_voice (dev, voiceL, LEFT_CH);
	      /* sblive_dump_regs(devc, voiceL); */
	      if (portc->channels > 1)
		start_audio_voice (dev, voiceR, RIGHT_CH);
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	    }
	}
      else
	{
	  if ((portc->audio_active & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	      portc->audio_active &= ~PCM_ENABLE_OUTPUT;
	      stop_audio_voice (dev, voiceL, LEFT_CH);
	      stop_audio_voice (dev, voiceR, RIGHT_CH);
	    }
	}
    }

  if (portc->mode & OPEN_READ && !(audio_engines[dev]->flags & ADEV_NOINPUT))
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_active & PCM_ENABLE_INPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      int tmp = sblive_read_reg (devc, ADCSR, 0);
	      unsigned int pos;

	      if (portc->input_type == ITYPE_SPDIF)
		{
		  /* Start recording from S/PDIF input A */
		  sblive_write_reg (devc, SPRC, 0, portc->in_szbits | 0x00);
		}
	      else
		{
		  if (devc->feature_mask & SB_AUDIGY)
		    {
		      tmp |= 0x10;	/* Left channel enable */
		      if (portc->channels > 1)
			tmp |= 0x20;	/* Right channel enable */
		    }
		  else
		    {
		      tmp |= 0x08;	/* Left channel enable */
		      if (portc->channels > 1)
			tmp |= 0x10;	/* Right channel enable */
		    }
		  sblive_write_reg (devc, ADCBS, 0, portc->in_szbits);
		  sblive_write_reg (devc, ADCSR, 0, tmp);	/* GO */
		}

	      pos = (INL (devc->osdev, devc->base + 0x10) >> 6) & 0xfffff;
	      portc->rec_starttime = pos;
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	    }
	}
      else
	{
	  if ((portc->audio_active & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      if (portc->input_type == ITYPE_SPDIF)
		sblive_write_reg (devc, SPRC, 0, 0);
	      else
		sblive_write_reg (devc, ADCSR, 0, 0);
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      portc->audio_active &= ~PCM_ENABLE_INPUT;
	    }
	}
    }

}

/*ARGSUSED*/
static int
sblive_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  int sz = -1;

  if (audio_engines[dev]->flags & ADEV_NOINPUT)
    {
      cmn_err (CE_WARN, "Audio device %d is output only\n", dev);
      return OSS_EIO;
    }

  if (dmap->buffsize > 65536)
    {
      cmn_err (CE_WARN, "Recording buffer bigger than 64k\n");
      dmap->buffsize = 65536;
    }

#ifdef sun1
  if (dmap->buffsize == 36864)
    {
      dmap->buffsize = 32768;
    }
#endif

  switch (dmap->buffsize)
    {
    case 4096:
      sz = 15;
      break;
    case 8192:
      sz = 19;
      break;
    case 16384:
      sz = 23;
      break;
    case 32768:
      sz = 27;
      break;
    case 65536:
      sz = 31;
      break;

    default:
      cmn_err (CE_WARN, "Unsupported input buffer size %d\n", dmap->buffsize);
      return OSS_ENOSPC;
    }

  if (portc->input_type == ITYPE_SPDIF)
    {
      sblive_write_reg (devc, SPRA, 0, dmap->dmabuf_phys);
      sblive_write_reg (devc, SPRC, 0, 0);
    }
  else
    {
      sblive_write_reg (devc, ADCBA, 0, dmap->dmabuf_phys);
      sblive_write_reg (devc, ADCBS, 0, 0);
    }
  portc->in_szbits = sz;

  sblive_write_reg (devc, ADCSR, 0, 0x0);

  if (portc->input_type == ITYPE_ANALOG)
    {
      if (devc->feature_mask & SB_AUDIGY)
	sblive_write_reg (devc, ADCSR, 0,
			  speed_tab[portc->speedsel].audigy_recbits);
      else
	sblive_write_reg (devc, ADCSR, 0, speed_tab[portc->speedsel].recbits);
    }

  devc->recording_dev = dev;

  portc->audio_active |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

  return 0;
}

/*ARGSUSED*/
static int
sblive_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;

  int voiceL = portc->voice_chn, voiceR = portc->voice_chn + 1;

  if (audio_engines[dev]->flags & ADEV_NOOUTPUT)
    return OSS_EIO;

  /* AC3 needs stereo too */
  if (portc->format == AFMT_AC3 || portc->input_type == ITYPE_SPDIF)
    {
      portc->channels = 2;
      portc->speed = 48000;
      portc->routing = spdif_routing;
    }
  else
    portc->routing = default_routing;

  /* Left channel */
  sblive_write_reg (devc, IFA, voiceL, 0xffff);	/* Intial filter cutoff and attenuation */
  sblive_write_reg (devc, VEDS, voiceL, 0x0);	/* Volume envelope decay and sustain */
  sblive_write_reg (devc, VTFT, voiceL, 0xffff);	/* Volume target and Filter cutoff target */
  sblive_write_reg (devc, PTAB, voiceL, 0x0);	/* Pitch target and sends A and B */
  /* The same for right channel */
  sblive_write_reg (devc, IFA, voiceR, 0xffff);
  sblive_write_reg (devc, VEDS, voiceR, 0x0);
  sblive_write_reg (devc, VTFT, voiceR, 0xffff);
  sblive_write_reg (devc, PTAB, voiceR, 0x0);

  /* now setup the voices and go! */
  setup_audio_voice (dev, voiceL, LEFT_CH);
  if (portc->channels == 2)
    setup_audio_voice (dev, voiceR, RIGHT_CH);

  portc->audio_active |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

  if (portc->uses_spdif)
    {
      if (portc->format == AFMT_AC3)
	{
	  sblive_write_reg (devc, SCS0, 0, 0x2109206);
	}
      else
	{
	  sblive_write_reg (devc, SCS0, 0, 0x2108504);
	}
    }
  return 0;
}

static int
sblive_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  int err, i, n;
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;

  if (dmap->dmabuf != NULL)
    return 0;

  if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
    {
      cmn_err (CE_WARN, "Cannot allocate DMA buffer\n");
      return err;
    }

  if (dmap->buffsize > DMABUF_SIZE)
    {
      cmn_err (CE_NOTE, "DMA buffer was too large - truncated\n");
      dmap->buffsize = DMABUF_SIZE;
    }

  if (devc->feature_mask & SB_LIVE)
    if (dmap->dmabuf_phys & 0x80000000)
      {
	cmn_err (CE_CONT, "Got DMA buffer address beyond 2G limit.\n");
	oss_free_dmabuf (dev, dmap);
	dmap->dmabuf = NULL;

	return OSS_ENOSPC;
      }

  if (direction == PCM_ENABLE_OUTPUT)
    {
      dmap->driver_use_value = portc->memptr;
      n = portc->memptr / 4096;

/* 
 * Fill the page table
 */
      for (i = 0; i < dmap->buffsize / 4096; i++)
	{
	  FILL_PAGE_MAP_ENTRY (n + i, dmap->dmabuf_phys + i * 4096);
	}
    }

  return 0;
}

/*ARGSUSED*/
static int
sblive_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;
  oss_free_dmabuf (dev, dmap);

  dmap->dmabuf = NULL;
  return 0;
}

/*ARGSUSED*/
static int
sblive_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  sblive_devc *devc = audio_engines[dev]->devc;
  sblive_portc *portc = audio_engines[dev]->portc;
  int pos;

  if (!(portc->trigger_bits & direction))
    return 0;

  if (direction == OPEN_WRITE)
    {
      pos = sblive_read_reg (devc, QKBCA, portc->voice_chn) & 0x00ffffff;	/* Curr pos */
      pos <<= portc->out_sz;
      pos -= dmap->driver_use_value;
    }
  else
    {
      /* Compute current pos based on the wall clock register */
      pos = (INL (devc->osdev, devc->base + 0x10) >> 6) & 0xfffff;
      if (pos > portc->rec_starttime)
	pos = pos - portc->rec_starttime;
      else
	pos = 0xfffff - (portc->rec_starttime - pos);
      pos = (pos * (portc->speed / 25)) / (48000 / 25);
      pos *= 2;			/* 16 bit->bytes */
      pos *= portc->channels;
      pos = pos % dmap->bytes_in_use;
    }

  if (pos < 0)
    pos = 0;

  return pos;
}

static audiodrv_t sblive_audio_driver = {
  sblive_audio_open,
  sblive_audio_close,
  sblive_audio_output_block,
  sblive_audio_start_input,
  sblive_audio_ioctl,
  sblive_audio_prepare_for_input,
  sblive_audio_prepare_for_output,
  sblive_audio_reset,
  NULL,
  NULL,
  sblive_audio_reset_input,
  sblive_audio_reset_output,
  sblive_audio_trigger,
  sblive_audio_set_rate,
  sblive_audio_set_format,
  sblive_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* sblive_check_input, */
  NULL,				/* sblive_check_output, */
  sblive_alloc_buffer,
  sblive_free_buffer,
  NULL,
  NULL,
  sblive_get_buffer_pointer,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  sblive_audio_ioctl		/* bind */
};

#define	DATAPORT   (devc->base)
#define	COMDPORT   (devc->base+1)
#define	STATPORT   (devc->base+1)

static __inline__ int
audigyuart_status (sblive_devc * devc)
{
  return sblive_read_reg (devc, MUASTAT, 0);
}

#define input_avail(devc) (!(audigyuart_status(devc)&INPUT_AVAIL))
#define output_ready(devc)	(!(audigyuart_status(devc)&OUTPUT_READY))
static void
audigyuart_cmd (sblive_devc * devc, unsigned char cmd)
{
  sblive_write_reg (devc, MUACMD, 0, cmd);
}

static __inline__ int
audigyuart_read (sblive_devc * devc)
{
  return sblive_read_reg (devc, MUADAT, 0);
}

static __inline__ void
audigyuart_write (sblive_devc * devc, unsigned char byte)
{
  sblive_write_reg (devc, MUADAT, 0, byte);
}

#define	OUTPUT_READY	0x40
#define	INPUT_AVAIL	0x80
#define	MPU_ACK		0xFE
#define	MPU_RESET	0xFF
#define	UART_MODE_ON	0x3F

static int reset_audigyuart (sblive_devc * devc);
static void enter_uart_mode (sblive_devc * devc);

typedef struct
{
  int keycode;
  int action;
  int local_action;
} ir_code_t;

static void
sblive_key_action (sblive_devc * devc, ir_code_t * code)
{
  int value, left, right, dev;

  dev = devc->mixer_dev;

  switch (code->local_action)
    {
    case 1:			/* Volume- */
      value = sblive_set_gpr (dev, GPR_VOLUME, SNDCTL_MIX_READ, 0);
      left = value & 0xff;
      right = (value >> 8) & 0xff;
      left -= 5;
      if (left < 0)
	left = 0;
      right -= 5;
      if (right < 0)
	right = 0;
      value = left | (right << 8);
      sblive_set_gpr (dev, GPR_VOLUME, SNDCTL_MIX_WRITE, value);
      mixer_devs[dev]->modify_counter++;
      return;
      break;

    case 2:			/* Volume+ */
      value = sblive_set_gpr (dev, GPR_VOLUME, SNDCTL_MIX_READ, 0);
      left = value & 0xff;
      right = (value >> 8) & 0xff;
      left += 5;
      if (left > 100)
	left = 100;
      right += 5;
      if (right > 100)
	right = 100;
      value = left | (right << 8);
      sblive_set_gpr (dev, GPR_VOLUME, SNDCTL_MIX_WRITE, value);
      mixer_devs[dev]->modify_counter++;
      return;
      break;
    }
}

static void
sblive_handle_ir (sblive_devc * devc, unsigned char c)
{
/*
 * Receive a MIDI SysEx message and check if it's an IR remote command
 */
#if 1

/*
 * Sysex code sent by the Live!DRIVE IR unit
 */
  static unsigned char remote_id[] =
    { 0xf0, 0x00, 0x20, 0x21, 0x60, 0x00, 0x02, 0x00, 0xf7 };

  static ir_code_t codes[] = {
    /* Creative RM-900B remote control unit */
    {0x09017e},			/* 0 */
    {0x0a512e},			/* 1 */
    {0x0a710e},			/* 2 */
    {0x090976},			/* 3 */
    {0x09512e},			/* 4 */
    {0x09215e},			/* 5 */
    {0x091e61},			/* 6 */
    {0x0a116e},			/* 7 */
    {0x0a413e},			/* 8 */
    {0x0a6e11},			/* 9 */
    {0x0a1e61},			/* Play/Pause */
    {0x0a215e},			/* Stop/Eject  */
    {0x0a3e41},			/* Slow */
    {0x0a7e01},			/* Prev */
    {0x095e21},			/* Next */
    {0x097e01},			/* Step */
    {0x097609},			/* Mute */
    {0x0a4639, 0, 1},		/* Vol- */
    {0x094639, 0, 2},		/* Vol+ */
    /* Speaker ??? */
    {0x09314e},			/* EAX */
    {0x09413e},			/* Options */
    {0x096e11},			/* Display */
    {0x09710e},			/* Return */
    {0x09116e},			/* Start */
    {0x093e41},			/* Cancel */
    {0x0a5e21},			/* Up */
    {0x0a611e},			/* << */
    {0x0a017e},			/* Select/OK */
    {0x0a2e51},			/* >> */
    {0x0a314e},			/* Down */

/* Creative RM-1000 remote control unit */
    {0x0a0679},			/* Power */
    {0x0a0e71},			/* CMSS */
    {0x0a4e31},			/* Rec */

/* Creative Inspire 5.1 Digital 5700 remote */
    {0x0a0778},			/* Power */
    {0x097708},			/* Mute */
    {0x0a7708},			/* Test */
    {0x0a4738},			/* Vol- */
    {0x094738},			/* Vol+ */
    {0x0a0f70},			/* Effect */
    {0x0a5728},			/* Analog */
    {0x0a2758},			/* Pro logic */
    {0x094f30},			/* Dynamic mode */
    {0x093748},			/* Digital/PCM audio */
    {0}
  };
#endif
  if (c == 0xf0)		/* Sysex start */
    {
      devc->sysex_buf[0] = c;
      devc->sysex_p = 1;
      return;
    }

  if (devc->sysex_p <= 0)
    return;

  if (devc->sysex_p >= 20)	/* Too long */
    {
      devc->sysex_p = 0;
      return;
    }

  if (c == 0xf7)		/* Sysex end */
    {
      int i, l, v;
      unsigned char *buf;

      devc->sysex_buf[devc->sysex_p] = c;
      devc->sysex_p++;
      l = devc->sysex_p;

      devc->sysex_p = 0;
      buf = devc->sysex_buf;

      if (l == 9)
	{
	  int ok = 1;

	  for (i = 0; i < sizeof (remote_id); i++)
	    if (buf[i] != remote_id[i])
	      ok = 0;

	  if (ok)
	    {
	      /* cmn_err (CE_CONT, "Live!DRIVE IR detected\n"); */
	      return;
	    }

	  return;
	}

      if (l != 13)		/* Wrong length */
	return;

      if (buf[0] != 0xf0 || buf[12] != 0xf7)	/* Not sysex */
	return;

      /* Verify that this is an IR receiver sysex */
      if (buf[1] != 0x00 || buf[2] != 0x20 || buf[3] != 0x21)
	return;
      if (buf[4] != 0x60 || buf[5] != 0x00 || buf[6] != 0x01)
	return;
#if 0
      if (buf[7] != 0x09 && buf[7] != 0x0a)	/* Remote ID */
	return;
#endif
      if (buf[8] != 0x41 || buf[9] != 0x44)
	return;

      v = (buf[7] << 16) | (buf[10] << 8) | buf[11];

      for (i = 0; codes[i].keycode != 0; i++)
	if (codes[i].keycode == v)
	  {
	    sblive_key_action (devc, &codes[i]);
	    return;
	  }

      return;
    }

  /* Ordinary byte */
  devc->sysex_buf[devc->sysex_p] = c;
  devc->sysex_p++;
}

static void
sblive_ir_callback (int dev, unsigned char c)
{
  sblive_devc *devc;
  oss_device_t *osdev = midi_devs[dev]->osdev;
  devc = osdev->devc;

  if (devc->midi_dev != dev)
    return;

  sblive_handle_ir (devc, c);
}

static void
audigyuart_input_loop (sblive_devc * devc)
{
  int t = 0;

  while (input_avail (devc) && t++ < 1000)
    {
      unsigned char c = audigyuart_read (devc);

      sblive_handle_ir (devc, c);

      if (c == MPU_ACK)
	devc->input_byte = c;
      else if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	devc->midi_input_intr (devc->midi_dev, c);
    }
}

static void
audigyuartintr (sblive_devc * devc)
{
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  audigyuart_input_loop (devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
audigyuart_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		 oss_midi_inputbuf_t inputbuf,
		 oss_midi_outputintr_t outputintr)
{
  sblive_devc *devc = (sblive_devc *) midi_devs[dev]->devc;

  if (devc->midi_opened)
    {
      return OSS_EBUSY;
    }

  while (input_avail (devc))
    audigyuart_read (devc);

  devc->midi_input_intr = inputbyte;
  devc->midi_opened = mode;
  enter_uart_mode (devc);
  devc->midi_disabled = 0;

  return 0;
}

/*ARGSUSED*/
static void
audigyuart_close (int dev, int mode)
{
  sblive_devc *devc = (sblive_devc *) midi_devs[dev]->devc;

  reset_audigyuart (devc);
  oss_udelay (10);
  enter_uart_mode (devc);
  reset_audigyuart (devc);
  devc->midi_opened = 0;
}

static int
audigyuart_out (int dev, unsigned char midi_byte)
{
  sblive_devc *devc = (sblive_devc *) midi_devs[dev]->devc;

  if (devc->midi_disabled)
    return 1;

  if (!output_ready (devc))
    {
      return 0;
    }

  audigyuart_write (devc, midi_byte);
  return 1;
}

/*ARGSUSED*/
static int
audigyuart_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t audigy_midi_driver = {
  audigyuart_open,
  audigyuart_close,
  audigyuart_ioctl,
  audigyuart_out
};

static void
enter_uart_mode (sblive_devc * devc)
{
  int ok, timeout;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

  devc->input_byte = 0;
  audigyuart_cmd (devc, UART_MODE_ON);

  ok = 0;
  for (timeout = 50000; timeout > 0 && !ok; timeout--)
    if (devc->input_byte == MPU_ACK)
      ok = 1;
    else if (input_avail (devc))
      if (audigyuart_read (devc) == MPU_ACK)
	ok = 1;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

void
attach_audigyuart (sblive_devc * devc)
{
  enter_uart_mode (devc);

  devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "AUDIGY", "Audigy UART", &audigy_midi_driver, sizeof (midi_driver_t),
					0, devc, devc->osdev);
  devc->midi_opened = 0;
}

static int
reset_audigyuart (sblive_devc * devc)
{
  int ok, timeout, n;
  oss_native_word flags;

  /*
   * Send the RESET command. Try again if no success at the first time.
   */

  ok = 0;
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  for (n = 0; n < 2 && !ok; n++)
    {
      for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

      devc->input_byte = 0;
      audigyuart_cmd (devc, MPU_RESET);

      /*
       * Wait at least 25 msec. This method is not accurate so let's make the
       * loop bit longer. Cannot sleep since this is called during boot.
       */

      for (timeout = 50000; timeout > 0 && !ok; timeout--)
	if (devc->input_byte == MPU_ACK)	/* Interrupt */
	  ok = 1;
	else if (input_avail (devc))
	  if (audigyuart_read (devc) == MPU_ACK)
	    ok = 1;

    }



  if (ok)
    audigyuart_input_loop (devc);	/*
					 * Flush input before enabling interrupts
					 */

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return ok;
}

int
probe_audigyuart (sblive_devc * devc)
{
  int ok = 0;

  DDB (cmn_err (CE_CONT, "Entered probe_audigyuart\n"));

  devc->midi_input_intr = NULL;
  devc->midi_opened = 0;
  devc->input_byte = 0;

  ok = reset_audigyuart (devc);

  if (ok)
    {
      DDB (cmn_err (CE_CONT, "Reset UART401 OK\n"));
    }
  else
    {
      DDB (cmn_err
	   (CE_CONT, "Reset UART401 failed (no hardware present?).\n"));
      DDB (cmn_err
	   (CE_CONT, "mpu401 status %02x\n", audigyuart_status (devc)));
    }

  DDB (cmn_err (CE_CONT, "audigyuart detected OK\n"));
  return ok;
}

void
unload_audigyuart (sblive_devc * devc)
{
  reset_audigyuart (devc);
}

static void
attach_mpu (sblive_devc * devc)
{
  char tmp[128];
  int ndevs = num_mididevs;
  oss_native_word flags;

  sprintf (tmp, "%s external MIDI", devc->card_name);

  if (devc->feature_mask & SB_AUDIGY)
    {
      if (!probe_audigyuart (devc))
	{
	  cmn_err (CE_NOTE, "MIDI UART was not detected\n");
	  return;
	}
      DDB (cmn_err (CE_CONT, "SB Audigy: MIDI UART detected - Good\n"));
      devc->mpu_attached = 1;
      attach_audigyuart (devc);
    }
  else
    {
      MUTEX_ENTER (devc->mutex, flags);
      if (uart401_init (&devc->uart401devc, devc->osdev, devc->base + 0x18,
			tmp) >= 0)
	devc->mpu_attached = 1;
      MUTEX_EXIT (devc->mutex, flags);

      if (ndevs != num_mididevs)
	{
	  devc->midi_dev = ndevs;
	  midi_devs[ndevs]->input_callback = sblive_ir_callback;
	}
    }
}

static void
load_dsp (sblive_devc * devc, unsigned char *buf, int len)
{
  emu10k1_file *code;
  int pc, i;

  if (len != sizeof (*code))
    {
      cmn_err (CE_NOTE, "DSP file size mismatch\n");
      return;
    }

  code = (emu10k1_file *) buf;

  for (pc = 0; pc < 1024; pc++)
    {
      write_efx (devc, UC0 + pc, code->code[pc]);
    }

  if (code->parms.ngpr < MAX_GPR_PARMS)
    for (i = 0; i < code->parms.ngpr; i++)
      {
	code->parms.gpr[i].name[GPR_NAME_SIZE - 1] = 0;	/* Overflow protection */
	if (strlen (code->parms.gpr[i].name) >= 32)	/* Name may be bad */
	  {
	    return;
	  }

/* cmn_err(CE_CONT, "Gpr %d = %s (vol %x) type=%x\n", gpr->gpr[i].num, gpr->gpr[i].name, gpr->gpr[i].def, gpr->gpr[i].type); */
	if (code->parms.gpr[i].num < MAX_GPR)
	  if (code->parms.gpr[i].type != MIXT_GROUP)
	    {
	      if (is_special_gpr (code->parms.gpr[i].num))
		sblive_set_gpr (devc->mixer_dev, code->parms.gpr[i].num,
				SNDCTL_MIX_WRITE,
				devc->gpr_values[code->parms.gpr[i].num]);
	      else
		sblive_set_gpr (devc->mixer_dev, code->parms.gpr[i].num,
				SNDCTL_MIX_WRITE, code->parms.gpr[i].def);
	    }
      }


  if (devc->gpr == NULL)
    {
      devc->gpr = PMALLOC (devc->osdev, sizeof (gpr_info));
      if (devc->gpr == NULL)
	{
	  cmn_err (CE_WARN, "Out of memory (gpr)\n");
	  return;
	}
      memset (devc->gpr, 0, sizeof (gpr_info));
    }
  memcpy (devc->gpr, &code->parms, sizeof (gpr_info));
  create_efx_mixer (devc->mixer_dev);

  if (code->consts.nconst >= MAX_CONST_PARMS)
    return;

  for (i = 0; i < code->consts.nconst; i++)
    {
      if (code->consts.consts[i].gpr >= MAX_GPR)
	{
	  return;
	}

      sblive_write_reg (devc, code->consts.consts[i].gpr + GPR0, 0,
			code->consts.consts[i].value);
    }
}

#define LIVE_NOP() \
        write_efx(devc, UC0+(pc*2), 0x10040); \
        write_efx(devc, UC0+(pc*2+1), 0x610040);pc++
#define LIVE_ACC3(r, a, x, y) /* z=w+x+y */ \
    write_efx(devc, UC0+(pc*2), (x << 10) | y); \
    write_efx(devc, UC0+(pc*2+1), (6 << 20) | (r << 10) | a);pc++

#define AUDIGY_ACC3(r, a, x, y) /* z=w+x+y */ \
    write_efx(devc, UC0+(pc*2), (x << 12) | y); \
    write_efx(devc, UC0+(pc*2+1), (6 << 24) | (r << 12) | a);pc++
#define AUDIGY_NOP() AUDIGY_ACC3(0xc0, 0xc0, 0xc0, 0xc0)

static int
init_effects (sblive_devc * devc)
{
  int i;
  unsigned short pc;

  if (devc->feature_mask & SB_AUDIGY)
    {
      pc = 0;
      for (i = 0; i < 512; i++)
	{
	  AUDIGY_NOP ();
	}

      for (i = 0; i < 256; i++)
	write_efx (devc, GPR0 + i, 0);
      sblive_write_reg (devc, AUDIGY_DBG, 0, 0);
      load_dsp (devc, emu10k2_dsp, sizeof (emu10k2_dsp));
    }
  else
    {
      pc = 0;
      for (i = 0; i < 512; i++)
	{
	  LIVE_NOP ();
	}

      for (i = 0; i < 256; i++)
	write_efx (devc, GPR0 + i, 0);
      sblive_write_reg (devc, DBG, 0, 0);
      load_dsp (devc, emu10k1_dsp, sizeof (emu10k1_dsp));
    }

  return 1;
}

static void
init_emu10k1 (sblive_devc * devc)
{
  unsigned int tmp, i;
  extern int sblive_memlimit;
#ifndef NO_EMU10K1_SYNTH
  extern int sblive_synth_enable;
#endif
  int xmem_mode = 0;
  unsigned int reg, val;
  extern int sblive_digital_din;
  extern int audigy_digital_din;
  oss_native_word phaddr;
  unsigned int memlimit = MEMLIMIT_31BITS;

  OUTL (devc->osdev, 0x00000000, devc->base + 0x0c);	/* Intr disable */
  OUTL (devc->osdev,
	HCFG_LOCKSOUNDCACHE | HCFG_LOCKTANKCACHE_MASK | HCFG_MUTEBUTTONENABLE,
	devc->base + 0x14);

  sblive_write_reg (devc, MBS, 0, 0x0);
  sblive_write_reg (devc, MBA, 0, 0x0);
  sblive_write_reg (devc, FXBS, 0, 0x0);
  sblive_write_reg (devc, FXBA, 0, 0x0);
  sblive_write_reg (devc, ADCBS, 0, 0x0);
  sblive_write_reg (devc, ADCBA, 0, 0x0);

  sblive_write_reg (devc, CLIEL, 0, 0x0);
  sblive_write_reg (devc, CLIEH, 0, 0x0);
  sblive_write_reg (devc, SOLL, 0, 0xffffffff);
  sblive_write_reg (devc, SOLH, 0, 0xffffffff);


  if (devc->feature_mask & SB_AUDIGY)
    {
      memlimit=MEMLIMIT_32BITS;
      sblive_write_reg (devc, 0x5e, 0, 0xf00);	/* ?? */
      sblive_write_reg (devc, 0x5f, 0, 0x3);	/* ?? */
    }

#ifndef NO_EMU10K1_SYNTH
  if (!sblive_synth_enable)
    sblive_memlimit = 0;
#endif

  if (sblive_memlimit < 4096)	/* Given in megabytes */
    sblive_memlimit *= (1024 * 1024);

  devc->max_mem = sblive_memlimit;
  if (devc->max_mem < 1024 * 1024)
    devc->max_mem = 1024 * 1024;

  devc->max_mem += AUDIO_MEMSIZE;

  /* SB Live/Audigy supports at most 32M of memory) */
  if (devc->max_mem > 32 * 1024 * 1024)
    devc->max_mem = 32 * 1024 * 1024;

  devc->max_pages = devc->max_mem / 4096;
  if (devc->max_pages < 1024)
    devc->max_pages = 1024;
  devc->page_map =
    (int *) CONTIG_MALLOC (devc->osdev, devc->max_pages * 4, memlimit,
			   &phaddr, devc->page_map_dma_handle);
  devc->vpage_map =
    KERNEL_MALLOC (devc->max_pages * sizeof (unsigned char *));
  if (devc->page_map == NULL || devc->vpage_map == NULL)
    {
      cmn_err (CE_WARN, "Can't allocate the PTBA table\n");
      return;
    }
  memset (devc->vpage_map, 0, devc->max_pages * 4);

  tmp = phaddr;
  if (devc->feature_mask & SB_LIVE)
    {
      if (tmp & 0x80000000)
	{
	  cmn_err (CE_CONT,
		   "SB Live Error: Page table is beyond the 2G limit\n");
	}
    }
  else
    {
      if (tmp & 0x80000000)
	{
	  DDB (cmn_err (CE_CONT, "Audigy: Using 4G PCI addressing mode\n"));
	  xmem_mode = 1;
	  devc->emu_page_shift = 0;
	  if (devc->max_mem > 16 * 1024 * 1034)
	    {
	      devc->max_mem = 16 * 1024 * 1024;

	      DDB (cmn_err
		   (CE_NOTE,
		    "Max memory dropped to 16M due to need for extended PCI address mode.\n"));
	    }
	}
    }

  devc->synth_memlimit = devc->max_mem - AUDIO_MEMSIZE;
  devc->synth_membase = SYNTH_MEMBASE;
  devc->synth_memtop = devc->synth_membase;
  devc->synth_memptr = devc->synth_membase;

  devc->silent_page =
    (int *) CONTIG_MALLOC (devc->osdev, 4096, memlimit, &phaddr, devc->silent_page_dma_handle);
  if (devc->silent_page == NULL)
    {
      cmn_err (CE_WARN, "Can't allocate a silent page\n");
      return;
    }

  devc->silent_page_phys = phaddr;
  if (devc->feature_mask & SB_LIVE)
    if (devc->silent_page_phys & 0x80000000)
      {
	cmn_err (CE_CONT,
		 "SB Live warning: Silent page is beyond the 2G limit\n");
      }

  devc->audio_memptr = 4096;	/* Skip the silence page */
  memset (devc->silent_page, 0, 4096);

  for (i = 0; i < devc->max_pages; i++)
    {
      FILL_PAGE_MAP_ENTRY (i, devc->silent_page_phys);
      devc->vpage_map[i] = NULL;
    }

  for (i = 0; i < 64; i++)
    sblive_init_voice (devc, i);

  if (devc->feature_mask & SB_AUDIGY)
    {
      sblive_write_reg (devc, SCS0, 0, 0x2108504);
      sblive_write_reg (devc, SCS1, 0, 0x2108504);
      sblive_write_reg (devc, SCS2, 0, 0x2108504);
    }
  else
    {
      sblive_write_reg (devc, SCS0, 0, 0x2109204);
      sblive_write_reg (devc, SCS1, 0, 0x2109204);
      sblive_write_reg (devc, SCS2, 0, 0x2109204);
    }

  sblive_write_reg (devc, PTBA, 0, tmp);
  tmp = sblive_read_reg (devc, PTBA, 0);

  sblive_write_reg (devc, TCBA, 0, 0x0);
  sblive_write_reg (devc, TCBS, 0, 0x4);

  OUTL (devc->osdev, IE_RXA | IE_AB | IE_IT, devc->base + IE);	/* Intr enable */

/*
 * SB Live 5.1 support. Turn on S/PDIF output
 */
  if (devc->subvendor == 0x80611102)	/* Live 5.1 */
    {
      tmp = INL (devc->osdev, devc->base + 0x14);
      tmp |= 0x00001000;	/* Turn GPO0 pin on to enable S/PDIF outputs */
      OUTL (devc->osdev, tmp, devc->base + 0x14);
    }

  if (devc->subvendor == 0x80661102)
    {
      sblive_write_reg (devc, AC97SLOT, 0,
			AC97SLOT_CENTER | AC97SLOT_LFE | AC97SLOT_REAR_LEFT |
			AC97SLOT_REAR_RIGHT);
    }

  if (devc->feature_mask & SB_AUDIGY2)
    {
      /* Enable analog outputs on Audigy2 */
      int tmp;

      /* Setup SRCMulti_I2S SamplingRate */
      tmp = sblive_read_reg (devc, EHC, 0);
      tmp &= 0xfffff1ff;
      tmp |= (0x2 << 9);
      sblive_write_reg (devc, EHC, 0, tmp);
      /* sblive_write_reg (devc, SOC, 0, 0x00000000); */

      /* Setup SRCSel (Enable Spdif,I2S SRCMulti) */
      OUTL (devc->osdev, 0x600000, devc->base + 0x20);
      OUTL (devc->osdev, 0x14, devc->base + 0x24);

      /* Setup SRCMulti Input Audio Enable */
      /* Setup SRCMulti Input Audio Enable */
      if (devc->feature_mask & SB_AUDIGY2VAL)
	OUTL (devc->osdev, 0x7B0000, devc->base + 0x20);
      else
	OUTL (devc->osdev, 0x6E0000, devc->base + 0x20);

      OUTL (devc->osdev, 0xFF00FF00, devc->base + 0x24);

      /* Setup I2S ASRC Enable  (HC register) */
      tmp = INL (devc->osdev, devc->base + 0x14);
      tmp |= 0x00000070;
      OUTL (devc->osdev, tmp, devc->base + 0x14);

      /*
       * Unmute Analog now.  Set GPO6 to 1 for Apollo.
       * This has to be done after init ALice3 I2SOut beyond 48KHz.
       * So, sequence is important
       */
      tmp = INL (devc->osdev, devc->base + 0x18);
      tmp |= 0x0040;
      if (devc->feature_mask & SB_AUDIGY2VAL)
	tmp |= 0x0060;

      OUTL (devc->osdev, tmp, devc->base + 0x18);
    }

  sblive_write_reg (devc, SOLL, 0, 0xffffffff);
  sblive_write_reg (devc, SOLH, 0, 0xffffffff);

  if (devc->feature_mask & SB_AUDIGY)
    {
      unsigned int mode = 0;

      if (devc->feature_mask & SB_AUDIGY2)
	mode |= HCFG_AC3ENABLE_GPSPDIF | HCFG_AC3ENABLE_CDSPDIF;
      if (xmem_mode)
	{
	  OUTL (devc->osdev,
		HCFG_AUDIOENABLE | HCFG_AUTOMUTE | HCFG_JOYENABLE |
		A_HCFG_VMUTE | A_HCFG_AUTOMUTE | A_HCFG_XM | mode,
		devc->base + 0x14);
	}
      else
	OUTL (devc->osdev,
	      HCFG_AUDIOENABLE | HCFG_AUTOMUTE | HCFG_JOYENABLE | A_HCFG_VMUTE
	      | A_HCFG_AUTOMUTE | mode, devc->base + 0x14);

      OUTL (devc->osdev, INL (devc->osdev, devc->base + 0x18) | 0x0004, devc->base + 0x18);	/* GPIO (S/PDIF enable) */


      /* enable IR port */
      tmp = INL (devc->osdev, devc->base + 0x18);
      OUTL (devc->osdev, tmp | A_IOCFG_GPOUT2, devc->base + 0x18);
      oss_udelay (500);
      OUTL (devc->osdev, tmp | A_IOCFG_GPOUT1 | A_IOCFG_GPOUT2,
	    devc->base + 0x18);
      oss_udelay (100);
      OUTL (devc->osdev, tmp, devc->base + 0x18);
    }
  else
    OUTL (devc->osdev,
	  HCFG_AUDIOENABLE | HCFG_LOCKTANKCACHE_MASK | HCFG_AUTOMUTE |
	  HCFG_JOYENABLE, devc->base + 0x14);


  /* enable IR port */
  tmp = INL (devc->osdev, devc->base + 0x14);
  OUTL (devc->osdev, tmp | HCFG_GPOUT2, devc->base + 0x14);
  oss_udelay (500);
  OUTL (devc->osdev, tmp | HCFG_GPOUT1 | HCFG_GPOUT2, devc->base + 0x14);
  oss_udelay (100);
  OUTL (devc->osdev, tmp, devc->base + 0x14);

  /* Switch the shared SPDIF/OUT3 to DIGITAL or ANALOG mode */
  /* depending on whether the port is SPDIF or analog */

  if ((devc->feature_mask == SB_AUDIGY) ||
      ((devc->feature_mask & SB_AUDIGY2) && (audigy_digital_din == 0)))
    {
      reg = INL (devc->osdev, devc->base + 0x18) & ~A_IOCFG_GPOUT0;
      val = (audigy_digital_din) ? 0x4 : 0;
      reg |= val;
      OUTL (devc->osdev, reg, devc->base + 0x18);
    }
  if (devc->feature_mask & SB_LIVE)	/* SBLIVE */
    {
      reg = INL (devc->osdev, devc->base + 0x14) & ~HCFG_GPOUT0;
      val = (sblive_digital_din) ? HCFG_GPOUT0 : 0;
      reg |= val;
      OUTL (devc->osdev, reg, devc->base + 0x14);
    }

}

void
sblive_init_voice (sblive_devc * devc, int voice)
{
  sblive_set_loop_stop (devc, voice, 1);

  sblive_write_reg (devc, VEDS, voice, 0x0);
  sblive_write_reg (devc, IP, voice, 0x0);
  sblive_write_reg (devc, VTFT, voice, 0xffff);
  sblive_write_reg (devc, CVCF, voice, 0xffff);
  sblive_write_reg (devc, PTAB, voice, 0x0);
  sblive_write_reg (devc, CPF, voice, 0x0);
  sblive_write_reg (devc, CCR, voice, 0x0);
  sblive_write_reg (devc, SCSA, voice, 0x0);
  sblive_write_reg (devc, SDL, voice, 0x10);
  sblive_write_reg (devc, QKBCA, voice, 0x0);
  sblive_write_reg (devc, Z1, voice, 0x0);
  sblive_write_reg (devc, Z2, voice, 0x0);

  if (devc->feature_mask & SB_AUDIGY)
    sblive_write_reg (devc, SRDA, voice, 0x03020100);
  sblive_write_reg (devc, FXRT, voice, 0x32100000);

  sblive_write_reg (devc, MEHA, voice, 0x0);
  sblive_write_reg (devc, MEDS, voice, 0x0);
  sblive_write_reg (devc, IFA, voice, 0xffff);
  sblive_write_reg (devc, PEFE, voice, 0x0);
  sblive_write_reg (devc, VFM, voice, 0x0);
  sblive_write_reg (devc, TMFQ, voice, 24);
  sblive_write_reg (devc, VVFQ, voice, 24);
  sblive_write_reg (devc, TMPE, voice, 0x0);
  sblive_write_reg (devc, VLV, voice, 0x0);
  sblive_write_reg (devc, MLV, voice, 0x0);
  sblive_write_reg (devc, VEHA, voice, 0x0);
  sblive_write_reg (devc, VEV, voice, 0x0);
  sblive_write_reg (devc, MEV, voice, 0x0);

  if (devc->feature_mask & SB_AUDIGY)
    {
      sblive_write_reg (devc, CSBA, voice, 0x0);
      sblive_write_reg (devc, CSDC, voice, 0x0);
      sblive_write_reg (devc, CSFE, voice, 0x0);
      sblive_write_reg (devc, CSHG, voice, 0x0);
      sblive_write_reg (devc, SRHE, voice, 0x3f3f3f3f);
    }
}

#ifndef NO_EMU10K1_SYNTH
extern void sblive_install_synth (sblive_devc * devc);
extern void sblive_remove_synth (sblive_devc * devc);
#endif

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

static void
set_equalizer (sblive_devc * devc, int ctrl, int band, int value)
{
  const unsigned int *row;
  int i;

  switch (band)
    {
    case 0:
      row = (unsigned int *) &eq_band1_data[value][0];
      break;
    case 1:
      row = (unsigned int *) &eq_band2_data[value][0];
      break;
    case 2:
      row = (unsigned int *) &eq_band3_data[value][0];
      break;
    case 3:
      row = (unsigned int *) &eq_band4_data[value][0];
      break;

    default:
      cmn_err (CE_CONT, "%s: bad equalizer band %d\n", devc->card_name, band);
      return;
    }

  for (i = 0; i < 5; i++)
    {
      sblive_write_reg (devc, ctrl + GPR0 + i, 0, row[i]);
    }
}

static const int db2lin_101[101] = { 0x00000000,
  0x0024B53A, 0x002750CA, 0x002A1BC6, 0x002D198D, 0x00304DBA, 0x0033BC2A,
  0x00376901, 0x003B58AF, 0x003F8FF1, 0x004413DF, 0x0048E9EA, 0x004E17E9,
  0x0053A419, 0x0059952C, 0x005FF24E, 0x0066C32A, 0x006E0FFB, 0x0075E18D,
  0x007E414F, 0x0087395B, 0x0090D482, 0x009B1E5B, 0x00A6234F, 0x00B1F0A7,
  0x00BE94A1, 0x00CC1E7C, 0x00DA9E8D, 0x00EA2650, 0x00FAC881, 0x010C9931,
  0x011FADDC, 0x01341D87, 0x014A00D8, 0x01617235, 0x017A8DE6, 0x01957233,
  0x01B23F8D, 0x01D118B1, 0x01F222D4, 0x021585D1, 0x023B6C57, 0x0264041D,
  0x028F7E19, 0x02BE0EBD, 0x02EFEE33, 0x032558A2, 0x035E8E7A, 0x039BD4BC,
  0x03DD7551, 0x0423BF61, 0x046F07B5, 0x04BFA91B, 0x051604D5, 0x0572830D,
  0x05D59354, 0x063FAD27, 0x06B15080, 0x072B0673, 0x07AD61CD, 0x0838FFCA,
  0x08CE88D3, 0x096EB147, 0x0A1A3A53, 0x0AD1F2E0, 0x0B96B889, 0x0C6978A5,
  0x0D4B316A, 0x0E3CF31B, 0x0F3FE155, 0x10553469, 0x117E3AD9, 0x12BC5AEA,
  0x14111457, 0x157E0219, 0x1704DC5E, 0x18A77A97, 0x1A67D5B6, 0x1C480A87,
  0x1E4A5C45, 0x2071374D, 0x22BF3412, 0x25371A37, 0x27DBE3EF, 0x2AB0C18F,
  0x2DB91D6F, 0x30F89FFD, 0x34733433, 0x382D0C46, 0x3C2AA6BD, 0x4070D3D9,
  0x4504BB66, 0x49EBE2F1, 0x4F2C346F, 0x54CC0565, 0x5AD21E86, 0x6145C3E7,
  0x682EBDBD, 0x6F9561C4, 0x77829D4D,
  0x7fffffff
};

static __inline__ int
convert_fixpoint (int val)
{
  if (val < 0)
    val = 0;
  if (val > 100)
    val = 100;
  return db2lin_101[val];
}

static int
sblive_set_gpr (int dev, int ctrl, unsigned int cmd, int value)
{
  sblive_devc *devc = mixer_devs[dev]->hw_devc;
  int typ, i;

  if (devc == NULL)
    return 0;

  if (devc->gpr == NULL)
    {
      int left, right;

      if (ctrl >= NEXT_FREE_GPR)
	return 0;

      if (cmd != SNDCTL_MIX_WRITE)
	return 0;

      left = value & 0xff;
      right = (value >> 8) & 0xff;

      if (left < 0)
	left = 0;
      if (left > 100)
	left = 100;
      if (right < 0)
	right = 0;
      if (right > 100)
	right = 100;
      value = left | (right << 8);
      devc->gpr_values[ctrl] = value;

      left = convert_fixpoint (left);
      sblive_write_reg (devc, ctrl + GPR0, 0, left);
      right = convert_fixpoint (right);
      sblive_write_reg (devc, ctrl + GPR0 + 1, 0, right);
      return value;
    }

  if (ctrl < 0 || ctrl >= MAX_GPR)
    return OSS_EIO;

  typ = MIXT_SLIDER;
  for (i = 0; i < devc->gpr->ngpr; i++)
    if (devc->gpr->gpr[i].num == ctrl && devc->gpr->gpr[i].type != MIXT_GROUP)
      typ = devc->gpr->gpr[i].type;

  if (typ == MIXT_GROUP)
    {
      return OSS_EIO;
    }

  if (cmd == SNDCTL_MIX_READ)
    {
      if (typ == MIXT_STEREOPEAK || typ == MIXT_STEREOVU)
	{
	  int v, l, r;

	  /* Get the sample values and scale them to 0-144 dB range */
	  v = sblive_read_reg (devc, ctrl + GPR0, 0);
	  l = v >> 23;

	  v = sblive_read_reg (devc, ctrl + GPR0 + 1, 0);
	  r = v >> 23;

	  if (l < 0)
	    l = -l;
	  if (r < 0)
	    r = -r;
	  l = peak_cnv[l];
	  r = peak_cnv[r];

	  /* Reset values back to 0 */
	  sblive_write_reg (devc, ctrl + GPR0, 0, 0);
	  sblive_write_reg (devc, ctrl + GPR0 + 1, 0, 0);

	  return l | (r << 8);
	}
      return devc->gpr_values[ctrl];
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (typ)
	{
	case MIXT_STEREOSLIDER:
	  {
	    int left, right;

	    left = value & 0xff;
	    right = (value >> 8) & 0xff;

	    if (left < 0)
	      left = 0;
	    if (left > 100)
	      left = 100;
	    if (right < 0)
	      right = 0;
	    if (right > 100)
	      right = 100;
	    value = left | (right << 8);
	    devc->gpr_values[ctrl] = value;

	    left = convert_fixpoint (left);
	    sblive_write_reg (devc, ctrl + GPR0, 0, left);
	    right = convert_fixpoint (right);
	    sblive_write_reg (devc, ctrl + GPR0 + 1, 0, right);
	  }
	  break;

	case MIXT_ONOFF:
	  {
	    value = !!value;
	    devc->gpr_values[ctrl] = value;

	    sblive_write_reg (devc, ctrl + GPR0, 0, value);
	    sblive_write_reg (devc, ctrl + GPR0 + 1, 0, !value);
	  }
	  break;

	case EMU_MIXT_EQ1:
	case EMU_MIXT_EQ2:
	case EMU_MIXT_EQ3:
	case EMU_MIXT_EQ4:
	  {
	    int band;

	    band = typ & 3;
	    value = value & 0xff;
	    set_equalizer (devc, ctrl, band, value);
	    devc->gpr_values[ctrl] = value;
	  }
	  break;

	default:
	  {
	    int tmp;

	    value = value & 0xff;
	    if (value > 100)
	      value = 100;

	    devc->gpr_values[ctrl] = value;

	    tmp = convert_fixpoint (value);
	    sblive_write_reg (devc, ctrl + GPR0, 0, tmp);
	  }
	}

      return value;
    }

  return OSS_EINVAL;
}

static int
sblive_set_vol (int dev, int ctrl, unsigned int cmd, int value)
{
  sblive_devc *devc = mixer_devs[dev]->hw_devc;
  sblive_portc *portc;

  if (ctrl < 0 || ctrl >= devc->n_audiodevs)
    return OSS_EINVAL;

  portc = &devc->portc[ctrl];

  if (portc->input_type == ITYPE_SPDIF)
    {
      mixer_devs[dev]->modify_counter++;
      return 100 | (100 << 8);
    }

  if (cmd == SNDCTL_MIX_READ)
    {
#ifdef TEST_3D
      return (devc->portc[ctrl].playvol & 0x00ff) |
	((devc->portc[ctrl].playangle & 0xffff) << 16) |
	((devc->portc[ctrl].playdist & 0xff) << 8);
#else
      return devc->portc[ctrl].playvol & 0xff;
#endif
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
#ifdef TEST_3D
      int angle, dist;
      angle = (value >> 16) & 0xffff;	/* Rotation angle */
      dist = (value >> 8) & 0xff;	/* Distance */
      value &= 0x00ff;		/* Volume */

      if (value < 0)
	value = 0;
      if (value > 100)
	value = 100;

      switch (portc->speaker_mode)
	{
	case SMODE_FRONT:
	  angle = 0;
	  dist = 50;
	  break;

	case SMODE_SURR:
	  angle = 180;
	  dist = 50;
	  break;

	case SMODE_FRONTREAR:
	  angle = 0;
	  dist = 50;
	  break;

	case SMODE_3D:
	  break;
	}
      devc->portc[ctrl].playvol = value;
      devc->portc[ctrl].playdist = dist;
      devc->portc[ctrl].playangle = angle;

      update_output_device (devc, &devc->portc[ctrl]);
      return (value & 0x00ff) | (angle << 16) | ((dist & 0xff) << 8);
#else
      value &= 0xff;		/* Only left channel */

      if (value < 0)
	value = 0;
      if (value > 100)
	value = 100;
      devc->portc[ctrl].playvol = value;

      update_output_device (devc, &devc->portc[ctrl]);
      return value;

#endif
    }

  return OSS_EINVAL;
}

/*ARGSUSED*/
static int
sblive_get_peak (int dev, int ctrl, unsigned int cmd, int value)
{
  sblive_devc *devc = mixer_devs[dev]->hw_devc;

  if (ctrl < 0 || ctrl >= devc->n_audiodevs)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      int l, r, vol;

      l = devc->portc[ctrl].vu_left & 0xff;
      r = devc->portc[ctrl].vu_right & 0xff;
#if 1
      vol = devc->portc[ctrl].playvol;
      /* if (vol<1) vol=5; */
      l = (l * vol + 50) / 100;
      r = (r * vol + 50) / 100;
#endif
      devc->portc[ctrl].vu_left = 0;
      devc->portc[ctrl].vu_right = 0;

      return peak_cnv[l] | (peak_cnv[r] << 8);
    }

  return OSS_EINVAL;
}

static int
sblive_set_parm (int dev, int ctrl, unsigned int cmd, int value)
{
  sblive_devc *devc = mixer_devs[dev]->hw_devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      switch (ctrl)
	{
	case 1:
	  return devc->autoreset;
	case 2:
	  return devc->speaker_mode;
	}
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case 1:
	  return devc->autoreset = !!(value);
	case 2:
	  if (devc->speaker_mode != value)
	    {
	      int i;
	      for (i = 0; i < devc->n_audiodevs; i++)
		devc->portc[i].resetvol = 1;
	    }
	  return devc->speaker_mode = value;
	  break;

	}
    }

  return OSS_EINVAL;
}

static int
create_soft_mixer (int dev)
{
  sblive_devc *devc = mixer_devs[dev]->hw_devc;
  int group = 0, err, i, n;
  char tmp[100];

  if ((err = mixer_ext_create_control (dev, 0,
				       1,
				       sblive_set_parm,
				       MIXT_ONOFF,
				       "SBLIVE_AUTORESET",
				       2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

#ifdef TEST_3D
  n = 5;
#else
  n = 4;
#endif
  if ((err = mixer_ext_create_control (dev, 0,
				       2,
				       sblive_set_parm,
				       MIXT_ENUM,
				       "SBLIVE_SPKMODE",
				       n,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  for (i = 0; i < devc->n_audiodevs; i++)
    {
      if (devc->n_audiodevs > devc->min_audiodevs)
	{
	  /*
	   * Use the traditional dspN naming system for sliders.
	   */
	  if ((i % 8) == 0)
	    if ((group = mixer_ext_create_group (dev, 0, "/dev")) < 0)
	      return group;

	  sprintf (tmp, "@pcm%d", devc->portc[i].audiodev);
	}
      else
	{
	  /*
	   * Use front/rear/etc naming style
	   */
	  if ((i % 8) == 0)
	    if ((group = mixer_ext_create_group (dev, 0, "pcm")) < 0)
	      return group;

	  switch (i)
	    {
	    case 0:
	      strcpy (tmp, "main");
	      break;		/* Duplex device */
	    case 1:
	      strcpy (tmp, "front");
	      break;
	    case 2:
	      strcpy (tmp, "side");
	      break;
	    case 3:
	      strcpy (tmp, "C/L");
	      break;
	    case 4:
	      strcpy (tmp, "rear");
	      break;
	    }
	}


      if ((err = mixer_ext_create_control (dev, group, i, sblive_set_vol,
#ifdef TEST_3D
					   MIXT_3D,
#else
					   MIXT_SLIDER,
#endif
					   tmp,
					   100,
					   MIXF_PCMVOL | MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;
      if ((err = mixer_ext_create_control (dev, group,
					   i,
					   sblive_get_peak,
					   MIXT_STEREOPEAK,
					   "-", 144, MIXF_READABLE)) < 0)
	return err;
    }

  return 0;
}

static int
create_efx_mixer (int dev)
{
  sblive_devc *devc = mixer_devs[dev]->hw_devc;
  int group = 0, err = 0, i, mode;
  int group_created = 0;
  int typ, maxval;

  if (!devc->extinfo_loaded)
    {
      return 0;
    }

  if (devc->gpr == NULL)
    {
      return 0;
    }

  if (devc->mixer_group >= 0)
    mixer_ext_truncate (dev, devc->mixer_group);
  devc->mixer_group = -1;

  if (devc->gpr->ngpr >= MAX_GPR_PARMS)
    return OSS_EINVAL;

  for (i = 0; i < devc->gpr->ngpr; i++)
    {
      if (devc->gpr->gpr[i].num >= MAX_GPR)
	continue;

      typ = devc->gpr->gpr[i].type;

      if (typ == MIXT_GROUP)
	{
	  if ((group =
	       mixer_ext_create_group (dev, 0, devc->gpr->gpr[i].name)) < 0)
	    return group;

	  if (!group_created)
	    devc->mixer_group = group;
	  group_created = 1;
	  continue;
	}

#if 0
      if (!group_created)
	{
	  cmn_err (CE_WARN, "Mixer initialization sequence error\n");
	  return OSS_EINVAL;
	}
#endif
      mode = MIXF_READABLE;
      maxval = 144;

      switch (typ)
	{
	case EMU_MIXT_EQ1:
	case EMU_MIXT_EQ2:
	case EMU_MIXT_EQ3:
	case EMU_MIXT_EQ4:
	  {
	    mode |= MIXF_WRITEABLE;
	    maxval = 255;
	    typ = MIXT_SLIDER;
	  }
	  break;

	case MIXT_STEREOSLIDER:
	case MIXT_SLIDER:
	case MIXT_MONOSLIDER:
	  {
	    mode |= MIXF_WRITEABLE;
	    maxval = 100;
	  }
	  break;

	case MIXT_STEREOVU:
	  typ = MIXT_STEREOPEAK;
	  break;

	case MIXT_ONOFF:
	  {
	    mode |= MIXF_WRITEABLE;
	    maxval = 1;
	  }
	  break;
	}

      if (devc->gpr->gpr[i].name[0] == '_')
	{
	  /* Hidden control */
	  if (strcmp (devc->gpr->gpr[i].name, "_PASSTHROUGH") == 0)
	    {
	      int ctrl = devc->gpr->gpr[i].num;
	      devc->passthrough_gpr = ctrl;

	      sblive_write_reg (devc, ctrl + GPR0, 0, 1);
	      sblive_write_reg (devc, ctrl + GPR0 + 1, 0, 0);
	    }
	}
      else
	{
	  /* Visible control */
	  if ((err = mixer_ext_create_control (dev, group,
					       devc->gpr->gpr[i].num,
					       sblive_set_gpr, typ,
					       devc->gpr->gpr[i].name,
					       maxval, mode)) < 0)
	    return err;
	}

      if (!group_created)
	devc->mixer_group = err;
      group_created = 1;

      if (is_special_gpr (devc->gpr->gpr[i].num))
	{
	  sblive_set_gpr (dev, devc->gpr->gpr[i].num, SNDCTL_MIX_WRITE,
			  devc->gpr_values[devc->gpr->gpr[i].num]);
	}
      else
	{
	  sblive_set_gpr (dev, devc->gpr->gpr[i].num, SNDCTL_MIX_WRITE,
			  devc->gpr->gpr[i].def);
	}
    }
  return 0;
}

static int
mixer_ext_init (int dev)
{
  sblive_devc *devc = mixer_devs[dev]->hw_devc;

  devc->extinfo_loaded = 1;
  create_soft_mixer (dev);
  create_efx_mixer (dev);
  return 0;
}

static int
mixer_override (int dev, int audiodev, unsigned int cmd, int val)
{
  sblive_devc *devc = mixer_devs[dev]->hw_devc;
  switch (cmd)
    {
    case SOUND_MIXER_READ_VOLUME:
      return sblive_set_gpr (dev, GPR_VOLUME, SNDCTL_MIX_READ, 0);
      break;

    case SOUND_MIXER_WRITE_VOLUME:
      return sblive_set_gpr (dev, GPR_VOLUME, SNDCTL_MIX_WRITE, val);
      break;

    case SOUND_MIXER_READ_PCM:
      if (audiodev >= 0 && audiodev < num_audio_engines)
	{
	  sblive_portc *portc = NULL;
	  int i;

	  for (i = 0; i < devc->n_audiodevs && portc == NULL; i++)
	    if (devc->portc[i].audiodev == audiodev)
	      portc = &devc->portc[i];

	  if (portc == NULL)
	    return OSS_EIO;

	  return portc->playvol | (portc->playvol << 8);
	}
      return sblive_set_gpr (dev, GPR_PCM, SNDCTL_MIX_READ, 0);
      break;

    case SOUND_MIXER_WRITE_PCM:
      if (audiodev >= 0 && audiodev < num_audio_engines)
	{
	  sblive_portc *portc = NULL;
	  int i, left, right;

	  for (i = 0; i < devc->n_audiodevs && portc == NULL; i++)
	    if (devc->portc[i].audiodev == audiodev)
	      portc = &devc->portc[i];

	  if (portc == NULL)
	    return OSS_EIO;

	  left = val & 0xff;
	  right = (val >> 8) & 0xff;

	  if (left < 0)
	    left = 0;
	  if (right < 0)
	    right = 0;
	  if (left > 100)
	    left = 100;
	  if (right > 100)
	    right = 100;

	  if (right > left)
	    left = right;
	  portc->playvol = left;
	  mixer_devs[devc->mixer_dev]->modify_counter++;	/* Force update of mixer */
	  update_output_device (devc, portc);

	  return portc->playvol | (portc->playvol << 8);
	}
      return sblive_set_gpr (dev, GPR_PCM, SNDCTL_MIX_WRITE, val);
      break;
    }

  return 0;
}

static const char *port_names[] =
  { "front out", "side out", "center/lfe out", "rear out" };

static const __inline__ char *
get_port_name (sblive_devc * devc, int n)
{
  int max_names = 3;

  if (devc->feature_mask & SB_AUDIGY)
    max_names = 3;
  if (devc->feature_mask & SB_LIVE)
    max_names = 2;

  n = n - 1;

  if (n > max_names)
    return "extra out";

  return port_names[n];
}

static void
unload_mpu (sblive_devc * devc)
{
  if (devc == NULL)
    return;

  if (devc->feature_mask & SB_AUDIGY)
    unload_audigyuart (devc);
  else
    uart401_disable (&devc->uart401devc);
}

int
oss_sblive_attach (oss_device_t * osdev)
{
  sblive_devc *devc;
  int i, err;
  int frontdev = -1, ndevs = 0;
  int first_dev = -1;
  unsigned char pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  unsigned int subvendor;
  adev_p adev;
  extern int sblive_devices;

  int audiodevs_to_create = sblive_devices;

  char tmp[64];

  DDB (cmn_err (CE_CONT, "sblive_attach entered\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != PCI_VENDOR_ID_CREATIVE)
    {
      cmn_err (CE_WARN, "Unrecognized SB live vendor %x\n", vendor);
      return 0;
    }

  if (device != PCI_DEVICE_ID_SBLIVE
      && device != PCI_DEVICE_ID_AUDIGY
      && device != PCI_DEVICE_ID_AUDIGY_CARDBUS
      && device != PCI_DEVICE_ID_AUDIGYVALUE)
    {
      cmn_err (CE_WARN, "Unrecognized SB live device %x:%x\n", vendor,
	       device);
      return 0;
    }

#ifdef AUDIGY_ONLY
  if (device == PCI_DEVICE_ID_SBLIVE)
    {
      cmn_err (CE_CONT,
	       "Error: Due to hardware limitations SB Live is not\n");
      cmn_err (CE_CONT, "supported under this hardware architecture.\n");
      cmn_err (CE_CONT,
	       "Consider upgrading to SB Audigy which is supported.\n");
      return 0;
    }
#endif

  pci_read_config_dword (osdev, 0x2c, &subvendor);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_command &= ~(PCI_COMMAND_SERR | PCI_COMMAND_PARITY);
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);


  if (pci_ioaddr == 0)
    {
      cmn_err (CE_WARN, "I/O address not assigned by BIOS.\n");
      return 0;
    }

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS.\n");
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  memset (devc, 0, sizeof (*devc));
  devc->osdev = osdev;
  osdev->devc = devc;
  MUTEX_INIT (osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (osdev, devc->low_mutex, MH_DRV + 1);

  devc->emu_page_shift = 1;	/* Default page shift */

  devc->card_name = "Generic SB Live!";
  devc->subvendor = subvendor;

  devc->min_audiodevs = 5;	/* Audigy supports 7.1 */

  if (device == PCI_DEVICE_ID_AUDIGYVALUE)
    {
      /* SOLWAY subvendor id is 0x10211103 */
      if ((devc->subvendor == 0x10211102) || (devc->subvendor == 0x10211103))
	devc->card_name = "SB Audigy4";
      else
	devc->card_name = "SB Audigy2 Value";
      devc->feature_mask = SB_AUDIGY | SB_AUDIGY2 | SB_AUDIGY2VAL;
    }
  else if (device == PCI_DEVICE_ID_AUDIGY)
    {
      if (devc->subvendor >= 0x10021102 && devc->subvendor <= 0x20051102)
	    {
      	      devc->card_name = "SB Audigy2";
      	      devc->feature_mask = SB_AUDIGY | SB_AUDIGY2;
	    }
	  else
	    {
      	      devc->card_name = "SB Audigy";
      	      devc->feature_mask = SB_AUDIGY;
	    }
    }
  else if (device == PCI_DEVICE_ID_AUDIGY_CARDBUS)
    {
      if (devc->subvendor >= 0x10021102 && devc->subvendor <= 0x20051102)
	{
	  devc->card_name = "SB Audigy2 ZS Notebook";
	  devc->feature_mask = SB_AUDIGY | SB_AUDIGY2;
	}
      else
	{
	  devc->card_name = "SB Audigy";
	  devc->feature_mask = SB_AUDIGY;
	}
      DDB (cmn_err (CE_CONT,
		    "emu10k2 chip rev %d, pcb rev %d\n", pci_revision,
		    sblive_read_reg (devc, 0x5f, 0)));
    }
  else
    {
      devc->card_name = "SB Live";
      devc->feature_mask = SB_LIVE;
      devc->min_audiodevs = 4;	/* Just 5.1 */
    }

  if (audiodevs_to_create < devc->min_audiodevs)
    audiodevs_to_create = devc->min_audiodevs;
  if (audiodevs_to_create > MAX_ADEV)
    audiodevs_to_create = MAX_ADEV;

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  devc->base &= ~0x3;

  devc->gpr = NULL;
  oss_register_device (osdev, devc->card_name);

  devc->irq = pci_irq_line;

  devc->page_map = NULL;
  devc->vpage_map = NULL;
  devc->nr_pages = 0;
  devc->max_pages = 0;
  devc->max_mem = 0;
  devc->silent_page = NULL;
  devc->subvendor = subvendor;
  devc->passthrough_gpr = -1;

  if ((err = oss_register_interrupts (devc->osdev, 0, sbliveintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  devc->mixer_group = -1;
  devc->extinfo_loaded = 0;
  devc->autoreset = 1;
  devc->speaker_mode = SMODE_FRONTREAR;

/*
 * Init mixer
 */
  devc->mixer_dev =
    ac97_install (&devc->ac97devc, devc->card_name, ac97_read, ac97_write,
		  devc, devc->osdev);
  if (devc->mixer_dev < 0)
    {
      cmn_err (CE_WARN, "Mixer install failed - cannot continue\n");
      return 0;
    }

  devc->ac97devc.mixer_ext = 0;
  devc->ac97devc.spdifout_support = 0;
  devc->ac97devc.spdifin_support = 0;
  if (ac97_init_ext
      (devc->mixer_dev, &devc->ac97devc, mixer_ext_init, 100) < 0)
    {
      cmn_err (CE_WARN, "Mixer ext install failed\n");
    }

  /* first set the AC97 PCM to max - otherwise sound is too low */
  ac97_mixer_set (&devc->ac97devc, SOUND_MIXER_PCM, 100 | (100 << 8));

  ac97_remove_control (&devc->ac97devc, BOGUS_MIXER_CONTROLS, 0);
  ac97_override_control (&devc->ac97devc, SOUND_MIXER_VOLUME,
			 mixer_override, 100 | (100 << 8));
  ac97_override_control (&devc->ac97devc, SOUND_MIXER_PCM,
			 mixer_override, 100 | (100 << 8));

  attach_mpu (devc);

/*
 * Audio initialization
 */
  init_emu10k1 (devc);

  for (i = 0; i < audiodevs_to_create; i++)
    {
      sblive_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE;
      int fmts = 0;
      devc->n_audiodevs = i + 1;

      portc->memptr = devc->audio_memptr;
      devc->audio_memptr += (DMABUF_SIZE + 4095) & ~4095;

      if (devc->audio_memptr > AUDIO_MEMSIZE)
	{
	  cmn_err (CE_WARN, "Audio memory block exhausted (%d/%d)\n",
		   devc->audio_memptr, AUDIO_MEMSIZE);
	  return OSS_ENOSPC;
	}

      if (i == 0)
	{
	  strcpy (tmp, devc->card_name);
	  sprintf (tmp, "%s main", devc->card_name);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  sprintf (tmp, "%s %s", devc->card_name, get_port_name (devc, i));
	  caps |= ADEV_NOINPUT;
#if 0
	  if (i >= devc->min_audiodevs)
	    caps |= ADEV_HWMIX;
#endif
	  if (i >= devc->min_audiodevs + 1)
	    caps |= ADEV_SHADOW;
	}
      if ((devc->feature_mask & SB_AUDIGY) && i == audiodevs_to_create - 1)
	{
	  sprintf (tmp, "%s raw S/PDIF (output only)", devc->card_name);
	  caps &= ~(ADEV_SHADOW /* | ADEV_HWMIX*/);
	  caps |= ADEV_SPECIAL;
	  fmts |= AFMT_AC3;
	}
#if 0
      if (devc->feature_mask & SB_AUDIGY)
	caps |= ADEV_COLD;
#endif
      if ((portc->audiodev =
	   oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				 devc->osdev,
				 devc->osdev,
				 tmp,
				 &sblive_audio_driver,
				 sizeof (audiodrv_t), caps,
				 fmts | AFMT_U8 | AFMT_S16_LE, devc, -1)) < 0)
	{
	  portc->audiodev = -1;
	  return (i > 0);
	}
      else
	{
	  int x;

	  adev = audio_engines[portc->audiodev];
	  adev->nrates=0;
	  for (x = 0; speed_tab[x].speed != 0; x++)
	    adev->rates[adev->nrates++] = speed_tab[x].speed;

	  if (i == 0)
	    first_dev = portc->audiodev;
	  adev->devc = devc;
	  adev->portc = portc;
	  adev->rate_source = first_dev;
	  adev->mixer_dev = devc->mixer_dev;
	  adev->min_rate = 8000;
	  adev->max_rate = 48000;

	  if (!(devc->feature_mask & SB_AUDIGY))
	    {
	      /*
	       * SB Live supports only 31 PCI address bits
	       */
	      adev->dmabuf_maxaddr = MEMLIMIT_31BITS;
	    }

	  portc->mode = 0;
	  adev->oformat_mask |= AFMT_AC3;
	  portc->input_type = ITYPE_ANALOG;
	  if ((devc->feature_mask & SB_AUDIGY) && i == audiodevs_to_create - 1)
	    portc->input_type = ITYPE_SPDIF;
	  if (i == 1)
	    frontdev = portc->audiodev;
	  if (i > 0)
	    ndevs++;

	  portc->playvol = 100;
	  portc->playangle = 0;
	  portc->playdist = 50;
	  portc->vu_left = 0;
	  portc->vu_right = 0;
	  portc->audio_active = 0;
	  portc->voice_chn = i * 2;
	  portc->port_number = i;
	  devc->voice_busy[i * 2] = 1;
	  devc->voice_busy[i * 2 + 1] = 1;
	  portc->resetvol = 0;
	  if (devc->feature_mask & SB_LIVE)
	    {
/*
 * Do not enable vmix by default on Live! It would cause enormous 
 * latencies because emu10k1 doesn't have working full/half buffer DMA
 * interrupts.
 */
	      adev->vmix_flags = VMIX_MULTIFRAG;
	      adev->max_intrate = 50;
	      adev->min_block = 4096;
	    }
	  else
	    {
	      adev->max_fragments = 2;
	    }

	  /*
	   * Hide vmix main volume control and peak meters if no 
	   * real HW mixing devices are enabled.
	   */
#if 0
	  if (audiodevs_to_create <= devc->min_audiodevs)
	    adev->vmix_flags |= VMIX_NOMAINVOL;
#endif
	  adev->iformat_mask = AFMT_S16_LE;	/* No 8 bit recording */

	  if (i == 0)
	    {
	      if (devc->feature_mask & SB_LIVE)
		adev->magic = EMU10K1_MAGIC;
	      else
		adev->magic = EMU10K2_MAGIC;
	    }
#ifdef CONFIG_OSS_VMIX
          if (i == 0)
            vmix_attach_audiodev(devc->osdev, first_dev, -1, 0);
#endif
	}
      adev->mixer_dev = devc->mixer_dev;
    }

#ifdef USE_REMUX
  /* Install Remux (only 5.1 support for the time being) */
  sprintf (tmp, "%s 5.1 output device", devc->card_name);
  if (frontdev > 0 && ndevs >= 3)	/* Have enough devices for 5.1 */
    remux_install (tmp, devc->osdev, frontdev, frontdev + 1, frontdev + 2,
		   -1);
#endif

#ifndef NO_EMU10K1_SYNTH
  sblive_install_synth (devc);
#endif

  touch_mixer (devc->mixer_dev);
  init_effects (devc);

  return 1;
}

int
oss_sblive_detach (oss_device_t * osdev)
{
  sblive_devc *devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  devc = osdev->devc;

  OUTL (devc->osdev, 0, devc->base + 0x0c);	/* Intr enable (all off) */
  OUTL (devc->osdev,
	HCFG_LOCKSOUNDCACHE | HCFG_LOCKTANKCACHE_MASK |
	HCFG_MUTEBUTTONENABLE, devc->base + 0x14);

  sblive_write_reg (devc, ADCSR, 0, 0x0);
  sblive_write_reg (devc, ADCBA, 0, 0x0);
  sblive_write_reg (devc, ADCBA, 0, 0x0);

  sblive_write_reg (devc, PTBA, 0, 0);

#ifndef NO_EMU10K1_SYNTH
  sblive_remove_synth (devc);
#endif
  if (devc->page_map != NULL)
    CONTIG_FREE (devc->osdev, devc->page_map, devc->max_pages * 4, devc->page_map_dma_handle);
  if (devc->vpage_map != NULL)
    KERNEL_FREE (devc->vpage_map);
  if (devc->silent_page != NULL)
    CONTIG_FREE (devc->osdev, devc->silent_page, 4096, devc->silent_page_dma_handle);
  devc->max_pages = 0;
  devc->max_mem = 0;
  devc->page_map = NULL;
  devc->vpage_map = NULL;
  devc->silent_page = NULL;
  unload_mpu (devc);

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);
  oss_unregister_device (osdev);

  return 1;
}
