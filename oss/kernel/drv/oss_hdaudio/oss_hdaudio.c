/*
 * Purpose: The High Definition Audio (HDA/Azalia) driver.
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

#include "oss_hdaudio_cfg.h"
#include "oss_pci.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "spdif.h"

#define CORB_DELAY 10
#define CORB_LOOPS 1000

#define INTEL_VENDOR_ID         0x8086
#define INTEL_DEVICE_ICH6       0x2668
#define INTEL_DEVICE_ICH7       0x27d8
#define INTEL_DEVICE_ESB2       0x269a
#define INTEL_DEVICE_ICH8       0x284b
#define INTEL_DEVICE_ICH9       0x293e
#define INTEL_DEVICE_ICH9_B     0x293f
#define INTEL_DEVICE_ICH10      0x3a3e
#define INTEL_DEVICE_ICH10_B    0x3a6e
#define INTEL_DEVICE_PCH        0x3b56
#define INTEL_DEVICE_PCH_B      0x3b57
#define INTEL_DEVICE_CPT        0x1c20
#define INTEL_DEVICE_PBG        0x1d20
#define INTEL_DEVICE_PPT        0x1e20
#define INTEL_DEVICE_SCH        0x811b
#define INTEL_DEVICE_PCH_C      0x8c20
#define INTEL_DEVICE_SUNRISE    0x9d70
#define INTEL_DEVICE_SKYLAKE    0xa170

#define NVIDIA_VENDOR_ID        0x10de
#define NVIDIA_DEVICE_MCP51     0x026c
#define NVIDIA_DEVICE_MCP55     0x0371
#define NVIDIA_DEVICE_MCP61     0x03e4
#define NVIDIA_DEVICE_MCP61A    0x03f0
#define NVIDIA_DEVICE_MCP65     0x044a
#define NVIDIA_DEVICE_MCP67     0x055c
#define NVIDIA_DEVICE_MCP73     0x07fc
#define NVIDIA_DEVICE_MCP78S    0x0774
#define NVIDIA_DEVICE_MCP79     0x0ac0

#define ATI_VENDOR_ID           0x1002
#define ATI_DEVICE_SB450        0x437b
#define ATI_DEVICE_SB600        0x4383

#define AMD_VENDOR_ID           0x1022
#define AMD_DEVICE_HUDSON       0x780d

#define VIA_VENDOR_ID           0x1106
#define VIA_DEVICE_HDA          0x3288

#define SIS_VENDOR_ID           0x1039
#define SIS_DEVICE_HDA          0x7502

#define ULI_VENDOR_ID           0x10b9
#define ULI_DEVICE_HDA          0x5461

#define CREATIVE_ID		0x1102
#define CREATIVE_XFI_HDA	0x0009

#define BDL_SIZE	32
#define HDA_MAX_ENGINES	8
#define MAX_OUTPUTS 	8
#define MAX_INPUTS	4

typedef struct
{
  oss_uint64_t addr;
  unsigned int len;
  unsigned int ioc;
} bdl_t;

typedef struct hda_portc_t hda_portc_t;

typedef struct
{
  int num;
  int busy;
  bdl_t *bdl;
  oss_uint64_t bdl_phys;
  oss_dma_handle_t bdl_dma_handle;
  int bdl_size, bdl_max;
  unsigned char *base;
  unsigned int intrmask;
  hda_portc_t *portc;
} hda_engine_t;

struct hda_portc_t
{
  int num;
  int open_mode;
  int speed, bits, channels;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
  int port_type;
#define PT_OUTPUT	0
#define PT_INPUT	1
  hdaudio_endpointinfo_t *endpoint;
  hda_engine_t *engine;
};

typedef struct
{
  unsigned int response, resp_ex;
} rirb_entry_t;

typedef struct hda_devc_t
{
  oss_device_t *osdev;
  oss_native_word base;
  unsigned int membar_addr;
  unsigned char *azbar;

  char *chip_name;
  unsigned int vendor_id, subvendor_id;

  int irq;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* CORB and RIRB */
  int corbsize, rirbsize;
  unsigned int *corb;
  rirb_entry_t *rirb;
  oss_uint64_t corb_phys, rirb_phys;
  int rirb_rp;
  unsigned int rirb_upper, rirb_lower;
  volatile int rirb_empty;

  oss_dma_handle_t corb_dma_handle;

  /* Mixer */
  unsigned short codecmask;
  hdaudio_mixer_t *mixer;
  int mixer_dev;
  spdif_devc spdc;

  /* Audio */
  int first_dev;
  hda_engine_t inengines[HDA_MAX_ENGINES];
  hda_engine_t outengines[HDA_MAX_ENGINES];
  int num_outengines, num_inengines;
  hdaudio_endpointinfo_t *spdifout_endpoint;

  hda_portc_t output_portc[MAX_OUTPUTS];
  hda_portc_t input_portc[MAX_INPUTS];
  int num_outputs, num_inputs;

  int num_spdin, num_spdout;
  int num_mdmin, num_mdmout;
}
hda_devc_t;

static int
rirb_intr (hda_devc_t * devc)
{
  int serviced = 0;
  unsigned char rirbsts;
  oss_native_word flags;

  rirbsts = PCI_READB (devc->osdev, devc->azbar + HDA_RIRBSTS);
  if (rirbsts != 0)
    {
      serviced = 1;

      if (rirbsts & 0x01)
	{
	  /* RIRB response interrupt */
	  int wp, rp;
	  unsigned int upper = 0, lower = 0;

	  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
	  wp = PCI_READB (devc->osdev, devc->azbar + HDA_RIRBWP) & 0x00ff;
	  while (devc->rirb_rp != wp)
	    {
	      devc->rirb_rp++;
	      devc->rirb_rp %= devc->rirbsize;
	      rp = devc->rirb_rp;
	      upper = devc->rirb[rp].response;
	      lower = devc->rirb[rp].resp_ex;

	      if (lower & 0x10)	/* Unsolicited response */
		{
		  hda_codec_unsol (devc->mixer, upper, lower);
		}
	      else if (devc->rirb_empty)
		{
		  devc->rirb_upper = upper;
		  devc->rirb_lower = lower;
		  devc->rirb_empty--;
		}
	    }
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	}
      PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBSTS, rirbsts);
    }

  return serviced;
}

static int
hdaintr (oss_device_t * osdev)
{
  hda_devc_t *devc = (hda_devc_t *) osdev->devc;
  unsigned int status;
  int serviced = 0;
  int i;

  if (devc == NULL)		/* Too bad */
    return 1;

  status = PCI_READL (devc->osdev, devc->azbar + HDA_INTSTS);
  if (status != 0)
    {
      serviced = 1;

      for (i = 0; i < devc->num_outengines; i++)
	{
	  hda_engine_t *engine = &devc->outengines[i];

	  hda_portc_t *portc;
	  if (status & engine->intrmask)
	    {
	      PCI_WRITEB (devc->osdev, engine->base + 0x03, 0x1e);

	      portc = engine->portc;

	      if (portc != NULL && (portc->trigger_bits & PCM_ENABLE_OUTPUT))
		oss_audio_outputintr (portc->audiodev, 1);
	    }
	}

      for (i = 0; i < devc->num_inengines; i++)
	{
	  hda_engine_t *engine = &devc->inengines[i];

	  hda_portc_t *portc;
	  if (status & engine->intrmask)
	    {
	      PCI_WRITEB (devc->osdev, engine->base + 0x03, 0x1e);

	      portc = engine->portc;

	      if (portc != NULL && (portc->trigger_bits & PCM_ENABLE_INPUT))
		oss_audio_inputintr (portc->audiodev, 0);
	    }
	}
      PCI_WRITEL (devc->osdev, devc->azbar + HDA_INTSTS, status);	/* ACK */

      if (status & (1 << 30))	/* Controller interrupt (RIRB) */
	{
	  if (rirb_intr (devc))
	    serviced = 1;
	}
    }
  return serviced;
}

