/*
 * This software module makes it possible to use Open Sound System for Linux
 * (the _professional_ version) as a low level driver source for ALSA.
 *
 * Copyright (C) 2004 Hannu Savolainen (hannu@voimakentta.net).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

/*
 * !!!!!!!!!!!!!!!!!!!! Important  !!!!!!!!!!!!!!!!!!
 *
 * If this file doesn't compile, you must not try to resolve the problem
 * without perfect understanding of internals of Linux kernel, ALSA and
 * Open Sound System.
 *
 * Instead you need to check that you are using the version of this file
 * that matches the versions of ALSA, OSS and Linux you are currently using.
 */

#include "cuckoo.h"

MODULE_AUTHOR ("Hannu Savolainen <hannu@opensound.com>");
MODULE_LICENSE ("GPL v2");
MODULE_DESCRIPTION ("OSS PCM low level driver interface for ALSA");

static snd_pcm_substream_t *cuckoo_playsubstream[256] = { NULL };
static snd_pcm_substream_t *cuckoo_capturesubstream[256] = { NULL };

static snd_pcm_hardware_t snd_cuckoo_playback = {
  .info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
	   SNDRV_PCM_INFO_BLOCK_TRANSFER |
	   SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_SYNC_START),
  .rates = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
  .rate_min = 4000,
  .rate_max = 48000,
  .channels_min = 1,
  .channels_max = 2,
  .buffer_bytes_max = (64 * 1024),
  .period_bytes_min = 64,
  .period_bytes_max = (32 * 1024),
  .periods_min = 2,
  .periods_max = 1024,
  .fifo_size = 0,
};

static snd_pcm_hardware_t snd_cuckoo_capture = {
  .info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
	   SNDRV_PCM_INFO_BLOCK_TRANSFER |
	   SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_SYNC_START),
  .rates = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
  .rate_min = 4000,
  .rate_max = 48000,
  .channels_min = 1,
  .channels_max = 2,
  .buffer_bytes_max = (128 * 1024),
  .period_bytes_min = 64,
  .period_bytes_max = (64 * 1024),
  .periods_min = 2,
  .periods_max = 1024,
  .fifo_size = 0,
};

static void
cuckoo_outputintr (int dev, int notify_only)
{
  snd_pcm_substream_t *substream;
  adev_t *adev;
  dmap_t *dmap;
  oss_native_word flags;

  if (dev < 0 || dev > 255)
    return;

  adev = audio_devfiles[dev];
  dmap = adev->dmap_out;

  dmap->fragment_counter = (dmap->fragment_counter + 1) % dmap->nfrags;

  substream = cuckoo_playsubstream[dev];
  if (substream == NULL)
    return;

  udi_spin_lock_irqsave (&adev->mutex, &flags);
  snd_pcm_period_elapsed (substream);
  udi_spin_unlock_irqrestore (&adev->mutex, flags);
}

static void
cuckoo_inputintr (int dev, int intr_flags)
{
  snd_pcm_substream_t *substream;
  adev_t *adev;
  dmap_t *dmap;
  oss_native_word flags;

  if (dev < 0 || dev > 255)
    return;

  adev = audio_devfiles[dev];
  dmap = adev->dmap_in;

  dmap->fragment_counter = (dmap->fragment_counter + 1) % dmap->nfrags;

  substream = cuckoo_capturesubstream[dev];
  if (substream == NULL)
    return;

  udi_spin_lock_irqsave (&adev->mutex, &flags);
  snd_pcm_period_elapsed (substream);
  udi_spin_unlock_irqrestore (&adev->mutex, flags);
}

