/*
 * Purpose: Support for the legacy (SADA) /dev/audio interfaces of Solaris
 *
 * Description:
 * This driver serves as a bridge between the OSS audio drivers and the
 * native audio support framework of Solaris.
 *
 * **** This driver supports only one device instance ****
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


#include "oss_sadasupport_cfg.h"
#include <oss_pci.h>
#include <sys/note.h>
#include <sys/audioio.h>
#include <sys/audio.h>
#include <sys/audiovar.h>
#include <sys/audio/audio_trace.h>
#include <sys/audio/audio_support.h>
#include <sys/audio/audio_src.h>
#include <sys/mixer.h>
#include <sys/audio/audio_mixer.h>
#ifdef SOL9
#include <sys/audio/am_src1.h>
#else
#include <sys/sunldi.h>
#endif

int sadasupport_rate = 48000;

static int do_byteswap = 0;

#ifdef SOL9
extern am_ad_src_entry_t am_src1;
#else
extern am_ad_src_entry_t am_src2;
#endif

#define	AUDIOS_DEV_NAME			"SUNW,oss"
#define	AUDIOS_DEV_CONFIG		"onboard1"
#define	AUDIOS_DEV_VERSION		"a"
#define	AUDIOS_NAME			"oss"
#define	AUDIOS_IDNUM			(0x6175)
#define	AUDIOS_MINPACKET			(0)
#define	AUDIOS_MAXPACKET			(1*1024)
#define	AUDIOS_HIWATER			(64*1024)
#define	AUDIOS_LOWATER			(32*1024)
#define AUDIOS_BUFFSIZE			(8*1024)

#define	AUDIOS_SAMPR5510			(5510)
#define	AUDIOS_SAMPR6620			(6620)
#define	AUDIOS_SAMPR8000			(8000)
#define	AUDIOS_SAMPR9600			(9600)
#define	AUDIOS_SAMPR11025			(11025)
#define	AUDIOS_SAMPR12000			(12000)
#define	AUDIOS_SAMPR16000			(16000)
#define	AUDIOS_SAMPR18900			(18900)
#define	AUDIOS_SAMPR22050			(22050)
#define	AUDIOS_SAMPR24000			(24000)
#define	AUDIOS_SAMPR27420			(27420)
#define	AUDIOS_SAMPR32000			(32000)
#define	AUDIOS_SAMPR33075			(33075)
#define	AUDIOS_SAMPR37800			(37800)
#define	AUDIOS_SAMPR44100			(44100)
#define	AUDIOS_SAMPR48000			(48000)

#define	AUDIOS_DEFAULT_SR			AUDIOS_SAMPR8000
#define	AUDIOS_DEFAULT_CH			AUDIO_CHANNELS_MONO
#define	AUDIOS_DEFAULT_PREC		AUDIO_PRECISION_8
#define	AUDIOS_DEFAULT_ENC		AUDIO_ENCODING_ULAW
#define	AUDIOS_DEFAULT_PGAIN		(AUDIO_MAX_GAIN * 3 / 4)
#define	AUDIOS_DEFAULT_RGAIN		(127)
#define	AUDIOS_DEFAULT_MONITOR_GAIN	(0)
#define	AUDIOS_DEFAULT_BAL		AUDIO_MID_BALANCE

/*
 * Entry point routine prototypes
 */
static int sadasupport_open (queue_t * q, dev_t * devp, int flag, int sflag,
			     cred_t * credp);
static int sadasupport_close (queue_t * q, int flag, cred_t * credp);
static int sadasupport_ad_set_config (audiohdl_t, int, int, int, int, int);
static int sadasupport_ad_set_format (audiohdl_t, int, int, int, int, int,
				      int);
static int sadasupport_ad_start_play (audiohdl_t, int);
static void sadasupport_ad_pause_play (audiohdl_t, int);
static void sadasupport_ad_stop_play (audiohdl_t, int);
static int sadasupport_ad_start_record (audiohdl_t, int);
static void sadasupport_ad_stop_record (audiohdl_t, int);

/* now, only support stereo */
static uint_t sadasupport_channels[] = {
  AUDIO_CHANNELS_STEREO,
  0
};

static am_ad_cap_comb_t sadasupport_combinations[] = {
  {AUDIO_PRECISION_16, AUDIO_ENCODING_LINEAR},
  {0}
};

static uint_t sadasupport_mixer_srs[] = {
  AUDIOS_SAMPR8000, AUDIOS_SAMPR48000, 0
};

