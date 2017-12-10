/*
 * Purpose: Audio driver for the NeoMagic NM2200 PCI audio controller (AV and ZX).
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

#include "oss_neomagic_cfg.h"
#include "oss_pci.h"
#include "ac97.h"
#include "neomagic.h"
#include "neomagic_coeff.h"

/* 
 * The size of the playback reserve.  When the playback buffer has less
 * than NEOMAGIC_PLAY_WMARK_SIZE bytes to output, we request a new
 * buffer.
 */
#define NEOMAGIC_PLAY_WMARK_SIZE 0

static audiodrv_t neomagic_audio_driver;

static int neomagic_interrupt_av (struct neomagic_devc *devc);
static int neomagic_interrupt_zx (struct neomagic_devc *devc);

#define NEOMAGIC_VENDOR_ID	0x10c8
#define PCI_DEVICE_ID_NEOMAGIC_NEOMAGICAV_AUDIO 0x8005
#define PCI_DEVICE_ID_NEOMAGIC_NEOMAGICZX_AUDIO 0x8006
#define PCI_DEVICE_ID_NEOMAGIC_NM2360_AUDIO  0x8016


static int buffertop = 0x18000;

/* The actual rates supported by the devc. */
static int samplerates[9] = {
  8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 99999999
};

/*
 * Set the devc samplerate, word size and stereo mode to correspond to
 * the settings in the CARD struct for the specified device in DEV.
 * We keep two separate sets of information, one for each device; the
 * hardware is not actually configured until a read or write is
 * attempted.
 */

int
neomagic_setInfo (int dev, struct neomagic_devc *devc)
{
  int x;
  int w;
  int targetrate;

  if (devc->dev[0] == dev)
    w = 0;
  else if (devc->dev[1] == dev)
    w = 1;
  else
    return OSS_ENODEV;

  targetrate = devc->portc[w].samplerate;

  if ((devc->portc[w].bits != 8 && devc->portc[w].bits != 16)
      || targetrate < samplerates[0] || targetrate > samplerates[7])
    return OSS_EINVAL;

  for (x = 0; x < 8; x++)
    if (targetrate < ((samplerates[x] + samplerates[x + 1]) / 2))
      break;

  if (x < 8)
    {
      unsigned char ratebits = ((x << 4) & NM_RATE_MASK);
      if (devc->portc[w].bits == 16)
	ratebits |= NM_RATE_BITS_16;
      if (devc->portc[w].stereo == 2)
	ratebits |= NM_RATE_STEREO;

      devc->portc[w].samplerate = samplerates[x];


      if (devc->dev_for_play == dev && devc->playing)
	{
	  DDB (cmn_err (CE_WARN, "Setting play ratebits to 0x%x\n",
			ratebits));
	  neomagic_loadCoefficient (devc, 0, x);
	  neomagic_writePort8 (devc, 2,
			       NM_PLAYBACK_REG_OFFSET + NM_RATE_REG_OFFSET,
			       ratebits);
	}

      if (devc->dev_for_record == dev && devc->recording)
	{
	  DDB (cmn_err (CE_WARN, "Setting record ratebits to 0x%x\n",
			ratebits));
	  neomagic_loadCoefficient (devc, 1, x);
	  neomagic_writePort8 (devc, 2,
			       NM_RECORD_REG_OFFSET + NM_RATE_REG_OFFSET,
			       ratebits);
	}
      return 0;
    }
  else
    return OSS_EINVAL;
}

/* Start the play process going. */
static void
startPlay (struct neomagic_devc *devc)
{
  if (!devc->playing)
    {
      devc->playing = 1;
      neomagic_setInfo (devc->dev_for_play, devc);

      /* Enable playback engine and interrupts. */
      neomagic_writePort8 (devc, 2, NM_PLAYBACK_ENABLE_REG,
			   NM_PLAYBACK_ENABLE_FLAG | NM_PLAYBACK_FREERUN);

      /* Enable both channels. */
      neomagic_writePort16 (devc, 2, NM_AUDIO_MUTE_REG, 0x0);
    }
}

/* 
 * Request one chunk of AMT bytes from the recording device.  When the
 * operation is complete, the data will be copied into BUFFER and the
 * function oss_audio_inputintr will be invoked.
 */