static void
copy_hw_caps (snd_pcm_runtime_t * runtime, adev_t * adev, int dir)
{
  u64 fmts = 0;
  int i;
  unsigned int fmtmask;

  if (dir == OPEN_WRITE)
    {
      fmtmask = adev->oformat_mask;
    }
  else
    {
      fmtmask = adev->iformat_mask;
    }

  for (i = 0; i < 32; i++)
    switch (fmtmask & (1 << i))
      {
      case AFMT_MU_LAW:
	fmts |= SNDRV_PCM_FMTBIT_MU_LAW;
	break;
      case AFMT_A_LAW:
	fmts |= SNDRV_PCM_FMTBIT_A_LAW;
	break;
      case AFMT_IMA_ADPCM:
	fmts |= SNDRV_PCM_FMTBIT_IMA_ADPCM;
	break;
      case AFMT_U8:
	fmts |= SNDRV_PCM_FMTBIT_U8;
	break;
      case AFMT_S8:
	fmts |= SNDRV_PCM_FMTBIT_S8;
	break;
      case AFMT_S16_LE:
	fmts |= SNDRV_PCM_FMTBIT_S16_LE;
	break;
      case AFMT_S16_BE:
	fmts |= SNDRV_PCM_FMTBIT_S16_BE;
	break;
      case AFMT_S24_LE:
	fmts |= SNDRV_PCM_FMTBIT_S24_LE;
	break;
      case AFMT_S24_BE:
	fmts |= SNDRV_PCM_FMTBIT_S24_BE;
	break;
      case AFMT_S32_LE:
	fmts |= SNDRV_PCM_FMTBIT_S32_LE;
	break;
      case AFMT_S32_BE:
	fmts |= SNDRV_PCM_FMTBIT_S32_BE;
	break;
      case AFMT_MPEG:
	fmts |= SNDRV_PCM_FMTBIT_MPEG;
	break;
      case AFMT_FLOAT:
	fmts |= SNDRV_PCM_FMTBIT_FLOAT_LE;
	break;
      case AFMT_SPDIF_RAW:
	fmts |= SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_LE;
	break;
      }

  runtime->hw.formats = fmts;

  if (adev->min_block > 0)
    runtime->hw.period_bytes_min = adev->min_block;
  if (adev->max_block > 0)
    runtime->hw.period_bytes_max = adev->max_block;

  if (adev->flags & ADEV_NOMMAP)
    runtime->hw.info &= ~(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID);

  if (adev->max_rate > adev->min_rate)
    {
      runtime->hw.rate_min = adev->min_rate;
      runtime->hw.rate_max = adev->max_rate;
    }

  if (!(adev->caps & DSP_CAP_FREERATE))
    runtime->hw.rates &=
      ~(SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000);
}

static int
snd_cuckoo_playback_open (snd_pcm_substream_t * substream)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number;
  int err;
  adev_t *adev;
  oss_native_word flags;
  struct fileinfo tmp_finfo;

  if (snum < 0 || snum >= chip->npcm)
    {
      printk ("cuckoo: Playback open - bad substream index %d\n", snum);
      return -EIO;
    }

  adev = chip->play_adev[snum];
  printk ("cuckoo_playback_open(%d=%s)\n", adev->engine_num, adev->name);

  cuckoo_playsubstream[adev->engine_num] = substream;

  if (adev->dmap_out == NULL || adev->dmap_out->dmabuf == NULL)
    {
      printk ("cuckoo: dev %d - no buffer available\n", adev->engine_num);
      return -ENOMEM;
    }

  if (adev->open_mode != 0)
    {
      udi_spin_unlock_irqrestore (&adev->mutex, flags);
      return -EBUSY;
    }

  tmp_finfo.mode = OPEN_WRITE;
  tmp_finfo.acc_flags = 0;
  if ((err =
       oss_audio_open_engine (adev->engine_num, OSS_DEV_DSP,
			      &tmp_finfo, 1, OF_SMALLBUF,
			      NULL)) < 0)
    {
      return err;
    }

  udi_spin_lock_irqsave (&adev->mutex, &flags);
  adev->open_mode = OPEN_WRITE;
  runtime->hw = snd_cuckoo_playback;
  copy_hw_caps (runtime, adev, OPEN_WRITE);
  snd_pcm_set_sync (substream);
  adev->pid = current->pid;
  strncpy (adev->cmd, current->comm, sizeof (adev->cmd) - 1);
  adev->cmd[sizeof (adev->cmd) - 1] = 0;
  adev->outputintr = cuckoo_outputintr;
  udi_spin_unlock_irqrestore (&adev->mutex, flags);

  return 0;
}

