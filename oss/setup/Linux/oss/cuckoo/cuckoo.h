/*
 * This software module makes it possible to use Open Ssund System for Linux
 * (the _professional_ version) as a low level driver source for ALSA.
 *
 * Copyright (C) 2004-2006 Hannu Savolainen (hannu@voimakentta.net).
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

#define _KERNEL

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

#define _KERNEL
#include "../include/sys/soundcard.h"

#include <linux/version.h>

#define _LOOSE_KERNEL_NAMES

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
#include <linux/config.h>
#else
#include <linux/autoconf.h>
#endif

#if !defined(__SMP__) && defined(CONFIG_SMP)
#define __SMP__
#endif
#include <linux/module.h>

#include <stdarg.h>

extern int oss_get_cardinfo (int cardnum, oss_card_info * ci);	/* from oss_config.h */

#include <linux/param.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/apm_bios.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/poll.h>

#include <asm/system.h>
#include <asm/dma.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ioport.h>
//#include <asm/mach-default/irq_vectors.h>
#include <linux/interrupt.h>
#include <linux/pm.h>

struct _oss_mutex_t
{
  /* Caution! This definition must match Linux/osscore.c */
  spinlock_t lock;
};

#define audio_devs dummy_audio_devs

#include "../include/internals/oss_exports.h"
#include "../build/wrap.h"
#include "../include/internals/ossddk.h"

typedef struct oss_wait_queue oss_wait_queue_t;	/* This must match oss_config.h */

#include "../include/internals/ossddk.h"

//#include <sound/driver.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>

#include "../build/osscore_symbols.inc"

#define SNDRV_GET_ID
#include <sound/initval.h>

typedef caddr_t ioctl_arg;
typedef char snd_rw_buf;

typedef int sound_os_info;

#define WR_BUF_CONST	const

#include "../include/internals/audio_core.h"
#include "../include/internals/mixer_core.h"

typedef struct _snd_cuckoo cuckoo_t, chip_t;

typedef struct
{
  adev_p adev;
} cuckoo_pcm_t;

#define MAX_OSSPCM 24		// Max # of PCM devices/card instance

#if 1
// Older ALSA versions used to define these...
typedef struct snd_card snd_card_t;
typedef struct snd_pcm snd_pcm_t;
typedef struct snd_rawmidi snd_rawmidi_t;
typedef struct snd_rawmidi_substream snd_rawmidi_substream_t;
typedef struct snd_rawmidi_ops snd_rawmidi_ops_t;
typedef struct snd_kcontrol snd_kcontrol_t;
typedef struct snd_kcontrol_new snd_kcontrol_new_t;
typedef struct snd_ctl_elem_info snd_ctl_elem_info_t;
typedef struct snd_ctl_elem_value snd_ctl_elem_value_t;
typedef struct snd_pcm_substream snd_pcm_substream_t;
typedef struct snd_pcm_hardware snd_pcm_hardware_t;
typedef struct snd_pcm_runtime snd_pcm_runtime_t;
typedef struct snd_pcm_hw_params snd_pcm_hw_params_t;
typedef struct snd_pcm_ops snd_pcm_ops_t;
typedef struct snd_device snd_device_t;
typedef struct snd_device_ops snd_device_ops_t;
#endif

struct _snd_cuckoo
{
  snd_card_t *card;
  snd_pcm_t *pcm[MAX_OSSPCM];
  adev_p play_adev[MAX_OSSPCM], capture_adev[MAX_OSSPCM];
  int osscard;
  int nplay, ncapture, npcm;
};

#define cuckoo_t_magic 0xaabbccdd
#define chip__tmagic cuckoo_t_magic

//#define OPEN_READ	PCM_ENABLE_INPUT
//#define OPEN_WRITE	PCM_ENABLE_OUTPUT

extern int install_mixer_instances (cuckoo_t * chip, int cardno);
extern int install_midiport_instances (cuckoo_t * chip, int cardno);
extern int install_pcm_instances (cuckoo_t * chip, int cardno);

// Disable locking for now
#define udi_spin_lock_irqsave(a, b) *(b)=0
#define udi_spin_unlock_irqrestore(a, b)

#define strlcpy(a, b) {strncpy(a, b, sizeof(a)-1);a[sizeof(a)-1]=0;}
