#ifndef _OSS_EXPORTS_H
#define _OSS_EXPORTS_H
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

/*
 * Driver (bus) types
 *
 * Note that OSSDDK drivers can only use DRV_PCI or DRV_VIRTUAL. The other
 * bus types will not work properly because support for them is missing
 * from the DDK layer.
 */

#define DRV_UNKNOWN	0
#define DRV_PCI		1
#define DRV_USB		2
#define DRV_VIRTUAL	3
#define DRV_VMIX	4	/* Like DRV_VIRTUAL. Used by the vmix module. */
#define DRV_STREAMS	5
#define DRV_ISA		6
#define DRV_FIREWIRE	7
#define DRV_CLONE	8	/* Clone of some other device */

/*
 * Device class numbers. Unlike with earlier OSS versions (up to v4.0)
 * the minor number doesn't have any special meaning. Minor numbers
 * will be allocated in the order the device files are initialized. Even
 * the major number doesn't necessaily be the same for all OSS devices.
 * Minor numbers will be just indexes to a table (in os.c) that contains
 * the device class code, instance number, driver call table and things like that.
 * Ideally OSS will use devfs to expose the available devices.
 */

extern int oss_max_audio_devfiles;
extern int oss_max_audio_engines;

#define MAX_SYNTH_DEV	6	// TODO: Remove
#define MAX_AUDIO_DEVFILES 	oss_max_audio_devfiles
#define MAX_AUDIO_ENGINES 	oss_max_audio_engines
#define SYNC_DEVICE_MASK	0xffff

#include "oss_limits.h"
/*
 * Device file class codes. Note that these numbers don't have any kind of relationship with minor numbers.
 */

#define OSS_DEV_STATUS		0	/* /dev/sndstat */
#define OSS_DEV_VDSP		1	/* /dev/dsp (multiplexer device) */
#define OSS_DEV_VAUDIO		2	/* /dev/audio (multiplexer device) */
#define OSS_DEV_AWFM		3	/* Reserved for historic purposes */
#define OSS_DEV_OSSD		4	/* /dev/ossd - ossd process support */
#define OSS_DEV_AUDIOCTL	5	/* Audioctl device */
#define OSS_DEV_MIXER		6	/* /dev/mixer0 */
#define OSS_DEV_SEQ		7	/* /dev/sequencer */
#define OSS_DEV_MUSIC		8	/* /dev/music AKA /dev/sequencer2 */
#define OSS_DEV_VMIDI		9	/* /dev/midi (multiplexer device) */
#define OSS_DEV_MIDI		10	/* /dev/midi## */
#define OSS_DEV_DEVAUDIO	11	/* /dev/audio# */
#define OSS_DEV_DSP		12	/* /dev/dsp# */
#define OSS_DEV_MISC		13	/* Special purpose device files */
#define OSS_DEV_DSP_ENGINE	14	/* Hidden DSP engines */

extern int oss_max_cdevs;
#define OSS_MAX_CDEVS	oss_max_cdevs

/*
 * Misc definitions 
 */

typedef struct _oss_device_t oss_device_t;

#if defined(linux) && defined(OSS_MAINLINE_BUILD)
#include <stdint.h>
#endif

/*
 * AC97
 */
typedef int (*ac97_readfunc_t) (void *, int);
typedef int (*ac97_writefunc_t) (void *, int, int);
typedef struct _ac97_handle_t ac97_handle_t;

/*
 * Mixer
 */
typedef struct _mixer_driver_t mixer_driver_t;
typedef struct _mixer_operations_t mixer_operations_t;
typedef int (*mixer_create_controls_t) (int dev);
typedef int (*mixer_ext_fn) (int dev, int ctrl, unsigned int cmd, int value);

extern void oss_audio_delayed_attach (void);
#endif
