/*
 * Purpose: Driver for RME MADI and AES32 audio interfaces
 *
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

#include "oss_madi_cfg.h"
#include "oss_pci.h"
#include "madi.h"

#define RME_VENDOR_ID		0x10ee	/* Xilinx actually */
#define RME_MADI		0x3fc6

#define HWINFO_SIZE	128

extern int madi_devsize;
extern int madi_maxchannels;

#define LSWAP(x) 	x

static void
set_output_enable (madi_devc_t * devc, unsigned int chn, int enabled)
{
  madi_write (devc, MADI_outputEnableStart + 4 * chn, enabled);
}

static void
set_input_enable (madi_devc_t * devc, unsigned int chn, int enabled)
{
  madi_write (devc, MADI_inputEnableStart + 4 * chn, enabled);
}

static void
start_audio (madi_devc_t * devc)
{
  madi_control (devc, devc->cmd | (MADI_AudioInterruptEnable | MADI_Start));
}

static void
stop_audio (madi_devc_t * devc)
{
  madi_control (devc, devc->cmd & ~(MADI_AudioInterruptEnable | MADI_Start));
}

static void
audio_interrupt (madi_devc_t * devc, unsigned int status)
{
  int i;

  for (i = 0; i < devc->num_outputs; i++)
    {
      madi_portc_t *portc = devc->out_portc[i];

      if (!(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	continue;

      oss_audio_outputintr (portc->audio_dev, 1);
    }

  for (i = 0; i < devc->num_inputs; i++)
    {
      madi_portc_t *portc = devc->in_portc[i];

      if (!(portc->trigger_bits & PCM_ENABLE_INPUT))
	continue;

      oss_audio_inputintr (portc->audio_dev, 1);
    }
}

static int
madiintr (oss_device_t * osdev)
{
  madi_devc_t *devc = osdev->devc;
  unsigned int status;

  status = madi_read (devc, MADI_status);

  if (status & MADI_audioIntPending)
    audio_interrupt (devc, status);

  madi_write (devc, MADI_interruptAck, 0);

  return !!(status &
	  (MADI_audioIntPending | MADI_midi0IRQStatus | MADI_midi1IRQStatus))
    != 0;
}

static void
madi_change_hw_rate (madi_devc_t * devc)
{
  unsigned int rate_bits = 0;
  unsigned long long v;

  devc->rate = devc->next_rate;
  /*
   * Compute and set the sample rate in control2 register.
   */

  rate_bits = MADI_Freq1;	// TODO

  devc->cmd &= ~MADI_FreqMask;
  devc->cmd |= rate_bits;

  madi_control (devc, devc->cmd);

  /*
   * Compute and set the DDS value
   */
  v = 110100480000000ULL / (unsigned long long) devc->rate;
  madi_write (devc, MADI_freq, (unsigned int) v);
}

static void
initialize_hardware (madi_devc_t * devc)
{
  int chn, src;

/*
 * Intialize the control register
 */
  devc->cmd = MADI_ClockModeMaster | MADI_LatencyMask | MADI_LineOut;

  if (devc->model == MDL_AES32)
    devc->cmd |= MADI_SyncSrc0 | MADI_ProBit;
  else
    devc->cmd |=
      MADI_InputCoax | MADI_SyncRef_MADI | MADI_TX_64ch_mode | MADI_AutoInput;

  madi_control (devc, devc->cmd);

/*
 * Set all input and output engines to stoped
 */
  for (chn = 0; chn < MAX_CHANNELS; chn++)
    {
      set_output_enable (devc, chn, 0);
      set_input_enable (devc, chn, 0);
    }

/*
 * Init control2 register
 */
  if (devc->model == MDL_MADI)
#ifdef OSS_BIG_ENDIAN
    madi_control2 (devc, MADI_BIGENDIAN_MODE);
#else
    madi_control2 (devc, 0x0);
#endif

  madi_change_hw_rate (devc);

/*
 * Write all HW mixer registers
 */

  for (chn = 0; chn < MAX_CHANNELS; chn++)
    for (src = 0; src < 2 * MAX_CHANNELS; src++)
      madi_write_gain (devc, chn, src, devc->mixer_values[chn][src]);
}

static int
madi_set_rate (int dev, int arg)
{
  madi_devc_t *devc = audio_engines[dev]->devc;

  return devc->rate;
}

static __inline__ void
reserve_rec_channels(madi_devc_t *devc, int c, int nc)
{
	int ch;

	for (ch=c;ch < c+nc;ch++)
	    devc->busy_rec_channels |= (1ULL << ch);
}

static __inline__ void
free_rec_channels(madi_devc_t *devc, int c, int nc)
{
	int ch;

	for (ch=c;ch < c+nc;ch++)
	    devc->busy_rec_channels &= ~(1ULL << ch);
}

static __inline__ int
rec_channels_avail(madi_devc_t *devc, int c, int nc)
{
	int ch;

	for (ch=c;ch < c+nc;ch++)
	    if (devc->busy_rec_channels & (1ULL << ch))
		    return 0;

	return 1;
}

static __inline__ void
reserve_play_channels(madi_devc_t *devc, int c, int nc)
{
	int ch;

	for (ch=c;ch < c+nc;ch++)
	    devc->busy_play_channels |= (1ULL << ch);
}

static __inline__ void
free_play_channels(madi_devc_t *devc, int c, int nc)
{
	int ch;

	for (ch=c;ch < c+nc;ch++)
	    devc->busy_play_channels &= ~(1ULL << ch);
}

static __inline__ int
play_channels_avail(madi_devc_t *devc, int c, int nc)
{
	int ch;

	for (ch=c;ch < c+nc;ch++)
	    if (devc->busy_play_channels & (1ULL << ch))
	    {
		    return 0;
	    }

	return 1;
}

static short
madi_set_channels (int dev, short arg)
{
  madi_portc_t *portc = audio_engines[dev]->portc;
  madi_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  int ok=1;

  if (arg < 1)
    return portc->channels;

  if (arg != 1 && arg != 2 && arg != 4 && arg != 8 && arg != 16 && arg != 32
      && arg != 64)
    return portc->channels;

  if (arg > portc->max_channels)
    return portc->channels;

  if (arg == portc->channels)
    return portc->channels;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (arg > portc->channels) /* Needs more channels */
     {
	  if (portc->direction == DIR_OUT)
	     {
		     if (!play_channels_avail(devc, portc->channel+portc->channels, arg - portc->channels))
		  	ok=0;
	     }
	  else
	     {
		     if (!rec_channels_avail(devc, portc->channel+portc->channels, arg - portc->channels))
		  	ok=0;
	     }
     }
  else
     {
	  if (portc->direction == DIR_OUT)
	     {
		     free_play_channels(devc, portc->channel+arg, portc->channels-arg);
	     }
	  else
	     {
		     free_rec_channels(devc, portc->channel+arg, portc->channels-arg);
	     }
     }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (!ok)
     return portc->channels;

  return portc->channels = arg;
}

 /*ARGSUSED*/ static unsigned int
madi_set_format (int dev, unsigned int arg)
{
  return AFMT_S32_LE;
}

static int
madi_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void
madi_trigger (int dev, int state)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;
  int ch;

  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode & OPEN_WRITE)
    {
      if ((state & PCM_ENABLE_OUTPUT)
	  && !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	{
	  for (ch = 0; ch < portc->channels; ch++)
	    set_output_enable (devc, portc->channel + ch, 1);
	  portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	  devc->active_outputs |= (1ULL << portc->channel);
	}
      else if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	{
	  for (ch = 0; ch < portc->channels; ch++)
	    set_output_enable (devc, portc->channel + ch, 0);
	  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	  devc->active_outputs &= ~(1ULL << portc->channel);
	}
    }

  if ((portc->open_mode & OPEN_READ)
      && !(portc->trigger_bits & PCM_ENABLE_INPUT))
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  for (ch = 0; ch < portc->channels; ch++)
	    set_input_enable (devc, portc->channel + ch, 1);
	  portc->trigger_bits |= PCM_ENABLE_INPUT;
	  devc->active_inputs |= (1ULL << portc->channel);
	}
      else if (portc->trigger_bits & PCM_ENABLE_INPUT)
	{
	  for (ch = 0; ch < portc->channels; ch++)
	    set_input_enable (devc, portc->channel + ch, 0);
	  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	  devc->active_inputs &= ~(1ULL << portc->channel);
	}
    }

  if (devc->active_inputs || devc->active_outputs)
    start_audio (devc);
  else
    stop_audio (devc);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static int