#ifdef SOL9
/* don't filter the higher sample rates, this causes the high to be lost */
static am_ad_src1_info_t sadasupport_play_sample_rates_info[] = {
  {AUDIOS_SAMPR8000, AUDIOS_SAMPR48000, 2,	/* up 6, down 1 */
   3, (2 | AM_SRC1_FILTER), 0, 0, 1, 1, 0, 0, 3},
  {AUDIOS_SAMPR9600, AUDIOS_SAMPR48000, 1,	/* up 5, down 1 */
   (5 | AM_SRC1_FILTER), 0, 0, 0, 1, 0, 0, 0, 3},
  {AUDIOS_SAMPR11025, AUDIOS_SAMPR48000, 3,	/* up 640, down 147 */
   10, 8, (8 | AM_SRC1_FILTER), 0, 7, 7, 3, 0, 3},
  {AUDIOS_SAMPR12000, AUDIOS_SAMPR48000, 1,	/* up 4, down 1 */
   (4 | AM_SRC1_FILTER), 0, 0, 0, 1, 0, 0, 0, 3},
  {AUDIOS_SAMPR16000, AUDIOS_SAMPR48000, 1,	/* up 3, down 1 */
   (3 | AM_SRC1_FILTER), 0, 0, 0, 1, 0, 0, 0, 3},
  {AUDIOS_SAMPR18900, AUDIOS_SAMPR48000, 3,	/* up 160, down 63 */
   8, 5, (4 | AM_SRC1_FILTER), 0, 7, 3, 3, 0, 3},
  {AUDIOS_SAMPR22050, AUDIOS_SAMPR48000, 3,	/* up 320, down 147 */
   10, 8, (4 | AM_SRC1_FILTER), 0, 7, 7, 3, 0, 3},
  {AUDIOS_SAMPR24000, AUDIOS_SAMPR48000, 1,	/* up 2, down 1 */
   (2 | AM_SRC1_FILTER), 0, 0, 0, 1, 0, 0, 0, 3},
  {AUDIOS_SAMPR32000, AUDIOS_SAMPR48000, 1,	/* up 3, down 2 */
   (3 | AM_SRC1_FILTER), 0, 0, 0, 2, 0, 0, 0, 3},
  {AUDIOS_SAMPR33075, AUDIOS_SAMPR48000, 4,	/* up 640, down 441 */
   8, 5, 4, 4, 7, 7, 3, 3, 3},
  {AUDIOS_SAMPR37800, AUDIOS_SAMPR48000, 3,	/* up 80, down 63 */
   5, 4, 4, 0, 7, 3, 3, 0, 3},
  {AUDIOS_SAMPR44100, AUDIOS_SAMPR48000, 3,	/* up 160, down 147 */
   8, 5, 4, 0, 7, 7, 3, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR48000, 1,	/* up 1, down 1 */
   1, 0, 0, 0, 1, 0, 0, 0, 3},
  {0}
};

static am_ad_src1_info_t sadasupport_record_sample_rates_info[] = {
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR8000, 1,	/* up 1, down 6 */
   1, 0, 0, 0, 6, 0, 0, 0, 0},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR9600, 1,	/* up 1, down 5 */
   1, 0, 0, 0, 5, 0, 0, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR11025, 3,	/* up 147, down 640 */
   7, 7, (3 | AM_SRC1_FILTER), 0, 10, 8, 8, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR12000, 1,	/* up 1, down 4 */
   1, 0, 0, 0, 4, 0, 0, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR16000, 1,	/* up 1, down 3 */
   1, 0, 0, 0, 3, 0, 0, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR18900, 3,	/* up 63, down 160 */
   7, 3, (3 | AM_SRC1_FILTER), 0, 8, 5, 4, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR22050, 3,	/* up 147, down 320 */
   7, 7, (3 | AM_SRC1_FILTER), 0, 10, 8, 4, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR24000, 1,	/* up 1, down 2 */
   1, 0, 0, 0, 2, 0, 0, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR32000, 1,	/* up 2, down 3 */
   (2 | AM_SRC1_FILTER), 0, 0, 0, 3, 0, 0, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR33075, 4,	/* up 441, down 640 */
   7, 7, 3, 3, 8, 5, 4, 4, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR37800, 3,	/* up 63, down 80 */
   7, 3, 3, 0, 5, 4, 4, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR44100, 3,	/* up 147, down 160 */
   7, 7, 3, 0, 8, 5, 4, 0, 3},
  {AUDIOS_SAMPR48000, AUDIOS_SAMPR48000, 3,	/* up 1, down 1 */
   1, 0, 0, 0, 1, 0, 0, 0, 3},
  {0}
};
#endif

#if 0
static uint_t sadasupport_compat_srs[] = {
  AUDIOS_SAMPR5510, AUDIOS_SAMPR6620, AUDIOS_SAMPR8000,
  AUDIOS_SAMPR9600, AUDIOS_SAMPR11025, AUDIOS_SAMPR12000,
  AUDIOS_SAMPR16000, AUDIOS_SAMPR18900, AUDIOS_SAMPR22050,
  AUDIOS_SAMPR24000, AUDIOS_SAMPR27420, AUDIOS_SAMPR32000,
  AUDIOS_SAMPR33075, AUDIOS_SAMPR37800, AUDIOS_SAMPR44100,
  AUDIOS_SAMPR48000, 0
};
#endif

static am_ad_sample_rates_t sadasupport_mixer_sample_rates = {
  MIXER_SRS_FLAG_SR_LIMITS,
  sadasupport_mixer_srs
};

#if 0
static am_ad_sample_rates_t sadasupport_compat_sample_rates = {
  MIXER_SRS_FLAG_SR_NOT_LIMITS,
  sadasupport_compat_srs
};
#endif

typedef struct
{
  oss_device_t *osdev;
  kmutex_t inst_lock;

/*
 * Fields related with the OSS audio device
 */

  volatile int audio_busy;
  int audio_mode;
  int oss_audiodev;
  int play_frag;
  int rec_frag;
  int start_flags;
  int sample_rate;
  int mute;			/* Play silence if mute is set */

/*
 * Fields related with the audiosup and mixer modules.
 */
  audiohdl_t audio_handle;
  am_ad_info_t ad_info;		/* audio device info state */
  uint16_t vol_bits_mask;	/* bits used to ctrl volume */

  audio_info_t audios_defaults;	/* default state for dev */
  audio_device_t audios_dev_info;	/* audio device info state */
#ifndef SOL9
  ldi_handle_t lh;
  ldi_ident_t li;
#endif
} sadasupport_devc;

static sadasupport_devc *devc = NULL;
static struct fileinfo tmp_file = { 0 };

static am_ad_entry_t sadasupport_entry = {
  NULL,				/* ad_setup() */
  NULL,				/* ad_teardown() */
  sadasupport_ad_set_config,	/* ad_set_config() */
  sadasupport_ad_set_format,	/* ad_set_format() */
  sadasupport_ad_start_play,	/* ad_start_play() */
  sadasupport_ad_pause_play,	/* ad_pause_play() */
  sadasupport_ad_stop_play,	/* ad_stop_play() */
  sadasupport_ad_start_record,	/* ad_start_record() */
  sadasupport_ad_stop_record,	/* ad_stop_record() */
  NULL,				/* ad_ioctl() */
  NULL				/* ad_iocdata() */
};

