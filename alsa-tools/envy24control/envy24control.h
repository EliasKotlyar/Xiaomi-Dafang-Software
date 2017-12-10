#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <alsa/asoundlib.h>

/* Profiles */
#ifdef PACKAGE
#define PROGRAM_NAME PACKAGE
#else
#define PROGRAM_NAME "envy24control"
#endif
#define MAX_PROFILES 8
#define MAX_PROFILE_NAME_LENGTH 20
#define DEFAULT_PROFILERC "~/envy24control/profiles.conf"
#define SYS_PROFILERC "/etc/envy24control/profiles.conf"
#ifndef MKDIR
#define MKDIR "/bin/mkdir"
#endif
#ifndef ALSACTL
#define ALSACTL "/usr/sbin/alsactl"
#endif

#include "profiles.h"

/* MidiMan */
#define ICE1712_SUBDEVICE_DELTA1010	0x121430d6
#define ICE1712_SUBDEVICE_DELTADIO2496	0x121431d6
#define ICE1712_SUBDEVICE_DELTA66	0x121432d6
#define ICE1712_SUBDEVICE_DELTA44	0x121433d6
#define ICE1712_SUBDEVICE_AUDIOPHILE    0x121434d6
#define ICE1712_SUBDEVICE_DELTA410      0x121438d6
#define ICE1712_SUBDEVICE_DELTA1010LT   0x12143bd6

/* Terratec */
#define ICE1712_SUBDEVICE_EWX2496       0x3b153011
#define ICE1712_SUBDEVICE_EWS88MT       0x3b151511
#define ICE1712_SUBDEVICE_EWS88D        0x3b152b11
#define ICE1712_SUBDEVICE_DMX6FIRE      0x3b153811

/* Hoontech */
#define ICE1712_SUBDEVICE_STDSP24       0x12141217      /* Hoontech SoundTrack Audio DSP 24 */

/* max number of cards for alsa */
#define MAX_CARD_NUMBERS	8
/* max number of HW input/output channels (analog lines)
 * the number of available HW input/output channels is defined
 * at 'adcs/dacs' in the driver
 */
/* max number of HW input channels (analog lines) */
#define MAX_INPUT_CHANNELS	8
/* max number of HW output channels (analog lines) */
#define MAX_OUTPUT_CHANNELS	8
/* max number of spdif input/output channels */
#define MAX_SPDIF_CHANNELS	2
/* max number of PCM output channels */
#define MAX_PCM_OUTPUT_CHANNELS	8

typedef struct {
	unsigned int subvendor;	/* PCI[2c-2f] */
	unsigned char size;	/* size of EEPROM image in bytes */
	unsigned char version;	/* must be 1 */
	unsigned char codec;	/* codec configuration PCI[60] */
	unsigned char aclink;	/* ACLink configuration PCI[61] */
	unsigned char i2sID;	/* PCI[62] */
	unsigned char spdif;	/* S/PDIF configuration PCI[63] */
	unsigned char gpiomask;	/* GPIO initial mask, 0 = write, 1 = don't */
	unsigned char gpiostate; /* GPIO initial state */
	unsigned char gpiodir;	/* GPIO direction state */
	unsigned short ac97main;
	unsigned short ac97pcm;
	unsigned short ac97rec;
	unsigned char ac97recsrc;
	unsigned char dacID[4];	/* I2S IDs for DACs */
	unsigned char adcID[4];	/* I2S IDs for ADCs */
	unsigned char extra[4];
} ice1712_eeprom_t;

extern snd_ctl_t *ctl;
extern ice1712_eeprom_t card_eeprom;

extern GtkWidget *mixer_mix_drawing;
extern GtkWidget *mixer_clear_peaks_button;
extern GtkWidget *mixer_drawing[20];
extern GtkObject *mixer_adj[20][2];
extern GtkWidget *mixer_vscale[20][2];
extern GtkWidget *mixer_solo_toggle[20][2];
extern GtkWidget *mixer_mute_toggle[20][2];
extern GtkWidget *mixer_stereo_toggle[20];

extern GtkWidget *router_radio[10][12];

//extern GtkWidget *hw_master_clock_xtal_radio;
extern GtkWidget *hw_master_clock_xtal_22050;
extern GtkWidget *hw_master_clock_xtal_32000;
extern GtkWidget *hw_master_clock_xtal_44100;
extern GtkWidget *hw_master_clock_xtal_48000;
extern GtkWidget *hw_master_clock_xtal_88200;
extern GtkWidget *hw_master_clock_xtal_96000;
extern GtkWidget *hw_master_clock_spdif_radio;
extern GtkWidget *hw_master_clock_word_radio;
extern GtkWidget *hw_master_clock_status_label;
extern GtkWidget *hw_master_clock_actual_rate_label;

extern GtkWidget *hw_rate_locking_check;
extern GtkWidget *hw_rate_reset_check;

extern GtkObject *hw_volume_change_adj;
extern GtkWidget *hw_volume_change_spin;

extern GtkWidget *hw_spdif_profi_nonaudio_radio;
extern GtkWidget *hw_spdif_profi_audio_radio;

extern GtkWidget *hw_profi_stream_stereo_radio;
extern GtkWidget *hw_profi_stream_notid_radio;

extern GtkWidget *hw_profi_emphasis_none_radio;
extern GtkWidget *hw_profi_emphasis_5015_radio;
extern GtkWidget *hw_profi_emphasis_ccitt_radio;
extern GtkWidget *hw_profi_emphasis_notid_radio;

extern GtkWidget *hw_consumer_copyright_on_radio;
extern GtkWidget *hw_consumer_copyright_off_radio;

