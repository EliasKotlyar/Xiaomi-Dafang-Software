/*
 * Purpose: Console output interface functions and related.
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

#include "ossplay_console.h"
#include "ossplay_parser.h"
#include "ossplay_decode.h"
#include <sys/wait.h>
#ifdef OGG_SUPPORT
#include <dlfcn.h>
#endif

extern int eflag, quiet, verbose;
extern flag from_stdin, loop;

static FILE * normalout;
static int dots = -11, direction = 0;

void
perror_msg (const char * s)
{
  if (quiet < 2) perror (s);
}

void
clear_update (void)
{
  if (verbose) fprintf (normalout, "\r\n");
  dots = -11;
  direction = 0;
}

void
print_update (int v, double secs, const char * total)
{
  char vu[12] = "-------++!!", * rtime;

  if (v > 0) vu[v] = '\0';
  else /* v == 0 */
    {
      vu[0] = '0';
      vu[1] = '\0';
    }

  rtime = totime (secs);
  fprintf (stdout, "\rTime: %s of %s VU %-11s", rtime, total, vu);
  fflush (stdout);
  ossplay_free (rtime);
}

void
print_record_update (int v, double secs, const char * fname, int update)
{
  char vu[12] = "-------++!!";

  int x1, x2, i;
  extern int level_meters;

  fprintf (stderr, "\r%s [", fname);
  x1 = dots;
  x2 = dots + 10;

  if (update)
    {
      if (direction == 0)
        {
          dots++;
          if (dots >= 10) direction = 1;
        }
      else
        {
          dots--;
          if (dots <= -10) direction = 0;
        }
    }

  if (dots < 0)
    {
      x1 = 0;
      x2 = dots + 10;
      if (x2 < 0) x2 = 0;
    }
  if (dots >= 0)
    {
      x2 = 10;
      x1 = dots;
    }

  for (i = 0; i < x1; i++)
    fprintf (stderr, " ");
  for (i = x1; i < x2; i++)
    fprintf (stderr, ".");
  for (i = 0; i < 10 - x2; i++)
    fprintf (stderr, " ");

  if (secs < 60.0)
    fprintf (stderr, "] %1.2f secs", secs);
  else
    {
      int hours, mins;

      mins = (int) (secs / 60.0);
      secs -= (mins * 60);

      hours = mins / 60;
      mins = mins % 60;
      fprintf (stderr, "] %02d:%02d:%02d", hours, mins, (int)secs);
    }

  if (!level_meters)
    {
      return;
    }
  else if (v > 0)
    {
      vu[v] = '\0';
      fprintf (stderr, " VU %-11s", vu);
    }
  else if (v == 0)
    {
      fprintf (stderr, " VU %-11s", "0");
    }

  fflush (stderr);
}

void print_msg (prtype_t type, const char * fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  switch (type)
    {
      case NOTIFYM:
        if (quiet) break;
      case WARNM:
        if (quiet == 2) break;
      case ERRORM:
        vfprintf (stderr, fmt, ap);
        break;
      case HELPM:
        vfprintf (stdout, fmt, ap);
        break;
      case VERBOSEM:
        if (verbose) vfprintf (normalout, fmt, ap);
        break;
      default: /* case NORMALM, STARTM, CONTM, ENDM: */
        if (!quiet) vfprintf (normalout, fmt, ap);
        break;
    }
  va_end (ap);
}

void *
ossplay_malloc (size_t sz)
{
  void *ptr;

  if ((sz == 0) || (sz > OSSPLAY_MAX_MALLOC)) {
    fprintf (stderr, "Unreasonable allocation size " _PRIbig_t ", aborting",
             (big_t)sz);
    exit (E_SETUP_ERROR);
  }
  ptr = malloc (sz);
  if (ptr == NULL) {
    /* Not all libcs support using %z for size_t */
    fprintf (stderr, "Can't allocate " _PRIbig_t " bytes\n", (big_t)sz);
    exit (-1);
  }
  return ptr;
}

void
ossplay_free (void * ptr)
{
  if (ptr == NULL) return;
  free (ptr);
}

off_t
ossplay_lseek_stdin (int fd, off_t off, int w)
{
  off_t i;
  ssize_t bytes_read;
  char buf[BUFSIZ];

  if (w == SEEK_END) return -1;
  if (off < 0) return -1;
  if (off == 0) return 0;
  i = off;
  while (i > 0) {
    bytes_read = read(fd, buf, (i > BUFSIZ)?BUFSIZ:i);
    if (bytes_read == -1) return -1;
    else if (bytes_read == 0) return off - i;
    i -= bytes_read;
  }
  return off;
}

