/*
 * Purpose: Driver for Sound Blaster X-Fi (emu20k)
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

#include "oss_sbxfi_cfg.h"
#include <oss_pci.h>
#include "sbxfi.h"
#include "20k1reg.h"
#include "hwaccess.h"

#define PCI_VENDOR_CREATIVE	0x1102
#define 	CREATIVE_SBXFI_K1	0x0005
#define 	CREATIVE_SBXFI_K2	0x000b
#define 	CREATIVE_SBXFI_E	0x0009

#define TIMER_INTERVAL	5	/* In milliseconds */

#define DEFAULT_PLAY_RATE	96000	/* Default rate of play devices */
#define DEFAULT_REC_RATE	96000	/* Default rate of rec devices */
#define HARDWARE_RATE	96000	/* Internal rate used by the hardware */

static void
set_interval_timer(sbxfi_devc_t *devc, int msecs)
{
  int tic = (HARDWARE_RATE*msecs)/1000;

  HwWrite20K1 (devc, TIMR, tic | TIMR_IE|TIMR_IP);
}

static int
sbxfi_intr(oss_device_t *osdev)
{
  unsigned int status;
  sbxfi_devc_t *devc = osdev->devc;

  status = HwRead20K1 (devc, GIP);

  if (status==0) /* Not for me */
     return 0;

  devc->interrupt_count++;

#if 0
  // Not using loop interrupts.
  if (status & SRC_INT)	/* SRC interrupt(s) pending */
  {
	  unsigned int srcipm, srcip;
	  int i;

  	  srcipm = HwRead20K1 (devc, SRCIPM); /* SRC interrupt pending map register */

	  for (i=0;i<7;i++)
	  if (srcipm & (1<<i))
	  {
		int j;
  	  	srcip = HwRead20K1 (devc, SRCIP(i)); /* SRC interrupt pending register for block(i) */

		for (j=0;j<32;j++)
		if (srcip & (1<<j))
		{
			int chn=i*32+j;
			sbxfi_portc_t *portc;

			portc=devc->src_to_portc[chn];

			if (portc==NULL)
			{
				cmn_err(CE_NOTE, "portc==NULL\n");
				continue;
			}

			oss_audio_outputintr(portc->dev, 0);
		}

  		HwWrite20K1 (devc, SRCIP(i), srcip);	/* Acknowledge SRC interrupts for block(i) */
	  }
  }
#endif

  if (status & IT_INT)
  {
  /*
   * Interval timer interrupt
   */
	sbxfi_portc_t *portc;
	int i;

	for (i=0;i<devc->nr_outdevs;i++)
	{
		portc=&devc->play_portc[i];
		if (portc->running)
	   	   oss_audio_outputintr(portc->dev, 0);
	}

	for (i=0;i<devc->nr_indevs;i++)
	{
		portc=&devc->rec_portc[i];
		if (portc->running)
	   	   oss_audio_inputintr(portc->dev, 0);
	}

	set_interval_timer(devc, TIMER_INTERVAL); /* Rearm interval timer */
  }

  HwWrite20K1 (devc, GIP, status & FI_INT);	/* Acknowledge interrupts */
  return 1;
}

 /*ARGSUSED*/ static int
sbxfi_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static mixer_driver_t sbxfi_mixer_driver = {
  sbxfi_mixer_ioctl
};

static int
sbxfi_set_rate (int dev, int arg)
{
  sbxfi_devc_t *devc = audio_engines[dev]->devc;
  sbxfi_portc_t *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
#if 0
  // TODO: Implement support for other rates

  if (arg == 0)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return portc->rate;
    }

#else
  if (portc->direction == PCM_ENABLE_OUTPUT)
     arg=DEFAULT_PLAY_RATE;
  else
     arg=DEFAULT_REC_RATE;
#endif
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return portc->rate = arg;
}

static short
sbxfi_set_channels (int dev, short arg)
{
  sbxfi_portc_t *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->channels;

  if (arg<2)
     arg=2;
  else
     if (arg > MAX_PLAY_CHANNELS)
	arg = MAX_PLAY_CHANNELS;
  arg &= ~1; /* Even number of channels */

  return portc->channels = arg;
}