static int
snd_cuckoo_capture_open (snd_pcm_substream_t * substream)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number;
  int err;
  adev_t *adev;
  oss_native_word flags;
  struct fileinfo tmp_finfo;

  if (snum < 0 || snum >= chip->npcm)
    {
      printk ("cuckoo: Capture open - bad substream index %d\n", snum);
      return -EIO;
    }

  adev = chip->capture_adev[snum];

  cuckoo_capturesubstream[adev->engine_num] = substream;

  if (adev->dmap_in == NULL || adev->dmap_in->dmabuf == NULL)
    {
      printk ("cuckoo: dev %d - no buffer available\n", adev->engine_num);
      return -ENOMEM;
    }

  if (adev->open_mode != 0)
    {
      udi_spin_unlock_irqrestore (&adev->mutex, flags);
      return -EBUSY;
    }

  tmp_finfo.mode = OPEN_READ;
  tmp_finfo.acc_flags = 0;
  if ((err =
       oss_audio_open_engine (adev->engine_num, OSS_DEV_DSP,
			      &tmp_finfo, 1, OF_SMALLBUF,
			      NULL)) < 0)
    {
      return err;
    }

  udi_spin_lock_irqsave (&adev->mutex, &flags);
  adev->open_mode = OPEN_READ;
  runtime->hw = snd_cuckoo_capture;
  copy_hw_caps (runtime, adev, OPEN_READ);
  snd_pcm_set_sync (substream);
  adev->pid = current->pid;
  strncpy (adev->cmd, current->comm, sizeof (adev->cmd) - 1);
  adev->cmd[sizeof (adev->cmd) - 1] = 0;
  adev->inputintr = cuckoo_inputintr;
  udi_spin_unlock_irqrestore (&adev->mutex, flags);

  return 0;
}

static int
snd_cuckoo_playback_close (snd_pcm_substream_t * substream)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  int snum = substream->number;
  adev_t *adev;
  oss_native_word flags;
  struct fileinfo tmp_finfo;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->play_adev[snum];

  udi_spin_lock_irqsave (&adev->mutex, &flags);
  cuckoo_playsubstream[adev->engine_num] = NULL;
  udi_spin_unlock_irqrestore (&adev->mutex, flags);

  tmp_finfo.mode = OPEN_WRITE;
  tmp_finfo.acc_flags = 0;
  oss_audio_release (adev->engine_num, &tmp_finfo);

  return 0;
}

static int
snd_cuckoo_capture_close (snd_pcm_substream_t * substream)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  int snum = substream->number;
  adev_t *adev;
  oss_native_word flags;
  struct fileinfo tmp_finfo;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->capture_adev[snum];

  udi_spin_lock_irqsave (&adev->mutex, &flags);
  cuckoo_capturesubstream[adev->engine_num] = NULL;
  udi_spin_unlock_irqrestore (&adev->mutex, flags);

  tmp_finfo.mode = OPEN_READ;
  tmp_finfo.acc_flags = 0;
  oss_audio_release (adev->engine_num, &tmp_finfo);

  return 0;
}

static int
snd_cuckoo_playback_hw_params (snd_pcm_substream_t * substream,
			       snd_pcm_hw_params_t * hw_params)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number;
  adev_t *adev;
  dmap_t *dmap;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->play_adev[snum];
  dmap = adev->dmap_out;

  if (dmap->dmabuf == NULL)
    return -ENOMEM;

  runtime->dma_area = dmap->dmabuf;
  runtime->dma_addr = dmap->dmabuf_phys;
  runtime->dma_bytes = dmap->buffsize;
  memset (dmap->dmabuf, 0, dmap->buffsize);

  return 0;
}

static int
snd_cuckoo_capture_hw_params (snd_pcm_substream_t * substream,
			      snd_pcm_hw_params_t * hw_params)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number;
  adev_t *adev;
  dmap_t *dmap;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->capture_adev[snum];
  dmap = adev->dmap_in;

  if (dmap->dmabuf == NULL)
    return -ENOMEM;

  runtime->dma_area = dmap->dmabuf;
  runtime->dma_addr = dmap->dmabuf_phys;
  runtime->dma_bytes = dmap->buffsize;
  memset (dmap->dmabuf, 0, dmap->buffsize);

  return 0;
}

