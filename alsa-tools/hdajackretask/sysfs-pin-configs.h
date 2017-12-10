#ifndef __SYSFS_PIN_CONFIGS_H__
#define __SYSFS_PIN_CONFIGS_H__

#include <glib.h>

typedef struct codec_name_t {
    int card;
    int device;
    gchar* name;
} codec_name_t;

typedef struct pin_configs_t {
    int nid;
    unsigned long init_pin_config;
    unsigned long driver_pin_config;
    unsigned long user_pin_config;
    unsigned long pin_caps;
    unsigned long wid_caps;
    gboolean user_override;
    gboolean driver_override;
} pin_configs_t;

typedef gboolean(*caps_limit_func_t)(unsigned long, unsigned long);

typedef struct typical_pins_t {
    char* name;
    caps_limit_func_t caps_limit;
    unsigned long pin_set;
    unsigned long match_mask;
    unsigned long pin_set_mask;
} typical_pins_t;

typedef struct free_override_t {
    const char* name;
    unsigned long value;
} free_override_t;

#define FREE_OVERRIDES_COUNT 8

free_override_t* get_free_override_list(int type);
unsigned long get_free_override_mask(int type);

int get_codec_name_list(codec_name_t* names, int entries);

int get_pin_configs_list(pin_configs_t* pins, int entries, int card, int device);

gchar* get_config_description(unsigned long config);

gchar* get_caps_description(unsigned long pin_caps);

const gchar** get_standard_hint_names();
gchar *get_hint_overrides(int card, int device);


/* 0 = Jack, 1 = N/A, 2 = Internal, 3 = Both (?!) */
int get_port_conn(unsigned long config);

int get_typical_pins(typical_pins_t* result, int entries, pin_configs_t* pin_cfg, int caps_limit); 

unsigned long actual_pin_config(pin_configs_t* pins);

void get_codec_header(int card, int device, unsigned int* address, 
    unsigned int* codec_vendorid, unsigned int* codec_ssid);

gboolean find_pin_channel_match(pin_configs_t* pins, int count, unsigned long pinval);

#endif
