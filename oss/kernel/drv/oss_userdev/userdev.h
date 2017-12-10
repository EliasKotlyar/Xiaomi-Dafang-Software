/*
 * Purpose: Definition file for the oss_userdev driver
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
#define MAX_RATE 	192000
#define MAX_CHANNELS	64
#define SUPPORTED_FORMATS	(AFMT_S16_NE|AFMT_S32_NE)

typedef struct _userdev_devc_t userdev_devc_t;
typedef struct _userdev_portc_t userdev_portc_t;

struct _userdev_portc_t
{
  userdev_devc_t *devc;
  userdev_portc_t *peer;
  int audio_dev;
  int open_mode;
  int port_type;
#define PT_CLIENT	1
#define PT_SERVER	2

  /* State variables */
  int input_triggered, output_triggered;
};

struct _userdev_devc_t
{
  oss_device_t *osdev;
  int active;
  oss_mutex_t mutex;

  int open_count;	/* 0=not in use, 2=both client and server in use */

  int create_flags;	/* Flags from ioctl(USERDEV_CREATE_INSTANCE) */

  unsigned int poll_ticks; /* Number of clock tickes (OSS_HZ) between polls. */

  unsigned int match_method;
  unsigned int match_key;
  int mixer_dev;

  userdev_devc_t *next_instance;

  int rate;
  int channels;
  unsigned int fmt, fmt_bytes;
  timeout_id_t timeout_id;

  userdev_portc_t client_portc;
  userdev_portc_t server_portc;

  /*
   * Mixer related fields
   */
  int modify_counter;
  int mixer_values[USERDEV_MAX_MIXERS];
};

extern oss_device_t *userdev_osdev;
extern oss_mutex_t userdev_global_mutex;
extern userdev_devc_t *userdev_active_device_list;
extern userdev_devc_t *userdev_free_device_list;

extern int userdev_create_device_pair(void);
extern void userdev_delete_device_pair(userdev_devc_t *devc);
extern int usrdev_find_free_device_pair(void);
extern void userdev_reinit_instance(userdev_devc_t *devc);

extern char *userdev_client_devnode;
extern char *userdev_server_devnode;