static void
neomagic_read_block (struct neomagic_devc *devc, oss_native_word physbuf,
		     unsigned int amt)
{
  unsigned int endpos;
  unsigned int ringsize = devc->recordBufferSize;
  unsigned int startOffset =
    physbuf - (devc->portc[0].physaddr + devc->abuf2);
  unsigned char samplesize = 1;
  int dev = devc->dev_for_record;
  int which;

  if (devc->dev[0] == dev)
    which = 0;
  else
    which = 1;

  if (devc->portc[which].bits == 16)
    samplesize *= 2;
  if (devc->portc[which].stereo == 2)
    samplesize *= 2;

  /* 
   * If we happen to go past the end of the buffer a bit (due to a
   * delayed interrupt) it's OK.  So might as well set the watermark
   * right at the end of the reqested data.
   */
  endpos = (startOffset + amt - samplesize) % ringsize;
  neomagic_writePort32 (devc, 2, NM_RBUFFER_WMARK, devc->abuf2 + endpos);
}

static void
startRecord (struct neomagic_devc *devc)
{
  if (!devc->recording)
    {
      devc->recording = 1;
      neomagic_setInfo (devc->dev_for_record, devc);
      /* Enable recording engine and interrupts. */
      neomagic_writePort8 (devc, 2, NM_RECORD_ENABLE_REG,
			   NM_RECORD_ENABLE_FLAG | NM_RECORD_FREERUN);
    }
}


/* Stop the play engine. */
static void
stopPlay (struct neomagic_devc *devc)
{
  /* Shut off sound from both channels. */
  neomagic_writePort16 (devc, 2, NM_AUDIO_MUTE_REG,
			NM_AUDIO_MUTE_LEFT | NM_AUDIO_MUTE_RIGHT);
  /* Disable play engine. */
  neomagic_writePort8 (devc, 2, NM_PLAYBACK_ENABLE_REG, 0);
  if (devc->playing)
    {
      /* Reset the relevant state bits. */
      devc->playing = 0;
    }
}

/* Stop recording. */
static void
stopRecord (struct neomagic_devc *devc)
{
  /* Disable recording engine. */
  neomagic_writePort8 (devc, 2, NM_RECORD_ENABLE_REG, 0);

  if (devc->recording)
    {
      devc->recording = 0;
    }
}

/*
 * Ring buffers, man.  That's where the hip-hop, wild-n-wooly action's at.
 * 1972?  (Well, I suppose it was cheep-n-easy to implement.)
 *
 * Write AMT bytes of BUFFER to the playback ring buffer. 

 * Actually, the write has been done for us, so all we have to do is 
 * note that the stop point has changed.
 */

static void
neomagic_write_block (struct neomagic_devc *devc, oss_native_word physbuf,
		      unsigned int amt)
{
  unsigned int ringsize = devc->playbackBufferSize;
  unsigned int physBufAddr = (devc->portc[0].physaddr + devc->abuf1);
  int sampsize = 1;
  unsigned int playstop;
  int dev = devc->dev_for_play;
  int which;

  if (devc->dev[0] == dev)
    which = 0;
  else
    which = 1;

  if (devc->portc[which].bits == 16)
    sampsize *= 2;
  if (devc->portc[which].stereo == 2)
    sampsize *= 2;

  playstop = (physbuf + amt - physBufAddr - sampsize) % ringsize;
  neomagic_writePort32 (devc, 2, NM_PBUFFER_WMARK, devc->abuf1 + playstop);
}



/*
 * Initialize the hardware. 
 */
static void
neomagic_initHw (struct neomagic_devc *devc)
{
  /* Reset everything. */
  neomagic_writePort8 (devc, 2, 0x0, 0x11);
  neomagic_writePort16 (devc, 2, 0x214, 0);

  stopRecord (devc);
  stopPlay (devc);
}


static int
neomagic_interrupt (oss_device_t * osdev)
{
  struct neomagic_devc *devc = (neomagic_devc *) osdev->devc;

  return devc->introutine (devc);
}

static int
neomagic_interrupt_av (struct neomagic_devc *devc)
{
  unsigned short status;
  int serviced = 0;

  status = neomagic_readPort16 (devc, 2, NM_INT_REG);

  if (status & NM_PLAYBACK_INT)
    {
      serviced = 1;
      status &= ~NM_PLAYBACK_INT;
      NM_ACK_INT (devc, NM_PLAYBACK_INT);

      if (devc->playing)
	oss_audio_outputintr (devc->dev_for_play, 1);
    }

  if (status & NM_RECORD_INT)
    {
      serviced = 1;
      status &= ~NM_RECORD_INT;
      NM_ACK_INT (devc, NM_RECORD_INT);

      if (devc->recording)
	oss_audio_inputintr (devc->dev_for_record, 0);
    }

  if (status & NM_MISC_INT_1)
    {
      unsigned char cbyte;

      serviced = 1;
      status &= ~NM_MISC_INT_1;
      cmn_err (CE_WARN, "Got misc interrupt #1\n");
      NM_ACK_INT (devc, NM_MISC_INT_1);
      neomagic_writePort16 (devc, 2, NM_INT_REG, 0x8000);
      cbyte = neomagic_readPort8 (devc, 2, 0x400);
      neomagic_writePort8 (devc, 2, 0x400, cbyte | 2);
    }

  if (status & NM_MISC_INT_2)
    {
      unsigned char cbyte;

      serviced = 1;
      status &= ~NM_MISC_INT_2;
      cmn_err (CE_WARN, "Got misc interrupt #2\n");
      NM_ACK_INT (devc, NM_MISC_INT_2);
      cbyte = neomagic_readPort8 (devc, 2, 0x400);
      neomagic_writePort8 (devc, 2, 0x400, cbyte & ~2);
    }

  /* Unknown interrupt. */
  if (status)
    {
      NM_ACK_INT (devc, status);
    }
  return serviced;
}