static unsigned int
sbxfi_set_format (int dev, unsigned int arg)
{
  sbxfi_portc_t *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->fmt;

  return portc->fmt = SUPPORTED_FORMAT;
}

static int
sbxfi_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  //sbxfi_portc_t *portc = audio_engines[dev]->portc;
  //sbxfi_devc_t *devc = audio_engines[dev]->devc;

  return OSS_EINVAL;
}

static void sbxfi_trigger (int dev, int state);

static void
sbxfi_reset (int dev)
{
  sbxfi_trigger (dev, 0);
}

 /*ARGSUSED*/ static int
sbxfi_open_input (int dev, int mode, int open_flags)
{
  sbxfi_portc_t *portc = audio_engines[dev]->portc;
  sbxfi_devc_t *devc = audio_engines[dev]->devc;
  adev_p adev = audio_engines[dev];
  oss_native_word flags;

  if (mode & OPEN_WRITE)
    {
      cmn_err (CE_CONT, "Playback is not possible with %s\n", adev->devnode);
      return OSS_ENOTSUP;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }
  portc->open_mode = mode;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

 /*ARGSUSED*/ static int
sbxfi_open_output (int dev, int mode, int open_flags)
{
  sbxfi_portc_t *portc = audio_engines[dev]->portc;
  sbxfi_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  adev_p adev = audio_engines[dev];

  if (mode == OPEN_READ)
    {
      cmn_err (CE_CONT, "Recording is not possible with %s\n", adev->devnode);
      return OSS_ENOTSUP;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
sbxfi_close (int dev, int mode)
{
  sbxfi_portc_t *portc = audio_engines[dev]->portc;
  sbxfi_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  portc->open_mode = 0;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

 /*ARGSUSED*/ static void
sbxfi_output_block (int dev, oss_native_word buf, int count, int fragsize,
		    int intrflag)
{
}

 /*ARGSUSED*/ static void
sbxfi_start_input (int dev, oss_native_word buf, int count, int fragsize,
		   int intrflag)
{
}

static void
sbxfi_trigger (int dev, int state)
{
  sbxfi_devc_t *devc = audio_engines[dev]->devc;
  sbxfi_portc_t *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->state_bits == state)	/* No change */
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return;
    }
  portc->state_bits = state;

  if (portc->direction == PCM_ENABLE_OUTPUT)
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
  		SetupAndStartPlaySRC (devc, portc);
		portc->running=1;
	}
      else
	{
  		StopPlaySRC (devc, portc);
		portc->running=0;
	}
    }

  if (portc->direction == PCM_ENABLE_INPUT)
  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	{
  		SetupAndStartRecordSRC (devc, portc);
		portc->running=1;
	}
      else
	{
  		StopRecordSRC (devc, portc);
		portc->running=0;
	}
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

 /*ARGSUSED*/ static int
sbxfi_prepare_for_input (int dev, int bsize, int bcount)
{
  sbxfi_devc_t *devc = audio_engines[dev]->devc;
  sbxfi_portc_t *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_in;
  oss_native_word flags;
  int i;

  if (audio_engines[dev]->flags & ADEV_NOINPUT)
    return OSS_EACCES;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  // Fill record buffer to the page table entries.
  // This need to sync up with setting SRCs start addx
  for (i=0;i<(bsize*bcount)/4096;i++)
     devc->pdwPageTable[portc->pgtable_index+i] = dmap->dmabuf_phys + (i*4096);

  InitADC (devc, portc->dwDAChan[0], FALSE);
 
  // Program input mapper
  SetupRecordInputMapper (devc, portc);
  
  // Program I2S
  SetupRecordFormat (devc);
  SetupRecordMixer (devc, portc);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

 /*ARGSUSED*/ static int
sbxfi_prepare_for_output (int dev, int bsize, int bcount)
{
  sbxfi_devc_t *devc = audio_engines[dev]->devc;
  sbxfi_portc_t *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_out;
  oss_native_word flags;
  int i;

  if (audio_engines[dev]->flags & ADEV_NOOUTPUT)
    return OSS_EACCES;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  // Fill play buffer to the page table entries.
  // This need to sync up with setting SRCs start addx
  for (i=0;i<(bsize*bcount)/4096;i++)
     devc->pdwPageTable[portc->pgtable_index+i] = dmap->dmabuf_phys + (i*4096);

  InitDAC (devc, portc);
  
  // Program I2S
  SetupPlayFormat (devc, portc);
  SetupPlayMixer (devc, portc);
 
  // Program input mapper
  SetupPlayInputMapper (devc, portc);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
sbxfi_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  sbxfi_devc_t *devc = audio_engines[dev]->devc;
  sbxfi_portc_t *portc = audio_engines[dev]->portc;
  int pos;
  
  pos = HwRead20K1 (devc, SRCCA(portc->SrcChan)) & 0x03ffffff;

  if (pos>=128)
     pos -= 128; /* The pointer is always 128 bytes ahead */

  pos -= portc->pgtable_index*4096;

  return pos;
}

static audiodrv_t sbxfi_output_driver = {
  sbxfi_open_output,
  sbxfi_close,
  sbxfi_output_block,
  sbxfi_start_input,
  sbxfi_ioctl,
  sbxfi_prepare_for_input,
  sbxfi_prepare_for_output,
  sbxfi_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  sbxfi_trigger,
  sbxfi_set_rate,
  sbxfi_set_format,
  sbxfi_set_channels,
  NULL,
  NULL,
  NULL,				/* check input */
  NULL,				/* sbxfi_check_output */
  NULL,				/* sbxfi_alloc_buffer */
  NULL,				/* sbxfi_free_buffer */
  NULL,
  NULL,
  sbxfi_get_buffer_pointer
};

static audiodrv_t sbxfi_input_driver = {
  sbxfi_open_input,
  sbxfi_close,
  sbxfi_output_block,
  sbxfi_start_input,
  sbxfi_ioctl,
  sbxfi_prepare_for_input,
  sbxfi_prepare_for_output,
  sbxfi_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  sbxfi_trigger,
  sbxfi_set_rate,
  sbxfi_set_format,
  sbxfi_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* sbxfi_alloc_buffer */
  NULL,				/* sbxfi_free_buffer */
  NULL,
  NULL,
  sbxfi_get_buffer_pointer
};

static int
init_play_device (sbxfi_devc_t * devc,
		  char *name, int dev_flags)
{
  int opts, dev, formats;
  char tmp[80];
  sbxfi_portc_t *portc = NULL;
  adev_p adev;

  sprintf (tmp, "%s %s", devc->name, name);

  if (devc->nr_outdevs > MAX_OUTPUTDEVS)
    {
      cmn_err (CE_CONT, "Too many audio devices\n");
      return -1;
    }

  opts = ADEV_AUTOMODE | ADEV_NOINPUT;

  formats = SUPPORTED_FORMAT;

  if ((dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				   devc->osdev,
				   devc->osdev,
				   tmp,
				   &sbxfi_output_driver,
				   sizeof (audiodrv_t),
				   opts, formats, devc, -1)) < 0)
    {
      return -1;
    }

  if (devc->first_dev == -1)
    {
      devc->first_dev = dev;
    }
  adev = audio_engines[dev];

  portc = &devc->play_portc[devc->nr_outdevs];

  adev->portc = portc;
  adev->devc = devc;
  adev->mixer_dev = devc->mixer_dev;
  adev->rate_source = devc->first_dev;
  adev->min_rate = 48000;
  adev->max_rate = 92600;
  adev->min_block=4096;
  adev->dmabuf_maxaddr = MEMLIMIT_ISA;

  portc->dev = dev;
  portc->open_mode = 0;
  portc->fmt = SUPPORTED_FORMAT;
  portc->dev_flags = dev_flags;
  portc->state_bits = 0;
  portc->direction = PCM_ENABLE_OUTPUT;

  portc->rate = DEFAULT_PLAY_RATE;

  // use the following SRC channels for Play
  portc->SrcChan = devc->next_src;
  devc->next_src += MAX_PLAY_CHANNELS;
  devc->src_to_portc[portc->SrcChan]=portc;

  portc->dwDAChan[0] = I2SA_L;
  portc->dwDAChan[1] = I2SA_R;
#if MAX_PLAY_CHANNELS>2
  portc->dwDAChan[2] = I2SB_L;
  portc->dwDAChan[3] = I2SB_R;
  portc->dwDAChan[4] = I2SC_L;
  portc->dwDAChan[5] = I2SC_R;
#endif

  portc->vol_left=portc->vol_right=MIXER_VOLSTEPS;

  adev->min_channels = 2;
  adev->max_channels = MAX_PLAY_CHANNELS;

  portc->pgtable_index = devc->next_pg;
  devc->next_pg += 128/4;	// Up to 128k for buffer

  devc->nr_outdevs++;

  return dev;
}

static int
init_rec_device (sbxfi_devc_t * devc,
		  char *name, int dev_flags)
{
  int opts, dev, formats;
  char tmp[80];
  sbxfi_portc_t *portc = NULL;
  adev_p adev;

  sprintf (tmp, "%s %s", devc->name, name);

  if (devc->nr_indevs > MAX_INPUTDEVS)
    {
      cmn_err (CE_CONT, "Too many audio devices\n");
      return -1;
    }

  opts = ADEV_AUTOMODE | ADEV_NOOUTPUT;

  formats = SUPPORTED_FORMAT;

  if ((dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				   devc->osdev,
				   devc->osdev,
				   tmp,
				   &sbxfi_input_driver,
				   sizeof (audiodrv_t),
				   opts, formats, devc, -1)) < 0)
    {
      return -1;
    }

  if (devc->first_dev == -1)
    {
      devc->first_dev = dev;
    }
  adev = audio_engines[dev];

  portc = &devc->rec_portc[devc->nr_indevs];

  adev->portc = portc;
  adev->devc = devc;
  adev->mixer_dev = devc->mixer_dev;
  adev->rate_source = devc->first_dev;
  adev->min_rate = 48000;
  adev->max_rate = 96000;
  adev->min_block=4096;
  adev->dmabuf_maxaddr = MEMLIMIT_ISA;

  portc->dev = dev;
  portc->open_mode = 0;
  portc->fmt = SUPPORTED_FORMAT;
  portc->dev_flags = dev_flags;
  portc->state_bits = 0;
  portc->direction = PCM_ENABLE_INPUT;

  portc->rate = DEFAULT_REC_RATE;

  // use the following SRC channels for record
  portc->SrcChan = devc->next_src;
  devc->next_src += 2;
  devc->src_to_portc[portc->SrcChan]=portc;

  portc->dwDAChan[0] = ADC_SRC_LINEIN;

  portc->vol_left=portc->vol_right=MIXER_VOLSTEPS;

  adev->min_channels = 2;
  adev->max_channels = 2;

  portc->pgtable_index = devc->next_pg;
  devc->next_pg += 128/4;	// Up to 128k for buffer

  devc->nr_indevs++;

  return dev;
}