static int
snd_cuckoo_hw_free (snd_pcm_substream_t * substream)
{
  snd_pcm_runtime_t *runtime = substream->runtime;

  runtime->dma_area = NULL;
  runtime->dma_addr = 0;
  runtime->dma_bytes = 0;

  return 0;
}

static int
snd_cuckoo_playback_prepare (snd_pcm_substream_t * substream)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number, err;
  adev_t *adev;
  oss_native_word flags;
  dmap_t *dmap;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->play_adev[snum];
  dmap = adev->dmap_out;

  udi_spin_lock_irqsave (&adev->mutex, &flags);

  adev->d->adrv_set_format (adev->engine_num,
		       snd_pcm_format_width (runtime->format));
  runtime->channels =
    adev->d->adrv_set_channels (adev->engine_num, runtime->channels);
  runtime->rate = adev->d->adrv_set_rate (adev->engine_num, runtime->rate);
  adev->user_parms.rate = adev->user_parms.rate = runtime->rate;

  dmap->bytes_in_use = snd_pcm_lib_buffer_bytes (substream);
  dmap->fragment_size = snd_pcm_lib_period_bytes (substream);

#if 1
  {
    int f, s;;

    f = dmap->fragment_size / 4;
    if (f < 128)
      f = dmap->fragment_size / 2;
    if (f < 128)
      f = dmap->fragment_size;

    s = dmap->bytes_in_use;
    while (s > f)
      s /= 2;

    dmap->fragment_size = s;
  }
#endif

  if (adev->max_block > 0 && dmap->fragment_size > adev->max_block)
    dmap->fragment_size = adev->max_block;
  if (adev->min_block > 0 && dmap->fragment_size < adev->min_block)
    dmap->fragment_size = adev->min_block;
  if (dmap->fragment_size < 8)
    dmap->fragment_size = 8;
  dmap->nfrags = dmap->bytes_in_use / dmap->fragment_size;

  err =
    adev->d->adrv_prepare_for_output (adev->engine_num, dmap->fragment_size,
				 dmap->nfrags);
  cuckoo_playsubstream[adev->engine_num] = substream;
  udi_spin_unlock_irqrestore (&adev->mutex, flags);
  return err;
}

static int
snd_cuckoo_capture_prepare (snd_pcm_substream_t * substream)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number, err;
  adev_t *adev;
  oss_native_word flags;
  dmap_t *dmap;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->capture_adev[snum];
  dmap = adev->dmap_in;

  udi_spin_lock_irqsave (&adev->mutex, &flags);

  adev->d->adrv_set_format (adev->engine_num,
		       snd_pcm_format_width (runtime->format));
  adev->d->adrv_set_channels (adev->engine_num, runtime->channels);
  adev->d->adrv_set_rate (adev->engine_num, runtime->rate);

  dmap->bytes_in_use = snd_pcm_lib_buffer_bytes (substream);
  dmap->fragment_size = snd_pcm_lib_period_bytes (substream);

#if 1
  {
    int f, s;;

    f = dmap->fragment_size / 4;
    if (f < 128)
      f = dmap->fragment_size / 2;
    if (f < 128)
      f = dmap->fragment_size;

    s = dmap->bytes_in_use;
    while (s > f)
      s /= 2;

    dmap->fragment_size = s;
  }
#endif

  if (adev->max_block > 0 && dmap->fragment_size > adev->max_block)
    dmap->fragment_size = adev->max_block;
  if (adev->min_block > 0 && dmap->fragment_size < adev->min_block)
    dmap->fragment_size = adev->min_block;
  if (dmap->fragment_size < 8)
    dmap->fragment_size = 8;
  dmap->nfrags = dmap->bytes_in_use / dmap->fragment_size;

  err =
    adev->d->adrv_prepare_for_input (adev->engine_num, dmap->fragment_size,
				dmap->nfrags);
  cuckoo_capturesubstream[adev->engine_num] = substream;
  udi_spin_unlock_irqrestore (&adev->mutex, flags);
  return err;
}