static int
do_corb_write (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	       unsigned int verb, unsigned int parm)
{
  unsigned int wp;
  unsigned int tmp;
  oss_native_word flags;
  hda_devc_t *devc = (hda_devc_t *) dc;

  tmp = (cad << 28) | (d << 27) | (nid << 20) | (verb << 8) | (parm & 0xffff);
  wp = PCI_READB (devc->osdev, devc->azbar + HDA_CORBWP) & 0x00ff;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  wp = (wp + 1) % devc->corbsize;

  devc->corb[wp] = tmp;
  devc->rirb_empty++;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_CORBWP, wp);

  return 1;
}

static int
do_corb_write_nomutex (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	       unsigned int verb, unsigned int parm)
{
  unsigned int wp;
  unsigned int tmp;
  hda_devc_t *devc = (hda_devc_t *) dc;

  tmp = (cad << 28) | (d << 27) | (nid << 20) | (verb << 8) | (parm & 0xffff);
  wp = PCI_READB (devc->osdev, devc->azbar + HDA_CORBWP) & 0x00ff;

  wp = (wp + 1) % devc->corbsize;

  devc->corb[wp] = tmp;
  devc->rirb_empty++;
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_CORBWP, wp);

  return 1;
}

static int
do_corb_read (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	      unsigned int verb, unsigned int parm, unsigned int *upper,
	      unsigned int *lower)
{
  int tmout;
  hda_devc_t *devc = (hda_devc_t *) dc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  do_corb_write_nomutex (devc, cad, nid, d, verb, parm);

  tmout = CORB_LOOPS;
  while (devc->rirb_empty)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

      if (--tmout < 0)
	{
	  devc->rirb_rp = PCI_READB (devc->osdev, devc->azbar + HDA_RIRBWP);
	  devc->rirb_empty = 0;
	  return 0;
	}
      oss_udelay (CORB_DELAY);
      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
    }

  *upper = devc->rirb_upper;
  *lower = devc->rirb_lower;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 1;
}

static int
do_corb_read_poll (void *dc, unsigned int cad, unsigned int nid,
		   unsigned int d, unsigned int verb, unsigned int parm,
		   unsigned int *upper, unsigned int *lower)
{
  int tmout = 0;
  hda_devc_t *devc = (hda_devc_t *) dc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  do_corb_write_nomutex (devc, cad, nid, d, verb, parm);

  tmout = CORB_LOOPS;
  while (devc->rirb_empty)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

      rirb_intr (devc);

      if (--tmout < 0)
	{
	  devc->rirb_rp = PCI_READB (devc->osdev, devc->azbar + HDA_RIRBWP);
	  devc->rirb_empty = 0;
	  return 0;
	}
      oss_udelay (CORB_DELAY);
      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
    }

  *upper = devc->rirb_upper;
  *lower = devc->rirb_lower;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 1;
}

#undef corb_read

static int
corb_read (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	   unsigned int verb, unsigned int parm, unsigned int *upper,
	   unsigned int *lower)
{
  hda_devc_t *devc = (hda_devc_t *) dc;

/*
 * Do three retries using different access methods
 */
  if (do_corb_read (dc, cad, nid, d, verb, parm, upper, lower))
    return 1;

  PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBCTL, 0x02);	/* Intr disable */

  if (do_corb_read_poll (dc, cad, nid, d, verb, parm, upper, lower))
    {
      PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBCTL, 0x03);	/* Intr re-enable */
      return 1;
    }

  if (do_corb_read_poll (dc, cad, nid, d, verb, parm, upper, lower))
    {
      PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBCTL, 0x03);	/* Intr re-enable */
      return 1;
    }

#if 0
  // TODO: Implement this
  if (do_corb_read_single (dc, cad, nid, d, verb, parm, upper, lower))
    {
      PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBCTL, 0x03);	/* Intr re-enable */
      return 1;
    }
#endif

  PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBCTL, 0x03);	/* Intr re-enable */

  cmn_err (CE_WARN,
	   "RIRB timeout (cad=%d, nid=%d, d=%d, verb=%03x, parm=%x)\n",
	   cad, nid, d, verb, parm);

  return 0;
}

/*
 * Audio routines
 */

static int
hda_audio_set_rate (int dev, int arg)
{
  hda_portc_t *portc = audio_engines[dev]->portc;
  adev_p adev = audio_engines[dev];

  int best = -1, bestdiff = 10000000;
  int i;

  if (arg == 0)
    return portc->speed;

  for (i = 0; i < adev->nrates; i++)
    {
      int diff = arg - adev->rates[i];
      if (diff < 0)
	diff = -diff;

      if (diff < bestdiff)
	{
	  best = i;
	  bestdiff = diff;
	}
    }

  if (best == -1)
    {
      cmn_err (CE_WARN, "No suitable rate found!\n");
      return portc->speed = 48000;	/* Some default */
    }
  portc->speed = adev->rates[best];
  return portc->speed;
}

static short
hda_audio_set_channels (int dev, short arg)
{
  hda_portc_t *portc = audio_engines[dev]->portc;
  hda_devc_t *devc = audio_engines[dev]->devc;
  adev_p adev = audio_engines[dev];
  int i, n1, n2;

  if (arg == 0)
    {
      return portc->channels;
    }

  if (arg < 1)
    arg = 1;

  if (arg > adev->max_channels)
  {
    arg = adev->max_channels;
  }

  if (arg != 1 && arg != 2 && arg != 4 && arg != 6 && arg != 8)
    {
      cmn_err (CE_NOTE, "Ignored request for odd number (%d) of channels\n", arg);
      return portc->channels = arg & ~1;
    }

  if (arg < adev->min_channels)
    arg = adev->min_channels;

  /*
   * Check if more output endpoints need to be allocated
   */
  n2 = (arg + 1) / 2;
  n1 = (portc->channels + 1) / 2;
  if (n1 < 1)
    n1 = 1;

  if (n2 > n1)			/* Needs more stereo pairs */
    {
      oss_native_word flags;

      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

      for (i = n1; i < n2; i++)
	{
	  if (portc->endpoint[i].busy)
	    {
	      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	      return portc->channels;
	    }
	}

      for (i = n1; i < n2; i++)
	{
	  portc->endpoint[i].busy = 1;
	  portc->endpoint->borrow_count++;
	}

      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
    }
  else if (n2 < n1)		/* Some stereo pairs can be released */
    {
      for (i = n2; i < n1; i++)
	{
	  portc->endpoint[i].busy = 0;
	  portc->endpoint->borrow_count--;
	}
    }

  return portc->channels = arg;
}