static int
sbxfi_set_playvol (int dev, int ctrl, unsigned int cmd, int value)
{
  sbxfi_devc_t *devc = mixer_devs[dev]->devc;
  sbxfi_portc_t *portc;
  int left, right;

  if (ctrl<0 || ctrl >= devc->nr_outdevs)
     return OSS_ENXIO;
  portc = &devc->play_portc[ctrl];

  if (cmd == SNDCTL_MIX_READ)
     {
	     return portc->vol_left | (portc->vol_right << 16);
     }

  if (cmd == SNDCTL_MIX_WRITE)
     {
	     left = value & 0xffff;
	     right = (value>>16) & 0xffff;

	     if (left > MIXER_VOLSTEPS)
		left=MIXER_VOLSTEPS;
	     if (right > MIXER_VOLSTEPS)
		right=MIXER_VOLSTEPS;

	     portc->vol_left=left;
	     portc->vol_right=right;
	     if (portc->running)
		SetupPlayMixer(devc, portc);

	     return portc->vol_left | (portc->vol_right << 16);
     }

  return OSS_EINVAL;
}

static int
sbxfi_set_recvol (int dev, int ctrl, unsigned int cmd, int value)
{
  sbxfi_devc_t *devc = mixer_devs[dev]->devc;
  sbxfi_portc_t *portc;
  int left, right;

  if (ctrl<0 || ctrl >= devc->nr_indevs)
     return OSS_ENXIO;
  portc = &devc->rec_portc[ctrl];

  if (cmd == SNDCTL_MIX_READ)
     {
	     return portc->vol_left | (portc->vol_right << 16);
     }

  if (cmd == SNDCTL_MIX_WRITE)
     {
	     left = value & 0xffff;
	     right = (value>>16) & 0xffff;

	     if (left > MIXER_VOLSTEPS)
		left=MIXER_VOLSTEPS;
	     if (right > MIXER_VOLSTEPS)
		right=MIXER_VOLSTEPS;

	     portc->vol_left=left;
	     portc->vol_right=right;
	     if (portc->running)
		SetupRecordMixer(devc, portc);

	     return portc->vol_left | (portc->vol_right << 16);
     }

  return OSS_EINVAL;
}