/*
 * STREAMS structures
 */

/* STREAMS driver id and limit value struct */
static struct module_info sadasupport_modinfo = {
  AUDIOS_IDNUM,			/* module ID number */
  AUDIOS_NAME,			/* module name */
  AUDIOS_MINPACKET,		/* minimum packet size */
  AUDIOS_MAXPACKET,		/* maximum packet size */
  AUDIOS_HIWATER,		/* high water mark */
  AUDIOS_LOWATER,		/* low water mark */
};

/* STREAMS queue processing procedures structures */
/* read queue */
static struct qinit sadasupport_rqueue = {
  audio_sup_rput,		/* put procedure */
  audio_sup_rsvc,		/* service procedure */
  sadasupport_open,		/* open procedure */
  sadasupport_close,		/* close procedure */
  NULL,				/* unused */
  &sadasupport_modinfo,		/* module parameters */
  NULL				/* module statistics */
};

/* write queue */
static struct qinit sadasupport_wqueue = {
  audio_sup_wput,		/* write procedure */
  audio_sup_wsvc,		/* service procedure */
  NULL,				/* open procedure */
  NULL,				/* close procedure */
  NULL,				/* unused */
  &sadasupport_modinfo,		/* module parameters */
  NULL				/* module statistics */
};

/* STREAMS entity declaration structure */
struct streamtab oss_sadasupport_str_info = {
  &sadasupport_rqueue,		/* read queue */
  &sadasupport_wqueue,		/* write queue */
  NULL,				/* mux lower read queue */
  NULL,				/* mux lower write queue */
};

/*ARGSUSED*/
static void
input_callback (int dev, int parm)
{
  dmap_p dmap;
  adev_p adev;
  unsigned char *buf;
  int samples, i;
  oss_native_word flags;

  adev = audio_engines[dev];
  dmap = adev->dmap_in;

  if (dmap->dmabuf == NULL)
    {
      cmn_err (CE_WARN, "Dmabuf==NULL\n");
      return;
    }

  MUTEX_ENTER (dmap->mutex, flags);

  samples = dmap->fragment_size / 2;	/* Number of 16 bit samples */

  buf = dmap->dmabuf + devc->rec_frag * dmap->fragment_size;

  if (dmap->nfrags == 0)
    {
      cmn_err (CE_WARN, "Nfrags==0\n");
      MUTEX_EXIT (dmap->mutex, flags);
      return;
    }

  devc->rec_frag = (devc->rec_frag + 1) % dmap->nfrags;
  dmap->user_counter += dmap->fragment_size;
  MUTEX_EXIT (dmap->mutex, flags);

  /* Perform byte swapping (if necessary) */
  if (do_byteswap)
    for (i = 0; i < samples * 2; i += 2)
      {
	unsigned char tmp;

	tmp = buf[i];
	buf[i] = buf[i + 1];
	buf[i + 1] = tmp;
      }
  am_send_audio (devc->audio_handle, buf, AUDIO_NO_CHANNEL, samples);

}

static int fill_play_buf (sadasupport_devc * devc);

/*ARGSUSED*/
static void
output_callback (int dev, int parm)
{
  fill_play_buf (devc);
}

static int
setup_device (sadasupport_devc * devc)
{
  int fmt = AFMT_S16_NE;
  int ch = 2;
  int rate = devc->sample_rate;
  int mode;

  devc->mute = 0;		/* Unmute every time the device gets opened */

  mode = 0;
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_COOKEDMODE,
		   (ioctl_arg) & mode);

#if 0
  int frag;

  //frag=0x0002000d;      /* Two 8k fragments */
  frag = 0x0008000a;		/* 1k fragments */
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETFRAGMENT,
		   (ioctl_arg) & frag);
#endif

  do_byteswap = 0;
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETFMT,
		   (ioctl_arg) & fmt);
  if (fmt != AFMT_S16_NE)
    {
      fmt = AFMT_S16_OE;
      oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETFMT,
		       (ioctl_arg) & fmt);
      if (fmt != AFMT_S16_OE)
	{
	  cmn_err (CE_WARN, "Internal error (format=%x)\n", fmt);
	  return AUDIO_FAILURE;
	}
      do_byteswap = 1;
    }

  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_CHANNELS,
		   (ioctl_arg) & ch);
  if (ch != 2)
    {
      cmn_err (CE_WARN, "Internal error (channels=%d)\n", ch);
      return AUDIO_FAILURE;
    }

  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SPEED,
		   (ioctl_arg) & rate);
  if (rate != devc->sample_rate)
    {
/*
 * Disabling SADA mixing/src may cause problems if the application wants
 * to use rate that is not supported by the device itself. In that case
 * we need to switch to the rate returned by the device.
 */
      cmn_err (CE_WARN, "Returned sample rate %d is different than requested %d)\n", rate,
	       devc->sample_rate);
      cmn_err (CE_CONT, "Switching to %d\n", rate);
      devc->sample_rate = rate;
      // return AUDIO_FAILURE;
    }

  return AUDIO_SUCCESS;
}