static unsigned int
hda_audio_set_format (int dev, unsigned int arg)
{
  hda_portc_t *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & audio_engines[dev]->oformat_mask))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

static int
do_corb_write_simple (void *dc, unsigned int v)
{
  unsigned int wp;
  hda_devc_t *devc = (hda_devc_t *) dc;

  wp = PCI_READB (devc->osdev, devc->azbar + HDA_CORBWP) & 0x00ff;

  wp = (wp + 1) % devc->corbsize;

  devc->corb[wp] = v;
  devc->rirb_empty++;
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_CORBWP, wp);

  return 1;
}

static int
do_corb_read_simple (void *dc, unsigned int cmd, unsigned int *v)
{
  int tmout = 0;
  hda_devc_t *devc = (hda_devc_t *) dc;

  do_corb_write_simple (dc, cmd);

  tmout = CORB_LOOPS;
  while (devc->rirb_empty)
    {
      if (--tmout < 0)
	{
	  cmn_err (CE_WARN, "RIRB timeout (cmd=%08x)\n", cmd);
	  devc->rirb_rp = PCI_READB (devc->osdev, devc->azbar + HDA_RIRBWP);
	  devc->rirb_empty = 0;
	  return 0;
	}
      oss_udelay (CORB_DELAY);
    }

  *v = devc->rirb_upper;

  return 1;
}

static int
hda_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  hda_devc_t *devc = audio_engines[dev]->devc;
  hda_portc_t *portc = audio_engines[dev]->portc;

  //if (hdaudio_snoopy)
    switch (cmd)
      {
      case HDA_IOCTL_WRITE:
#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	do_corb_write_simple (devc, *(unsigned int *) arg);
	return 0;
	break;

      case HDA_IOCTL_READ:
#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	if (!do_corb_read_simple
	    (devc, *(unsigned int *) arg, (unsigned int *) arg))
	  return OSS_EIO;

	return 0;
	break;

      case HDA_IOCTL_NAME:
#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	return hda_codec_getname (devc->mixer, (hda_name_t *) arg);
	break;

      case HDA_IOCTL_WIDGET:
#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
	return hda_codec_getwidget (devc->mixer, (hda_widget_info_t *) arg);
	break;

      }
  return hdaudio_codec_audio_ioctl (devc->mixer, portc->endpoint, cmd, arg);
}

static void hda_audio_trigger (int dev, int state);

static void
hda_audio_reset (int dev)
{
  hda_audio_trigger (dev, 0);
}

/*ARGSUSED*/
static int
hda_audio_open (int dev, int mode, int openflags)
{
  hda_portc_t *portc = audio_engines[dev]->portc;
  hda_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  hda_engine_t *engines, *engine = NULL;
  int i, n;
  unsigned int tmp;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode || portc->endpoint->busy)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (portc->port_type == PT_INPUT)
    {
      engines = devc->inengines;
      n = devc->num_inengines;
    }
  else
    {
      engines = devc->outengines;
      n = devc->num_outengines;
    }

  for (i = 0; i < n && engine == NULL; i++)
    {
      if (!engines[i].busy)
	engine = &engines[i];
    }

  if (engine == NULL)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      cmn_err (CE_WARN, "No free DMA engines.\nn");
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  portc->audio_enabled &= ~mode;
  portc->endpoint->busy = 1;
  portc->endpoint->borrow_count = 1;

  portc->engine = engine;
  engine->portc = portc;
  portc->engine->busy = 1;

  portc->endpoint->stream_number = portc->endpoint->default_stream_number;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

/*
 * Reset the DMA engine and wait for the reset to complete
 */
  tmp = PCI_READL (devc->osdev, engine->base);
  PCI_WRITEL (devc->osdev, engine->base, tmp | 0x01);	/* Stream reset */

  for (i = 0; i < 1000; i++)
    {
      if (PCI_READL (devc->osdev, engine->base) & 0x01)
	break;
      oss_udelay (1000);
    }

  /* Unreset and wait */

  tmp = PCI_READL (devc->osdev, engine->base);
  PCI_WRITEL (devc->osdev, engine->base, tmp & ~0x01);	/* Release reset */

  for (i = 0; i < 1000; i++)
    {
      if (!(PCI_READL (devc->osdev, engine->base) & 0x01))
	break;
      oss_udelay (1000);
    }

  return 0;
}