madi_sync_control (int dev, int event, int mode)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  int ch;

  if (event == SYNC_PREPARE)
    {
      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
      if ((mode & PCM_ENABLE_OUTPUT) && portc->direction == DIR_OUT)
	{
	  for (ch = 0; ch < portc->channels; ch++)
	    set_output_enable (devc, portc->channel + ch, 1);
	  portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	  devc->active_outputs |= (1ULL << portc->channel);
	}

      if ((mode & PCM_ENABLE_INPUT) && portc->direction == DIR_IN)
	{
	  for (ch = 0; ch < portc->channels; ch++)
	    set_input_enable (devc, portc->channel + ch, 1);
	  portc->trigger_bits |= PCM_ENABLE_INPUT;
	  devc->active_inputs |= (1ULL << portc->channel);
	}
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
    }

  if (event == SYNC_TRIGGER)
    {
      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

      if (devc->active_inputs || devc->active_outputs)
         start_audio (devc);

      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

      return 0;
    }

  return OSS_EIO;
}

static void
madi_halt (int dev)
{
  madi_trigger (dev, 0);
}

 /*ARGSUSED*/ static int
madi_open (int dev, int mode, int open_flags)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  int ok=1;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode != 0)
     ok=0;
  else
  if (portc->direction == DIR_OUT)
     {
	     if (!play_channels_avail(devc, portc->channel, 2))
	     {
		ok=0;
	     }
     }
  else
     {
	     if (!rec_channels_avail(devc, portc->channel, 2))
	     {
		ok=0;
	     }
     }

  if (!ok)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  devc->open_audiodevs++;

  portc->channels = 2;
  if (portc->direction == DIR_OUT)
     reserve_play_channels(devc, portc->channel, 2);
  else
     reserve_rec_channels(devc, portc->channel, 2);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

 /*ARGSUSED*/ static void
