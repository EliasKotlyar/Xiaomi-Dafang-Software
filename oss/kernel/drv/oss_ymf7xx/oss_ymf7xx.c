/*
 * Purpose: Driver for Yamaha YMF7xx PCI audio controller.
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

#include "oss_ymf7xx_cfg.h"
#include "ymf7xx.h"
#include "oss_pci.h"
#include "ac97.h"
#include "uart401.h"

#define YAMAHA_VENDOR_ID	0x1073
#define YAMAHA_YMF724_ID	0x0004
#define YAMAHA_YMF724F_ID	0x000d
#define YAMAHA_YMF734_ID	0x0005
#define YAMAHA_YMF740_ID	0x000a
#define YAMAHA_YMF740C_ID	0x000c
#define YAMAHA_YMF744_ID	0x0010
#define YAMAHA_YMF754_ID	0x0012

#define WRITEB(a,d) devc->bRegister[a] = d
#define READB(a) devc->bRegister[a]
#define WRITEW(a,d) devc->wRegister[a>>1] = d
#define READW(a) devc->wRegister[a>>1]
#define WRITEL(a,d) devc->dwRegister[a>>2] = d
#define READL(a) (devc->dwRegister[a>>2])

#ifdef OSS_BIG_ENDIAN
static __inline__ unsigned int
ymf_swap (unsigned int x)
{
  return ((x & 0x000000ff) << 24) |
    ((x & 0x0000ff00) << 8) |
    ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
}

#define LSWAP(x) ymf_swap(x)
#else
#define LSWAP(x) 	x
#endif

#define MAX_PORTC 8

extern int yamaha_mpu_ioaddr;
extern int yamaha_mpu_irq;
extern int yamaha_fm_ioaddr;


typedef struct ymf7xx_portc
{
  int speed, bits, channels;
  int open_mode;
  int trigger_bits;
  int audio_enabled;
  int audiodev;
  int devs_opened;
  int devnum;
  PLAY_BANK *bank1, *bank2, *bank3, *bank4;
  EFFECT_CNTRL_SLOT effectslot;
  REC_CNTRL_SLOT recslot;
  int dacfmt;
}
ymf7xx_portc;

typedef struct ymf7xx_devc
{
  oss_device_t *osdev;
  char *chip_name;
  int deviceid;
  unsigned int base0addr;
  unsigned int *base0virt;
  volatile unsigned int *dwRegister;
  volatile unsigned short *wRegister;
  volatile unsigned char *bRegister;
  int irq;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Legacy */
  int mpu_base, mpu_irq;
  int fm_base;
  int fm_attached, mpu_attached;

  /* Mixer parameters */
  ac97_devc ac97devc, ac97devc2;
  int mixer_dev;
  int mixlevels[10];

  /* Audio parameters */
  int audio_initialized;
  ymf7xx_portc portc[MAX_PORTC];

  /* Play Table */
  volatile oss_native_word slt;
  oss_native_word slt_phys;
  volatile unsigned int *tab;
  oss_native_word play_table;
  oss_native_word play_table_virt;
  unsigned char *dmabuf1;
  oss_dma_handle_t dmabuf1_dma_handle;

  /* Effect Table */
  volatile unsigned int *effecttab;
  oss_native_word effect_table;
  oss_native_word effect_table_virt, eff_buf_phys;
  unsigned char *dmabuf2, *eff_buf;
  oss_dma_handle_t dmabuf2_dma_handle;
  oss_dma_handle_t eff_buf_dma_handle;

  /* Recording Table */
  volatile unsigned int *rectab;
  oss_native_word rec_table;
  oss_native_word rec_table_virt;
  unsigned char *dmabuf3;
  oss_dma_handle_t dmabuf3_dma_handle;
  int spdif_in;
}
ymf7xx_devc;

int SetupPlaySlot (int dev, int slot);
int ymf7xx_spdif_control (int dev, int ctrl, unsigned int cmd, int value);

static int
ac97_read (void *devc_, int addr)
{
  ymf7xx_devc *devc = devc_;
  int count;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  WRITEW (AC97_CMD_ADDRESS, addr | 0x8000);

  for (count = 0; count < 1000; count++)
    if ((READW (AC97_STATUS_ADDRESS) >> 15) == 0)
      {
	MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	return READW (AC97_STATUS_DATA);
      }
  DDB (cmn_err (CE_WARN, "AC97 mixer read timed out\n"));
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return OSS_EIO;
}