/*
 * Handle a potential interrupt for the device referred to by DEV_ID.
 * This handler is for the 256ZX, and is very similar to the non-ZX
 * routine.
 */

static int
neomagic_interrupt_zx (struct neomagic_devc *devc)
{
  unsigned int status;
  int serviced = 0;

  status = neomagic_readPort32 (devc, 2, NM_INT_REG);

  if (status & NMZX_PLAYBACK_INT)
    {
      serviced = 1;
      status &= ~NMZX_PLAYBACK_INT;
      NMZX_ACK_INT (devc, NMZX_PLAYBACK_INT);

      if (devc->playing)
	oss_audio_outputintr (devc->dev_for_play, 1);
    }

  if (status & NMZX_RECORD_INT)
    {
      serviced = 1;
      status &= ~NMZX_RECORD_INT;
      NMZX_ACK_INT (devc, NMZX_RECORD_INT);

      if (devc->recording)
	oss_audio_inputintr (devc->dev_for_record, 0);
    }

  if (status & NMZX_MISC_INT_1)
    {
      unsigned char cbyte;

      serviced = 1;
      status &= ~NMZX_MISC_INT_1;
      cmn_err (CE_WARN, "Got misc interrupt #1\n");
      NMZX_ACK_INT (devc, NMZX_MISC_INT_1);
      cbyte = neomagic_readPort8 (devc, 2, 0x400);
      neomagic_writePort8 (devc, 2, 0x400, cbyte | 2);
    }

  if (status & NMZX_MISC_INT_2)
    {
      unsigned char cbyte;

      serviced = 1;
      status &= ~NMZX_MISC_INT_2;
      cmn_err (CE_WARN, "Got misc interrupt #2\n");
      NMZX_ACK_INT (devc, NMZX_MISC_INT_2);
      cbyte = neomagic_readPort8 (devc, 2, 0x400);
      neomagic_writePort8 (devc, 2, 0x400, cbyte & ~2);
    }

  /* Unknown interrupt. */
  if (status)
    {
      NMZX_ACK_INT (devc, status);
    }
  return serviced;
}


/*
 * Waits for the mixer to become ready to be written; returns a zero value
 * if it timed out.
 */

static int
neomagic_isReady (struct neomagic_devc *devc)
{
  int t2 = 10;
  unsigned int testaddr;
  unsigned short testb;
  int done = 0;

  testaddr = devc->mixer_status_offset;
  testb = devc->mixer_status_mask;

  /* 
   * Loop around waiting for the mixer to become ready. 
   */
  while (!done && t2-- > 0)
    {
      if ((neomagic_readPort16 (devc, 2, testaddr) & testb) == 0)
	done = 1;
      else
	oss_udelay (10);
    }
  return done;
}

/*
 * Return the contents of the AC97 mixer register REG.  Returns a positive
 * value if successful, or a negative error code.
 */
static int
neomagic_readAC97Reg (void *devc_, int reg)
{
  struct neomagic_devc *devc = (struct neomagic_devc *) devc_;
  oss_native_word flags;
  int res;

  /* for some reason any access to register 28 hangs the devc */
  if (reg == 0x28)
    return 0;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (reg < 128)
    {
      if (reg < 64 && devc->mixer_cache[reg] >= 0)
	{
	  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	  return devc->mixer_cache[reg / 2];
	}
      else
	{
	  neomagic_isReady (devc);
	  res = neomagic_readPort16 (devc, 2, NM_MIXER_OFFSET + reg);
	  /* Magic delay.  Bleah yucky.  */
	  oss_udelay (1000);
	  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	  return res;
	}
    }
  else
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EINVAL;
    }
}

/* 
 * Writes VALUE to AC97 mixer register REG.  Returns 0 if successful, or
 * a negative error code. 
 */
