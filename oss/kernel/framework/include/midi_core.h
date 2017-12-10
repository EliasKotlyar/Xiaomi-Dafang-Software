#ifndef MIDI_CORE_H
#define MIDI_CORE_H
/*
 * Purpose: Structure and function definitions for OSS MIDI core
 *
 * IMPORTANT NOTICE!
 *
 * This file contains internal structures used by Open Sound Systems.
 * They will change without any notice between OSS versions. Care must be taken
 * to make sure any software using this header gets properly re-compiled before
 * use.
 *
 * 4Front Technologies (or anybody else) takes no responsibility of damages
 * caused by use of this file.
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

#define OSS_MIDI_DRIVER_VERSION			1

struct midi_input_info
{				/* MIDI input scanner variables */
#define MI_MAX	32
  int m_busy;
  unsigned char m_buf[MI_MAX];
  unsigned char m_prev_status;	/* For running status */
  int m_ptr;
#define MST_INIT			0
#define MST_DATA			1
#define MST_SYSEX			2
  int m_state;
  int m_left;
  int m_f1_flag;		/* MTC Quarter frame message flag */
};


typedef int (*oss_midi_inputbyte_t) (int dev, unsigned char data);
typedef int (*oss_midi_inputbuf_t) (int dev, unsigned char *data, int len);
typedef int (*oss_midi_outputintr_t) (int dev);

typedef struct _midi_driver
{
  int (*open) (int dev, int mode, oss_midi_inputbyte_t inputbyte,
	       oss_midi_inputbuf_t inputbuf,
	       oss_midi_outputintr_t outputintr);
  void (*close) (int dev, int mode);
  int (*ioctl) (int dev, unsigned int cmd, ioctl_arg arg);
  int (*outputc) (int dev, unsigned char data);
  int (*bulk_write) (int dev, unsigned char *buf, int len);
  int max_bulk_bytes;		/* Maximum len parameter for the bulk_write() entry point */
  void (*flush_output) (int dev);
  int (*wait_output) (int dev);
  void (*timer_setup) (int dev);
  void (*init_device) (int dev);
} midi_driver_t;

/*
 * MIDI ports (physical and virtual)
 */

typedef struct midi_queue_t midi_queue_t;

typedef struct _midi_operations
{
  int dev;			/* MIDI device number */
  char name[64];
  midi_driver_t *d;
  int caps;
  int open_mode;		/* OPEN_READ | OPEN_WRITE */

  int working_mode;		/* See SNDCTL_MIDI_SETMODE */
  int timebase;
  int tempo;
  struct midi_input_info in_info;
  struct coproc_operations *coproc;
  void *devc;
  void *os_id;			/* The device ID (dip) given by the system. */
  int enabled;
  int unloaded;
  int is_killed;
  oss_device_t *osdev;
  oss_device_t *master_osdev;	/* osdev struct of the master device (for virtual drivers) */
  int card_number;
  int port_number;
  oss_devnode_t devnode;
  int real_dev;
  char handle[32];
  unsigned long flags;
#define MFLAG_NOSEQUENCER		0x00000001	/* Device not to be opened by the sequencer driver */
#define MFLAG_VIRTUAL			0x00000002
#define MFLAG_SELFTIMING		0x00000004	/* Generates MTC timing itself */
#define MFLAG_CLIENT			0x00000008	/* Client side virtual device */
#define MFLAG_SERVER			0x00000010	/* Client side virtual device */
#define MFLAG_INTERNAL			0x00000020	/* Internal device */
#define MFLAG_EXTERNAL			0x00000040	/* External device */

#define MFLAG_INPUT			0x00000080
#define MFLAG_OUTPUT			0x00000100

#define MFLAG_OUTINTR			0x00000200	/* Supports output interrupts - no polling needed */
#define MFLAG_MTC			0x00000400	/* Device is MTC/SMPTE capable */
#define MFLAG_QUIET			0x00000800	/* No automatic messages */

  void (*input_callback) (int dev, unsigned char midich);
  void (*event_input) (int dev, unsigned char *data, int len);

#ifndef CONFIGURE_C
  oss_mutex_t mutex;
  oss_wait_queue_t *in_wq, *out_wq;
  timeout_id_t out_timeout;
#endif
  pid_t pid;
  int magic;
  char cmd[16];

  int prech_timeout;		/* Wait time for the first input byte */

/*
 * MTC generator
 */

  int mtc_timebase;
  int mtc_phase;		/* Which part should be sent next */
  long long mtc_t0;
  int mtc_prev_t, mtc_codetype, mtc_current;
  timeout_id_t mtc_timeout_id;

/*
 * Event queues
 */

  struct midi_queue_t *in_queue, *out_queue;

/*
 * Timer
 */
  int timer_driver;
  int timer_dev;
  int latency;			/* In usecs, -1=unknown */
  int is_timing_master;
} mididev_t, *mididev_p;

