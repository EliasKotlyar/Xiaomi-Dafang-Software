#ifndef __APPLY_CHANGES_H__
#define __APPLY_CHANGES_H__

#include "sysfs-pin-configs.h"
#include <glib.h>

gboolean apply_changes_reconfig(pin_configs_t* pins, int entries, int card, int device, 
    const char* model, const char* hints, GError** err);

gboolean apply_changes_boot(pin_configs_t* pins, int entries, int card, int device, 
    const char* model, const char* hints, GError** err);
gboolean reset_changes_boot();

#endif