static int
neomagic_writeAC97Reg (void *devc_, int reg, int value)
{
  oss_native_word flags;
  int tries = 2;
  int done = 0;

  struct neomagic_devc *devc = (struct neomagic_devc *) devc_;

  /* for some reason any access to register 0x28 hangs the device */
  if (reg == 0x28)
    return 0;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  neomagic_isReady (devc);

  /* Wait for the write to take, too. */
  while ((tries-- > 0) && !done)
    {
      neomagic_writePort16 (devc, 2, NM_MIXER_OFFSET + reg, value);
      if (neomagic_isReady (devc))
	{
	  done = 1;
	  break;
	}
    }

  oss_udelay (1000);
  devc->mixer_cache[reg / 2] = value;
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return !done;
}

/* 
 * Initial register values to be written to the AC97 mixer.
 * While most of these are identical to the reset values, we do this
 * so that we have most of the register contents cached--this avoids
 * reading from the mixer directly (which seems to be problematic,
 * probably due to ignorance).
 */
struct initialValues
{
  unsigned short port;
  unsigned short value;
};

/* Initialize the AC97 into a known state.  */
static int
neomagic_resetAC97 (struct neomagic_devc *devc)
{
  int x;

  for (x = 0; x < 64; x++)
    {
      devc->mixer_cache[x] = -1;
    }

  /* Enable the internal AC97 controller. */
  neomagic_writePort8 (devc, 2, 0x6c0, 1);
  /* Now cycle it through test modes (presumably to reset it).  */
#if 0
  neomagic_writePort8 (devc, 2, 0x6cc, 0x87);
  neomagic_writePort8 (devc, 2, 0x6cc, 0x80);
#endif
  neomagic_writePort8 (devc, 2, 0x6cc, 0x0);
  return 0;
}


/* 
 * See if the signature left by the NEOMAGIC BIOS is intact; if so, we use
 * the associated address as the end of our audio buffer in the video
 * RAM.
 */

static void
neomagic_peek_for_sig (struct neomagic_devc *devc)
{
  unsigned int port1offset
    = devc->portc[0].physaddr + devc->portc[0].end_offset - 0x0400;

  /* The signature is located 1K below the end of video RAM.  */
  char *temp = (char *) MAP_PCI_MEM (devc->osdev, 0, port1offset, 1024);
  unsigned int *ptemp = (unsigned int *) temp;

  /* Default buffer end is 5120 bytes below the top of RAM.  */
  unsigned int default_value = devc->portc[0].end_offset - 0x1400;

  unsigned int sig;

  /* Install the default value first, so we don't have to repeatedly
     do it if there is a problem.  */
  devc->portc[0].end_offset = default_value;

  if (temp == NULL)
    {
      cmn_err (CE_WARN, "Unable to scan for devc signature in video RAM\n");
      return;
    }

  sig = PCI_READL (devc->osdev, ptemp);

  if ((sig & NM_SIG_MASK) == NM_SIGNATURE)
    {
      unsigned int pointer = PCI_READL (devc->osdev, ptemp + 4);

      /*
       * If it's obviously invalid, don't use it (the port already has a
       * suitable default value set).
       */
      if (pointer != 0xffffffff)
	devc->portc[0].end_offset = pointer;

      cmn_err (CE_WARN, "Found devc signature in video RAM: 0x%x\n", pointer);
    }

  UNMAP_PCI_MEM (devc->osdev, 0, port1offset, temp, 1024);
}

/* 
 * Install a driver for the Neomagic 2200 device referenced by PORT1ADDR,
 */