#ifndef SOL9
static int
sadasupport_open (queue_t * q, dev_t * devp, int flag, int sflag,
		  cred_t * credp)
{
  int retval, i;
  int devnum;
  int dev = *devp;
  char *cmd;
  unsigned int bl_flags = 0;
  int oss_mode = 0;

  DDB (cmn_err
       (CE_CONT, "sadasupport_open(dev=%x, flag=%x, sflag=%x)\n",
	dev, flag, sflag));

  if (devc == NULL)
    return EIO;

  oss_mode = OPEN_READWRITE;

  mutex_enter (&devc->inst_lock);

  if (devc->audio_busy++ == 0)
    {
      oss_audioinfo ai;
      int err;

      mutex_exit (&devc->inst_lock);

      tmp_file.acc_flags = 0;
      tmp_file.mode = oss_mode;

      devc->audio_mode = tmp_file.mode;

      DDB (cmn_err (CE_CONT, "Opening /dev/dsp (mode=%x)\n", tmp_file.mode));

      if (ldi_ident_from_dev (dev, &devc->li) != 0)
	{
	  cmn_err (CE_WARN, "ldi_ident_from_dev failed\n");
	  devc->audio_busy--;
	  return EIO;
	}

      if ((err =
	   ldi_open_by_name ("/dev/dsp", FWRITE | FREAD, credp, &devc->lh,
			     devc->li)) != 0)
	{
	  cmn_err (CE_WARN,
		   "ldi_open_by_name(\"/dev/dsp\") failed, errno=%d\n", err);
	  devc->audio_busy--;
	  return -err;
	}

      ai.dev = -1;
      if ((err =
	   ldi_ioctl (devc->lh, SNDCTL_ENGINEINFO, (intptr_t) & ai, FKIOCTL,
		      credp, &retval)) != 0)
	{
	  ldi_close (devc->lh, FREAD | FWRITE, credp);
	  ldi_ident_release (devc->li);
	  cmn_err (CE_WARN, "ldi_ioctl(SNDCTL_AUDIOINFO) failed, err=%d\n",
		   err);
	  devc->audio_busy--;
	  return err;
	}

      devnum = ai.dev;

      DDB (cmn_err
	   (CE_CONT, "Opened OSS audio engine %d:%s\n", devnum, ai.name));

      if (devnum < 0 || devnum >= num_audio_engines)
	{
	  cmn_err (CE_WARN, "OSS audio engine number %d is out of range.\n",
		   devnum);
	  ldi_close (devc->lh, FREAD | FWRITE, credp);
	  ldi_ident_release (devc->li);
	  devc->audio_busy--;
	  return ENXIO;
	}

      devc->oss_audiodev = devnum;

      if (tmp_file.mode & OPEN_READ)
	audio_engines[devnum]->dmap_in->audio_callback = input_callback;
      if (tmp_file.mode & OPEN_WRITE)
	audio_engines[devnum]->dmap_out->audio_callback = output_callback;
      strcpy (audio_engines[devnum]->cmd, "SADA");
      strcpy (audio_engines[devnum]->label, "SADA");
      audio_engines[devnum]->pid = 0;

      if ((retval = setup_device (devc)) != AUDIO_SUCCESS)
	{
	  cmn_err (CE_NOTE, "setup_device failed\n");
	  mutex_enter (&devc->inst_lock);
	  if (--devc->audio_busy == 0)
	    {
	      ldi_close (devc->lh, FREAD | FWRITE, credp);
	      ldi_ident_release (devc->li);
	      mutex_exit (&devc->inst_lock);
	    }
	  else
	    mutex_exit (&devc->inst_lock);

	  return EIO;
	}
    }
  else
    {
      if (oss_mode & ~devc->audio_mode)
	{
	  cmn_err (CE_NOTE, "Conflicting access modes\n");

	  if (--devc->audio_busy == 0)
	    {
	      mutex_exit (&devc->inst_lock);
	      ldi_close (devc->lh, FREAD | FWRITE, credp);
	      ldi_ident_release (devc->li);
	      return EBUSY;
	    }

	  mutex_exit (&devc->inst_lock);
	  return EBUSY;
	}

      mutex_exit (&devc->inst_lock);

    }

  DDB (cmn_err (CE_CONT, "Open count %d\n", devc->audio_busy));

  DDB (cmn_err
       (CE_CONT, "audio_sup_open(q=%p, dev=%x, flag=%x, sflag=%x)\n", q, dev,
	flag, sflag));
  if ((retval = audio_sup_open (q, devp, flag, sflag, credp)) != 0)
    {
      cmn_err (CE_NOTE, "audio_sup_open() returned %d\n", retval);
      mutex_enter (&devc->inst_lock);
      if (--devc->audio_busy == 0)
	{
	  mutex_exit (&devc->inst_lock);
	  ldi_close (devc->lh, FREAD | FWRITE, credp);
	  ldi_ident_release (devc->li);
	}
      else
	mutex_exit (&devc->inst_lock);
    }

  return retval;
}

static int
sadasupport_close (queue_t * q, int flag, cred_t * credp)
{
  int retval, count;

  DDB (cmn_err (CE_CONT, "sadasupport_close(q=%p, flag=%x)\n", q, flag));

  if (!devc->audio_busy)
    cmn_err (CE_WARN, "Close while not busy.\n");

  retval = audio_sup_close (q, flag, credp);

  mutex_enter (&devc->inst_lock);
  count = --devc->audio_busy;
  mutex_exit (&devc->inst_lock);

  DDB (cmn_err (CE_CONT, "Open count (close) %d\n", count));
  if (count == 0)
    {
      DDB (cmn_err
	   (CE_CONT, "Closing OSS audiodev %d\n", devc->oss_audiodev));
      ldi_close (devc->lh, FREAD | FWRITE, credp);
      ldi_ident_release (devc->li);
      devc->audio_mode = 0;
    }

  return retval;
}
#else
#include "sadasupport_sol9.h"
#endif

static void
set_port (sadasupport_devc * devc, int dir, int port)
{
  int recdev = SOUND_MASK_MIC;

  if (dir == AUDIO_PLAY)	/* Don't support this */
    return;

  switch (port)
    {
    case AUDIO_MICROPHONE:
      recdev = SOUND_MASK_MIC;
      break;
    case AUDIO_LINE_IN:
      recdev = SOUND_MASK_LINE;
      break;
    case AUDIO_CD:
      recdev = SOUND_MASK_CD;
      break;
    }

  oss_audio_ioctl (devc->oss_audiodev, NULL, SOUND_MIXER_WRITE_RECSRC,
		   (ioctl_arg) & recdev);
  /* Let's hope that worked */
}