static void
hda_audio_close (int dev, int mode)
{
  hda_portc_t *portc = audio_engines[dev]->portc;
  hda_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  int i;

  if (!portc->open_mode)
    {
      cmn_err (CE_WARN, "Bad close %d\n", dev);
      return;
    }

  hda_audio_reset (dev);
  hdaudio_codec_reset_endpoint (devc->mixer, portc->endpoint,
				portc->channels);

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->audio_enabled &= ~mode;
  portc->engine->busy = 0;
  portc->engine->portc = NULL;
  portc->engine = NULL;
  portc->endpoint->busy = 0;
  portc->endpoint->stream_number = portc->endpoint->default_stream_number;

  for (i = 1; i < portc->endpoint->borrow_count; i++)
    {
      portc->endpoint[i].busy = 0;
    }

  portc->endpoint->borrow_count = 1;
  portc->open_mode = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static void
hda_audio_output_block (int dev, oss_native_word buf, int count,
			int fragsize, int intrflag)
{
  hda_portc_t *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
hda_audio_start_input (int dev, oss_native_word buf, int count,
		       int fragsize, int intrflag)
{
  hda_portc_t *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
hda_audio_trigger (int dev, int state)
{
  hda_devc_t *devc = audio_engines[dev]->devc;
  hda_portc_t *portc = audio_engines[dev]->portc;
  hda_engine_t *engine = portc->engine;
  oss_native_word flags;
  unsigned char tmp, intr;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  tmp = PCI_READB (devc->osdev, engine->base + 0x00);
  intr = PCI_READB (devc->osdev, devc->azbar + HDA_INTCTL);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	      tmp |= 0x1e;	/* DMA and Stream Interrupt enable */
	      intr |= engine->intrmask;
	      PCI_WRITEB (devc->osdev, engine->base + 0x00, tmp);
	      PCI_WRITEB (devc->osdev, devc->azbar + HDA_INTCTL, intr);
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	      tmp &= ~0x1e;	/* Run-off & intr disable */
	      intr &= ~engine->intrmask;
	      /* Stop DMA and Stream Int */
	      PCI_WRITEB (devc->osdev, engine->base + 0x00, tmp);
	      /* Ack pending ints */
	      PCI_WRITEB (devc->osdev, engine->base + 0x03, 0x1c);
	      /* Disable Interrupts */
	      PCI_WRITEB (devc->osdev, devc->azbar + HDA_INTCTL, intr);
	    }
	}
    }

  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	      tmp |= 0x1e;	/* dma & intr enable */
	      intr |= engine->intrmask;
	      PCI_WRITEB (devc->osdev, engine->base + 0x00, tmp);
	      PCI_WRITEB (devc->osdev, devc->azbar + HDA_INTCTL, intr);
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_INPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      tmp &= ~0x1e;	/* Run-off & intr disable */
	      intr &= ~engine->intrmask;
	      /* Stop DMA and Stream Int */
	      PCI_WRITEB (devc->osdev, engine->base + 0x00, tmp);
	      /* Ack pending ints */
	      PCI_WRITEB (devc->osdev, engine->base + 0x03, 0x1c);
	      /* Disable Interrupts */
	      PCI_WRITEB (devc->osdev, devc->azbar + HDA_INTCTL, intr);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
init_bdl (hda_devc_t * devc, hda_engine_t * engine, dmap_p dmap)
{
  int i;
  bdl_t *bdl = engine->bdl;

  PCI_WRITEL (devc->osdev, engine->base + HDA_SD_BDLPL, 0);
  PCI_WRITEL (devc->osdev, engine->base + HDA_SD_BDLPU, 0);
  for (i = 0; i < dmap->nfrags; i++)
    {
      bdl[i].addr = dmap->dmabuf_phys + (i * dmap->fragment_size);
      bdl[i].len = dmap->fragment_size;
      bdl[i].ioc = 0x01;
    }
}

static int
setup_audio_engine (hda_devc_t * devc, hda_engine_t * engine, hda_portc_t * portc,
		    dmap_t * dmap)
{
  unsigned int tmp, tmout;

/* make sure the run bit is zero for SD */
  PCI_WRITEB (devc->osdev, engine->base + HDA_SD_CTL,
	      PCI_READB (devc->osdev, engine->base + HDA_SD_CTL) & ~0x2);
/*
 * First reset the engine.
 */

  tmp = PCI_READB (devc->osdev, engine->base + HDA_SD_CTL);
  PCI_WRITEB (devc->osdev, engine->base + HDA_SD_CTL, tmp | 0x01);	/* Reset */
  oss_udelay (1000);

  tmout = 300;
  while (!(PCI_READB (devc->osdev, engine->base + HDA_SD_CTL) & 0x01)
	 && --tmout)
    oss_udelay (1000);

  PCI_WRITEB (devc->osdev, engine->base + HDA_SD_CTL, tmp & ~0x01);	/* Release reset */
  oss_udelay (1000);
  tmout = 300;
  while ((PCI_READB (devc->osdev, engine->base + HDA_SD_CTL) & 0x01)
	 && --tmout)
    oss_udelay (1000);

/*
 * Set the engine tag number field
 */
  tmp = PCI_READL (devc->osdev, engine->base + HDA_SD_CTL);
  tmp &= ~(0xf << 20);
  tmp |= portc->endpoint->stream_number << 20;
  PCI_WRITEL (devc->osdev, engine->base + HDA_SD_CTL, tmp);

/* program buffer size and fragments in the engine */

  PCI_WRITEL (devc->osdev, engine->base + HDA_SD_CBL, dmap->bytes_in_use);
  PCI_WRITEW (devc->osdev, engine->base + HDA_SD_LVI, dmap->nfrags - 1);

/* setup the BDL base address */
  tmp = engine->bdl_phys;
  PCI_WRITEL (devc->osdev, engine->base + HDA_SD_BDLPL, tmp & 0xffffffff);
  tmp = engine->bdl_phys >> 32;
  PCI_WRITEL (devc->osdev, engine->base + HDA_SD_BDLPU, tmp & 0xffffffff);
  PCI_WRITEB (devc->osdev, engine->base + HDA_SD_STS, 0x1c);	/* mask out ints */

/*
 * Next the sample rate and format setup
 */
  if (hdaudio_codec_setup_endpoint
      (devc->mixer, portc->endpoint, portc->speed, portc->channels,
       portc->bits, portc->endpoint->stream_number, &tmp) < 0)
    {
      cmn_err (CE_WARN, "Codec setup failed\n");
      return OSS_EIO;
    }

  PCI_WRITEW (devc->osdev, engine->base + HDA_SD_FORMAT, tmp);
  tmp = PCI_READW (devc->osdev, engine->base + HDA_SD_FORMAT);

  return 0;
}

/*ARGSUSED*/
static int
hda_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  hda_devc_t *devc = audio_engines[dev]->devc;
  hda_portc_t *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  oss_native_word flags;
  hda_engine_t *engine;

  if (portc->port_type != PT_INPUT)
    return OSS_ENOTSUP;

  engine = portc->engine;

  if (dmap->nfrags > engine->bdl_max)	/* Overflow protection */
    {
      dmap->nfrags = engine->bdl_max;
      dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;
    }

  init_bdl (devc, engine, dmap);

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return setup_audio_engine (devc, engine, portc, dmap);
}

/*ARGSUSED*/
static int
hda_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  hda_devc_t *devc = audio_engines[dev]->devc;
  hda_portc_t *portc = audio_engines[dev]->portc;
  hda_engine_t *engine;

  dmap_t *dmap = audio_engines[dev]->dmap_out;
  oss_native_word flags;

  if (portc->port_type != PT_OUTPUT)
    return OSS_ENOTSUP;

  engine = portc->engine;

  if (dmap->nfrags > engine->bdl_max)	/* Overflow protection */
    {
      dmap->nfrags = engine->bdl_max;
      dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;
    }

  init_bdl (devc, engine, dmap);

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return setup_audio_engine (devc, engine, portc, dmap);
}

/*ARGSUSED*/
static int
hda_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  hda_portc_t *portc = audio_engines[dev]->portc;
#ifdef sun
  // PCI_READL under solaris needs devc->osdev
  hda_devc_t *devc = audio_engines[dev]->devc;
#endif
  hda_engine_t *engine;
  unsigned int ptr;

  engine = portc->engine;
  ptr =
    PCI_READL (devc->osdev, engine->base + HDA_SD_LPIB) % dmap->bytes_in_use;

  return ptr;
}

static const audiodrv_t hda_audio_driver = {
  hda_audio_open,
  hda_audio_close,
  hda_audio_output_block,
  hda_audio_start_input,
  hda_audio_ioctl,
  hda_audio_prepare_for_input,
  hda_audio_prepare_for_output,
  hda_audio_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  hda_audio_trigger,
  hda_audio_set_rate,
  hda_audio_set_format,
  hda_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* hda_check_input, */
  NULL,				/* hda_check_output, */
  NULL,				/* hda_alloc_buffer */
  NULL,				/* hda_free_buffer */
  NULL,
  NULL,
  hda_get_buffer_pointer
};

static int
reset_controller (hda_devc_t * devc)
{
  unsigned int tmp, tmout;

  /*reset the controller by writing a 0*/
  tmp = PCI_READL (devc->osdev, devc->azbar + HDA_GCTL);
  tmp &= ~CRST;
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_GCTL, tmp);

  /*wait until the controller writes a 0 to indicate reset is done or until 50ms have passed*/
  tmout = 50;
  while ((PCI_READL (devc->osdev, devc->azbar + HDA_GCTL) & CRST) && --tmout)
    oss_udelay (1000);

  oss_udelay (1000);

  /*bring the controller out of reset  by writing a 1*/
  tmp = PCI_READL (devc->osdev, devc->azbar + HDA_GCTL);
  tmp |= CRST;
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_GCTL, tmp);

  /*wait until the controller writes a 1 to indicate it is ready is or until 50ms have passed*/
  tmout = 50;
  while (!(PCI_READL (devc->osdev, devc->azbar + HDA_GCTL) & CRST) && --tmout)
    oss_udelay (1000);

  oss_udelay (1000);

  /*if the controller is not ready now, abort*/
  if (!(PCI_READL (devc->osdev, devc->azbar + HDA_GCTL)))
    {
      cmn_err (CE_WARN, "Controller not ready\n");
      return 0;
    }

  if (!devc->codecmask)
    {
      devc->codecmask = PCI_READW (devc->osdev, devc->azbar + HDA_STATESTS);
      DDB (cmn_err (CE_CONT, "Codec mask %x\n", devc->codecmask));
    }

  return 1;
}