static int
neomagic_install (neomagic_devc * devc, unsigned int port1addr,
		  unsigned int port2addr)
{
  int x, adev, err;
  oss_native_word physaddr;
  int size;

  devc->playing = 0;
  devc->recording = 0;

  /* Init the memory port info.  */
  for (x = 0; x < 2; x++)
    {
      devc->portc[x].ptr = NULL;
      devc->portc[x].start_offset = 0;
      devc->portc[x].end_offset = 0;
    }
  devc->portc[0].physaddr = port1addr;

  /* Port 2 is easy.  */
  devc->portc[1].physaddr = port2addr;
  devc->portc[1].start_offset = 0;
  devc->portc[1].end_offset = NM_PORT2_SIZE;

  /* But we have to map in port 2 so we can check how much RAM the
     card has.  */

  physaddr = devc->portc[1].physaddr + devc->portc[1].start_offset;
  size = devc->portc[1].end_offset - devc->portc[1].start_offset;
  if (size == 0)
    cmn_err (CE_NOTE, "Bad I/O region size\n");
  devc->portc[1].ptr = (char *) MAP_PCI_MEM (devc->osdev, 1, physaddr, size);

  if (devc->portc[1].ptr == NULL)
    {
      cmn_err (CE_WARN, "Unable to remap port 1\n");
      return 0;
    }

  /* 
   * The NEOMAGIC has two memory ports.  The first port is nothing
   * more than a chunk of video RAM, which is used as the I/O ring
   * buffer.  The second port has the actual juicy stuff (like the
   * mixer and the playback engine control registers).
   */

  if (devc->rev == REV_NEOMAGICAV)
    {
      /* Ok, try to see if this is a non-AC97 version of the hardware. */
      /* unsigned char oldval = neomagic_readPort8 (devc, 2, 0x6c0); */

      /* First enable the AC97 controller. */
      neomagic_writePort8 (devc, 2, 0x6c0, 1);
      devc->portc[0].end_offset = 2560 * 1024;
      devc->introutine = neomagic_interrupt_av;
      devc->mixer_status_offset = NM_MIXER_STATUS_OFFSET;
      devc->mixer_status_mask = NM_MIXER_READY_MASK;
    }
  else
    {
      /* Not sure if there is any relevant detect for the ZX or not.  */
      if (neomagic_readPort8 (devc, 2, 0xa0b) != 0)
	devc->portc[0].end_offset = 6144 * 1024;
      else
	devc->portc[0].end_offset = 4096 * 1024;

      devc->introutine = neomagic_interrupt_zx;
      devc->mixer_status_offset = NMZX_MIXER_STATUS_OFFSET;
      devc->mixer_status_mask = NMZX_MIXER_READY_MASK;
    }

#if 1
  if (buffertop >= 0x18000 && buffertop < devc->portc[0].end_offset)
    devc->portc[0].end_offset = buffertop;
  else
    neomagic_peek_for_sig (devc);
#endif

  devc->portc[0].start_offset = devc->portc[0].end_offset - 0x18000;

  /* Map in this buffer so the driver can use it as DMA memory */
  physaddr = devc->portc[0].physaddr + devc->portc[0].start_offset;
  size = devc->portc[0].end_offset - devc->portc[0].start_offset;
  devc->portc[0].ptr = (char *) MAP_PCI_MEM (devc->osdev, 0, physaddr, size);

  if (devc->portc[0].ptr == NULL)
    {
      cmn_err (CE_WARN, "Unable to remap port 0\n");
      return 0;
    }

  /*
   *  Init the board.
   */

  devc->playbackBufferSize = 16384;
  devc->recordBufferSize = 16384;

  devc->coeffBuf = devc->portc[0].end_offset - NM_MAX_COEFFICIENT * 2;
  /* set record buffer start address */
  devc->abuf2 = (devc->coeffBuf - devc->recordBufferSize);
  /* set playback buffer start address */
  devc->abuf1 = (devc->abuf2 - devc->playbackBufferSize);

  devc->abuf2virt = devc->portc[0].ptr +
    devc->abuf2 - devc->portc[0].start_offset;

  devc->abuf1virt = devc->portc[0].ptr +
    devc->abuf1 - devc->portc[0].start_offset;

  devc->is_open_play = 0;
  devc->is_open_record = 0;
  devc->opencnt[0] = 0;
  devc->opencnt[1] = 0;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  neomagic_initHw (devc);
  neomagic_resetAC97 (devc);

  /* 
   * Install the AC97 mixer
   */

  devc->mixer_dev = ac97_install (&devc->ac97devc, "Neomagic AC97 Mixer",
				  neomagic_readAC97Reg,
				  neomagic_writeAC97Reg, devc, devc->osdev);
  if (devc->mixer_dev < 0)
    {
      cmn_err (CE_WARN, "Failed to install AC97 mixer device\n");
      return 0;
    }

  for (x = 0; x < 2; x++)
    {
      if ((adev =
	   oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				 devc->osdev,
				 devc->osdev,
				 devc->chip_name,
				 &neomagic_audio_driver,
				 sizeof (audiodrv_t),
				 0, AFMT_U8 | AFMT_S16_LE, devc, -1)) < 0)
	{
	  adev = -1;
	  return 0;
	}

      else
	{
	  audio_engines[adev]->mixer_dev = devc->mixer_dev;
	  audio_engines[adev]->min_rate = 6023;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  devc->dev[x] = adev;
	  devc->portc[x].bits = 8;
	  devc->portc[x].stereo = 1;
	  devc->portc[x].samplerate = 8000;
	}
    }

  if ((err =
       oss_register_interrupts (devc->osdev, 0, neomagic_interrupt,
				NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  return 1;
}

/*
 * Open the device
 *
 * DEV  - device
 * MODE - mode to open device (logical OR of OPEN_READ and OPEN_WRITE)
 *
 * Called when opening the DMAbuf               (dmabuf.c:259)
 */
/*ARGSUSED*/
static int
neomagic_audio_open (int dev, int mode, int open_flags)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;
  int w;

  if (devc == NULL)
    return OSS_ENODEV;

  if (devc->dev[0] == dev)
    w = 0;
  else if (devc->dev[1] == dev)
    w = 1;
  else
    return OSS_ENODEV;

  if (devc->opencnt[w] > 0)
    return OSS_EBUSY;

  /* No bits set? Huh? */
  if (!((mode & OPEN_READ) || (mode & OPEN_WRITE)))
    return OSS_EIO;

  /*
   * If it's open for both read and write, and the devc's currently
   * being read or written to, then do the opposite of what has
   * already been done.  Otherwise, don't specify any mode until the
   * user actually tries to do I/O.  (Some programs open the device
   * for both read and write, but only actually do reading or writing.)
   */

  if ((mode & OPEN_WRITE) && (mode & OPEN_READ))
    {
      if (devc->is_open_play)
	mode = OPEN_WRITE;
      else if (devc->is_open_record)
	mode = OPEN_READ;
      else
	mode = 0;
    }

  if (mode & OPEN_WRITE)
    {
      if (devc->is_open_play == 0)
	{
	  devc->dev_for_play = dev;
	  devc->is_open_play = 1;
	  audio_engines[dev]->fixed_rate = 0;
	  audio_engines[dev]->min_rate = 5000;
	  audio_engines[dev]->max_rate = 48000;
	  audio_engines[dev]->flags &=
	    ~(ADEV_FIXEDRATE | ADEV_16BITONLY | ADEV_STEREOONLY);
	}
      else
	return OSS_EBUSY;
    }

  if (mode & OPEN_READ)
    {
      if (devc->is_open_record == 0)
	{
	  devc->dev_for_record = dev;
	  devc->is_open_record = 1;
	  audio_engines[dev]->fixed_rate = 48000;
	  audio_engines[dev]->min_rate = 48000;
	  audio_engines[dev]->max_rate = 48000;
	  audio_engines[dev]->flags |=
	    ADEV_FIXEDRATE | ADEV_16BITONLY | ADEV_STEREOONLY;
	}
      else
	return OSS_EBUSY;
    }

  devc->opencnt[w]++;
  return 0;
}

/*
 * Close the device
 *
 * DEV  - device
 *
 * Called when closing the DMAbuf               (dmabuf.c:477)
 *      after halt_xfer
 */
/*ARGSUSED*/
static void
neomagic_audio_close (int dev, int mode)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;

  if (devc != NULL)
    {
      int w;

      if (devc->dev[0] == dev)
	w = 0;
      else if (devc->dev[1] == dev)
	w = 1;
      else
	return;

      devc->opencnt[w]--;
      if (devc->opencnt[w] <= 0)
	{
	  devc->opencnt[w] = 0;

	  if (devc->dev_for_play == dev)
	    {
	      stopPlay (devc);
	      devc->is_open_play = 0;
	      devc->dev_for_play = -1;
	    }

	  if (devc->dev_for_record == dev)
	    {
	      stopRecord (devc);
	      devc->is_open_record = 0;
	      devc->dev_for_record = -1;
	    }
	}
    }
}