static int
ac97_write (void *devc_, int addr, int data)
{
  ymf7xx_devc *devc = devc_;
  oss_native_word flags;
  int count;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  WRITEW (AC97_CMD_ADDRESS, addr);
  WRITEW (AC97_CMD_DATA, data);

  for (count = 0; count < 1000; count++)
    if ((READW (AC97_STATUS_ADDRESS) >> 15) == 0)
      {
	MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	return 1;
      }
  DDB (cmn_err (CE_WARN, "AC97 mixer write timed out\n"));
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

static void
install_ucode (ymf7xx_devc * devc, int addr, unsigned int *src, int len)
{
  int i;

  for (i = 0; i < len; i++)
    {
      WRITEL (addr, *src);
      addr += 4;
      src++;
    }
}

static int
ymf7xxintr (oss_device_t * osdev)
{
  ymf7xx_devc *devc = (ymf7xx_devc *) osdev->devc;
  ymf7xx_portc *portc;
  dmap_t *dmapin, *dmapout;
  unsigned int status;
  int i, n;
  int currdac = 0;
  int curradc = 0;
  int serviced = 0;


  status = READL (STATUS);

  if ((status & 0x80000000))
    {
      serviced = 1;
      for (i = 0; i < MAX_PORTC; i++)
	{
	  portc = &devc->portc[i];

	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    {
	      dmapout = audio_engines[portc->audiodev]->dmap_out;
	      currdac = LSWAP (portc->bank1->PgStart);
	      currdac /= dmapout->fragment_size / portc->dacfmt;

	      if (currdac < 0 || currdac >= dmapout->nfrags)
		currdac = 0;
	      n = 0;
	      while (dmap_get_qhead (dmapout) != currdac
		     && n++ < dmapout->nfrags)
		oss_audio_outputintr (portc->audiodev, 1);
	    }
	  if (portc->trigger_bits & PCM_ENABLE_INPUT)
	    {
	      dmapin = audio_engines[portc->audiodev]->dmap_in;

	      if (devc->spdif_in)
		curradc = LSWAP (portc->recslot.bank1->PgStartAdr);
	      else
		curradc = LSWAP (portc->recslot.bank3->PgStartAdr);

	      curradc /= dmapin->fragment_size;

	      if (curradc < 0 || curradc >= dmapin->nfrags)
		curradc = 0;
	      n = 0;
	      while (dmap_get_qtail (dmapin) != curradc
		     && n++ < dmapin->nfrags)
		oss_audio_inputintr (portc->audiodev, 0);
	    }
	  WRITEL (STATUS, 0x80000000);
	  WRITEL (MODE, READL (MODE) | 0x00000002);
	}
    }

  return serviced;
}

/*
 *****************************************************************************
 */

static int
ymf7xx_audio_set_rate (int dev, int arg)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->speed;

  if (arg > 48000)
    arg = 48000;
  if (arg < 5000)
    arg = 5000;
  portc->speed = arg;
  return portc->speed;
}

static short
ymf7xx_audio_set_channels (int dev, short arg)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
ymf7xx_audio_set_format (int dev, unsigned int arg)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE | AFMT_AC3)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
ymf7xx_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void ymf7xx_audio_trigger (int dev, int state);

static void
ymf7xx_audio_reset (int dev)
{
  ymf7xx_audio_trigger (dev, 0);
}

static void
ymf7xx_audio_reset_input (int dev)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;
  ymf7xx_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
ymf7xx_audio_reset_output (int dev)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;
  ymf7xx_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
ymf7xx_audio_open (int dev, int mode, int open_flags)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;
  ymf7xx_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  portc->audio_enabled = ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
ymf7xx_audio_close (int dev, int mode)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  ymf7xx_audio_reset (dev);
  portc->open_mode = 0;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
ymf7xx_audio_output_block (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
ymf7xx_audio_start_input (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
ymf7xx_audio_trigger (int dev, int state)
{
  ymf7xx_devc *devc = audio_engines[dev]->devc;
  ymf7xx_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->devs_opened = SetupPlaySlot (dev, portc->devnum);

	      WRITEL (CONTROL_SELECT, 1);
	      WRITEL (MODE, READL (MODE) | 0x00000003);
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

	      devc->tab[portc->devs_opened] = 0;
	      if (portc->channels > 1)
		devc->tab[portc->devs_opened - 1] = 0;
	    }
	}
    }

  if ((portc->open_mode & OPEN_READ)
      && !(audio_engines[dev]->flags & ADEV_NOINPUT))
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      /* 0x01 = REC SLOT, 0x02 = ADC SLOT */
	      if (devc->spdif_in)
		WRITEL (MAP_OF_REC, 0x1);
	      else
		WRITEL (MAP_OF_REC, 0x2);
	      WRITEL (CONTROL_SELECT, 0);
	      WRITEL (MODE, READL (MODE) | 0x00000003);
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_INPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;

	      /* set the map of  rec 0 */
	      WRITEL (MAP_OF_REC, 0x0);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static unsigned int
SetFormatField (ymf7xx_portc * portc)
{
  unsigned int val;

  val = 0x00000000;
  if (portc->bits == 8)
    val |= 0x80000000;
  if (portc->channels > 1)
    val |= 0x00010000;
  return val;
}

static unsigned int
SetPgDeltaField (ymf7xx_portc * portc)
{
  unsigned int val;

#if 0
  double x;
  x = (double) portc->speed / 48000.0;
  val = (int) (x * (1 << 28)) & 0x7FFFFF00;
#else
  oss_native_word x;
  x = (portc->speed * (1 << 15) + 187) / 375;
  val = (x * (1 << 6)) & 0x7FFFFF00;
#endif
  return val;
}

static unsigned int
SetLpfKField (ymf7xx_portc * portc)
{
  unsigned int i, val = 0, sr = portc->speed;
  int freq[8] = { 100, 2000, 8000, 11025, 16000, 22050, 32000, 48000 };
  int LpfK[8] =
    { 0x0057, 0x06aa, 0x18B2, 0x2093, 0x2b9a, 0x35a1, 0x3eaa, 0x4000 };

  if (sr == 44100)
    {
      val = 0x4646 << 16;
    }
  else
    {
      for (i = 0; i < 8; i++)
	{
	  if (sr <= freq[i])
	    {
	      val = LpfK[i] << 16;
	      break;
	    }
	}
    }
  return val;
}

static unsigned int
SetLpfQField (ymf7xx_portc * portc)
{
  unsigned int i, val = 0, sr = portc->speed;
  int freq[8] = { 100, 2000, 8000, 11025, 16000, 22050, 32000, 48000 };
  int LpfQ[8] =
    { 0x3528, 0x34a7, 0x3202, 0x3177, 0x3139, 0x31c9, 0x33d0, 0x4000 };

  if (sr == 44100)
    {
      val = 0x370A << 16;
    }
  else
    {
      for (i = 0; i < 8; i++)
	{
	  if (sr <= freq[i])
	    {
	      val = LpfQ[i] << 16;
	      break;
	    }
	}
    }
  return val;
}