static int
setup_controller (hda_devc_t * devc)
{
  unsigned int tmp, tmout;
  oss_native_word phaddr;

/*
 * Allocate the CORB and RIRB buffers.
 */
  tmp = PCI_READB (devc->osdev, devc->azbar + HDA_CORBSIZE) & 0x03;

  switch (tmp)
    {
    case 0:
      devc->corbsize = 2;
      break;
    case 1:
      devc->corbsize = 16;
      break;
    case 2:
      devc->corbsize = 256;
      break;
    default:
      cmn_err (CE_WARN, "Bad CORB size\n");
      return 0;
    }

  tmp = PCI_READB (devc->osdev, devc->azbar + HDA_RIRBSIZE) & 0x03;

  switch (tmp)
    {
    case 0:
      devc->rirbsize = 2;
      break;
    case 1:
      devc->rirbsize = 16;
      break;
    case 2:
      devc->rirbsize = 256;
      break;
    default:
      cmn_err (CE_WARN, "Bad CORB size\n");
      return 0;
    }

  if ((devc->corb =
       CONTIG_MALLOC (devc->osdev, 4096, MEMLIMIT_32BITS, &phaddr, devc->corb_dma_handle)) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory (CORB)\n");
      return 0;
    }

  devc->corb_phys = phaddr;

  devc->rirb = (rirb_entry_t *) (devc->corb + 512);	/* 512 dwords = 2048 bytes */
  devc->rirb_phys = devc->corb_phys + 2048;

/*
 * Initialize CORB registers
 */

  PCI_WRITEB (devc->osdev, devc->azbar + HDA_CORBCTL, 0);	/* Stop */

  tmout = 0;

  while (tmout++ < 500
	 && (PCI_READB (devc->osdev, devc->azbar + HDA_CORBCTL) & 0x02));

  if (PCI_READB (devc->osdev, devc->azbar + HDA_CORBCTL) & 0x02)
    {
      cmn_err (CE_WARN, "CORB didn't stop\n");
    }

  tmp = devc->corb_phys & 0xffffffff;
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_CORBLBASE, tmp);
  tmp = (devc->corb_phys >> 32) & 0xffffffff;
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_CORBUBASE, tmp);

  PCI_WRITEW (devc->osdev, devc->azbar + HDA_CORBWP, 0x0);	/* Reset to 0 */
  PCI_WRITEW (devc->osdev, devc->azbar + HDA_CORBRP, 0x8000);	/* Reset to 0 */
  PCI_WRITEB (devc->osdev, devc->azbar + HDA_CORBCTL, 0x02);	/* Start */

/*
 * Initialize RIRB registers
 */
  PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBCTL, 0);	/* Stop */

  tmout = 0;

  while (tmout++ < 500
	 && (PCI_READB (devc->osdev, devc->azbar + HDA_RIRBCTL) & 0x02));

  if (PCI_READB (devc->osdev, devc->azbar + HDA_RIRBCTL) & 0x02)
    {
      cmn_err (CE_WARN, "RIRB didn't stop\n");
    }

  tmp = devc->rirb_phys & 0xffffffff;
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_RIRBLBASE, tmp);
  tmp = (devc->rirb_phys >> 32) & 0xffffffff;
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_RIRBUBASE, tmp);
  devc->rirb_rp = 0;

  PCI_WRITEW (devc->osdev, devc->azbar + HDA_RIRBWP, 0x8000);	/* Reset to 0 */
  PCI_WRITEW (devc->osdev, devc->azbar + HDA_RINTCNT, 1);	/* reset hw write ptr */
  PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBCTL, 0x03);	/* Start & Intr enable */

  return 1;
}

static int
setup_engines (hda_devc_t * devc)
{
  int i, p;
  unsigned int gcap;
  unsigned int num_inputs, num_outputs;
  oss_native_word phaddr;
  gcap = PCI_READW (devc->osdev, devc->azbar + HDA_GCAP);

  num_outputs = (gcap >> 12) & 0x0f;
  num_inputs = (gcap >> 8) & 0x0f;

  /* Init output engines */
  p = num_inputs;		/* Output engines are located after the input ones */

  if (num_outputs > HDA_MAX_ENGINES)
    {
      cmn_err (CE_WARN,
	       "Using only %d out of %d available output engines\n",
	       HDA_MAX_ENGINES, num_outputs);
      num_outputs = HDA_MAX_ENGINES;
    }

  if (num_inputs > HDA_MAX_ENGINES)
    {
      cmn_err (CE_WARN,
	       "Using only %d out of %d available input engines\n",
	       HDA_MAX_ENGINES, num_inputs);
      num_inputs = HDA_MAX_ENGINES;
    }

  for (i = 0; i < num_outputs; i++)
    {
      hda_engine_t *engine = &devc->outengines[i];

      engine->bdl =
	CONTIG_MALLOC (devc->osdev, 4096, MEMLIMIT_32BITS, &phaddr, engine->bdl_dma_handle);
      if (engine->bdl == NULL)
	{
	  cmn_err (CE_WARN, "Out of memory\n");
	  return 0;
	}
      engine->bdl_max = 4096 / sizeof (bdl_t);
      engine->bdl_phys = phaddr;
      engine->base = devc->azbar + 0x80 + (p * 0x20);
      engine->num = i;
      engine->intrmask = (1 << p);
      engine->portc = NULL;
      p++;
      devc->num_outengines = i + 1;
    }

  /* Init input engines */
  p = 0;
  for (i = 0; i < num_inputs; i++)
    {
      hda_engine_t *engine = &devc->inengines[i];

      engine->bdl =
	CONTIG_MALLOC (devc->osdev, 4096, MEMLIMIT_32BITS, &phaddr, engine->bdl_dma_handle);
      if (engine->bdl == NULL)
	{
	  cmn_err (CE_WARN, "Out of memory\n");
	  return 0;
	}
      engine->bdl_max = 4096 / sizeof (bdl_t);
      engine->bdl_phys = phaddr;
      engine->base = devc->azbar + 0x80 + (p * 0x20);
      engine->num = i;
      engine->intrmask = (1 << p);
      engine->portc = NULL;
      p++;
      devc->num_inengines = i + 1;
    }
  return 1;
}