static int
snd_cuckoo_playback_trigger (snd_pcm_substream_t * substream, int cmd)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  //snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number;
  adev_t *adev;
  oss_native_word flags;
  dmap_t *dmap;
  int err = 0;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->play_adev[snum];
  dmap = adev->dmap_out;

  udi_spin_lock_irqsave (&adev->mutex, &flags);

  switch (cmd)
    {
    case SNDRV_PCM_TRIGGER_START:
      adev->d->adrv_output_block (adev->engine_num, dmap->dmabuf_phys,
			     dmap->bytes_in_use, dmap->fragment_size, 0);
      adev->d->adrv_trigger (adev->engine_num, PCM_ENABLE_OUTPUT);
      break;

    case SNDRV_PCM_TRIGGER_STOP:
      adev->d->adrv_trigger (adev->engine_num, 0);
      break;

    default:
      printk ("cuckoo: Bad trigger cmd %x\n", cmd);
      err = -EIO;
      goto fail;
    }

fail:
  udi_spin_unlock_irqrestore (&adev->mutex, flags);
  return err;
}

static int
snd_cuckoo_capture_trigger (snd_pcm_substream_t * substream, int cmd)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  //snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number;
  adev_t *adev;
  oss_native_word flags;
  dmap_t *dmap;
  int err = 0;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->capture_adev[snum];
  dmap = adev->dmap_in;

  udi_spin_lock_irqsave (&adev->mutex, &flags);

  switch (cmd)
    {
    case SNDRV_PCM_TRIGGER_START:
      adev->d->adrv_start_input (adev->engine_num, dmap->dmabuf_phys,
			    dmap->bytes_in_use, dmap->fragment_size, 0);
      adev->d->adrv_trigger (adev->engine_num, PCM_ENABLE_INPUT);
      break;

    case SNDRV_PCM_TRIGGER_STOP:
      adev->d->adrv_trigger (adev->engine_num, 0);
      break;

    default:
      printk ("cuckoo: Bad trigger cmd %x\n", cmd);
      err = -EIO;
      goto fail;
    }

fail:
  udi_spin_unlock_irqrestore (&adev->mutex, flags);
  return err;
}

static snd_pcm_uframes_t
snd_cuckoo_playback_pointer (snd_pcm_substream_t * substream)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  //snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number;
  adev_t *adev;
  dmap_t *dmap;
  int pos;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->play_adev[snum];
  dmap = adev->dmap_out;

  if (adev->d->adrv_get_output_pointer != NULL)
    pos =
      adev->d->adrv_get_output_pointer (adev->engine_num, dmap, PCM_ENABLE_OUTPUT);
  else
    {
      pos = dmap->fragment_counter * dmap->fragment_size;
    }
  pos = bytes_to_frames (substream->runtime, pos);

  return pos;
}

static snd_pcm_uframes_t
snd_cuckoo_capture_pointer (snd_pcm_substream_t * substream)
{
  cuckoo_t *chip = snd_pcm_substream_chip (substream);
  //snd_pcm_runtime_t *runtime = substream->runtime;
  int snum = substream->number;
  adev_t *adev;
  dmap_t *dmap;
  int pos;

  if (snum < 0 || snum >= chip->npcm)
    return -ENXIO;

  adev = chip->capture_adev[snum];
  dmap = adev->dmap_in;

  if (adev->d->adrv_get_input_pointer != NULL)
    pos =
      adev->d->adrv_get_input_pointer (adev->engine_num, dmap, PCM_ENABLE_INPUT);
  else
    {
      pos = dmap->fragment_counter * dmap->fragment_size;
    }
  pos = bytes_to_frames (substream->runtime, pos);

  return pos;
}

