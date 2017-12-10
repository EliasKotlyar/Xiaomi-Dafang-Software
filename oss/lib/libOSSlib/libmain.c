#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define OSSLIB
#include "../../include/soundcard.h"

int __seqfd = -1;
static int initialized = 0;

extern void _dump_midi (void);	/* From play_event.c */

unsigned char *_seqbuf;
int _seqbuflen = 0;
int _seqbufptr = 0;

int synth_types[64] = { 0 };

#define ST_NONE		0
#define ST_GUS		1
#define ST_OPL		2

int nrsynths = 0;

extern void play_event (unsigned char *ev);

void
seqbuf_dump (void)
{
  int l = _seqbufptr;

  unsigned char *ev = _seqbuf;

  while (l >= 8)
    {
      play_event (ev);
      l -= 8;
      ev += 8;
    }

  _seqbufptr = 0;
  _dump_midi ();
}

static int
oss_init_lib (void)
{
  if (_seqbuflen < 32 || _seqbuflen > 2048)
    _seqbuflen = 2048;

  if ((_seqbuf = malloc (_seqbuflen)) == NULL)
    return 3;

  initialized = 1;
  return 0;			/* OK */
}

int
OSS_init (int userfd, int buflen)
{
  if (_seqbuflen || _seqbuflen || __seqfd != -1 || initialized)
    {
      fprintf (stderr, "libOSS: OSS_init called too late\n");
      return 1;
    }

  __seqfd = userfd;

  if (buflen < 32 || buflen > 2048)
    {
      fprintf (stderr, "libOSS: OSS_init called with invalid buflen\n");
      return 2;
    }

  _seqbuflen = buflen;

  return oss_init_lib ();
}

static void
sanity_check (int fd, unsigned char *buf, int buflen)
{
  if (__seqfd != fd)
    if (__seqfd == -1)
      __seqfd = fd;
    else
      {
	fprintf (stderr, "OSSlib: seqfd is inconsistent\n");
      }

  if (buf != _seqbuf)
    {
      fprintf (stderr, "OSSlib: _seqbuf is inconsistent\n");
    }

  if (buflen != _seqbuflen)
    {
      fprintf (stderr, "OSSlib: _seqbuf is inconsistent\n");
    }

  if (!initialized)
    if (oss_init_lib () != 0)
      {
	fprintf (stderr, "OSSlib: Library initialization failed\n");
	exit (-1);
      }
}

void
OSS_seqbuf_dump (int fd, unsigned char *buf, int buflen)
{
  seqbuf_dump ();
}

void
OSS_seq_advbuf (int len, int fd, unsigned char *buf, int buflen)
{
  sanity_check (fd, buf, buflen);
  _seqbufptr += len;
}

void
OSS_seq_needbuf (int len, int fd, unsigned char *buf, int buflen)
{
  sanity_check (fd, buf, buflen);

  if ((_seqbufptr + len) > _seqbuflen)
    {
      seqbuf_dump ();
    }
}

void
OSS_patch_caching (int dev, int chn, int patch, int fd, unsigned char *buf,
		   int buflen)
{
}

void
OSS_drum_caching (int dev, int chn, int patch, int fd, unsigned char *buf,
		  int buflen)
{
}

void
OSS_write_patch (int fd, unsigned char *buf, int len)
{
  sanity_check (fd, _seqbuf, _seqbuflen);

  if (write (fd, buf, len) == -1)
    {
      perror ("OSS_write_patch");
      exit (-1);
    }
}

int
OSS_write_patch2 (int fd, unsigned char *buf, int len)
{
  sanity_check (fd, _seqbuf, _seqbuflen);

  return write (fd, buf, len);
}

/*
 * audio routines
 */

int
osslib_open (const char *path, int flags, int dummy)
{
  return open (path, flags, dummy);
}

void
osslib_close (int fd)
{
  close (fd);
}

int
osslib_write (int fd, const void *buf, int count)
{
  return write (fd, buf, count);
}

int
osslib_read (int fd, void *buf, int count)
{
  return read (fd, buf, count);
}

int
osslib_ioctl (int fd, unsigned int request, void *arg)
{
  return ioctl (fd, request, arg);
}