madi_close (int dev, int mode)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->open_mode = 0;
  devc->open_audiodevs--;
  if (portc->direction == DIR_OUT)
  {
     free_play_channels(devc, portc->channel, portc->channels);
  }
  else
  {
     free_rec_channels(devc, portc->channel, portc->channels);
  }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

 /*ARGSUSED*/ static void
madi_output_block (int dev, oss_native_word buf, int count,
		   int fragsize, int intrflag)
{
}

 /*ARGSUSED*/ static void
madi_start_input (int dev, oss_native_word buf, int count, int fragsize,
		  int intrflag)
{
}

 /*ARGSUSED*/ static int
madi_prepare_for_input (int dev, int bsize, int bcount)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  int i;
  int offs = portc->channel * CHBUF_PAGES;

  for (i = 0; i < CHBUF_PAGES * portc->channels; i++)
    madi_write (devc, MADI_RecPageTable + 4 * (offs + i),
		LSWAP(dmap->dmabuf_phys + i * 4096));

  return 0;
}

 /*ARGSUSED*/ static int
madi_prepare_for_output (int dev, int bsize, int bcount)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  int i;
  int offs = portc->channel * CHBUF_PAGES;

  for (i = 0; i < CHBUF_PAGES * portc->channels; i++)
    madi_write (devc, MADI_PlayPageTable + 4 * (offs + i),
		LSWAP(dmap->dmabuf_phys + i * 4096));
  return 0;
}

 /*ARGSUSED*/ static int