static snd_pcm_ops_t snd_cuckoo_playback_ops = {
  .open = snd_cuckoo_playback_open,
  .close = snd_cuckoo_playback_close,
  .ioctl = snd_pcm_lib_ioctl,
  .hw_params = snd_cuckoo_playback_hw_params,
  .hw_free = snd_cuckoo_hw_free,
  .prepare = snd_cuckoo_playback_prepare,
  .trigger = snd_cuckoo_playback_trigger,
  .pointer = snd_cuckoo_playback_pointer,
};

static snd_pcm_ops_t snd_cuckoo_capture_ops = {
  .open = snd_cuckoo_capture_open,
  .close = snd_cuckoo_capture_close,
  .ioctl = snd_pcm_lib_ioctl,
  .hw_params = snd_cuckoo_capture_hw_params,
  .hw_free = snd_cuckoo_hw_free,
  .prepare = snd_cuckoo_capture_prepare,
  .trigger = snd_cuckoo_capture_trigger,
  .pointer = snd_cuckoo_capture_pointer,
};

int
install_pcm_instances (cuckoo_t * chip, int cardno)
{
  int dev, err, ok = 0;
  int ninputs = 0, noutputs = 0;

  for (dev = 0; dev < num_audio_devfiles; dev++)
    if (audio_devfiles[dev]->card_number == cardno)
      {
	adev_t *adev = audio_devfiles[dev];
	adev_t *nextdev = audio_devfiles[dev + 1];
	snd_pcm_t *pcm;

	ninputs = noutputs = 0;

	ok = 0;
/* Special handling for shadow devices */
	if (dev < num_audio_devfiles - 1 && (adev->flags & ADEV_DUPLEX))
	  if ((nextdev->flags & ADEV_DUPLEX)
	      && (nextdev->flags & ADEV_SHADOW))
	    ok = 1;

// Devices with one recording engine and multiple playback ones
	if (dev < num_audio_devfiles - 1 && (adev->flags & ADEV_DUPLEX))
	  if (adev->card_number == nextdev->card_number)
	    if ((nextdev->flags & ADEV_NOINPUT))
	      ok = 1;

	if (ok)			// Device needs special handling
	  {
	    if ((err =
		 snd_pcm_new (chip->card, "OSS/Linux", chip->npcm, 1, 1,
			      &pcm)) < 0)
	      {
		printk ("cuckoo: snd_pcm_new failed - error %d\n", err);
		return err;
	      }

	    pcm->private_data = chip;
	    chip->pcm[chip->npcm++] = pcm;
	    strlcpy (pcm->name, adev->name);

	    chip->capture_adev[chip->ncapture++] = nextdev;
	    snd_pcm_set_ops (pcm, SNDRV_PCM_STREAM_CAPTURE,
			     &snd_cuckoo_capture_ops);

	    chip->play_adev[chip->nplay++] = adev;
	    snd_pcm_set_ops (pcm, SNDRV_PCM_STREAM_PLAYBACK,
			     &snd_cuckoo_playback_ops);

	    dev++;
	    continue;
	  }

	if (!(adev->flags & ADEV_NOINPUT))
	  ninputs = 1;
	if (!(adev->flags & ADEV_NOOUTPUT))
	  noutputs = 1;

	if ((err =
	     snd_pcm_new (chip->card, "OSS/Linux", chip->npcm, noutputs,
			  ninputs, &pcm)) < 0)
	  {
	    printk ("cuckoo: snd_pcm_new failed - error %d\n", err);
	    return err;
	  }

	pcm->private_data = chip;
	chip->pcm[chip->npcm++] = pcm;
	strlcpy (pcm->name, adev->name);

	if (noutputs > 0)
	  {
	    chip->play_adev[chip->nplay++] = adev;
	    snd_pcm_set_ops (pcm, SNDRV_PCM_STREAM_PLAYBACK,
			     &snd_cuckoo_playback_ops);
	  }

	if (ninputs > 0)
	  {
	    chip->capture_adev[chip->ncapture++] = adev;
	    snd_pcm_set_ops (pcm, SNDRV_PCM_STREAM_CAPTURE,
			     &snd_cuckoo_capture_ops);
	  }
      }

  return 0;
}

EXPORT_SYMBOL (install_pcm_instances);
