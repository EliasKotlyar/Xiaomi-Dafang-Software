/*
 * Purpose: Various global services for OSS.
 *
 * This source file contains some initialization and cleanup code
 * that is called by the OS modules for all operating systems.
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

#include <oss_config.h>
#include "midi_core.h"

#ifdef DO_TIMINGS
extern oss_mutex_t oss_timing_mutex;
#endif

oss_history_t oss_history[OSS_HISTORY_SIZE] = { {0} };
int oss_history_p = 0;

int oss_num_cdevs = 0;
oss_cdev_t **oss_cdevs = NULL;
int oss_max_cdevs = 0;

static int drivers_loaded = 0;

void
oss_unload_drivers (void)
{

  if (!drivers_loaded)
    return;
  drivers_loaded = 0;

#ifdef CONFIG_OSS_VMIX
  vmix_core_uninit ();
#endif

  oss_audio_uninit ();

  /* oss_midi_uninit(); *//* TODO: This causes crashes */
#ifdef DO_TIMINGS
  MUTEX_CLEANUP (oss_timing_mutex);
#endif

  /*
   * Release all global memory
   */
  oss_memblk_unalloc(&oss_global_memblk);
}

/*ARGSUSED*/
void
create_new_card (char *shortname, char *longname)
{
}

void
oss_common_init (oss_device_t * osdev)
{
  if (drivers_loaded)
    return;
#ifdef DO_TIMINGS
  MUTEX_INIT (osdev, oss_timing_mutex, MH_TOP);
#endif
  drivers_loaded = 1;
  oss_audio_init (osdev);
  install_sndstat (osdev);
  install_vdsp (osdev);
  oss_midi_init (osdev);
  install_vmidi (osdev);
  install_dev_mixer (osdev);
#ifdef CONFIG_OSS_VMIX
  vmix_core_init (osdev);
#endif
}