static void
set_gain (sadasupport_devc * devc, int dir, int gain, int channel)
{
  int cmd;
  int val;

  if (gain < 0)
    gain = 0;
  if (gain > 255)
    gain = 255;

  gain = (gain * 100) / 255;

  if (dir == AUDIO_PLAY)
    cmd = SNDCTL_DSP_GETPLAYVOL;
  else
    cmd = SNDCTL_DSP_GETRECVOL;

  if (oss_audio_ioctl (devc->oss_audiodev, NULL, cmd, (ioctl_arg) & val) < 0)
    return;

  switch (channel)
    {
    case 0:			/* Left channel */
      val = (val & 0xff00) | gain;
      break;

    case 1:			/* Right channel */
      val = (val & 0x00ff) | (gain << 8);
      break;
    }

  if (dir == AUDIO_PLAY)
    cmd = SNDCTL_DSP_SETPLAYVOL;
  else
    cmd = SNDCTL_DSP_SETRECVOL;

  oss_audio_ioctl (devc->oss_audiodev, NULL, cmd, (ioctl_arg) & val);
}

/*ARGSUSED*/
static void
set_monitor_gain (sadasupport_devc * devc, int gain)
{
  /* NOP. SADA applications are not allowed to change monitor gain. */
}

/*ARGSUSED*/
static int
sadasupport_ad_set_config (audiohdl_t ahandle, int stream, int command,
			   int dir, int arg1, int arg2)
{
/*
 * We do not support most gain settings. They are incompatible with the way
 * how OSS 4.0 (and later) works. 
 */

#if 0
  typedef struct
  {
    uint_t cmd;
    char *name;
  } cmds_t;

  static const cmds_t cmds[] = {
    {AM_SET_GAIN, "AM_SET_GAIN"},
    {AM_SET_GAIN_BAL, "AM_SET_GAIN_BAL"},
    {AM_SET_PORT, "AM_SET_PORT"},
    {AM_SET_MONITOR_GAIN, "AM_SET_MONITOR_GAIN"},
    {AM_OUTPUT_MUTE, "AM_OUTPUT_MUTE"},
    {AM_MONO_MIC, "AM_MONO_MIC"},
    {AM_MIC_BOOST, "AM_MIC_BOOST"},
    {AM_BASS_BOOST, "AM_BASS_BOOST"},
    {AM_MID_BOOST, "AM_MID_BOOST"},
    {AM_TREBLE_BOOST, "AM_TREBLE_BOOST"},
    {AM_LOUDNESS, "AM_LOUDNESS"},
    {AM_SET_DIAG_MODE, "AM_SET_DIAG_MODE"},
    {0, NULL}
  };
#endif

  if (devc->audio_mode == 0)
    return AUDIO_SUCCESS;

  switch (command)
    {
    case AM_SET_PORT:
      set_port (devc, dir, arg1);
      break;

    case AM_SET_GAIN_BAL:
      return AUDIO_FAILURE;
      break;

    case AM_SET_GAIN:
      set_gain (devc, dir, arg1, arg2);
      break;

    case AM_SET_MONITOR_GAIN:
      set_monitor_gain (devc, arg1);
      break;

    case AM_OUTPUT_MUTE:
      devc->mute = arg1;
      break;

    default:
#if 0
      int i;

      cmn_err (CE_CONT,
	       "sadasupport_ad_set_config(stream=%d, cmd=%x, dir=%d, arg1=%x, arg2=%x)\n",
	       stream, command, dir, arg1, arg2);

      for (i = 0; cmds[i].cmd != 0; i++)
	if (cmds[i].cmd == command)
	  {
	    cmn_err (CE_CONT, "   Unsupported mixer command =%s\n",
		     cmds[i].name);
	    break;
	  }
#endif
      break;
    }
  return AUDIO_SUCCESS;
}

/*ARGSUSED*/
static int
sadasupport_ad_set_format (audiohdl_t ahandle, int stream, int dir,
			   int sample_rate, int channels, int precision,
			   int encoding)
{

  DDB (cmn_err (CE_CONT,
		"sadasupport_ad_set_format(stream=%d, dir=%d, sr=%d, ch=%d, prec=%d, enc=%d)\n",
		stream, dir, sample_rate, channels, precision, encoding));

  if (precision != AUDIO_PRECISION_16)
    {
      cmn_err (CE_WARN, "Sample size must be 16 bits.\n");
      return AUDIO_FAILURE;
    }

  if (channels != AUDIO_CHANNELS_STEREO)
    {
      cmn_err (CE_WARN, "Channels must be stereo.\n");
      return AUDIO_FAILURE;
    }

  if (encoding != AUDIO_ENCODING_LINEAR)
    {
      cmn_err (CE_WARN, "Channels must be stereo.\n");
      return AUDIO_FAILURE;
    }

  devc->sample_rate = sample_rate;

  return AUDIO_SUCCESS;
}