void
SetupRecSlot (int dev)
{
  ymf7xx_devc *devc = audio_engines[dev]->devc;
  ymf7xx_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  int banksize;

  banksize = READL (REC_CNTRL_SIZE);
  portc->recslot.base = (devc->rec_table_virt);

  /* banks 1 and 2 are for REC Slot and 3 and 4 are for ADC Slot */
  if (devc->spdif_in)
    {
      portc->recslot.bank1 =
	(REC_BANK *) (portc->recslot.base + (banksize * 0));
      portc->recslot.bank2 =
	(REC_BANK *) (portc->recslot.base + (banksize * 4));

      WRITEB (REC_FORMAT,
	      (unsigned char) ((portc->channels & 0x02) +
			       ((portc->bits & 0x08) >> 3)));
      WRITEW (REC_SAMPLING_RATE,
	      (unsigned short) (48000 * 4096 / portc->speed) - 1);

      portc->recslot.bank1->PgBase = portc->recslot.bank2->PgBase = LSWAP (dmap->dmabuf_phys);	/* use the REC slot */

      portc->recslot.bank1->PgLoopEndAdr = portc->recslot.bank2->PgLoopEndAdr = LSWAP (dmap->bytes_in_use);	/* use the REC slot */
    }
  else
    {
      portc->recslot.bank3 =
	(REC_BANK *) (portc->recslot.base + (banksize * 8));
      portc->recslot.bank4 =
	(REC_BANK *) (portc->recslot.base + (banksize * 12));

      WRITEB (ADC_FORMAT,
	      (unsigned char) ((portc->channels & 0x02) +
			       ((portc->bits & 0x08) >> 3)));
      WRITEW (ADC_SAMPLING_RATE,
	      (unsigned short) (48000 * 4096 / portc->speed) - 1);

      portc->recslot.bank3->PgBase = portc->recslot.bank4->PgBase = LSWAP (dmap->dmabuf_phys);	/* use the ADC slot */

      portc->recslot.bank3->PgLoopEndAdr = portc->recslot.bank4->PgLoopEndAdr = LSWAP (dmap->bytes_in_use);	/* use the ADC slot */
    }

}

/*ARGSUSED*/
static int
ymf7xx_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  ymf7xx_devc *devc = audio_engines[dev]->devc;
  ymf7xx_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  SetupRecSlot (dev);

  /* set the input level to max and go! */
  WRITEL (NATIVE_REC_INPUT, 0xffffffff);
  WRITEL (NATIVE_ADC_INPUT, 0xffffffff);
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

void
SetupEffectSlot (int dev)
{
  ymf7xx_devc *devc = audio_engines[dev]->devc;
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  int banksize;

  banksize = READL (EFF_CNTRL_SIZE);
  portc->effectslot.base = (devc->effect_table_virt);

  /* slots 1-5 each having 2 banks */
  portc->effectslot.bank1 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 0));
  portc->effectslot.bank2 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 4));
  portc->effectslot.bank3 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 8));
  portc->effectslot.bank4 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 12));
  portc->effectslot.bank5 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 16));
  portc->effectslot.bank6 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 20));
  portc->effectslot.bank7 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 24));
  portc->effectslot.bank8 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 28));
  portc->effectslot.bank9 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 32));
  portc->effectslot.bank10 =
    (EFFECT_BANK *) (portc->effectslot.base + (banksize * 36));

#if 0
/* Dry Left Channel */
  portc->effectslot.bank1->PgBase = portc->effectslot.bank2->PgBase =
    LSWAP (devc->eff_buf_phys + 0 * 8192);
  portc->effectslot.bank1->PgLoopEnd = portc->effectslot.bank2->PgLoopEnd =
    LSWAP (4096);
  portc->effectslot.bank1->PgStart = portc->effectslot.bank2->PgStart = 0;

/* Dry Right Channel */
  portc->effectslot.bank3->PgBase = portc->effectslot.bank4->PgBase =
    LSWAP (devc->eff_buf_phys + 1 * 8192);
  portc->effectslot.bank3->PgLoopEnd = portc->effectslot.bank4->PgLoopEnd =
    LSWAP (4096);
  portc->effectslot.bank3->PgStart = portc->effectslot.bank4->PgStart = 0;

/* Effect 1 */
  portc->effectslot.bank5->PgBase = portc->effectslot.bank6->PgBase =
    LSWAP (devc->eff_buf_phys + 2 * 8192);
  portc->effectslot.bank5->PgLoopEnd = portc->effectslot.bank6->PgLoopEnd =
    LSWAP (4096);
  portc->effectslot.bank5->PgStart = portc->effectslot.bank6->PgStart = 0;

#endif

/* Effect 2 */
  portc->effectslot.bank7->PgBase = portc->effectslot.bank8->PgBase =
    LSWAP (devc->eff_buf_phys + 0 * 8192);
  portc->effectslot.bank7->PgLoopEnd = portc->effectslot.bank8->PgLoopEnd =
    LSWAP (4096);
  portc->effectslot.bank7->PgStart = portc->effectslot.bank8->PgStart = 0;

/* Effect 3 */
  portc->effectslot.bank9->PgBase = portc->effectslot.bank10->PgBase =
    LSWAP (devc->eff_buf_phys + 1 * 8192);
  portc->effectslot.bank9->PgLoopEnd = portc->effectslot.bank10->PgLoopEnd =
    LSWAP (4096);
  portc->effectslot.bank9->PgStart = portc->effectslot.bank10->PgStart = 0;

  WRITEL (AC97_SEC_CONFIG, (READL (AC97_SEC_CONFIG) & ~0x0030) | 0x0010);
  WRITEL (MAP_OF_EFFECTS, 0x18);	/* effect 2,3 */
  WRITEL (MODE, READL (MODE) | (1 << 30));	/* AC3 Setup */
}