static int
sbxfi_set_recsrc (int dev, int ctrl, unsigned int cmd, int value)
{
  sbxfi_devc_t *devc = mixer_devs[dev]->devc;
  sbxfi_portc_t *portc;

  if (ctrl<0 || ctrl >= devc->nr_indevs)
     return OSS_ENXIO;
  portc = &devc->rec_portc[ctrl];

  if (cmd == SNDCTL_MIX_READ)
     {
	     return portc->dwDAChan[0];
     }

  if (cmd == SNDCTL_MIX_WRITE)
     {
	     if (value<0 || value>ADC_SRC_NONE)
	        return portc->dwDAChan[0];

	     return portc->dwDAChan[0]=value;
     }

  return OSS_EINVAL;
}

static int
sbxfi_mix_init (int dev)
{
      int root=0, ctl;

      if ((ctl = mixer_ext_create_control (dev, root,
					   0, sbxfi_set_playvol,
					   MIXT_STEREOSLIDER16,
					   "play", MIXER_VOLSTEPS,
					   MIXF_PCMVOL | MIXF_READABLE |
					   MIXF_WRITEABLE | MIXF_CENTIBEL)) <
	  0)
	return ctl;

      if ((ctl = mixer_ext_create_control (dev, root,
					   0, sbxfi_set_recvol,
					   MIXT_STEREOSLIDER16,
					   "rec", MIXER_VOLSTEPS,
					   MIXF_RECVOL | MIXF_READABLE |
					   MIXF_WRITEABLE | MIXF_CENTIBEL)) <
	  0)
	return ctl;

      if ((ctl = mixer_ext_create_control (dev, root,
					   0, sbxfi_set_recsrc,
					   MIXT_ENUM,
					   "recsrc", 5,
					   MIXF_READABLE |
					   MIXF_WRITEABLE | MIXF_CENTIBEL)) <
	  0)
	return ctl;

      mixer_ext_set_strings (dev, ctl, "mic line video aux none", 0);

      return 0;
}