static int
fill_play_buf (sadasupport_devc * devc)
{
  int rs;
  int dev;
  dmap_p dmap;
  adev_p adev;
  unsigned char *buf;
  int samples, i;
  oss_native_word flags;

  dev = devc->oss_audiodev;

  adev = audio_engines[dev];
  dmap = adev->dmap_out;

  if (dmap->dmabuf == NULL)
    {
      cmn_err (CE_WARN, "Dmabuf==NULL\n");
      return AUDIO_FAILURE;
    }

  MUTEX_ENTER (dmap->mutex, flags);

  samples = dmap->fragment_size / 2;	/* Number of 16 bit samples */

  buf = dmap->dmabuf + devc->play_frag * dmap->fragment_size;

  if (dmap->nfrags == 0)
    {
      cmn_err (CE_WARN, "Nfrags==0\n");
      MUTEX_EXIT (dmap->mutex, flags);
      return AUDIO_FAILURE;
    }

  rs = am_get_audio (devc->audio_handle, buf, AUDIO_NO_CHANNEL, samples);

  if (devc->mute || rs == 0)	/* Device paused or muted */
    {
      memset (buf, 0, dmap->fragment_size);
    }
  else
    {
      /* Perform byte swapping (if necessary) */
      if (do_byteswap)
	for (i = 0; i < samples * 2; i += 2)
	  {
	    unsigned char tmp;

	    tmp = buf[i];
	    buf[i] = buf[i + 1];
	    buf[i + 1] = tmp;
	  }
    }

  devc->play_frag = (devc->play_frag + 1) % dmap->nfrags;
  dmap->user_counter += dmap->fragment_size;
  MUTEX_EXIT (dmap->mutex, flags);

  return rs;
}

/*ARGSUSED*/
static int
sadasupport_ad_start_play (audiohdl_t ahandle, int stream)
{
  int dev;
  dmap_p dmap;
  adev_p adev;

  dev = devc->oss_audiodev;

  if (dev < 0 || dev >= num_audio_engines)
    {
      cmn_err (CE_CONT, "Bad oss_audiodev %d\n", dev);
      return AUDIO_FAILURE;
    }

  adev = audio_engines[dev];
  dmap = adev->dmap_out;

  mutex_enter (&devc->inst_lock);
/*
 * Already playing?
 */
  if (devc->start_flags & PCM_ENABLE_OUTPUT)
    {
      mutex_exit (&devc->inst_lock);
      return 0;			/* All buffers are full at this moment */
    }

  DDB (cmn_err (CE_CONT, "sadasupport_ad_start_play(stream=%d)\n", stream));

/*
 * Skip the first fragment (fill with silence) to avoid underruns
 */
  devc->play_frag = 1;
  dmap->user_counter = dmap->fragment_size;

/*
 * Trigger the play engine
 */
  dmap->dma_mode = PCM_ENABLE_OUTPUT;
  devc->start_flags &= ~PCM_ENABLE_OUTPUT;
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & devc->start_flags);
  devc->start_flags |= PCM_ENABLE_OUTPUT;
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & devc->start_flags);

  //dmap->user_counter += dmap->fragment_size;
  devc->audios_defaults.play.buffer_size = dmap->fragment_size;
  devc->ad_info.ad_play.ad_bsize = dmap->fragment_size;

  mutex_exit (&devc->inst_lock);
  return fill_play_buf (devc);
}

/*ARGSUSED*/
static void
sadasupport_ad_stop_play (audiohdl_t ahandle, int stream)
{
  DDB (cmn_err (CE_CONT, "sadasupport_ad_stop_play (stream=%d)\n", stream));
  mutex_enter (&devc->inst_lock);
  devc->start_flags &= ~PCM_ENABLE_OUTPUT;
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & devc->start_flags);
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_HALT_OUTPUT,
		   (ioctl_arg) NULL);
  mutex_exit (&devc->inst_lock);
}

static void
sadasupport_ad_pause_play (audiohdl_t ahandle, int stream)
{
  DDB (cmn_err (CE_CONT, "sadasupport_ad_pause_play\n"));

  sadasupport_ad_stop_play (ahandle, stream);
}

/*ARGSUSED*/
static int
sadasupport_ad_start_record (audiohdl_t ahandle, int stream)
{
  int dev;
  dmap_p dmap;
  adev_p adev;

  dev = devc->oss_audiodev;

  if (dev < 0 || dev >= num_audio_engines)
    {
      cmn_err (CE_CONT, "Bad oss_audiodev %d\n", dev);
      return AUDIO_FAILURE;
    }

  mutex_enter (&devc->inst_lock);
  adev = audio_engines[dev];
  dmap = adev->dmap_in;

/*
 * Already Recording?
 */
  if (devc->start_flags & PCM_ENABLE_INPUT)
    {
      mutex_exit (&devc->inst_lock);
      return AUDIO_SUCCESS;	/* All buffers are full at this moment */
    }

  DDB (cmn_err (CE_CONT, "sadasupport_ad_start_record(stream=%d)\n", stream));

/*
 * Skip the first fragment (fill with silence) to avoid underruns
 */
  devc->rec_frag = 0;
  dmap->user_counter = 0;

/*
 * Trigger the rec engine
 */
  dmap->dma_mode = PCM_ENABLE_INPUT;
  devc->start_flags &= ~PCM_ENABLE_INPUT;
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & devc->start_flags);
  devc->start_flags |= PCM_ENABLE_INPUT;
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & devc->start_flags);

  devc->audios_defaults.record.buffer_size = dmap->fragment_size;
  devc->ad_info.ad_record.ad_bsize = dmap->fragment_size;

  mutex_exit (&devc->inst_lock);
  return AUDIO_SUCCESS;
}

/*ARGSUSED*/
static void
sadasupport_ad_stop_record (audiohdl_t ahandle, int stream)
{
  DDB (cmn_err (CE_CONT, "sadasupport_ad_stop_record(stream=%d)\n", stream));
  mutex_enter (&devc->inst_lock);
  devc->start_flags &= ~PCM_ENABLE_INPUT;
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & devc->start_flags);
  oss_audio_ioctl (devc->oss_audiodev, NULL, SNDCTL_DSP_HALT_INPUT,
		   (ioctl_arg) NULL);
  mutex_exit (&devc->inst_lock);
}