void
SetupPlayBank (int dev, int slot, oss_native_word sbase)
{
  ymf7xx_devc *devc = audio_engines[dev]->devc;
  ymf7xx_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;

  PLAY_BANK *bank1;
  PLAY_BANK *bank2;
  bank1 = (PLAY_BANK *) sbase;
  bank2 =
    (PLAY_BANK *) (sbase + READL (PLAY_CNTRL_SIZE) * sizeof (unsigned int));

  memset ((void *) bank1, 0, sizeof (PLAY_BANK));
  memset ((void *) bank2, 0, sizeof (PLAY_BANK));

  if (slot == 2)
    {
      portc->bank3 = bank1;
      portc->bank4 = bank2;
    }
  else
    {
      portc->bank1 = bank1;
      portc->bank2 = bank2;
    }

  /* setup format field */
  bank1->Format = LSWAP (SetFormatField (portc));
  bank1->EgGain = bank1->EgGainEnd = LSWAP (0x40000000);

  if (portc->channels == 1)
    {
      /* Gain */
      bank1->LchGain = bank1->LchGainEnd = LSWAP (0x40000000);
      bank1->RchGain = bank1->RchGainEnd = LSWAP (0x40000000);
      bank1->Effect2Gain = bank1->Effect2GainEnd = LSWAP (0x40000000);
      bank1->Effect3Gain = bank1->Effect3GainEnd = LSWAP (0x40000000);
    }
  else
    {
      if (slot == 2)
	{
	  bank1->Format = LSWAP (LSWAP (bank1->Format) + 1);
	  bank1->RchGain = bank1->RchGainEnd = LSWAP (0x40000000);
	  bank1->Effect2Gain = bank1->Effect2GainEnd = LSWAP (0x40000000);
	}
      else
	{
	  bank1->LchGain = bank1->LchGainEnd = LSWAP (0x40000000);
	  bank1->Effect3Gain = bank1->Effect3GainEnd = LSWAP (0x40000000);
	}
    }

  bank1->LoopDefault = 0;
  bank1->NumOfFrames = 0;
  bank1->LoopCount = 0;
  bank1->PgStart = 0;
  bank1->PgLoop = 0;

  /* PgBase */
  bank1->PgBase = LSWAP (dmap->dmabuf_phys);

  /* PgLoopEnd */
  bank1->PgLoopEnd = LSWAP (dmap->bytes_in_use / portc->dacfmt);

  /* PgDelta & PgDeltaEnd */
  bank1->PgDelta = bank1->PgDeltaEnd = LSWAP (SetPgDeltaField (portc));

  if (portc->channels == 4)
    bank1->PgDelta = bank1->PgDeltaEnd *= 2;

  /* LpfK & LpfKEnd */
  bank1->LpfK = bank1->LpfKEnd = LSWAP (SetLpfKField (portc));

  /* LpfQ */
  bank1->LpfQ = LSWAP (SetLpfQField (portc));

  memcpy (bank2, bank1, sizeof (PLAY_BANK));
}

int
SetupPlaySlot (int dev, int slot)
{
  int tmp;
  oss_native_word sltbase;
  oss_native_word physbase;

  ymf7xx_devc *devc = audio_engines[dev]->devc;
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  tmp = slot * 2 + 1;
  sltbase = (oss_native_word) devc->slt + (tmp * 0x120);
  physbase = devc->slt_phys + (tmp * 0x120);
  SetupPlayBank (dev, 1, sltbase);
  devc->tab[tmp] = LSWAP (physbase);

  if (portc->channels > 1)
    {
      sltbase += 0x120;
      physbase += 0x120;
      SetupPlayBank (dev, 2, sltbase);
      tmp++;
      devc->tab[tmp] = LSWAP (physbase);
    }
  return tmp;
}

/*ARGSUSED*/
static int
ymf7xx_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  ymf7xx_devc *devc = audio_engines[dev]->devc;
  ymf7xx_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->bits == AFMT_AC3)
    {
      portc->channels = 2;
      portc->bits = 16;
      ymf7xx_spdif_control (devc->mixer_dev, 1, SNDCTL_MIX_WRITE, 1);	/* enable SPDIF */
      ymf7xx_spdif_control (devc->mixer_dev, 4, SNDCTL_MIX_WRITE, 1);	/* enable AC3 */
    }
  else
    ymf7xx_spdif_control (devc->mixer_dev, 4, SNDCTL_MIX_WRITE, 0);	/* enable AC3 */


  portc->dacfmt = portc->channels * (portc->bits / 8);

  if (portc->dacfmt > 4)
    portc->dacfmt = 4;
  /* portc->devs_opened = SetupPlaySlot (dev, portc->devnum); -> trigger */
  /* set effects slot */
  SetupEffectSlot (dev);

  if (portc->bits != AFMT_AC3)
    WRITEL (NATIVE_DAC, 0xFFFFFFFF);
  else
    WRITEL (NATIVE_DAC, 0x00000000);

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
ymf7xx_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  ymf7xx_portc *portc = audio_engines[dev]->portc;

  unsigned int ptr = 0;

  if (direction == PCM_ENABLE_OUTPUT)
    {
      ptr = LSWAP (portc->bank1->PgStart);
      ptr *= portc->dacfmt;
    }
  if (direction == PCM_ENABLE_INPUT)
    {
      ptr = LSWAP (portc->recslot.bank3->PgStartAdr);
    }
  return ptr;
}