madi_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;

  if (direction == OPEN_READ)
    {
      dmap->dmabuf = devc->recbuf + portc->channel * CHBUF_SIZE;
      dmap->dmabuf_phys = devc->recbuf_phys + portc->channel * CHBUF_SIZE;
      dmap->buffsize = CHBUF_SIZE;
    }
  else
    {
      dmap->dmabuf = devc->playbuf + portc->channel * CHBUF_SIZE;
      dmap->dmabuf_phys = devc->playbuf_phys + portc->channel * CHBUF_SIZE;
      dmap->buffsize = CHBUF_SIZE;
    }

  return 0;
}

 /*ARGSUSED*/ static int
madi_free_buffer (int dev, dmap_t * dmap, int direction)
{
  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;
  dmap->buffsize = 0;

  return 0;
}

 /*ARGSUSED*/ static int
madi_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;
  unsigned int status;
#if 1
  /*
   * Use only full/half buffer resolution.
   */
  int bytes;
  bytes = ((devc->cmd & MADI_LatencyMask) >> 1) + 8;
  bytes = 1 << bytes;
  bytes *= portc->channels;

  status = madi_read (devc, MADI_status);
  return (status & MADI_BufferHalf) ? bytes : 0;
#else
  /*
   * Use full resolution
   */
  status = madi_read (devc, MADI_status) & MADI_BufferPosMask;

  return status * portc->channels;
#endif
}

/*ARGSUSED*/
static void
madi_setup_fragments (int dev, dmap_t * dmap, int direction)
{
  madi_devc_t *devc = audio_engines[dev]->devc;
  madi_portc_t *portc = audio_engines[dev]->portc;
  int bytes;

  bytes = ((devc->cmd & MADI_LatencyMask) >> 1) + 8;
  bytes = 1 << bytes;

  dmap->buffsize = portc->channels * CHBUF_SIZE;

#if 1
  /*
   * Use two fragment (ping-pong) buffer that is also used by the hardware.
   */
  dmap->fragment_size = bytes * portc->channels;
  dmap->nfrags = 2;
#else
/*
 * Use 4 fragments instead of 2 to avoid clicking caused by accessing the
 * DMA buffer too close to the active DMA position.
 *
 * This means that the DMA pointer will jump by 2 fregments every time there
 * is a half/full bufferinterrupt.
 */
  dmap->fragment_size = (bytes * portc->channels) / 2;
  dmap->nfrags = 4;
#endif

  dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;
}

static audiodrv_t madi_driver = {
  madi_open,
  madi_close,
  madi_output_block,
  madi_start_input,
  madi_ioctl,
  madi_prepare_for_input,
  madi_prepare_for_output,
  madi_halt,
  NULL,				// madi_local_qlen,
  NULL,
  NULL,				// madi_halt_input,
  NULL,				// madi_halt_output,
  madi_trigger,
  madi_set_rate,
  madi_set_format,
  madi_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  madi_alloc_buffer,
  madi_free_buffer,
  NULL,
  NULL,
  madi_get_buffer_pointer,
  NULL,
  madi_sync_control,
  NULL,
  NULL,
  NULL,
  NULL,
  madi_setup_fragments
};

static int
create_output_device (madi_devc_t * devc, char *name, int chn)
{
  madi_portc_t *portc;
  adev_t *adev;
  int n;

  if ((portc = PMALLOC (devc->osdev, sizeof (*portc))) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate portc structure\n");
      return OSS_ENOMEM;
    }
  memset (portc, 0, sizeof (*portc));

  n = devc->num_outputs++;;
  portc->channel = chn;
  portc->channels = 2;
  portc->max_channels = 64 - chn;

  devc->out_portc[n] = portc;

  if ((portc->audio_dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
						devc->osdev,
						devc->osdev,
						name,
						&madi_driver,
						sizeof (audiodrv_t),
						ADEV_NOINPUT |
						ADEV_NONINTERLEAVED |
						ADEV_NOMMAP, AFMT_S32_LE,
						devc, -1)) < 0)
    {
      return portc->audio_dev;
    }

  adev = audio_engines[portc->audio_dev];

  if (devc->first_audiodev == -1)
    devc->first_audiodev = portc->audio_dev;
  adev->rate_source = devc->first_audiodev;

  adev->mixer_dev = devc->mixer_dev;
  adev->portc = portc;
  adev->min_rate = devc->rate;
  adev->max_rate = devc->rate;
  adev->min_channels = 1;
  adev->max_channels = portc->max_channels;
  adev->min_block = CHBUF_SIZE / 2;
  adev->max_block = CHBUF_SIZE / 2;

  portc->direction = DIR_OUT;

  return portc->audio_dev;
}