/*
 * MIDI clients (/dev/midi device files)
 */

typedef struct
{
  int num;			/* Own index in the oss_midi_clients[] table */
  int open_mode;		/* OPEN_READ and/or OPEN_WRITE */
  mididev_p mididev;		/* Pointer to the MIDI device (or NULL) */
} oss_midi_client_t;

#define MAX_MIDI_CLIENTS		(MAX_MIDI_DEV + 20)
extern int num_midi_clients;
extern oss_midi_client_t *oss_midi_clients[MAX_MIDI_CLIENTS];

extern mididev_t **midi_devs;
extern int num_mididevs;

struct oss_timer_driver;
typedef void (*oss_midi_wait_callback_t) (void *arg);

typedef struct
{
  oss_device_t *osdev;
  oss_mutex_t mutex;
  int driver_dev;
  int timer_dev;
  char *name;			/* Pointer to driver->name */
  int refcount;			/* Number of /dev/midi* devices currently using this timer */
  unsigned long password;	/* Random number given when an instance is created */

  struct oss_timer_driver *d;

  oss_midi_time_t next_tick, prev_tick;
  oss_midi_time_t bookmark_tick;
  oss_uint64_t bookmark_time;
  int tempo, pending_tempo;
  int timebase;
  int run_state;		/* 0=stopped, 1=run */
  oss_uint64_t prev_time, next_time;

  void *devc;
  void *timerc;
  oss_midi_wait_callback_t callback;

  /*
   * Bitmap of MIDI devices linked with this one
   */
  unsigned int midimap[(MAX_MIDI_DEV + 31) / 32];
  oss_uint64_t midi_times[MAX_MIDI_DEV];	/* Next event times in usecs */
  oss_midi_time_t midi_ticks[MAX_MIDI_DEV];
} tdev_t;

typedef struct oss_timer_driver
{
/*
 * Driver methods
 */
  int (*create_instance) (int timer_dev, int driver_dev);
  void (*free_instance) (int timer_dev, int driver_dev);
  int (*attach_client) (int timer_dev, int mididev);
  void (*detach_client) (int timer_dev, int mididev);
  int (*ioctl) (int timer_dev, unsigned int cmd, ioctl_arg arg);
  int (*wait) (int timer_dev, unsigned long long time,
	       oss_midi_wait_callback_t callback, void *arg);
  unsigned long long (*get_current_time) (int timer_dev);
  int (*set_source) (int timer_dev, int source);
  int (*set_tempo) (int timer_dev, int tempo);
  int (*set_timebase) (int timer_dev, int timebase);
  int (*start) (int timer_dev, oss_uint64_t tick);
  int (*stop) (int timer_dev);
  int (*cont) (int timer_dev);

/*
 * Dynamic content
 */
  char name[64];
  int driver_dev;
  oss_device_t *osdev;
  void *devc;
  oss_mutex_t mutex;
  int max_instances;
  int num_instances;
  unsigned int flags;
  int priority;
  int resolution;		/* In usecs */
} oss_timer_driver_t;

#define OSS_TIMER_DRIVER_VERSION	1

extern int oss_install_timer (int version, char *name, oss_timer_driver_t * d, int driver_size, unsigned int flags, int max_instances, int resolution,	/* In usecs */
			      void *devc, oss_device_t * osdev);