/*ARGSUSED*/
static void
init_adev_caps (hda_devc_t * devc, adev_p adev, hdaudio_endpointinfo_t * ep)
{
  int i;

  adev->oformat_mask = ep->fmt_mask;
  adev->iformat_mask = ep->fmt_mask;

  adev->min_rate = 500000;
  adev->max_rate = 0;

  adev->nrates = ep->nrates;
  if (adev->nrates > OSS_MAX_SAMPLE_RATES)
  {
     cmn_err(CE_NOTE, "Too many supported sample rates (%d)\n", adev->nrates);
     adev->nrates = OSS_MAX_SAMPLE_RATES;
  }

  for (i = 0; i < adev->nrates; i++)
    {
      int r = ep->rates[i];

      adev->rates[i] = r;

      if (adev->min_rate > r)
	adev->min_rate = r;
      if (adev->max_rate < r)
	adev->max_rate = r;
    }
}

/*
 * S/PDIF lowlevel driver
 */
/*ARGSUSED*/
static int
hdaudio_reprogram_spdif (void *_devc, void *_portc,
			 oss_digital_control * ctl, unsigned int mask)
{
  unsigned short cbits = 0;
  hda_devc_t *devc = _devc;
  oss_native_word flags;
  hdaudio_endpointinfo_t *endpoint = devc->spdifout_endpoint;

  if (endpoint == NULL)
    return OSS_EIO;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  cbits = 0x01;			/* Digital interface enabled */

  cbits |= (1 << 2);		/* VCFG */

  if (ctl->out_vbit == VBIT_ON)
    cbits |= (1 << 1);		/* Turn on the V bit */

  if (ctl->cbitout[0] & 0x02)	/* Audio/Data */
    cbits |= (1 << 5);		/* /AUDIO */

  if (ctl->cbitout[0] & 0x01)	/* Pro */
    cbits |= (1 << 6);
  else
    {

      if (ctl->cbitout[0] & 0x04)	/* Copyright */
	cbits |= (1 << 4);	/* COPY */

      if (ctl->cbitout[1] & 0x80)	/* Generation level */
	cbits |= (1 << 7);	/* L */

      if (ctl->emphasis_type & 1)	/* Pre-emphasis */
	cbits |= (1 << 3);	/* PRE */
    }
  do_corb_write (devc, endpoint->cad, endpoint->base_wid, 0,
		 SET_SPDIF_CONTROL1, cbits);

  cbits = ctl->cbitout[1] & 0x7f;	/* Category code */
  do_corb_write (devc, endpoint->cad, endpoint->base_wid, 0,
		 SET_SPDIF_CONTROL2, cbits);

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

spdif_driver_t hdaudio_spdif_driver = {
/* reprogram_device: */ hdaudio_reprogram_spdif,
};

static int
spdif_mixer_init (int dev)
{
  hda_devc_t *devc = mixer_devs[dev]->hw_devc;
  int err;

  if ((err = oss_spdif_mix_init (&devc->spdc)) < 0)
    return err;

  return 0;
}

static void
install_outputdevs (hda_devc_t * devc)
{
  int i, n, pass, audio_dev, output_num=0;
  char tmp_name[64];
  hdaudio_endpointinfo_t *endpoints;

  if ((n =
       hdaudio_mixer_get_outendpoints (devc->mixer, &endpoints,
				       sizeof (*endpoints))) < 0)
    return;

#if 0
  // This check will be done later.
  if (n > MAX_OUTPUTS)
    {
      cmn_err (CE_WARN,
	       "Only %d out of %d output devices can be installed\n",
	       MAX_OUTPUTS, n);
      n = MAX_OUTPUTS;
    }
#endif

/*
 * Install the output devices in two passes. First install the analog
 * endpoints and then the digital one(s).
 */
  for (pass = 0; pass < 3; pass++)
    for (i = 0; i < n; i++)
      {
	adev_p adev;
	hda_portc_t *portc = &devc->output_portc[i];
	unsigned int formats = AFMT_S16_LE;
	int opts = ADEV_AUTOMODE | ADEV_NOINPUT;
	char *devfile_name = "";
	char devname[16];

/* Skip endpoints that are not physically connected on the motherboard. */
	if (endpoints[i].skip)
	  continue;

	if (output_num >= MAX_OUTPUTS)
	{
		cmn_err(CE_CONT, "Too many output endpoints. Endpoint %d ignored.\n", i);
		continue;
	}

	switch (pass)
	  {
	  case 0:		/* Pick analog and non-modem ones */
	    if (endpoints[i].is_digital || endpoints[i].is_modem)
	      continue;
	    break;

	  case 1:		/* Pick digital one(s) */
	    if (!endpoints[i].is_digital)
	      continue;
            break;

          case 2:               /* Pick modem one(s) */
            if (!endpoints[i].is_modem)
              continue;
	  }

	if (endpoints[i].is_digital)
	  {
	    opts |= ADEV_SPECIAL;
	    sprintf (devname, "spdout%d", devc->num_spdout++);
	    devfile_name = devname;
	  }
        if (endpoints[i].is_modem)
          {
            opts |= ADEV_SPECIAL;
            sprintf (devname, "mdmout%d", devc->num_mdmout++);
            devfile_name = devname;
          }

	// sprintf (tmp_name, "%s %s", devc->chip_name, endpoints[i].name);
	sprintf (tmp_name, "HD Audio play %s", endpoints[i].name);

	if ((audio_dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
					       devc->osdev,
					       devc->osdev,
					       tmp_name,
					       &hda_audio_driver,
					       sizeof (audiodrv_t),
					       opts, formats, devc, -1,
					       devfile_name)) < 0)
	  {
	    return;
	  }

	if (output_num == 0)
	  devc->first_dev = audio_dev;

	adev = audio_engines[audio_dev];
	devc->num_outputs = i+1;

	adev->devc = devc;
	adev->portc = portc;
	adev->rate_source = devc->first_dev;
	adev->mixer_dev = devc->mixer_dev;
	adev->min_rate = 8000;
	adev->max_rate = 192000;
	adev->min_channels = 2;

	if (output_num == 0)
	  adev->max_channels = 8;
	else
	  adev->max_channels = 2;

        if (endpoints[i].is_modem)
          {
            adev->min_channels = 1;
            adev->max_channels = 1;
            adev->caps |= PCM_CAP_MODEM;
          }

	if (adev->max_channels > 4)
	  adev->dmabuf_alloc_flags |= DMABUF_LARGE | DMABUF_QUIET;
	adev->min_block = 128;	/* Hardware limitation */
	adev->min_fragments = 4;	/* Vmix doesn't work without this */
	portc->num = output_num;
	portc->open_mode = 0;
	portc->audio_enabled = 0;
	portc->audiodev = audio_dev;
	portc->port_type = PT_OUTPUT;
	portc->endpoint = &endpoints[i];
	init_adev_caps (devc, adev, portc->endpoint);
	portc->engine = NULL;

	if (portc->endpoint->is_digital)
	  {
	    int err;

	    devc->spdifout_endpoint = portc->endpoint;

/*
 * To be precise the right place for the spdc field might be portc instead of
 * devc. In this way it's possible to have multiple S/PDIF output DACs connected
 * to the same HDA controller. OTOH having it in devc saves space. Multiple 
 * oss_spdif_install() calls on the same spdc structure doesn't cause any 
 * problems.
 *
 * If spdc is moved to portc then care must be taken that oss_spdif_uninstall()
 * is called for all all output_portc instances.
 */
	    if ((err = oss_spdif_install (&devc->spdc, devc->osdev,
					  &hdaudio_spdif_driver,
					  sizeof (spdif_driver_t), devc, NULL,
					  devc->mixer_dev, SPDF_OUT,
					  DIG_PASSTHROUGH | DIG_EXACT |
					  DIG_CBITOUT_LIMITED | DIG_VBITOUT |
					  DIG_PRO | DIG_CONSUMER)) != 0)
	      {
		cmn_err (CE_WARN,
			 "S/PDIF driver install failed. Error %d\n", err);
	      }
	    else
	      {
		hdaudio_mixer_set_initfunc (devc->mixer, spdif_mixer_init);
	      }
	  }
	  output_num++;
      }
}