static int
create_input_device (madi_devc_t * devc, char *name, int chn)
{
  madi_portc_t *portc;
  adev_t *adev;
  int n;

  if ((portc = PMALLOC (devc->osdev, sizeof (*portc))) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate portc structure\n");
      return OSS_ENOMEM;
    }
  memset (portc, 0, sizeof (*portc));

  n = devc->num_inputs++;;
  portc->channel = chn;
  portc->channels = 2;
  portc->max_channels = 64 - chn;

  devc->in_portc[n] = portc;

  if ((portc->audio_dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
						devc->osdev,
						devc->osdev,
						name,
						&madi_driver,
						sizeof (audiodrv_t),
						ADEV_NOOUTPUT |
						ADEV_NONINTERLEAVED |
						ADEV_NOMMAP, AFMT_S32_LE,
						devc, -1)) < 0)
    {
      return portc->audio_dev;
    }

  adev = audio_engines[portc->audio_dev];

  if (devc->first_audiodev == -1)
    devc->first_audiodev = portc->audio_dev;
  adev->rate_source = devc->first_audiodev;

  adev->mixer_dev = devc->mixer_dev;
  adev->portc = portc;
  adev->min_rate = devc->rate;
  adev->max_rate = devc->rate;
  adev->min_channels = 1;
  adev->max_channels = portc->max_channels;
  adev->min_block = CHBUF_SIZE / 2;
  adev->max_block = CHBUF_SIZE / 2;

  portc->direction = DIR_IN;

  return portc->audio_dev;
}

