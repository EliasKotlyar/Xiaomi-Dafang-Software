#ifndef SPDIF_H
#define SPDIF_H
/*
 * Purpose: Definitions for S/PDIF (IEC958) control bit and mixer extension manager
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
typedef struct oss_spdif_driver spdif_driver_t;

#define set_cbit(array, byte, bit, v) {array[byte] &= ~(1 << bit);array[byte] |= ((v&1)<<bit);}
#define get_cbit(array, byte, bit) ((array[byte] >> bit)&1)


typedef struct
{
  int is_ok;
  oss_mutex_t mutex;
  oss_device_t *osdev;

  void *host_devc;
  void *host_portc;
  int flags;
  unsigned int caps;
#define SPDF_IN					0x00000001
#define SPDF_OUT				0x00000002
#define SPDF_ENABLE				0x00000004
#define SPDF_RATECTL				0x00000008
  spdif_driver_t *d;
  oss_digital_control *ctl, hot_ctl, cold_ctl;
  int open_mode;
  int mixer_dev;

  int group;
  int mixer_blocked;
} spdif_devc;


/*
 * reprogram 'mask' bits
 */

#define REPGM_ALL		0xffffffff
#define REPGM_NOTHING		0x00000000
#define REPGM_RATE		0x00000001
#define REPGM_VBIT		0x00000002
#define REPGM_DENABLE		0x00000004
#define REPGM_AENABLE		0x00000008
#define REPGM_CBIT0		0x00000010
#define REPGM_CBIT1		0x00000020
#define REPGM_CBIT2		0x00000040
#define REPGM_CBIT3		0x00000080
#define REPGM_CBIT4_23		0x00000100
#define REPGM_CBITALL		0x000001f0
#define REPGM_UBITALL		0x00000200
#define REPGM_OUTSEL		0x00000400

#define SPDIF_NOIOCTL		0x12344321	/* Bits of magic */

struct oss_spdif_driver
{
  int (*reprogram_device) (void *_devc, void *_portc,
			   oss_digital_control * ctl, unsigned int mask);
  int (*get_status) (void *_devc, void *_portc, oss_digital_control * ctl,
		     int rqbits);
  int (*exec_request) (void *_devc, void *_portc, int rq, int param);
};

extern int oss_spdif_install (spdif_devc * devc, oss_device_t * osdev,
			      spdif_driver_t * d, int driver_size,
			      void *host_devc, void *host_portc,
			      int mixer_dev, int flags, unsigned int caps);
extern void oss_spdif_uninstall (spdif_devc * devc);
extern int oss_spdif_open (spdif_devc * devc, int open_mode);
extern void oss_spdif_close (spdif_devc * devc, int open_mode);
extern void oss_spdif_setrate (spdif_devc * devc, int rate);
extern int oss_spdif_mix_init (spdif_devc * devc);
extern int oss_spdif_ioctl (spdif_devc * devc, int open_mode, unsigned int cmd,
			    ioctl_arg arg);

/*
 * oss_digital_control->filler usage
 */
#define pro_mode filler[0]
#define rate_bits filler[1]
#define emphasis_type filler[2]

#endif