extern int oss_timer_create_instance (int driver_dev, mididev_t * mididev);
extern int oss_timer_attach_client (int timer_dev, mididev_t * mididev);
extern void oss_timer_detach_client (int timer_dev, mididev_t * mididev);
extern int oss_timer_set_tempo (int timer_dev, int tempo);
extern int oss_timer_set_timebase (int timer_dev, int timebase);
extern int oss_timer_wait_time (int timer_dev, int midi_dev, int latency,
				oss_midi_time_t time,
				oss_midi_wait_callback_t callback,
				unsigned short options);
extern oss_midi_time_t oss_timer_get_current_time (int timer_dev, int latency,
						   int use_abs);
extern int oss_timer_get_timeout (int timer_dev, int midi_dev);
extern void oss_timer_start (int timer_dev);
extern void oss_timer_stop (int timer_dev);
extern void oss_timer_continue (int timer_dev);
extern int oss_timer_is_running (int timer_dev);

extern oss_timer_driver_t *oss_timer_drivers[MAX_TIMER_DEV];
extern int oss_num_timer_drivers;

extern tdev_t *oss_timer_devs[MAX_TIMER_DEV];
extern int oss_num_timers;

#define MIDI_MARK_OPENED(md, mode) \
{ \
      char *cmd; \
      md->open_mode=mode; \
      if ((cmd = GET_PROCESS_NAME ()) != NULL) \
	{ \
	  strncpy (md->cmd, cmd, sizeof (md->cmd)); \
	  md->cmd [sizeof (md->cmd) - 1] = '\0'; \
	} \
      md->pid = GET_PROCESS_PID (); \
}

#define MIDI_MARK_CLOSED(md) \
{ \
      memset (md->cmd, 0, sizeof (md->cmd)); \
      md->pid = -1; \
      md->open_mode=0; \
}

extern void oss_midi_init (oss_device_t * osdev);
extern void oss_midi_uninit (void);
extern void oss_midi_copy_timer (int dev, int source_dev);
extern void midi_mapper_init (oss_device_t * osdev);
extern void midi_mapper_uninit (void);
extern int midi_mapper_autobind (int client_dev, int mode);
extern int oss_midi_ioctl (int dev, struct fileinfo *file,
			   unsigned int cmd, ioctl_arg arg);

/* Interrupt callback functions */
extern int oss_midi_input_byte (int dev, unsigned char data);
extern int oss_midi_input_buf (int dev, unsigned char *data, int len);
extern int oss_midi_output_intr (int dev);

extern int
oss_install_mididev (int version,
		     char *id, char *name,
		     midi_driver_t * d, int driver_size,
		     unsigned int flags, void *devc, oss_device_t * osdev);

extern void install_vmidi (oss_device_t * osdev);

/*
 * Prototypes for midi_queue.c
 */
extern midi_queue_t *midi_queue_alloc (oss_device_t * osdev,
				       const char *name);
extern void midi_queue_free (midi_queue_t * queue);

extern int midi_queue_alloc_record (midi_queue_t * queue,
				    unsigned char **data, int len,
				    midi_packet_header_t * hdr);
extern int midi_queue_put (midi_queue_t * queue, unsigned char *data, int len,
			   midi_packet_header_t * hdr);
extern int midi_queue_get (midi_queue_t * queue, unsigned char **data,
			   int max_len, midi_packet_header_t ** hdr);
extern int midi_queue_find_buffer (midi_queue_t * queue, unsigned char **data,
				   midi_packet_header_t ** hdr);
extern void midi_queue_remove_chars (midi_queue_t * queue, int len);
extern void midi_queue_removeall (midi_queue_t * queue);
extern void oss_midi_set_defaults (mididev_t * mididev);
extern int midi_queue_isempty (midi_queue_t * queue);
extern int midi_queue_spaceleft (midi_queue_t * queue);
extern void midi_queue_debugging (midi_queue_t * queue);
extern void midi_queue_trace (midi_queue_t * queue);
extern int oss_init_timers (oss_device_t * osdev);
extern int oss_uninit_timers (void);
extern void attach_oss_default_timer (oss_device_t * osdev);
extern void detach_oss_default_timer (void);

#endif