static audiodrv_t ymf7xx_audio_driver = {
  ymf7xx_audio_open,
  ymf7xx_audio_close,
  ymf7xx_audio_output_block,
  ymf7xx_audio_start_input,
  ymf7xx_audio_ioctl,
  ymf7xx_audio_prepare_for_input,
  ymf7xx_audio_prepare_for_output,
  ymf7xx_audio_reset,
  NULL,
  NULL,
  ymf7xx_audio_reset_input,
  ymf7xx_audio_reset_output,
  ymf7xx_audio_trigger,
  ymf7xx_audio_set_rate,
  ymf7xx_audio_set_format,
  ymf7xx_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* ymf7xx_alloc_buffer */
  NULL,				/* ymf7xx_free_buffer */
  NULL,
  NULL,
  ymf7xx_get_buffer_pointer
};

#if 0
static void
init_audio (ymf7xx_devc * devc)
{
  devc->audio_initialized = 1;
}

uninit_audio (ymf7xx_devc * devc)
{
  devc->audio_initialized = 0;
  WRITEL (CONFIG, 0);
  WRITEL (MODE, 0);
}
#endif

#ifdef OBSOLETED_STUFF
/*
 * This device has "ISA style" MIDI and FM subsystems. Such devices don't
 * use PCI config space for the I/O ports and interrupts. Instead the driver
 * needs to allocate proper resources itself. This functionality is no longer
 * possible. For this reason the MIDI and FM parts are not accessible.
 */
static void
attach_fm (ymf7xx_devc * devc)
{
  devc->fm_attached = 0;
  if (!opl3_detect (0x388, devc->osdev))
    {
      cmn_err (CE_WARN, "OPL3 not detected\n");
      return;
    }
  opl3_init (0x388, devc->osdev);
  devc->fm_attached = 1;
}


static void
attach_mpu (ymf7xx_devc * devc)
{
  struct address_info hw_config;

  hw_config.io_base = devc->mpu_base;
  hw_config.irq = devc->mpu_irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "Yamaha DS-XG MPU401";
  hw_config.driver_use_1 = 0;
  hw_config.driver_use_2 = 0;
  hw_config.osdev = devc->osdev;
#ifdef CREATE_OSP
  CREATE_OSP (hw_config.osdev);
#endif
  hw_config.card_subtype = 0;

  if (!probe_uart401 (&hw_config))
    {
      cmn_err (CE_WARN, "MPU-401 was not detected\n");
      return;
    }
  DDB (cmn_err (CE_WARN, "MPU-401 detected - Good\n"));
  devc->mpu_attached = 1;
  attach_uart401 (&hw_config);
}

static void
unload_mpu (ymf7xx_devc * devc)
{
  struct address_info hw_config;

  hw_config.io_base = devc->mpu_base;
  hw_config.irq = devc->mpu_irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "Yahama DS-XG MPU401";
  hw_config.driver_use_1 = 0;
  hw_config.driver_use_2 = 0;
  hw_config.osdev = devc->osdev;
#ifdef CREATE_OSP
  CREATE_OSP (hw_config.osdev);
#endif
  hw_config.card_subtype = 0;

  devc->mpu_attached = 0;
  unload_uart401 (&hw_config);
}
#endif

int
ymf7xx_spdif_control (int dev, int ctrl, unsigned int cmd, int value)
{
  ymf7xx_devc *devc = mixer_devs[dev]->hw_devc;
/*  int left, right; */

  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case 1:		/* S/PDIF Enable/Disable */
	  value = READL (SPDIFOUT_CONTROL) & 0x1;
	  break;

	case 2:		/* S/PDIF Record Enable */
	  value = devc->spdif_in;
	  break;

	case 3:		/* S/PDIF Loopback */
	  value = READL (SPDIFIN_CONTROL) & (1 << 4);
	  break;

	case 4:		/* AC3 Output */
	  value = READL (SPDIFOUT_CONTROL) & (1 << 1);
	  break;
#if 0
	case 5:		/* CopyProtection Bit */
	  value = READL (SPDIFOUT_STATUS) & (1 << 2);
	  break;
	case 6:		/* SPDIF OUTVOL */
	  value = devc->mixlevels[0];
	  break;
	case 7:		/* SPDIF LOOPVOL */
	  value = devc->mixlevels[1];
	  break;
	case 8:		/* SPDIF AC3VOL */
	  value = devc->mixlevels[2];
	  break;
#endif
	}
    }
  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case 1:		/* S/PDIF OUTPUT ENABLE/DISABLE */
	  if (value)
	    WRITEL (SPDIFOUT_CONTROL, READL (SPDIFOUT_CONTROL) | 0x1);
	  else
	    WRITEL (SPDIFOUT_CONTROL, READL (SPDIFOUT_CONTROL) & ~0x1);
	  break;

	case 2:		/* Record S/PDIF IN ENABLE DISABLE */
	  if (value)
	    {
	      WRITEL (SPDIFIN_CONTROL, READL (SPDIFIN_CONTROL) | 0x1);
	      devc->spdif_in = 1;
	    }
	  else
	    {
	      WRITEL (SPDIFIN_CONTROL, READL (SPDIFIN_CONTROL) & ~0x1);
	      devc->spdif_in = 0;
	    }
	  break;

	case 3:		/* S/PDIF Loopback Mode */
	  if (value)
	    WRITEL (SPDIFIN_CONTROL, READL (SPDIFIN_CONTROL) | (1 << 4));
	  else
	    WRITEL (SPDIFIN_CONTROL, READL (SPDIFIN_CONTROL) & ~(1 << 4));
	  break;

	case 4:		/* AC3 Output Mode */
	  if (value)
	    WRITEL (SPDIFOUT_CONTROL, READL (SPDIFOUT_CONTROL) | (1 << 1));
	  else
	    WRITEL (SPDIFOUT_CONTROL, READL (SPDIFOUT_CONTROL) & ~(1 << 1));
	  break;