static int
sadasupport_init_state (sadasupport_devc * devc, dev_info_t * dip)
{
  int rints = 175;
  int pints = 175;
  int cdrom;
  int mode;

  devc->vol_bits_mask = 5;

  cdrom = ddi_prop_get_int (DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
			    "cdrom", 0);

  /* get the mode from the .conf file */
  if (ddi_prop_get_int (DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
			"mixer-mode", 1))
    {
      mode = AM_MIXER_MODE;
    }
  else
    {
      mode = AM_COMPAT_MODE;
    }

  /* fill in the device default state */
  devc->audios_defaults.play.sample_rate = AUDIOS_DEFAULT_SR;
  devc->audios_defaults.play.channels = AUDIOS_DEFAULT_CH;
  devc->audios_defaults.play.precision = AUDIOS_DEFAULT_PREC;
  devc->audios_defaults.play.encoding = AUDIOS_DEFAULT_ENC;
  devc->audios_defaults.play.gain = AUDIOS_DEFAULT_PGAIN;
  devc->audios_defaults.play.port = AUDIO_SPEAKER | AUDIO_LINE_OUT;
  devc->audios_defaults.play.avail_ports = AUDIO_SPEAKER | AUDIO_LINE_OUT;
  devc->audios_defaults.play.mod_ports = AUDIO_SPEAKER | AUDIO_LINE_OUT;
  devc->audios_defaults.play.buffer_size = AUDIOS_BUFFSIZE;
  devc->audios_defaults.play.balance = AUDIOS_DEFAULT_BAL;

  devc->audios_defaults.record.sample_rate = AUDIOS_DEFAULT_SR;
  devc->audios_defaults.record.channels = AUDIOS_DEFAULT_CH;
  devc->audios_defaults.record.precision = AUDIOS_DEFAULT_PREC;
  devc->audios_defaults.record.encoding = AUDIOS_DEFAULT_ENC;
  devc->audios_defaults.record.gain = AUDIOS_DEFAULT_PGAIN;
  devc->audios_defaults.record.port = AUDIO_MICROPHONE;
  devc->audios_defaults.record.avail_ports =
    AUDIO_MICROPHONE | AUDIO_LINE_IN | AUDIO_CD;
  devc->audios_defaults.record.mod_ports =
    AUDIO_MICROPHONE | AUDIO_LINE_IN | AUDIO_CD;
  devc->audios_defaults.record.buffer_size = AUDIOS_BUFFSIZE;
  devc->audios_defaults.record.balance = AUDIOS_DEFAULT_BAL;

  devc->audios_defaults.monitor_gain = AUDIOS_DEFAULT_MONITOR_GAIN;
  devc->audios_defaults.output_muted = B_FALSE;
  devc->audios_defaults.ref_cnt = B_FALSE;
  devc->audios_defaults.hw_features =
    AUDIO_HWFEATURE_DUPLEX | AUDIO_HWFEATURE_PLAY |
    AUDIO_HWFEATURE_IN2OUT | AUDIO_HWFEATURE_RECORD;
  devc->audios_defaults.sw_features = AUDIO_SWFEATURE_MIXER;

  if (cdrom)
    {
      devc->audios_defaults.record.avail_ports |= AUDIO_CD;
      devc->audios_defaults.record.mod_ports |= AUDIO_CD;
    }

#if 0
  devc->audios_psample_rate = devc->audios_defaults.play.sample_rate;
  devc->audios_pchannels = devc->audios_defaults.play.channels;
  devc->audios_pprecision = devc->audios_defaults.play.precision;
  devc->audios_csample_rate = devc->audios_defaults.record.sample_rate;
  devc->audios_cchannels = devc->audios_defaults.record.channels;
  devc->audios_cprecision = devc->audios_defaults.record.precision;
#endif

  /*
   * fill in the ad_info structure
   */
  devc->ad_info.ad_mode = mode;
  devc->ad_info.ad_int_vers = AM_VERSION;
  devc->ad_info.ad_add_mode = NULL;
  devc->ad_info.ad_codec_type = AM_TRAD_CODEC;
  devc->ad_info.ad_defaults = &devc->audios_defaults;
  devc->ad_info.ad_play_comb = sadasupport_combinations;
  devc->ad_info.ad_rec_comb = sadasupport_combinations;
  devc->ad_info.ad_entry = &sadasupport_entry;
  devc->ad_info.ad_dev_info = &devc->audios_dev_info;
  devc->ad_info.ad_diag_flags = AM_DIAG_INTERNAL_LOOP;
  devc->ad_info.ad_diff_flags =
    AM_DIFF_SR | AM_DIFF_CH | AM_DIFF_PREC | AM_DIFF_ENC;
  devc->ad_info.ad_assist_flags = AM_ASSIST_MIC;
  devc->ad_info.ad_misc_flags = AM_MISC_RP_EXCL | AM_MISC_MONO_DUP;
  devc->ad_info.ad_translate_flags =
    AM_MISC_8_P_TRANSLATE | AM_MISC_8_R_TRANSLATE;
  devc->ad_info.ad_num_mics = 1;

  /* play capabilities */
  devc->ad_info.ad_play.ad_mixer_srs = sadasupport_mixer_sample_rates;

  devc->ad_info.ad_play.ad_compat_srs = sadasupport_mixer_sample_rates;

#ifdef SOL9
  devc->ad_info.ad_play.ad_conv = &am_src1;
  devc->ad_info.ad_play.ad_sr_info = sadasupport_play_sample_rates_info;
#else
  devc->ad_info.ad_play.ad_conv = &am_src2;
  devc->ad_info.ad_play.ad_sr_info = NULL;
#endif
  devc->ad_info.ad_play.ad_chs = sadasupport_channels;
  devc->ad_info.ad_play.ad_int_rate = pints;
  devc->ad_info.ad_play.ad_max_chs = 32;
  devc->ad_info.ad_play.ad_bsize = AUDIOS_BUFFSIZE;

  /* record capabilities */
  devc->ad_info.ad_record.ad_mixer_srs = sadasupport_mixer_sample_rates;
  devc->ad_info.ad_record.ad_compat_srs = sadasupport_mixer_sample_rates;
#ifdef SOL9
  devc->ad_info.ad_record.ad_conv = &am_src1;
  devc->ad_info.ad_record.ad_sr_info = sadasupport_record_sample_rates_info;
#else
  devc->ad_info.ad_record.ad_conv = &am_src2;
  devc->ad_info.ad_record.ad_sr_info = NULL;
#endif
  devc->ad_info.ad_record.ad_chs = sadasupport_channels;
  devc->ad_info.ad_record.ad_int_rate = rints;
  devc->ad_info.ad_record.ad_max_chs = 32;
  devc->ad_info.ad_record.ad_bsize = AUDIOS_BUFFSIZE;

  /* fill in device info strings */
  (void) strcpy (devc->audios_dev_info.name, AUDIOS_DEV_NAME);
  (void) strcpy (devc->audios_dev_info.config, AUDIOS_DEV_CONFIG);
  (void) strcpy (devc->audios_dev_info.version, AUDIOS_DEV_VERSION);

#if 0
  devc->play_buf_size = AUDIOS_SAMPR48000 * AUDIO_CHANNELS_STEREO *
    (AUDIO_PRECISION_16 >> AUDIO_PRECISION_SHIFT) / pints;
  devc->play_buf_size += AUDIOS_MOD_SIZE -
    (devc->play_buf_size % AUDIOS_MOD_SIZE);
  devc->record_buf_size = AUDIOS_SAMPR48000 * AUDIO_CHANNELS_STEREO *
    (AUDIO_PRECISION_16 >> AUDIO_PRECISION_SHIFT) / rints;
  devc->record_buf_size += AUDIOS_MOD_SIZE -
    (devc->record_buf_size % AUDIOS_MOD_SIZE);
#endif
  return (AUDIO_SUCCESS);

}				/* sadasupport_init_state */

