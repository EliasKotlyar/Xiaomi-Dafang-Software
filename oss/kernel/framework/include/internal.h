/*
 * Purpose: Definitions for internal use by OSS
 *
 * Definitions for private use by the ossctl program. Everything defined
 * in this file is likely to change without notice between OSS versions.
 *
 * Note that thse ioctl calls are not for public use. They cannot be used by 
 * applications that are not part of the official OSS distribution.
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

#ifndef _INTERNAL_H_
#define _INTERNAL_H_

/*
 * This file defines ioctl codes 200 to 255 in the 'X' block. These codes may be
 * reused for internal purposes by other OSSv4 implementations that don't use
 * this file. Implementations based on the 'stock' OSS code can add their 
 * private use ioctl codes in this file. However in such case it's recommended
 * to notify the OSS development community (4Front Technologies) so that the
 * code can be marked as reserved in the official sources.
 *
 */

#define OSS_MAXERR 200
typedef struct
{
  int nerrors;
  int errors[OSS_MAXERR];
  int error_parms[OSS_MAXERR];
}
oss_error_info;
#define BOOTERR_BAD_PCIIRQ				  1
#define BOOTERR_AC97CODEC				  2
#define BOOTERR_IRQSTORM				  3
#define BOOTERR_BIGMEM					  4

extern oss_error_info oss_booterrors;

#if 0
typedef struct
{
  int mode;			/* OPEN_READ and/or OPEN_WRITE */
  oss_devlist_t devlist;
}
oss_reroute_t;
#endif

typedef struct
{
/*
 * Private structure for renumbering legacy dsp, mixer and MIDI devices
 */
  int n;
  short map[HARD_MAX_AUDIO_DEVFILES];
} oss_renumber_t;
/*
 * Some internal use only ioctl calls ('X', 200-255)
 */
#if 0
#define OSSCTL_GET_REROUTE	__SIOWR('X', 200, oss_reroute_t)
#define OSSCTL_SET_REROUTE	__SIOW ('X', 200, oss_reroute_t)
#endif

#ifdef APPLIST_SUPPORT
/*
 * Application redirection list for audio.c.
 */
typedef struct
{
  char name[32 + 1];		/* Command name (such as xmms) */
  int mode;			/* OPEN_READ|OPEN_WRITE */
  int dev;			/* "Target" audio device number */
  int open_flags;		/* Open flags to be passed to oss_audio_open_engine */
} app_routing_t;

#define APPLIST_SIZE	64
extern app_routing_t oss_applist[APPLIST_SIZE];
extern int oss_applist_size;

#define OSSCTL_RESET_APPLIST	__SIO  ('X', 201)
#define OSSCTL_ADD_APPLIST	__SIOW ('X', 201, app_routing_t)
#endif

/*
 * Legacy device file numbering calls
 */
#define OSSCTL_RENUM_AUDIODEVS	__SIOW ('X', 202, oss_renumber_t)
#define OSSCTL_RENUM_MIXERDEVS	__SIOW ('X', 203, oss_renumber_t)
#define OSSCTL_RENUM_MIDIDEVS	__SIOW ('X', 204, oss_renumber_t)

/*
 * vmixctl related ioctl calls
 */

typedef struct
{
	int masterdev;
	int inputdev;

	int attach_flags;

/*
 * 						0x000000xx reserved
 * 						for #clients to prealloc
 */
#define VMIX_INSTALL_NOPREALLOC			0x00000100
#define VMIX_INSTALL_NOINPUT			0x00000200
#define VMIX_INSTALL_VISIBLE			0x00000400
#define VMIX_INSTALL_MANUAL			0x00000800 /* By vmxctl */
} vmixctl_attach_t;

typedef struct
{
	int masterdev;
	int rate;
} vmixctl_rate_t;

typedef int oss_chninfo[128];
typedef struct
{
	int masterdev;
	oss_chninfo map;
} vmixctl_map_t;

#define VMIXCTL_ATTACH		__SIOW ('X', 220, vmixctl_attach_t)
#define VMIXCTL_DETACH		__SIOW ('X', 221, vmixctl_attach_t)
#define VMIXCTL_RATE		__SIOW ('X', 222, vmixctl_rate_t)
#define VMIXCTL_REMAP		__SIOW ('X', 223, vmixctl_map_t)

/*
 * FreeBSD compatibility ioctl
 */
#define FREEBSD_GETBLKSIZE	__SIOR ('P', 4, int)

#ifdef DO_TIMINGS
#define DFLAG_ALL		0x00000001
#define DFLAG_PROFILE		0x00000002
/*
 * Time counters 
 */
#define DF_IDDLE	0
#define DF_WRITE	1
#define DF_READ		2
#define DF_INTERRUPT	3
#define DF_SLEEPWRITE	4
#define DF_SLEEPREAD	5
#define DF_SRCWRITE	6
#define DF_SRCREAD	7
#define DF_NRBINS	8
#endif

#endif