static void
install_inputdevs (hda_devc_t * devc)
{
  int i, n, audio_dev;
  char tmp_name[64];
  hdaudio_endpointinfo_t *endpoints;
  char devname[16], *devfile_name = "";

  if ((n =
       hdaudio_mixer_get_inendpoints (devc->mixer, &endpoints,
				      sizeof (*endpoints))) < 0)
    return;

  if (n > MAX_INPUTS)
    {
      cmn_err (CE_WARN,
	       "Only %d out of %d input devices can be installed\n",
	       MAX_INPUTS, n);
      n = MAX_INPUTS;
    }

  for (i = 0; i < n; i++)
    {
      adev_p adev;
      hda_portc_t *portc = &devc->input_portc[i];
      unsigned int formats = AFMT_S16_LE;
      int opts = ADEV_AUTOMODE | ADEV_NOOUTPUT;

/* Skip endpoints that are not physically connected on the motherboard. */
      if (endpoints[i].skip)
	continue;

      if (endpoints[i].is_digital)
	{
	  opts |= ADEV_SPECIAL;
	  sprintf (devname, "spdin%d", devc->num_spdin++);
	  devfile_name = devname;
	}
      if (endpoints[i].is_modem)
        {
          opts |= ADEV_SPECIAL;
          sprintf (devname, "mdmin%d", devc->num_mdmin++);
          devfile_name = devname;
        }

      //sprintf (tmp_name, "%s %s", devc->chip_name, endpoints[i].name);
      sprintf (tmp_name, "HD Audio rec %s", endpoints[i].name);

      if ((audio_dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
					     devc->osdev,
					     devc->osdev,
					     tmp_name,
					     &hda_audio_driver,
					     sizeof (audiodrv_t),
					     opts, formats, devc, -1,
					     devfile_name)) < 0)
	{
	  return;
	}

      if (i == 0 && devc->first_dev == -1)
	devc->first_dev = audio_dev;

      adev = audio_engines[audio_dev];
      devc->num_inputs = i+1;

      adev->devc = devc;
      adev->portc = portc;
      adev->rate_source = devc->first_dev;
      adev->mixer_dev = devc->mixer_dev;
      adev->min_rate = 8000;
      adev->max_rate = 192000;
      adev->min_channels = 2;
      adev->max_channels = 2;

      if (endpoints[i].is_modem)
        {
          adev->min_channels = 1;
          adev->max_channels = 1;
          adev->caps |= PCM_CAP_MODEM;
        }

      adev->min_block = 128;	/* Hardware limitation */
      adev->min_fragments = 4;	/* Vmix doesn't work without this */
      portc->num = i;
      portc->open_mode = 0;
      portc->audio_enabled = 0;
      portc->audiodev = audio_dev;
      portc->port_type = PT_INPUT;
      portc->endpoint = &endpoints[i];
      portc->engine = NULL;
      init_adev_caps (devc, adev, portc->endpoint);
    }
}

static void
activate_vmix (hda_devc_t * devc)
{
#ifdef CONFIG_OSS_VMIX
/*
 * Attach vmix engines to all outputs and inputs.
 */

	if (devc->num_outputs > 0)
	   {
		   if (devc->num_inputs < 1)
		      vmix_attach_audiodev(devc->osdev, devc->output_portc[0].audiodev, -1, 0);
		   else
  		      vmix_attach_audiodev(devc->osdev, devc->output_portc[0].audiodev, devc->input_portc[0].audiodev, 0);
	   };
#endif
}

static int
init_HDA (hda_devc_t * devc)
{
  unsigned int gcap;
  unsigned int tmp;

  /* Reset controller */

  if (!reset_controller (devc))
    return 0;

  PCI_WRITEL (devc->osdev, devc->azbar + HDA_INTCTL, PCI_READL (devc->osdev, devc->azbar + HDA_INTCTL) | 0xc0000000);	/* Intr enable */

  /*
   * Set CORB and RIRB sizes to 256, 16 or 2 entries. 
   *
   */
  tmp = (PCI_READB (devc->osdev, devc->azbar + HDA_CORBSIZE) >> 4) & 0x07;
  if (tmp & 0x4)		/* 256 entries */
    PCI_WRITEB (devc->osdev, devc->azbar + HDA_CORBSIZE, 0x2);
  else if (tmp & 0x2)		/* 16 entries */
    PCI_WRITEB (devc->osdev, devc->azbar + HDA_CORBSIZE, 0x1);
  else				/* Assume that 2 entries is supported */
    PCI_WRITEB (devc->osdev, devc->azbar + HDA_CORBSIZE, 0x0);

  tmp = (PCI_READB (devc->osdev, devc->azbar + HDA_RIRBSIZE) >> 4) & 0x07;
  if (tmp & 0x4)		/* 256 entries */
    PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBSIZE, 0x2);
  else if (tmp & 0x2)		/* 16 entries */
    PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBSIZE, 0x1);
  else				/* Assume that 2 entries is supported */
    PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBSIZE, 0x0);

  /* setup the CORB/RIRB structs */
  if (!setup_controller (devc))
    return 0;

  /* setup the engine structs */
  if (!setup_engines (devc))
    return 0;

  if (!devc->codecmask)
    {
      cmn_err (CE_WARN, "No codecs found after reset\n");
      return 0;
    }
  else
    devc->mixer = hdaudio_mixer_create (devc->chip_name, devc, devc->osdev,
					do_corb_write, corb_read,
					devc->codecmask, devc->vendor_id,
					devc->subvendor_id);
  if (devc->mixer == NULL)
    return 0;

  devc->mixer_dev = hdaudio_mixer_get_mixdev (devc->mixer);

  gcap = PCI_READW (devc->osdev, devc->azbar + HDA_GCAP);
  DDB (cmn_err (CE_CONT, " GCAP register content 0x%x\n", gcap));

  if (((gcap >> 3) & 0x0f) > 0)
    cmn_err (CE_WARN, "Bidirectional engines not supported\n");
  if (((gcap >> 1) & 0x03) > 0)
    cmn_err (CE_WARN, "Multiple SDOs not supported\n");

  install_outputdevs (devc);
  install_inputdevs (devc);
  activate_vmix (devc);

  return 1;
}