int
oss_sbxfi_attach (oss_device_t * osdev)
{
  unsigned short pci_command, vendor, device, revision;
  unsigned short subvendor, subdevice;
  int pdev, rdev;
  extern int sbxfi_type;

  sbxfi_devc_t *devc;
  sbxfi_portc_t *portc;

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  memset (devc, 0, sizeof (*devc));

  portc = &devc->play_portc[0];

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->name = "Sound Blaster X-Fi";

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  DDB (cmn_err
       (CE_CONT, "oss_sbxfi_attach(Vendor %x, device %x)\n", vendor, device));

  if (vendor != PCI_VENDOR_CREATIVE ||
		  (device != CREATIVE_SBXFI_K1 && device != CREATIVE_SBXFI_K2 &&
		   device != CREATIVE_SBXFI_E))
    {
      cmn_err (CE_WARN, "Hardware not recognized (vendor=%x, dev=%x)\n",
	       vendor, device);
      return 0;
    }
  MUTEX_INIT (osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (osdev, devc->low_mutex, MH_DRV + 1);

  pci_read_config_word (osdev, PCI_SUBSYSTEM_VENDOR_ID, &subvendor);
  pci_read_config_word (osdev, PCI_SUBSYSTEM_ID, &subdevice);
  pci_read_config_word (osdev, PCI_REVISION_ID, &revision);

  devc->wVendorID = vendor;
  devc->wDeviceID = device;
  devc->wSubsystemVendorID = subvendor;
  devc->wSubsystemID = subdevice;
  devc->wChipRevision = revision;

  switch (sbxfi_type)
    {
    case 1:
      devc->name = "Sound Blaster X-Fi (SB046x/067x/076x)";
      devc->hw_family = HW_ORIG;
      break;

    case 2:
      devc->name = "Sound Blaster X-Fi (SB073x)";
      devc->hw_family = HW_073x;
      break;

    case 3:
      devc->name = "Sound Blaster X-Fi (SB055x)";
      devc->hw_family = HW_055x;
      break;

    case 4:
      devc->name = "Sound Blaster X-Fi (UAA)";
      devc->hw_family = HW_UAA;
      break;

    case 5:
      devc->name = "Sound Blaster X-Fi (SB076x)";
      devc->hw_family = HW_0760;
      break;

    case 6:
      devc->name = "Sound Blaster X-Fi (SB0880-1)";
      devc->hw_family = HW_08801;
      break;

    case 7:
      devc->name = "Sound Blaster X-Fi (SB0880-2)";
      devc->hw_family = HW_08802;
      break;

    case 8:
      devc->name = "Sound Blaster X-Fi (SB0880-3)";
      devc->hw_family = HW_08803;
      break;

    case 0:
    default:
      devc->hw_family = 0;
      break;
    }

  if (!devc->hw_family && device == CREATIVE_SBXFI_K1) // EMU20K1 models
  switch (subdevice)
    {
    case 0x0021:		/* SB0460 */
    case 0x0023:
    case 0x0024:
    case 0x0025:
    case 0x0026:
    case 0x0027:
    case 0x0028:
    case 0x002a:
    case 0x002b:
    case 0x002c:
    case 0x002d:
    case 0x002e:
    case 0x0032:
    case 0x0033:
    case 0x0034: /* This is actually Auzentech Prelude (subvendor 415a) */
      /*
       * Original X-Fi hardware revision (SB046x/067x/076x)
       */
      devc->name = "Sound Blaster X-Fi (SB046x/067x/076x)";
      devc->hw_family = HW_ORIG;
      break;

    case 0x0029:
    case 0x0031:
      devc->name = "Sound Blaster X-Fi (SB073x)";
      devc->hw_family = HW_073x;
      break;

    case 0x0022:
    case 0x002f:
      devc->name = "Sound Blaster X-Fi (SB055x)";
      devc->hw_family = HW_055x;
      break;

    default:
      if (subdevice >= 0x6000 && subdevice <= 0x6fff)	/* "Vista compatible" HW */
	{
	  devc->name = "Sound Blaster X-Fi (UAA)";
	  devc->hw_family = HW_UAA;
	}
    }

  if (!devc->hw_family && device == CREATIVE_SBXFI_K2) // EMU 20K2 models
  switch (subdevice)
    {
    case PCI_SUBDEVICE_ID_CREATIVE_SB0760:
      devc->name = "Sound Blaster X-Fi (SB076x)";
      devc->hw_family = HW_0760;
      break;

    case PCI_SUBDEVICE_ID_CREATIVE_SB08801:
      devc->name = "Sound Blaster X-Fi (SB0880-1)";
      devc->hw_family = HW_08801;
      break;

    case PCI_SUBDEVICE_ID_CREATIVE_SB08802:
      devc->name = "Sound Blaster X-Fi (SB0880-2)";
      devc->hw_family = HW_08802;
      break;

    case PCI_SUBDEVICE_ID_CREATIVE_SB08803:
      devc->name = "Sound Blaster X-Fi (SB0880-3)";
      devc->hw_family = HW_08803;
      break;

    default:
      devc->name = "Sound Blaster X-Fi (20K2)";
      devc->hw_family = HW_UAA;	// Just a wild guess
  }

  if (!devc->hw_family && device == CREATIVE_SBXFI_E) // PCI-e models
     {
	devc->name = "Sound Blaster X-Fi (PCI-e)";
        devc->hw_family = HW_UAA;	// Just a wild guess
     }
     

#if 1
// Temporary hacking until proper 20K2 support is in place
  if (devc->hw_family > HW_UAA) devc->hw_family = HW_UAA;
#endif

  oss_register_device (osdev, devc->name);

  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  if ((osdev->hw_info = PMALLOC (osdev, 200)) != NULL)
    {
      sprintf (osdev->hw_info, "PCI device %04x:%04x, subdevice %04x:%04x\n",
	       vendor, device, subvendor, subdevice);
    }

  devc->interrupt_count=0;
  if (oss_register_interrupts (devc->osdev, 0, sbxfi_intr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Unable to install interrupt handler\n");
      return 0;
    }

  // Detect and Configure X-Fi PCI config space.
  // Obtain the resource configuration from PCI config space.
  if (!DetectAndConfigureHardware (devc))
    {
      cmn_err (CE_WARN, "Cannot configure X-Fi hardware...\n");
      return 0;
    }

  if (IsVistaCompatibleHardware (devc))
    {
      // Switch to audio core to X-Fi core.
      SwitchToXFiCore (devc);
    }

  // Initialize hardware.  This include setup the PLL etc.
  if (InitHardware (devc) != CTSTATUS_SUCCESS)
    {
      cmn_err (CE_WARN, "Init Hardware failed...\n");
      return 0;
    }

  devc->dwPageTableSize = 1024; /* For up to 4M of memory */
  devc->pdwPageTable = CONTIG_MALLOC (devc->osdev,
				      devc->dwPageTableSize,
				      MEMLIMIT_ISA, &devc->dwPTBPhysAddx, devc->pgtable_dma_handle);

  HwWrite20K1 (devc, PTPALX, devc->dwPTBPhysAddx);
  HwWrite20K1 (devc, PTPAHX, 0);

  HwWrite20K1 (devc, TRNCTL, 0x13);
  HwWrite20K1 (devc, TRNIS, 0x200c01);

  HwWrite20K1 (devc, GIE, FI_INT);	/* Enable "forced" interrupts */
  HwWrite20K1 (devc, GIP, FI_INT);	/* Trigger forced interrupt */

  oss_udelay(1000);
  if (devc->interrupt_count==0)
     cmn_err(CE_WARN, "Interrupts don't seem to be working.\n");

  set_interval_timer(devc, TIMER_INTERVAL);

/*
 * Disable FI and enable selected global interrupts
 * (SRC, Interval Timer).
 */
  HwWrite20K1 (devc, GIE, SRC_INT | IT_INT);

  if ((devc->mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
					    devc->osdev,
					    devc->osdev,
					    devc->name,
					    &sbxfi_mixer_driver,
					    sizeof (mixer_driver_t),
					    devc)) >= 0)
    {
      mixer_devs[devc->mixer_dev]->hw_devc = devc;
      mixer_devs[devc->mixer_dev]->priority = 1;	/* Possible default mixer candidate */
      mixer_ext_set_init_fn (devc->mixer_dev, sbxfi_mix_init, 10);
    }

  devc->first_dev=-1; /* Not assigned */
  pdev = init_play_device (devc, "output", 0);
  rdev = init_rec_device (devc, "input", 0);
#ifdef CONFIG_OSS_VMIX
   if (pdev != -1)
     {
         vmix_attach_audiodev (devc->osdev, pdev, rdev, 0);
     }
#endif

#if 0
  // Initialize ADC
  InitADC (devc, ADC_SRC_LINEIN, FALSE);
#endif

  return 1;
}

int
oss_sbxfi_detach (oss_device_t * osdev)
{
  sbxfi_devc_t *devc = (sbxfi_devc_t *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  HwWrite20K1 (devc, GIE, 0);	/* Disable global interrupts */
  oss_unregister_interrupts (devc->osdev);

  HwWrite20K1 (devc, PTPALX, 0);
  if (devc->pdwPageTable != NULL)
    {
      CONTIG_FREE (devc->osdev, devc->pdwPageTable, devc->dwPageTableSize, devc->pgtable_dma_handle);
      devc->pdwPageTable = NULL;
    }
  oss_unregister_device (osdev);
  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  return 1;
}