int
oss_madi_attach (oss_device_t * osdev)
{
  madi_devc_t *devc;
  unsigned char pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int i;
  int err;
  int my_mixer;

  int chn, src;

  DDB (cmn_err (CE_NOTE, "Entered RME MADI detect routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != RME_VENDOR_ID || device != RME_MADI)
    return 0;

  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &pci_ioaddr);

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  if (pci_ioaddr == 0)
    {
      cmn_err (CE_WARN, "BAR0 not initialized by BIOS\n");
      return 0;
    }

  if ((devc = PMALLOC (osp, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate devc\n");
      return 0;
    }
  memset (devc, 0, sizeof (*devc));
  devc->osdev = osdev;
  osdev->devc = devc;
  devc->first_audiodev = -1;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);

  DDB (cmn_err (CE_CONT, "RME HDSPM revision %d\n", pci_revision));

  if (pci_revision < 230)
    {
      devc->name = "RME MADI";
      devc->model = MDL_MADI;
    }
  else
    {
      devc->name = "RME AES32";
      devc->model = MDL_AES32;
    }

  DDB (cmn_err (CE_CONT, "Card model is %s\n", devc->name));

  devc->rate = devc->next_rate = 48000;	// TODO: Also set the other rate control fields */

  osdev->hw_info = PMALLOC (osdev, HWINFO_SIZE);	/* Text buffer for additional device info */
  sprintf (osdev->hw_info, "PCI revision %d", pci_revision);

  oss_register_device (osdev, devc->name);

  devc->physaddr = pci_ioaddr;
  devc->registers = MAP_PCI_MEM (devc->osdev, 0, devc->physaddr, 65536);

  if (devc->registers == NULL)
    {
      cmn_err (CE_WARN, "Can't map PCI registers (0x%08x)\n", devc->physaddr);
      return 0;
    }

  devc->playbuf = CONTIG_MALLOC (devc->osdev, DMABUF_SIZE, MEMLIMIT_32BITS,
				 &devc->playbuf_phys, devc->play_dma_handle);
  if (devc->playbuf == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate play DMA buffers\n");
      return 0;
    }

  devc->recbuf = CONTIG_MALLOC (devc->osdev, DMABUF_SIZE, MEMLIMIT_32BITS,
				&devc->recbuf_phys, devc->rec_dma_handle);
  if (devc->recbuf == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate rec DMA buffers\n");
      return 0;
    }

  if ((err = oss_register_interrupts (devc->osdev, 0, madiintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  if (madi_maxchannels < 2) madi_maxchannels=2;
  if (madi_maxchannels > MAX_CHANNELS)madi_maxchannels=MAX_CHANNELS;

  if (madi_devsize < 1) madi_devsize = 1;

  if (madi_devsize != 1 && madi_devsize != 2 && madi_devsize != 4)
     madi_devsize = 2;

  /*
   * Set the hw mixer shadow values to sane levels so that 
   * initialize_hardware() can write them to the actual registers.
   */

  for (chn = 0; chn < MAX_CHANNELS; chn++)
    for (src = 0; src < 2 * MAX_CHANNELS; src++)
      devc->mixer_values[chn][src] = MUTE_GAIN;	/* Set everything off */

#if 0
  for (src = 0; src < MAX_CHANNELS; src += 2)
    {
      /* Setup playback monitoring to analog out */
      devc->mixer_values[MONITOR_CH][SRC_PLAY | src] = UNITY_GAIN / 10;
      devc->mixer_values[MONITOR_CH + 1][SRC_PLAY | (src + 1)] =
	UNITY_GAIN / 10;
    }
#endif

  for (chn = 0; chn < MAX_CHANNELS; chn++)
    devc->mixer_values[chn][SRC_PLAY | chn] = UNITY_GAIN;	/* N->N outputs on */

  initialize_hardware (devc);

  if ((my_mixer = madi_install_mixer (devc)) < 0)
    {
      devc->mixer_dev = -1;
      return 0;
    }
  else
    {
      devc->mixer_dev = my_mixer;
    }

  if (madi_devsize == 1)
     {
	  for (i = 0; i < madi_maxchannels; i++)
	    {
	      char tmp[32];
	      sprintf (tmp, "%s out%d", devc->name, i+1);
	      create_output_device (devc, tmp, i);
	    }
	
	  for (i = 0; i < madi_maxchannels; i++)
	    {
	      char tmp[32];
	      sprintf (tmp, "%s in%d", devc->name, i+1);
	      create_input_device (devc, tmp, i);
	    }
     }
  else
     {
	  for (i = 0; i < madi_maxchannels; i += madi_devsize)
	    {
	      char tmp[32];
	      sprintf (tmp, "%s out%d-%d", devc->name, i+1, i + madi_devsize);
	      create_output_device (devc, tmp, i);
	    }
	
	  for (i = 0; i < madi_maxchannels; i += madi_devsize)
	    {
	      char tmp[32];
	      sprintf (tmp, "%s in%d-%d", devc->name, i+1, i + madi_devsize);
	      create_input_device (devc, tmp, i);
	    }
     }

  madi_activate_mixer (devc);

  return 1;
}

int
oss_madi_detach (oss_device_t * osdev)
{
  madi_devc_t *devc = (madi_devc_t *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  stop_audio (devc);

  /*
   * Shut up the hardware
   */
  devc->cmd &= ~MADI_FreqMask;
  madi_control (devc, devc->cmd);

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);

  if (devc->registers != NULL)
    UNMAP_PCI_MEM (osdev, 0, devc->physaddr, devc->registers, 65536);
  CONTIG_FREE (devc->osdev, devc->playbuf, DMABUF_SIZE,
	       devc->play_dma_handle);
  CONTIG_FREE (devc->osdev, devc->recbuf, DMABUF_SIZE, devc->rec_dma_handle);

  oss_unregister_device (devc->osdev);

  return 1;
}