static int
neomagic_set_rate (int dev, int arg)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;
  int which;

  if (devc == NULL)
    return OSS_ENODEV;

  if (devc->dev[0] == dev)
    which = 0;
  else
    which = 1;

  if (audio_engines[dev]->flags & ADEV_FIXEDRATE)
    {
      arg = 48000;
    }


  if (arg == 0)
    return devc->portc[which].samplerate;

  if (arg > 48000)
    arg = 48000;
  if (arg < 5000)
    arg = 5000;

  devc->portc[which].samplerate = arg;
  return devc->portc[which].samplerate;
}

static short
neomagic_set_channels (int dev, short arg)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;
  int which;

  if (devc == NULL)
    return OSS_ENODEV;

  if (devc->dev[0] == dev)
    which = 0;
  else
    which = 1;

  if (audio_engines[dev]->flags & ADEV_STEREOONLY)
    {
      arg = 2;
    }
  if ((arg != 1) && (arg != 2))
    return devc->portc[which].stereo;
  devc->portc[which].stereo = arg;
  return devc->portc[which].stereo;
}

static unsigned int
neomagic_set_format (int dev, unsigned int arg)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;
  int which;

  if (devc->dev[0] == dev)
    which = 0;
  else
    which = 1;

  if (audio_engines[dev]->flags & ADEV_16BITONLY)
    {
      arg = 16;
    }

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return devc->portc[which].bits;
  devc->portc[which].bits = arg;
  return devc->portc[which].bits;
}