#if 0
	case 5:		/* Copy Protect Mode */
	  {
	    int ac3_mode;

	    ac3_mode = READL (SPDIFOUT_CONTROL) & (1 << 1);

	    if (value)
	      {
		if (ac3_mode)
		  WRITEL (SPDIFOUT_STATUS,
			  READL (SPDIFOUT_STATUS) | (1 << 1));
		else
		  WRITEL (SPDIFOUT_STATUS, READL (SPDIFOUT_STATUS) & ~0x3E);
	      }
	    else
	      {
		if (ac3_mode)
		  WRITEL (SPDIFOUT_STATUS,
			  READL (SPDIFOUT_STATUS) | (3 << 1));
		else

		  WRITEL (SPDIFOUT_STATUS,
			  READL (SPDIFOUT_STATUS) | (1 << 2));
	      }
	  }
	  break;

	case 6:
	  left = value & 0xff;
	  right = (value >> 8) & 0xff;
	  if (left > 100)
	    left = 100;
	  if (right > 100)
	    right = 100;
	  value = left | (right << 8);
	  left = left * 0xFFFF / 100;
	  right = right * 0xFFFF / 100;
	  WRITEL (SPDIFOUTVOL, left << 16 | right);
	  devc->mixlevels[0] = value;
	  break;
	case 7:
	  left = value & 0xff;
	  right = (value >> 8) & 0xff;
	  if (left > 100)
	    left = 100;
	  if (right > 100)
	    right = 100;
	  value = left | (right << 8);
	  left = left * 0xFFFF / 100;
	  right = right * 0xFFFF / 100;
	  WRITEL (SPDIFLOOPVOL, left << 16 | right);
	  devc->mixlevels[1] = value;
	  break;
	case 8:
	  left = value & 0xff;
	  right = (value >> 8) & 0xff;
	  if (left > 100)
	    left = 100;
	  if (right > 100)
	    right = 100;
	  value = left | (right << 8);
	  left = left * 0xFFFF / 100;
	  right = right * 0xFFFF / 100;
	  WRITEL (AC3_OUTPUT, left << 16 | right);
	  devc->mixlevels[2] = value;
	  break;
#endif
	}
    }
  return value;
}