char *
ossplay_strdup (const char * s)
{
  char * p;

  if (s == NULL) return NULL;
  p = strdup (s);
  if (p == NULL)
    {
      fprintf (stderr, "Can't allocate memory for strdup\n");
      exit (-1);
    }
  return p;
}

#ifdef OGG_SUPPORT
int
ossplay_dlclose (void * handle)
{
  return dlclose (handle);
}

void *
ossplay_dlopen (const char * filename)
{
  return dlopen (filename, RTLD_LAZY | RTLD_LOCAL);
}

int
ossplay_vdlsym (void * handle, ...)
{
  va_list ap;
  const char * symbol;
  void ** v = NULL;

  va_start (ap, handle);

  while (1)
    {
      v = va_arg (ap, void **);
      if (v == (void **)NULL) break;
      symbol = va_arg (ap, const char *);
      *v = dlsym (handle, symbol);
      if (*v == NULL)
        {
          const char * msg = dlerror();

          print_msg (ERRORM, "Can't find symbol %s! (Error: %s)\n",
                     symbol, msg?msg:"");
          return -1;
        }
    }

  return 0;
}

const char *
ossplay_dlerror (void)
{
  return dlerror();
}
#endif

static int
ossplay_main (int argc, char ** argv)
{
  int i, loop_flag;
  dspdev_t dsp = { -1 };
  errors_t ret = E_OK;
  dlopen_funcs_t * vft = NULL;

  normalout = stdout;

  i = ossplay_parse_opts (argc, argv, &dsp);

  argc -= i - 1;
  argv += i - 1;

  dsp.flags = O_WRONLY;
  open_device (&dsp);
  if (dsp.playtgt != NULL) select_playtgt (&dsp);

  do {
    loop_flag = 0;
    for (i = 1; i < argc; i++) {
      if (argv[i][0] == '\0') continue;
      strncpy (dsp.current_songname, filepart (argv[i]),
               sizeof (dsp.current_songname));
      dsp.current_songname[sizeof (dsp.current_songname) - 1] = '\0';
      from_stdin = !strcmp (argv[i], "-");
      ret = play_file (&dsp, argv[i], &vft);
      if (ret || from_stdin) argv[i] = "";
      if ((ret == 0) && (!from_stdin)) loop_flag = 1;
      eflag = 0;
    }
  } while (loop && loop_flag);

#ifdef OGG_SUPPORT
  if (vft != NULL)
    {
      ossplay_dlclose (vft->vorbisfile_handle);
      ossplay_free (vft);
    }
#endif

  close_device (&dsp);
  return ret;
}

static int
ossrecord_main (int argc, char ** argv)
{
  int i, oind;
  char current_filename[512];
  dspdev_t dsp = { -1 };
  errors_t err;

  extern int force_fmt, force_channels, force_speed, nfiles;
  extern double datalimit;
  extern fctypes_t type;
  extern char script[512];

  normalout = stderr;
  /* Since recording can be redirected to stdout, we always output to stderr */

  oind = ossrecord_parse_opts (argc, argv, &dsp);

  dsp.flags = O_RDONLY;
  open_device (&dsp);
  if (dsp.recsrc != NULL) select_recsrc (&dsp);

  strncpy (dsp.current_songname, filepart (argv[oind]),
           sizeof (dsp.current_songname));
  dsp.current_songname[sizeof (dsp.current_songname) - 1] = 0;

  for (i = 0; i < nfiles; i++)
    {
      if (nfiles > 1)
        /* XXX */
        snprintf (current_filename, sizeof (current_filename),
                  argv[oind], i + 1);
      else
        snprintf (current_filename, sizeof (current_filename),
                  "%s", argv[oind]);
      err = encode_sound (&dsp, type, current_filename, force_fmt,
                          force_channels, force_speed, datalimit);
      if (*script)
        {
          if (fork () == 0)
            {
              if (execlp (script, script, current_filename, (char *)NULL) == -1)
                {
                  perror (script);
                  exit (-1);
                }
            }

          print_msg (NORMALM,
                     "Waiting for the '%s' script(s) to finish - please stand"
                     " by\n", script);
          while (wait (NULL) != -1);
        }

      if (err) return err;
    }

  close_device (&dsp);
  return 0;
}

char *
totime (double secs)
{
  char time[20];
  unsigned long min = secs / 60;

  snprintf (time, 20, "%.2lu:%05.2f", min, secs - min * 60);

  return ossplay_strdup (time);
}

int
main (int argc, char **argv)
{
  if (strstr (filepart (argv[0]), "ossplay")) exit(ossplay_main (argc, argv));
  exit(ossrecord_main (argc, argv));
}