int
oss_hdaudio_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision, btmp;
  unsigned short pci_command, vendor, device, wtmp;
  unsigned short subvendor, subdevice;
  hda_devc_t *devc;
  static int already_attached = 0;
  int err;
  unsigned short devctl;	
	
  DDB (cmn_err (CE_CONT, "oss_hdaudio_attach entered\n"));

  if (already_attached)
    {
      cmn_err (CE_WARN, "oss_hdaudio_attach: Already attached\n");
      return 0;
    }
  already_attached = 1;

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_word (osdev, PCI_SUBSYSTEM_VENDOR_ID, &subvendor);
  pci_read_config_word (osdev, PCI_SUBSYSTEM_ID, &subdevice);

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->first_dev = -1;

  osdev->hw_info = PMALLOC (osdev, HWINFO_SIZE);	/* Text buffer for additional device info */
  memset (osdev->hw_info, 0, HWINFO_SIZE);

  devc->vendor_id = (vendor << 16) | device;
  devc->subvendor_id = (subvendor << 16) | subdevice;

  oss_pci_byteswap (osdev, 1);

  switch (device)
    {
    case INTEL_DEVICE_CPT:
    case INTEL_DEVICE_PBG:
    case INTEL_DEVICE_PPT:
    case INTEL_DEVICE_SCH:
  	  pci_read_config_word (osdev, 0x78, &devctl);
 	  DDB (cmn_err (CE_CONT, " DEVC register content  0x%04x\n", devctl);)
  	  pci_write_config_word (osdev, 0x78, (devctl & (~0x0800)) );
  	  DDB (pci_read_config_word (osdev, 0x78, &devctl);)
 	  DDB (cmn_err (CE_CONT, " DEVC register content (after clearing DEVC.NSNPEN)  0x%04x\n", devctl);)
	  /* continue is intentional */
    case INTEL_DEVICE_ICH6:
    case INTEL_DEVICE_ICH7:
    case INTEL_DEVICE_ESB2:
    case INTEL_DEVICE_ICH8:
    case INTEL_DEVICE_ICH9:
    case INTEL_DEVICE_ICH9_B:
    case INTEL_DEVICE_ICH10:
    case INTEL_DEVICE_ICH10_B:
    case INTEL_DEVICE_PCH:
    case INTEL_DEVICE_PCH_B:
    case INTEL_DEVICE_PCH_C:
    case INTEL_DEVICE_SUNRISE:
    case INTEL_DEVICE_SKYLAKE:
      devc->chip_name = "Intel HD Audio";
      break;

    case NVIDIA_DEVICE_MCP51:
    case NVIDIA_DEVICE_MCP55:
    case NVIDIA_DEVICE_MCP61:
    case NVIDIA_DEVICE_MCP61A:
    case NVIDIA_DEVICE_MCP65:
    case NVIDIA_DEVICE_MCP67:
    case NVIDIA_DEVICE_MCP73:
    case NVIDIA_DEVICE_MCP78S:
    case NVIDIA_DEVICE_MCP79:
      devc->chip_name = "nVidia HD Audio";
      pci_read_config_byte (osdev, 0x4e, &btmp);
      pci_write_config_byte (osdev, 0x4e, (btmp & 0xf0) | 0x0f);
      pci_read_config_byte (osdev, 0x4d, &btmp);
      pci_write_config_byte (osdev, 0x4d, (btmp & 0xfe) | 0x01);
      pci_read_config_byte (osdev, 0x4c, &btmp);
      pci_write_config_byte (osdev, 0x4c, (btmp & 0xfe) | 0x01);
      break;

    case ATI_DEVICE_SB450:
    case ATI_DEVICE_SB600:
      devc->chip_name = "ATI HD Audio";
      pci_read_config_byte (osdev, 0x42, &btmp);
      pci_write_config_byte (osdev, 0x42, (btmp & 0xf8) | 0x2);
      break;

    case AMD_DEVICE_HUDSON:
	devc->chip_name = "AMD HD Audio";
	break;

    case VIA_DEVICE_HDA:
      devc->chip_name = "VIA HD Audio";
      break;

    case SIS_DEVICE_HDA:
      devc->chip_name = "SiS HD Audio";
      break;

    case ULI_DEVICE_HDA:
      devc->chip_name = "ULI HD Audio";
      pci_read_config_word (osdev, 0x40, &wtmp);
      pci_write_config_word (osdev, 0x40, wtmp | 0x10);
      pci_write_config_dword (osdev, PCI_MEM_BASE_ADDRESS_1, 0);
      break;

    case CREATIVE_XFI_HDA:
      devc->chip_name = "Creative Labs XFi XTreme Audio";
      break;

    default:
      devc->chip_name = "Azalia High Definition audio device";
    }

  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &devc->membar_addr);

  devc->membar_addr &= ~7;

  /* get virtual address */
  devc->azbar =
    (void *) MAP_PCI_MEM (devc->osdev, 0, devc->membar_addr, 16 * 1024);

  /*verify interrupt*/
  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS.\n");
      return 0;
    }

  devc->irq = pci_irq_line;
   
  /* activate the device */
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);


  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, hdaintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  devc->base = devc->membar_addr;

  /* Setup the TCSEL register. Don't do this with ATI chipsets. */
  if (vendor != ATI_VENDOR_ID)
    {
      pci_read_config_byte (osdev, 0x44, &btmp);
      pci_write_config_byte (osdev, 0x44, btmp & 0xf8);
     }

  err = init_HDA (devc);
  return err;
}

int
oss_hdaudio_detach (oss_device_t * osdev)
{
  hda_devc_t *devc = (hda_devc_t *) osdev->devc;
  int j;

  if (oss_disable_device (osdev) < 0)
    return 0;

  PCI_WRITEL (devc->osdev, devc->azbar + HDA_INTSTS, 0xc0000000);	/* ack pending ints */

  PCI_WRITEL (devc->osdev, devc->azbar + HDA_INTCTL, 0);	/* Intr disable */
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_STATESTS, 0x7);	/* Intr disable */
  PCI_WRITEL (devc->osdev, devc->azbar + HDA_RIRBSTS, 0x5);	/* Intr disable */
  PCI_WRITEB (devc->osdev, devc->azbar + HDA_RIRBCTL, 0);	/* Stop */
  PCI_WRITEB (devc->osdev, devc->azbar + HDA_CORBCTL, 0);	/* Stop */

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  if (devc->corb != NULL)
    CONTIG_FREE (devc->osdev, devc->corb, 4096, devc->corb_dma_handle);

  devc->corb = NULL;

  for (j = 0; j < devc->num_outengines; j++)
    {
      hda_engine_t *engine = &devc->outengines[j];

      if (engine->bdl == NULL)
	continue;
      CONTIG_FREE (devc->osdev, engine->bdl, 4096, engine->bdl_dma_handle);
    }

  for (j = 0; j < devc->num_inengines; j++)
    {
      hda_engine_t *engine = &devc->inengines[j];

      if (engine->bdl == NULL)
	continue;
      CONTIG_FREE (devc->osdev, engine->bdl, 4096, engine->bdl_dma_handle);
    }

  if (devc->membar_addr != 0)
    {
      UNMAP_PCI_MEM (devc->osdev, 0, devc->membar_addr, devc->azbar,
		     16 * 1024);
      devc->membar_addr = 0;
    }

  oss_spdif_uninstall (&devc->spdc);

  oss_unregister_device (devc->osdev);
  return 1;
}