static int
ymf7xx_mix_init (int dev)
{
  int group, err;

  if ((group = mixer_ext_create_group (dev, 0, "SPDIF")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group, 1, ymf7xx_spdif_control,
				       MIXT_ONOFF, "ENABLE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 2, ymf7xx_spdif_control,
				       MIXT_ONOFF, "RECORD", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 3, ymf7xx_spdif_control,
				       MIXT_ONOFF, "LOOP", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

#if 0
  if ((err = mixer_ext_create_control (dev, group, 4, ymf7xx_spdif_control,
				       MIXT_ONOFF, "AC3", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  if ((err = mixer_ext_create_control (dev, group, 5, ymf7xx_spdif_control,
				       MIXT_ONOFF, "COPYPROT", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 6, ymf7xx_spdif_control,
				       MIXT_STEREOSLIDER, "OUTVOL", 100,
				       MIXF_MAINVOL | MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  if ((err = mixer_ext_create_control (dev, group, 7, ymf7xx_spdif_control,
				       MIXT_STEREOSLIDER, "LOOPVOL", 100,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  if ((err = mixer_ext_create_control (dev, group, 8, ymf7xx_spdif_control,
				       MIXT_STEREOSLIDER, "AC3VOL", 100,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
#endif

  return 0;
}

static int
init_ymf7xx (ymf7xx_devc * devc)
{

  int my_mixer, i;
  int first_dev = 0;

  WRITEL (NATIVE_DAC, 0x00000000);
  WRITEL (MODE, 0x00010000);
  WRITEL (MODE, 0x00000000);
  WRITEL (MAP_OF_REC, 0x00000000);
  WRITEL (MAP_OF_EFFECTS, 0x00000000);
  WRITEL (PLAY_CNTRL_BASE, 0x00000000);
  WRITEL (REC_CNTRL_BASE, 0x00000000);
  WRITEL (EFF_CNTRL_BASE, 0x00000000);
  WRITEL (CONTROL_SELECT, 1);
  WRITEL (GLOBAL_CONTROL, READL (GLOBAL_CONTROL) & ~0x0007);
  WRITEL (ZVOUTVOL, 0xFFFFFFFF);
  WRITEL (ZVLOOPVOL, 0xFFFFFFFF);
  WRITEL (SPDIFOUTVOL, 0xFFFFFFFF);
  WRITEL (SPDIFLOOPVOL, 0x3FFF3FFF);
  devc->spdif_in = 0;

  if (ac97_read (devc, 0x02) == 0xFFFF)
    {
      for (i = 0; i < 100; i++)
	if (ac97_read (devc, 0x02) != 0xFFFF)
	  {
	    break;
	  }
    }

  install_ucode (devc, 0x1000, dsp, dsp_size);
  switch (devc->deviceid)
    {
    case YAMAHA_YMF724F_ID:
    case YAMAHA_YMF740C_ID:
    case YAMAHA_YMF744_ID:
    case YAMAHA_YMF754_ID:
      install_ucode (devc, 0x4000, cntrl1E, cntrl1E_size);
      break;
    default:
      install_ucode (devc, 0x4000, cntrl, cntrl_size);
    }
  WRITEL (CONFIG, 1);

/* add an extra reset to init the mixers */
  ac97_write (devc, 0x02, 0x0000);
  ac97_write (devc, 0x18, 0x0808);

/* Now check to see if the DSP is started or not */
  {
    int fEnd;
    i = 10000;
    do
      {
	if (i-- <= 0)
	  {
	    cmn_err (CE_WARN, "CTR/DSP Init/ TimeOut\n");
	    return 0;
	  }
	fEnd = READL (CONTROL_SELECT) & 0x1;
      }
    while (fEnd == 0x1);
  }

  my_mixer =
    ac97_install (&devc->ac97devc, "Yamaha DS-XG", ac97_read, ac97_write,
		  devc, devc->osdev);
  if (my_mixer >= 0)
    {
      devc->mixer_dev = my_mixer;
      mixer_ext_set_init_fn (my_mixer, ymf7xx_mix_init, 20);
    }
  else
    return 0;

#ifdef OBSOLETED_STUFF
  if (devc->fm_base > 0)
    attach_fm (devc);
  if (devc->mpu_base > 0)
    attach_mpu (devc);
#endif

  if (devc->play_table_virt == 0)
    {
      /* Allocate the Play Table */
      oss_native_word phaddr;
      devc->dmabuf1 =
	CONTIG_MALLOC (devc->osdev, 0x1000, MEMLIMIT_32BITS, &phaddr, devc->dmabuf1_dma_handle);
      devc->play_table_virt = (oss_native_word) devc->dmabuf1;
      devc->play_table = phaddr;

      /* Allocate Effect Table */
      devc->dmabuf2 =
	CONTIG_MALLOC (devc->osdev, 1024, MEMLIMIT_32BITS, &phaddr, devc->dmabuf2_dma_handle);
      devc->effect_table_virt = (oss_native_word) devc->dmabuf2;
      devc->effect_table = phaddr;

      /* Allocate Effect Scratch Buffer */
      devc->eff_buf =
	CONTIG_MALLOC (devc->osdev, 2 * 8192, MEMLIMIT_32BITS, &phaddr, devc->eff_buf_dma_handle);
      devc->eff_buf_phys = phaddr;

      /* Allocate the Record Table */
      devc->dmabuf3 =
	CONTIG_MALLOC (devc->osdev, 1024, MEMLIMIT_32BITS, &phaddr, devc->dmabuf3_dma_handle);
      devc->rec_table_virt = (oss_native_word) devc->dmabuf3;
      devc->rec_table = phaddr;

      /* Setup Play Table */
      devc->tab = (unsigned int *) devc->play_table_virt;
      devc->slt = (oss_native_word) (devc->play_table_virt + 0x100);
      devc->slt_phys = (devc->play_table + 0x100);
      memset ((void *) devc->tab, 0, 0x1000);
      WRITEL (PLAY_CNTRL_BASE, (unsigned int) devc->play_table);
      devc->tab[0] = LSWAP (20);	/* setup 20 slots for 8 playback devices */
      /* Setup Effect Table and init Effect Slots */
      devc->effecttab = (unsigned int *) devc->effect_table_virt;
      memset ((void *) devc->effecttab, 0, 1024);
      WRITEL (EFF_CNTRL_BASE, (unsigned int) devc->effect_table);

      //SetupEffectSlot (dev);

      /* Setup Record Table */
      devc->rectab = (unsigned int *) devc->rec_table_virt;
      memset ((void *) devc->rectab, 0, 1024);
      WRITEL (REC_CNTRL_BASE, (unsigned int) devc->rec_table);
    }

  for (i = 0; i < MAX_PORTC; i++)
    {
      int adev;
      char tmp_name[100];
      ymf7xx_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE | ADEV_NOVIRTUAL;

      if (i == 0)
	{
	  strcpy (tmp_name, devc->chip_name);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  strcpy (tmp_name, devc->chip_name);
	  caps |= ADEV_NOINPUT;
	  if (i > 1)
	    caps |= ADEV_SHADOW;
	}


      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&ymf7xx_audio_driver,
					sizeof (audiodrv_t),
					caps,
					AFMT_U8 | AFMT_S16_LE | AFMT_AC3,
					devc, -1)) < 0)
	{
	  adev = -1;
	  return 0;
	}
      else
	{
	  if (i == 0)
	    first_dev = adev;
	  audio_engines[adev]->mixer_dev = my_mixer;
	  audio_engines[adev]->portc = portc;
	  audio_engines[adev]->rate_source = first_dev;
	  audio_engines[adev]->min_block = 64;
	  audio_engines[adev]->min_rate = 5000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->vmix_flags |= VMIX_MULTIFRAG;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  audio_engines[adev]->min_channels = 2;
	  audio_engines[adev]->max_channels = 4;
	  portc->open_mode = 0;
	  portc->audiodev = adev;
	  portc->devnum = i;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
    }
  return 1;
}

int
oss_ymf7xx_attach (oss_device_t * osdev)
{


  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned char pci_reset;
  unsigned short pci_legacy;
  ymf7xx_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered DS-XG attach routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (((vendor != YAMAHA_VENDOR_ID)) ||
      (device != YAMAHA_YMF740_ID &&
       device != YAMAHA_YMF744_ID &&
       device != YAMAHA_YMF754_ID &&
       device != YAMAHA_YMF740C_ID &&
       device != YAMAHA_YMF724_ID &&
       device != YAMAHA_YMF734_ID && device != YAMAHA_YMF724F_ID))
    return 0;

  oss_pci_byteswap (osdev, 1);

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &devc->base0addr);

  if (devc->base0addr == 0)
    {
      cmn_err (CE_WARN, "I/O address not assigned by BIOS.\n");
      return 0;
    }

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS (%d).\n", pci_irq_line);
      return 0;
    }

  DDB (cmn_err
       (CE_WARN, "rev %x I/O base %04x\n", pci_revision, devc->base0addr));

  devc->deviceid = device;
  switch (device)
    {
    case YAMAHA_YMF724_ID:
      devc->chip_name = "Yamaha YMF724";
      break;

    case YAMAHA_YMF724F_ID:
      devc->chip_name = "Yamaha YMF724F";
      break;

    case YAMAHA_YMF734_ID:
      devc->chip_name = "Yamaha YMF734";
      break;

    case YAMAHA_YMF740_ID:
      devc->chip_name = "Yamaha YMF740";
      break;

    case YAMAHA_YMF740C_ID:
      devc->chip_name = "Yamaha YMF740C";
      break;

    case YAMAHA_YMF744_ID:
      devc->chip_name = "Yamaha YMF744";
      break;

    case YAMAHA_YMF754_ID:
      devc->chip_name = "Yamaha YMF754";
      break;

    default:
      devc->chip_name = "Yamaha DS-XG";
    }

  devc->fm_base = yamaha_fm_ioaddr;
  devc->mpu_base = yamaha_mpu_ioaddr;
  devc->mpu_irq = yamaha_mpu_irq;

  /* reset the device */
  pci_read_config_byte (osdev, 0x48, &pci_reset);

  if (pci_reset & 0x03)
    {
      pci_write_config_byte (osdev, 0x48, pci_reset & 0xFC);
      pci_write_config_byte (osdev, 0x48, pci_reset | 0x03);
      pci_write_config_byte (osdev, 0x48, pci_reset & 0xFC);
    }

/*  Legacy I/O setup - setup MPU and FM io/irq values */
  devc->fm_attached = 0;
  devc->mpu_attached = 0;
  /*pcipci_legacy = 0x0020|0x00C0|0x0004; // 10 bit address decode and Joystick */
  pci_legacy = 0x4;
  if (devc->fm_base)
    pci_legacy |= 0x0002;	/* FM enable */

  if (devc->mpu_base)
    {
      pci_legacy |= 0x0008;	/* MPU I/O enable */

      switch (devc->mpu_irq)
	{
	case 5:
	  pci_legacy |= 0x0010;
	  break;
	case 7:
	  pci_legacy |= 0x0810;
	  break;
	case 9:
	  pci_legacy |= 0x1010;
	  break;
	case 10:
	  pci_legacy |= 0x1810;
	  break;
	case 11:
	  pci_legacy |= 0x2010;
	  break;
	default:
	  devc->mpu_irq = 0;
	}
    }

  pci_write_config_word (osdev, 0x40, pci_legacy);

  pci_legacy = 0x0000;

  switch (devc->fm_base)
    {
    case 0x388:
      pci_legacy |= 0x0000;
      break;
    case 0x398:
      pci_legacy |= 0x0001;
      break;
    case 0x3a0:
      pci_legacy |= 0x0002;
      break;
    case 0x3a8:
      pci_legacy |= 0x0003;
      break;
    default:
      devc->fm_base = 0;
    }

  switch (devc->mpu_base)
    {
    case 0x330:
      pci_legacy |= 0x0000;
      break;
    case 0x300:
      pci_legacy |= 0x0010;
      break;
    case 0x332:
      pci_legacy |= 0x0020;
      break;
    case 0x334:
      pci_legacy |= 0x0020;
      break;
    default:
      devc->mpu_base = 0;
    }
  pci_write_config_word (osdev, 0x42, pci_legacy);

  /* activate the device */
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  /* Map the shared memory area */
  devc->base0virt =
    (unsigned int *) MAP_PCI_MEM (devc->osdev, 0, devc->base0addr, 32 * 1024);
  devc->dwRegister = (unsigned int *) devc->base0virt;
  devc->wRegister = (unsigned short *) devc->base0virt;
  devc->bRegister = (unsigned char *) devc->base0virt;


  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, ymf7xxintr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d\n", pci_irq_line);
      return 0;
    }

  return init_ymf7xx (devc);	/* Detected */
}


int
oss_ymf7xx_detach (oss_device_t * osdev)
{
  ymf7xx_devc *devc = (ymf7xx_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  WRITEL (CONFIG, 0);
  WRITEL (MODE, 0);

  if (devc->dmabuf1)
    {
      CONTIG_FREE (devc->osdev, devc->dmabuf1, 0x1000, devc->dmabuf1_dma_handle);
      devc->dmabuf1 = 0;
    }

  if (devc->dmabuf2)
    {
      CONTIG_FREE (devc->osdev, devc->dmabuf2, 1024, devc->dmabuf2_dma_handle);
      devc->dmabuf2 = 0;
    }

  if (devc->eff_buf)
    {
      CONTIG_FREE (devc->osdev, devc->eff_buf, 16 * 1024, devc->eff_buf_dma_handle);
      devc->eff_buf = 0;
    }

  if (devc->dmabuf3)
    {
      CONTIG_FREE (devc->osdev, devc->dmabuf3, 1024, devc->dmabuf3_dma_handle);
      devc->dmabuf3 = 0;
    }

  devc->play_table_virt = 0;
  devc->effect_table_virt = 0;
  devc->rec_table_virt = 0;

#ifdef OBSOLETED_STUFF
  if (devc->mpu_attached)
    {
      unload_mpu (devc);
      devc->mpu_attached = 0;
    }
#endif

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  if (devc->base0addr != 0)
    {
      UNMAP_PCI_MEM (devc->osdev, 0, devc->base0addr, devc->base0virt,
		     32 * 1024);
      devc->base0addr = 0;
    }
  oss_unregister_device (devc->osdev);
  return 1;
}