/*ARGSUSED*/
static int
neomagic_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

/*
 * Output a block to sound device
 *
 * dev          - device number
 * buf          - physical address of buffer
 * count  	- byte count in buffer
 * intrflag     - set if this has been called from an interrupt
 *				  (via oss_audio_outputintr)
 * restart_dma  - set if engine needs to be re-initialised
 *
 * Called when:
 *  1. Starting output                                  (dmabuf.c:1327)
 *  2.                                                  (dmabuf.c:1504)
 *  3. A new buffer needs to be sent to the device      (dmabuf.c:1579)
 */
/*ARGSUSED*/
static void
neomagic_audio_output_block (int dev, oss_native_word physbuf,
			     int count, int fragsize, int intrflag)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;

  if (devc != NULL)
    {
      devc->is_open_play = 1;
      devc->dev_for_play = dev;
      neomagic_write_block (devc, physbuf, count);
    }
}

/* Ditto, but do recording instead.  */
/*ARGSUSED*/
static void
neomagic_audio_start_input (int dev, oss_native_word physbuf, int count,
			    int fragsize, int intrflag)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;

  if (devc != NULL)
    {
      devc->is_open_record = 1;
      devc->dev_for_record = dev;
      neomagic_read_block (devc, physbuf, count);
    }
}

/* 
 * Prepare for inputting samples to DEV. 
 * Each requested buffer will be BSIZE byes long, with a total of
 * BCOUNT buffers. 
 */

