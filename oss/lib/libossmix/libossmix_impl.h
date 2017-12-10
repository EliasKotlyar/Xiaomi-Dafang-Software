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

extern int mixlib_trace;

typedef struct
{
  int (*connect) (const char *hostname, int port);
  int (*get_fd) (ossmix_select_poll_t * cb);
  void (*disconnect) (void);
  void (*enable_events) (void);
  int (*get_nmixers) (void);
  int (*get_mixerinfo) (int mixernum, oss_mixerinfo * mi);
  int (*open_mixer) (int mixernum);
  void (*close_mixer) (int mixernum);
  int (*get_nrext) (int mixernum);
  int (*get_nodeinfo) (int mixernum, int node, oss_mixext * ext);
  int (*get_enuminfo) (int mixernum, int node, oss_mixer_enuminfo * ei);
  int (*get_description) (int mixernum, int node, oss_mixer_enuminfo * desc);
  int (*get_value) (int mixernum, int ctl, int timestamp);
  void (*set_value) (int mixernum, int ctl, int timestamp, int value);
  void (*timertick)(void);
} ossmix_driver_t;

extern ossmix_driver_t ossmix_local_driver, ossmix_tcp_driver;
extern void _client_event (int cmd, int p1, int p2, int p3, int p4, int p5);
extern int _ossmix_refresh_mixer(int mixernum, int prev_nmixers);
