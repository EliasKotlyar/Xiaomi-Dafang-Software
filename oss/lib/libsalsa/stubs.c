#include <stdio.h>
extern int alib_verbose;

#define STUB(x) \
int x(void) \
{ \
  if (alib_verbose>0)printf("libsalsa: Stub for " #x " called.\n"); \
  return -53; \
}

#define STUB_NULL(x) \
void* x(void) \
{ \
  if (alib_verbose>0)printf("libsalsa: Stub for " #x " called.\n"); \
  return (void*)0; \
}

STUB (snd_ctl_elem_info_get_count)
STUB (snd_hctl_elem_write)
STUB (snd_hctl_first_elem)
STUB (snd_ctl_elem_info_get_item_name)
STUB (snd_hctl_elem_get_callback_private)
STUB_NULL (snd_ctl_card_info_get_driver)
STUB (snd_ctl_elem_value_get_enumerated)
STUB (snd_hctl_set_callback)
STUB (snd_ctl_elem_info_set_item)
STUB (snd_hctl_load)
STUB (snd_ctl_elem_info_get_step)
STUB (snd_ctl_elem_value_set_enumerated)
STUB (snd_ctl_elem_info_get_name)
STUB (snd_hctl_handle_events)
STUB (snd_ctl_elem_info_sizeof)
STUB (snd_ctl_elem_value_get_boolean)
STUB (snd_ctl_elem_value_malloc)
STUB (snd_ctl_elem_id_set_index)
STUB (snd_ctl_elem_value_get_integer)
STUB (snd_hctl_nonblock)
STUB (snd_hctl_get_count)
STUB (snd_ctl_card_info_get_longname)
STUB (snd_hctl_elem_next)
STUB (snd_ctl_elem_info_get_index)
STUB (snd_hctl_elem_read)
STUB (snd_hctl_find_elem)
STUB (snd_ctl_elem_value_set_integer)
STUB (snd_hctl_elem_set_callback)
STUB (snd_hctl_poll_descriptors)
STUB (snd_ctl_elem_info_get_items)
STUB (snd_ctl_elem_info_get_max)
STUB (snd_hctl_open)
STUB (snd_ctl_elem_value_set_boolean)
STUB (snd_ctl_elem_id_set_interface)
STUB (snd_hctl_set_callback_private)
STUB (snd_ctl_elem_id_sizeof)
STUB (snd_ctl_elem_info_get_type)
STUB (snd_hctl_close)
STUB (snd_ctl_elem_info_malloc)
STUB (snd_ctl_elem_info_get_min)
STUB (snd_hctl_elem_info)
STUB (snd_hctl_elem_set_callback_private) STUB (snd_ctl_elem_id_set_name)
