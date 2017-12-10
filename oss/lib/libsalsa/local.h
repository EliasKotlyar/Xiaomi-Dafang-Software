#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "alsa-symbols.h"
#include <alsa/asoundlib.h>
#include "../../include/soundcard.h"
#include "alsakernel.h"
#include <sys/poll.h>
#include "../../kernel/framework/include/midiparser.h"

extern int alib_verbose;
extern int mixer_fd;

extern char alib_appname[64];

extern oss_sysinfo sysinfo;

#define dbg_printf	if (alib_verbose>0)printf
#define dbg_printf0	if (alib_verbose>=0)printf
#define dbg_printf1	if (alib_verbose>=1)printf
#define dbg_printf2	if (alib_verbose>=2)printf
#define dbg_printf3	if (alib_verbose>=3)printf

extern int alib_initialized;

extern int init_alib (void);

#define ALIB_INIT() \
{ \
	int init_err; \
	if (!alib_initialized) \
	   if ((init_err=alib_init())<0) \
	      return init_err; \
}

struct _snd_pcm_info
{
  oss_audioinfo *ainfo;
};


extern int alib_appcheck (void);

struct _snd_seq_port_info
{
  oss_longname_t name;
  int port;
  int capability;
  int midi_channels;
  int synth_voices;
  int midi_voices;
  int type;
};

struct _snd_seq_client_info
{
  oss_longname_t name;
  int client;
};

/* Size of the local event buffer */
#define MAX_EVENTS	128

struct _snd_seq
{
  int fd;
  int streams;
  int nonblock;
  int oss_mode;
  oss_longname_t name;
  midiparser_common_t *parser;

  snd_seq_event_t events[MAX_EVENTS];
  int nevents, nextevent;
};

struct _snd_rawmidi_info
{
  int dummy;
};

struct _snd_seq_queue_status
{
  int dummy;
};

struct _snd_seq_queue_timer
{
  int dummy;
};

extern int convert_event (snd_seq_t * seq, snd_seq_event_t * ev);
extern void midiparser_callback (void *context, int category,
				 unsigned char msg, unsigned char ch,
				 unsigned char *parms, int len);