static int
neomagic_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  if (devc == NULL)
    return OSS_ENODEV;

  if (devc->is_open_record && devc->dev_for_record != dev)
    return OSS_EBUSY;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /*
   * If we're not currently recording, set up the start and end registers
   * for the recording engine.
   */
  if (!devc->recording)
    {
      neomagic_writePort32 (devc, 2, NM_RBUFFER_START, devc->abuf2);
      neomagic_writePort32 (devc, 2, NM_RBUFFER_END,
			    devc->abuf2 + bsize * bcount);

      neomagic_writePort32 (devc, 2, NM_RBUFFER_CURRP, devc->abuf2);
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*
 * Prepare for outputting samples to `dev'
 *
 * Each buffer that will be passed will be `bsize' bytes long,
 * with a total of `bcount' buffers.
 *
 * Called when:
 *  1. A trigger enables audio output                   (dmabuf.c:978)
 *  2. We get a write buffer without dma_mode setup     (dmabuf.c:1152)
 *  3. We restart a transfer                            (dmabuf.c:1324)
 */

static int
neomagic_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  if (devc == NULL)
    return OSS_ENODEV;

  if (devc->is_open_play && devc->dev_for_play != dev)
    return OSS_EBUSY;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /*
   * Setup the buffer start and end registers.
   */
  if (!devc->playing)
    {
      /* The PBUFFER_END register in this case points to one sample
         before the end of the buffer. */
      int w = (devc->dev_for_play == devc->dev[0] ? 0 : 1);
      int sampsize = (devc->portc[w].bits == 16 ? 2 : 1);

      if (devc->portc[w].stereo == 2)
	sampsize *= 2;

      /* Need to set the not-normally-changing-registers up. */
      neomagic_writePort32 (devc, 2, NM_PBUFFER_START, devc->abuf1);
      neomagic_writePort32 (devc, 2, NM_PBUFFER_END,
			    devc->abuf1 + bsize * bcount - sampsize);
      neomagic_writePort32 (devc, 2, NM_PBUFFER_CURRP, devc->abuf1);
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/* Stop the current operations associated with DEV.  */
static void
neomagic_audio_reset (int dev)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;

  if (devc != NULL)
    {
      if (devc->dev_for_play == dev)
	stopPlay (devc);
      if (devc->dev_for_record == dev)
	stopRecord (devc);
    }
}


static int
neomagic_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;

  if (direction & PCM_ENABLE_OUTPUT)
    {
      dmap->dmabuf = (void *) devc->abuf1virt;
      dmap->dmabuf_phys = devc->portc[0].physaddr + devc->abuf1;
      dmap->buffsize = devc->playbackBufferSize;
    }
  else if (direction & PCM_ENABLE_INPUT)
    {
      dmap->dmabuf = (void *) devc->abuf2virt;
      dmap->dmabuf_phys = devc->portc[0].physaddr + devc->abuf2;
      dmap->buffsize = devc->recordBufferSize;
    }

  return 0;
}

/*ARGSUSED*/
static int
neomagic_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;
  dmap->dmabuf = NULL;
  return 0;
}

static void
neomagic_trigger (int dev, int state)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  if (devc != NULL)
    {
      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
      if (devc->dev_for_play == dev)
	{
	  if (state & PCM_ENABLE_OUTPUT)
	    {
	      startPlay (devc);
	    }
	  else
	    {
	      stopPlay (devc);
	    }
	}
      if (devc->dev_for_record == dev)
	{
	  if (state & PCM_ENABLE_INPUT)
	    {
	      startRecord (devc);
	    }
	  else
	    {
	      stopRecord (devc);
	    }
	}
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
    }
}

#if 0
static int
neomagic_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  struct neomagic_devc *devc = audio_engines[dev]->devc;
  unsigned int amt = 0;
  oss_native_word flags;

  if (devc == NULL)
    return OSS_ENODEV;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      if (devc->dev_for_play == dev && devc->playing)
	{
	  amt = neomagic_readPort32 (devc, 2, NM_PBUFFER_CURRP) - devc->abuf1;
	}
    }

  if (direction == PCM_ENABLE_INPUT)
    {
      if (devc->dev_for_record == dev && devc->recording)
	{
	  amt = neomagic_readPort32 (devc, 2, NM_RBUFFER_CURRP) - devc->abuf2;
	}
    }

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return amt;
}
#endif


static audiodrv_t neomagic_audio_driver = {
  neomagic_audio_open,		/* open                 */
  neomagic_audio_close,		/* close                */
  neomagic_audio_output_block,	/* output_block         */
  neomagic_audio_start_input,	/* start_input          */
  neomagic_audio_ioctl,		/* ioctl                */
  neomagic_audio_prepare_for_input,	/* prepare_for_input    */
  neomagic_audio_prepare_for_output,	/* prepare_for_output   */
  neomagic_audio_reset,		/* reset                */
  NULL,				/*+local_qlen           */
  NULL,				/*+copy_from_user       */
  NULL,				/*+halt_input           */
  NULL,				/* halt_output          */
  neomagic_trigger,		/*+trigger              */
  neomagic_set_rate,		/*+set_rate            */
  neomagic_set_format,		/*+set_format             */
  neomagic_set_channels,	/*+set_channels         */
  NULL,
  NULL,
  NULL,
  NULL,
  neomagic_alloc_buffer,
  neomagic_free_buffer,
  NULL,
  NULL,
  NULL				/*neomagic_get_buffer_pointer */
};

int
oss_neomagic_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int port1addr, port2addr;
  neomagic_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered NM2200 probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if ((vendor != NEOMAGIC_VENDOR_ID) ||
      (device != PCI_DEVICE_ID_NEOMAGIC_NEOMAGICAV_AUDIO &&
       device != PCI_DEVICE_ID_NEOMAGIC_NEOMAGICZX_AUDIO &&
       device != PCI_DEVICE_ID_NEOMAGIC_NM2360_AUDIO))

    return 0;

  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &port1addr);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_1, &port2addr);

  if ((port1addr == 0) || (port2addr == 0))
    {
      cmn_err (CE_WARN, "undefined MEMORY I/O address.\n");
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

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->irq = pci_irq_line;


  switch (device)
    {
    case PCI_DEVICE_ID_NEOMAGIC_NEOMAGICAV_AUDIO:
      devc->rev = REV_NEOMAGICAV;
      devc->chip_name = "Neomagic NM2200AV";
      break;
    case PCI_DEVICE_ID_NEOMAGIC_NEOMAGICZX_AUDIO:
      devc->rev = REV_NEOMAGICZX;
      devc->chip_name = "Neomagic NM2200ZX";
      break;
    case PCI_DEVICE_ID_NEOMAGIC_NM2360_AUDIO:
      devc->rev = REV_NEOMAGICZX;
      devc->chip_name = "Neomagic NM22360";
      break;
    default:
      return 0;
      break;
    }


  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  port1addr &= ~0x0F;
  port2addr &= ~0x0F;

  oss_register_device (osdev, devc->chip_name);

  return neomagic_install (devc, port1addr, port2addr);
}


int
oss_neomagic_detach (oss_device_t * osdev)
{
  struct neomagic_devc *devc = (neomagic_devc *) osdev->devc;
  int i;

  if (oss_disable_device (osdev) < 0)
    return 0;

#if 0
  stopPlay (devc);
  stopRecord (devc);
#endif

  neomagic_initHw (devc);
  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  for (i = 0; i < MAX_PORTC; i++)
    {
      UNMAP_PCI_MEM (devc->osdev, 0,
		     devc->portc[i].physaddr + devc->portc[i].start_offset,
		     devc->portc[i].ptr,
		     devc->portc[i].end_offset - devc->portc[i].start_offset);

      devc->portc[i].ptr = NULL;
    }

  oss_unregister_device (devc->osdev);
  return 1;
}
