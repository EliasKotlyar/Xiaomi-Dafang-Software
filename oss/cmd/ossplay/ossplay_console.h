#ifndef _OSSPLAY_CONSOLE_H
#define _OSSPLAY_CONSOLE_H

#include <stddef.h>
#include <sys/types.h>

typedef enum {
  ERRORM,
  HELPM,
  NORMALM,
  NOTIFYM,
  WARNM,
  STARTM,
  CONTM,
  ENDM,
  VERBOSEM
}
prtype_t;

void clear_update (void);
void ossplay_free (void *);
void * ossplay_malloc (size_t);
off_t ossplay_lseek_stdin (int, off_t, int);
char * ossplay_strdup (const char *);
#ifdef OGG_SUPPORT
int ossplay_dlclose (void *);
void * ossplay_dlopen (const char *);
const char * ossplay_dlerror (void);
int ossplay_vdlsym (void *, ...);
#endif
void perror_msg (const char * s);
void print_msg (prtype_t, const char *, ...);
void print_record_update (int, double, const char *, int);
void print_update (int, double, const char *);
char * totime (double);

#endif