extern GtkWidget *hw_consumer_copy_1st_radio;
extern GtkWidget *hw_consumer_copy_original_radio;

extern GtkWidget *hw_consumer_emphasis_none_radio;
extern GtkWidget *hw_consumer_emphasis_5015_radio;

extern GtkWidget *hw_consumer_category_dat_radio;
extern GtkWidget *hw_consumer_category_pcm_radio;
extern GtkWidget *hw_consumer_category_cd_radio;
extern GtkWidget *hw_consumer_category_general_radio;

extern GtkWidget *hw_spdif_professional_radio;
extern GtkWidget *hw_spdif_consumer_radio;
extern GtkWidget *hw_spdif_output_notebook;

extern GtkWidget *hw_spdif_input_coaxial_radio;
extern GtkWidget *hw_spdif_input_optical_radio;
extern GtkWidget *hw_spdif_switch_off_radio;

extern GtkWidget *hw_phono_input_on_radio;
extern GtkWidget *hw_phono_input_off_radio;

extern GtkWidget *input_interface_internal;
extern GtkWidget *input_interface_front_input;
extern GtkWidget *input_interface_rear_input;
extern GtkWidget *input_interface_wavetable;
extern GtkObject *av_dac_volume_adj[];
extern GtkObject *av_adc_volume_adj[];
extern GtkObject *av_ipga_volume_adj[];
extern GtkLabel *av_dac_volume_label[];
extern GtkLabel *av_adc_volume_label[];
extern GtkLabel *av_ipga_volume_label[];
extern GtkWidget *av_dac_sense_radio[][4];
extern GtkWidget *av_adc_sense_radio[][4];

/* flags */
extern int card_is_dmx6fire;

gint level_meters_configure_event(GtkWidget *widget, GdkEventConfigure *event);
gint level_meters_expose_event(GtkWidget *widget, GdkEventExpose *event);
gint level_meters_timeout_callback(gpointer data);
void level_meters_reset_peaks(GtkButton *button, gpointer data);
void level_meters_init(void);
void level_meters_postinit(void);

int mixer_stream_is_active(int stream);
void mixer_update_stream(int stream, int vol_flag, int sw_flag);
void mixer_toggled_solo(GtkWidget *togglebutton, gpointer data);
void mixer_toggled_mute(GtkWidget *togglebutton, gpointer data);
void mixer_adjust(GtkAdjustment *adj, gpointer data);
void mixer_init(void);
void mixer_postinit(void);

int patchbay_stream_is_active(int stream);
void patchbay_update(void);
void patchbay_toggled(GtkWidget *togglebutton, gpointer data);
void patchbay_init(void);
void patchbay_postinit(void);

void master_clock_update(void);
gint master_clock_status_timeout_callback(gpointer data);
gint internal_clock_status_timeout_callback(gpointer data);
gint rate_locking_status_timeout_callback(gpointer data);
gint rate_reset_status_timeout_callback(gpointer data);
void internal_clock_toggled(GtkWidget *togglebutton, gpointer data);
void rate_locking_update(void);
void rate_locking_toggled(GtkWidget *togglebutton, gpointer data);
void rate_reset_update(void);
void rate_reset_toggled(GtkWidget *togglebutton, gpointer data);
void volume_change_rate_update(void);
void volume_change_rate_adj(GtkAdjustment *adj, gpointer data);
void profi_data_toggled(GtkWidget *togglebutton, gpointer data);
void profi_stream_toggled(GtkWidget *togglebutton, gpointer data);
void profi_emphasis_toggled(GtkWidget *togglebutton, gpointer data);
void consumer_copyright_toggled(GtkWidget *togglebutton, gpointer data);
void consumer_copy_toggled(GtkWidget *togglebutton, gpointer data);
void consumer_emphasis_toggled(GtkWidget *togglebutton, gpointer data);
void consumer_category_toggled(GtkWidget *togglebutton, gpointer data);
void spdif_output_update(void);
void spdif_output_toggled(GtkWidget *togglebutton, gpointer data);
void spdif_input_update(void);
void spdif_input_toggled(GtkWidget *togglebutton, gpointer data);
void analog_input_select_toggled(GtkWidget *togglebutton, gpointer data);
void phono_input_toggled(GtkWidget *togglebutton, gpointer data);

void hardware_init(void);
void hardware_postinit(void);
void analog_volume_init(void);
void analog_volume_postinit(void);
int envy_dac_volumes(void);
int envy_dac_max(void);
int envy_adc_volumes(void);
int envy_adc_max(void);
int envy_ipga_volumes(void);
int envy_dac_senses(void);
int envy_adc_senses(void);
int envy_dac_sense_items(void);
int envy_adc_sense_items(void);
const char *envy_dac_sense_enum_name(int i);
const char *envy_adc_sense_enum_name(int i);
int envy_analog_volume_available(void);

void dac_volume_update(int idx);
void adc_volume_update(int idx);
void ipga_volume_update(int idx);
void dac_sense_update(int idx);
void adc_sense_update(int idx);
void dac_volume_adjust(GtkAdjustment *adj, gpointer data);
void adc_volume_adjust(GtkAdjustment *adj, gpointer data);
void ipga_volume_adjust(GtkAdjustment *adj, gpointer data);
void dac_sense_toggled(GtkWidget *togglebutton, gpointer data);
void adc_sense_toggled(GtkWidget *togglebutton, gpointer data);

void control_input_callback(gpointer data, gint source, GdkInputCondition condition);
void mixer_input_callback(gpointer data, gint source, GdkInputCondition condition);