int
oss_sadasupport_attach (oss_device_t * osdev)
{
#ifndef SOL9
  audio_sup_reg_data_t data;
#endif
  int instance;
  /* extern struct cb_ops ossdrv_streams_cb_ops; */

  sadasupport_mixer_srs[1] = sadasupport_rate;	/* Set the maximum rate */

  instance = ddi_get_instance (osdev->dip);

  DDB (cmn_err (CE_CONT, "sadasupport_attach\n"));

  if (devc != NULL)
    {
      cmn_err (CE_WARN, "Multiple instances are not permitted\n");
      return 0;
    }

  devc = PMALLOC (osdev, sizeof (*devc));
  devc->osdev = osdev;
  devc->oss_audiodev = -1;
  devc->start_flags = 0;
  mutex_init (&devc->inst_lock, NULL, MUTEX_DRIVER, NULL);

  osdev->devc = devc;

#ifdef SOL9
  if ((devc->audio_handle =
       audio_sup_attach (osdev->dip, DDI_ATTACH)) == NULL)
    {
      audio_sup_log (NULL, CE_WARN,
		     "!%s%d: attach() audio_sup_attach () failed",
		     DRIVER_NICK, instance);
      return 0;
    }
  DDB (cmn_err (CE_CONT, "audio_sup_attach() OK, handle=%p\n",
		devc->audio_handle));
#else
  data.asrd_version = AUDIOSUP_VERSION;
  data.asrd_key = NULL;

  if ((devc->audio_handle = audio_sup_register (osdev->dip, &data)) == NULL)
    {
      audio_sup_log (NULL, CE_WARN,
		     "!%s%d: attach() audio_sup_register() failed",
		     DRIVER_NICK, instance);
      return 0;
    }
  DDB (cmn_err (CE_CONT, "audio_sup_register() OK, handle=%x\n",
		devc->audio_handle));
#endif
  /* Save private data */
  audio_sup_set_private (devc->audio_handle, devc);

  if ((sadasupport_init_state (devc, osdev->dip)) != AUDIO_SUCCESS)
    {
      audio_sup_log (devc->audio_handle, CE_WARN,
		     "!attach() init state structure failed");
      return 0;
    }

  /* call the mixer attach() routine */
  if (am_attach (devc->audio_handle, DDI_ATTACH, &devc->ad_info) !=
      AUDIO_SUCCESS)
    {
      audio_sup_log (devc->audio_handle, CE_WARN,
		     "!attach() am_attach() failed");
      return 0;
    }

  oss_register_device (osdev, "SADA compatibility layer");
  ddi_report_dev (osdev->dip);

/*
 * Take a copy of the callback options and replace the open/close
 * entry points with our own version.
 */

  return 1;
}

int
oss_sadasupport_detach (oss_device_t * osdev)
{
  /* int instance; */
  sadasupport_devc *devc = osdev->devc;

  if (devc != NULL && devc->audio_busy)
    return 0;

  /* instance = ddi_get_instance (osdev->dip); */

#if 0
  /* stop DMA engines */
  mutex_enter (&devc->inst_lock);
  sadasupport_stop_dma (devc);
  mutex_exit (&devc->inst_lock);

  /* remove the interrupt handler */
  ddi_remove_intr (dip, 0, devc->intr_iblock);

  /* free DMA memory */
  sadasupport_free_sample_buf (devc, &devc->play_buf);
  sadasupport_free_sample_buf (devc, &devc->record_buf);

  /* free the kernel statistics structure */
  if (devc->audios_ksp)
    {
      kstat_delete (devc->audios_ksp);
    }
#endif
  /* detach audio mixer */
  (void) am_detach (devc->audio_handle, DDI_DETACH);

  /*
   * call the audio support module's detach routine to remove this
   * driver completely from the audio driver architecture.
   */
#ifdef SOL9
  (void) audio_sup_detach (devc->audio_handle, DDI_DETACH);
#else
  (void) audio_sup_unregister (devc->audio_handle);
#endif
  mutex_destroy (&devc->inst_lock);
  devc = NULL;

  return 1;
}
