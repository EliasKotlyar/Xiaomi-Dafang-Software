/*
 * ALSA SoundScape control utility
 *
 * Copyright (c) 2003 by Chris Rankin
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>

#include <alsa/sound/sscape_ioctl.h>
#include <alsa/asoundlib.h>


const char default_dir[] = "/sndscape";
const char scope[] = "scope.cod";
unsigned char _microcode[SSCAPE_MICROCODE_SIZE];

static void
show_usage(void)
{
  printf("sscape_ctl: [--card card number]\n"
         "            [--directory firmware directory]\n"
         "sscape_ctl: --help\n"
         "sscape_ctl: --version\n");
}


static void
show_version(void)
{
  printf("ALSA SoundScape control utility: v" VERSION "\n");
}


void
safe_close(int fd)
{
  int err;
  while (((err = close(fd)) != 0) && (errno == EINTR)) {}
}

size_t
get_directory(const char *dir, char *buffer, size_t bufsize)
{
  size_t len;

  len = snprintf(buffer, bufsize, "%s/", dir);
  if (len >= bufsize)
    return 0;

  if ((len > 1) && (buffer[len - 1] == '/') && (buffer[len - 2] == '/'))
  {
    buffer[--len] = '\0';
  }

  return len;
}


size_t
get_bootfile(const char *filename, char *buffer, size_t bufsize)
{
  size_t len = snprintf(buffer, bufsize, "%s", filename);
  if (len >= bufsize)
    return 0;
  return len;
}


size_t
get_mcodefile(unsigned version, char *buffer, size_t bufsize)
{
  static const char sndscape[] = "sndscape.co%u";

  size_t len = snprintf(buffer, bufsize, sndscape, version);
  if (len >= bufsize)
    return 0;
  return len;
}


int
load_bootblock(const char *fname, struct sscape_bootblock *boot)
{
  int err;
  int fd;

  printf("Bootblock: %s\n", fname);

  err = fd = open(fname, O_RDONLY);
  if (err >= 0)
  {
    int save_errno;

    err = read(fd, boot->code, sizeof(boot->code));
    if (err >= 0)
    {
      printf("Bootblock: read %d bytes\n", err);
      err = 0;
    }

    save_errno = errno;
    safe_close(fd);
    errno = save_errno;
  }

  return err;
}


int
load_microcode(const char *fname, struct sscape_microcode *microcode)
{
  int err;
  int fd;

  printf("Microcode: %s\n", fname);

  err = fd = open(fname, O_RDONLY);
  if (err >= 0)
  {
    int save_errno;

    err = read(fd, microcode->code, sizeof(_microcode));
    if (err >= 0)
    {
      printf("Microcode: read %d bytes\n", err);
      err = 0;
    }

    save_errno = errno;
    safe_close(fd);
    errno = save_errno;
  }

  return err;
}


static const struct option long_option[] = {
  { "card", 1, NULL, 'c' },
  { "directory", 1, NULL, 'd' },
  { "help", 0, NULL, 'h' },
  { "version", 0, NULL, 'v' },
  { NULL, 0, NULL, '\0' }
};

static const char option[] = "c:d:hv";

int
main(int argc, char *argv[])
{
  char devicename[32];
  int ret, err;
  snd_hwdep_t *handle;

  const char *directory = default_dir;
  int card = 0;

  int oindex;
  int c;

  while ( (c = getopt_long(argc, argv, option, long_option, &oindex)) != EOF )
  {
    switch(c)
    {
    case 'c':
      card = snd_card_get_index(optarg);
      if (card < 0 || card > 31) {
        fprintf(stderr, "Wrong -c argument '%s'\n", optarg);
        return EXIT_FAILURE;
      }
      break;

    case 'd':
      directory = optarg;
      break;

    case 'h':
      show_usage();
      return EXIT_SUCCESS;

    case 'v':
      show_version();
      return EXIT_SUCCESS;

    default:
      return EXIT_FAILURE;
    } /* switch */
  } /* while */

  ret = EXIT_FAILURE;
  snprintf(devicename, sizeof(devicename), "hw:%i,0", card);
  err = snd_hwdep_open(&handle, devicename, O_WRONLY);
  if (err < 0)
  {
    fprintf(stderr, "Error opening %s: %s\n", devicename, snd_strerror(err)); 
  }
  else
  {
    char filename[FILENAME_MAX];
    size_t len;

    struct sscape_bootblock  boot;
    struct sscape_microcode  microcode;

    microcode.code = _microcode;
    if ((len = get_directory(directory, filename, sizeof(filename))) == 0)
    {
      fprintf(stderr, "Invalid directory - pathname too long\n");
    }
    else if (get_bootfile(scope, filename + len, sizeof(filename) - len) == 0)
    {
      fprintf(stderr, "Invalid filename - full pathname too long\n");
    }
    else if (load_bootblock(filename, &boot) < 0)
    {
      fprintf(stderr, "Failed to load file [%s]: %s\n",
                      filename, strerror(errno));
    }
    else if (snd_hwdep_ioctl(handle, SND_SSCAPE_LOAD_BOOTB, &boot) < 0)
    {
      fprintf(stderr, "IOCTL error: %s\n", strerror(errno));
    }
    else if (get_mcodefile(boot.version & 0x0f,
                           filename + len, sizeof(filename) - len) == 0)
    {
      fprintf(stderr, "Invalid filename - full pathname too long\n"); 
    }
    else if (load_microcode(filename, &microcode) < 0)
    {
      fprintf(stderr, "Failed to load microcode [%s]\n", filename);
    }
    else if (snd_hwdep_ioctl(handle, SND_SSCAPE_LOAD_MCODE, &microcode) < 0)
    {
      fprintf(stderr, "IOCTL error: %s\n", strerror(errno));
    }
    else
    {
      printf("Microcode loaded.\n");
      ret = EXIT_SUCCESS;
    }
    snd_hwdep_close(handle);
  }

  return ret;
}

