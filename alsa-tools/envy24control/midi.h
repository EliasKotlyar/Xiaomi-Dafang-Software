#ifndef MIDI__H
#define MIDI__H

#include <gdk/gdk.h>

int midi_init(char *appname, int channel, int midi_enhanced);
int midi_close();
void midi_maxstreams(int);
int midi_controller(int c, int v);
void midi_process(gpointer data, gint source, GdkInputCondition condition);
int midi_button(int b, int v);

#endif
