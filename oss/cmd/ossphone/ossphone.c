/*
 * Purpose: Utility to make phone calls using Open Sound System modem support.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <soundcard.h>

char *cmdname=NULL;

int modem_in_fd = -1;
int modem_out_fd = -1;
int dev_dsp_fd = -1;

char *dspdev_name = "/dev/dsp"; /* Local audio device (headset) */
int srate = 8000;
double digit_duration = 0.2;
double silence_duration = 0.1;

/*
 * http://en.wikipedia.org/wiki/DTMF#Keypad
 */

const double dtmf_keypad_row[] = { 697.0,  770.0,  852.0,  941.0  };
const double dtmf_keypad_col[] = { 1209.0, 1336.0, 1477.0, 1633.0 };
const char *dtmf_keypad_keys = "123A456B789C*0#D";

static void
modem_write(int fd, const void *buf, size_t count)
{
	if (write(fd, buf, count) != count)
	{
		perror("Modem write");
		exit(1);
	}
}

static void
modem_read(int fd, void *buf, size_t count)
{
	if (read(fd, buf, count) != count)
	{
		perror("Modem read");
		exit(1);
	}
}

static int
dtmf_fill_digit(uint16_t *buf, size_t buf_len, int digit)
{
  const double A = 65536.0 / 8.0;
  const double pi = 3.14159265358979323846;

  const char *keypad_digit;
  double w1, w2;
  double t, sample_duration;
  size_t i, pos;

  keypad_digit = strchr(dtmf_keypad_keys, toupper(digit));

  if (keypad_digit == NULL)
      return -1;

  pos = (int)(keypad_digit - dtmf_keypad_keys);

  /* compute angular frequencies */
  w1 = 2.0*pi*dtmf_keypad_row[pos / 4];
  w2 = 2.0*pi*dtmf_keypad_col[pos % 4];

  t = 0.0;
  sample_duration = 1.0 / (double)srate;

  for (i = 0; i < buf_len; i++)
     {
       buf[i] = (uint16_t)(A + A*sin(w1*t) + A + A*sin(w2*t));
       t += sample_duration;
     }
  
  return 0;
}

static void
dtmf_fill_silence(uint16_t *buf, size_t buf_len)
{
  const uint16_t A = (uint16_t)(65536.0 / 4.0);
  size_t i;

  for (i = 0; i < buf_len; i++)
     {
       buf[i] = A;
     }
}

static double
evaluate_dc_level(uint16_t *buf, size_t buf_len)
{
  double sum = 0.0;
  size_t i;

  for (i = 0; i < buf_len; i++)
     {
       sum += ((double)buf[i]) / 65536.0;
     }

  return (sum / (double)buf_len);
}

static void
go_offhook()
{
  int offhook = 1;

  printf("Off-hook\n");
  ioctl (modem_out_fd, SNDCTL_DSP_MODEM_OFFHOOK, &offhook);
}

static int wait_dialtone()
{
  double dc_level = 0.0;
  const double min_dc_level = 0.2;
  
  uint16_t buf[4096];

  int retries = 0;
  const int max_retries = 10;

  printf("Waiting for dial tone...\n");
  while (dc_level < min_dc_level)
       {
	 int dummy;
         modem_read(modem_in_fd, buf, sizeof(buf));
         dummy=write(dev_dsp_fd, buf, sizeof(buf));

         dc_level = evaluate_dc_level(buf, sizeof(buf)/sizeof(uint16_t));

         if (retries++ > max_retries)
           {
             return -1;
           }
       }

  return 0;
}

static void
dial_phone_number(const char *phone_number)
{
  size_t digit_len = (size_t)(digit_duration*srate);
  size_t silence_len = (size_t)(silence_duration*srate);
  size_t digit_size = digit_len * sizeof(uint16_t);
  size_t silence_size = silence_len * sizeof(uint16_t);
  
  uint16_t *digit = (uint16_t *)malloc(digit_size);
  uint16_t *silence = (uint16_t *)malloc(silence_size);
  uint16_t *buf = (uint16_t *)malloc(silence_size);

  dtmf_fill_silence (silence, silence_len);

  printf("Dialing... ");
  fflush(stdout);

  while (*phone_number != '\0')
       {
         if (dtmf_fill_digit (digit, digit_len, *phone_number) >= 0)
           {
	     int dummy;

             printf("%c", *phone_number);
             fflush(stdout);

             modem_write (modem_out_fd, digit, digit_size);
             modem_read (modem_in_fd, digit, digit_size);
             dummy=write (dev_dsp_fd, digit, digit_size);

             modem_write (modem_out_fd, silence, silence_size);
             modem_read (modem_in_fd, buf, silence_size);
             dummy=write (dev_dsp_fd, buf, silence_size);
           }
         phone_number++;
       }
  
  free(buf);
  free(silence);
  free(digit);

  printf("\n");
}

static void
usage(void)
{
  fprintf (stderr, "Usage: %s [options] mdmin-dev mdmout-dev [phone-number]\n", cmdname);
  fprintf (stderr, "  Options:  -d<devname>    Change sound device (default: %s)\n", dspdev_name);
  fprintf (stderr, "            -s<rate>       Change sampling rate (default: %d)\n", srate);
  fprintf (stderr, "            -t<duration>   Change DTMF digit duration (default: %.1f s)\n", digit_duration);
  fprintf (stderr, "            -l<duration>   Change DTMF silence duration (default: %.1f s)\n", silence_duration);
  exit (-1);
}

static void
exit_handler(void)
{
  if (modem_in_fd >= 0)
    {
      close (modem_in_fd);
      modem_in_fd = -1;
    }
  if (modem_out_fd >= 0)
    {
      int offhook = 0;
      printf("On-hook\n");
      ioctl (modem_out_fd, SNDCTL_DSP_MODEM_OFFHOOK, &offhook);
      close (modem_out_fd);
      modem_out_fd = -1;
    }
  if (dev_dsp_fd >= 0)
    {
      close (dev_dsp_fd);
      dev_dsp_fd = -1;
    }
}

static void
sigint_handler(int sig)
{
  exit (0);
}

int
main(int argc, char **argv)
{
  char *phone_number = "";

  int channels = 1;
  int format = AFMT_S16_LE;

  extern int optind;
  extern char *optarg;
  int c, tmp;

  cmdname=argv[0];

  signal(SIGINT, sigint_handler);

  if (argc < 3)
    {
      usage ();
    }

  while ((c = getopt (argc, argv, "d:s:t:l:")) != EOF)
    {
      switch (c)
      {
      case 'd':
              dspdev_name = optarg;
              break;
      case 's':
              srate = atoi (optarg);
              break;
      case 't':
              digit_duration = atof (optarg);
              break;
      case 'l':
              silence_duration = atof (optarg);
              break;
      default:
              usage ();
      }
    }

  if ((argc - optind) < 2)
      usage ();

  atexit(exit_handler);

  dev_dsp_fd = open (dspdev_name, O_RDWR);
  if (dev_dsp_fd < 0)
    {
      perror (dspdev_name);
      exit (-1);
    }

  modem_in_fd = open (argv[optind], O_RDWR);
  if (modem_in_fd < 0)
    {
      perror (argv[optind]);
      exit (-1);
    }
  tmp=0;
  ioctl(modem_in_fd, SNDCTL_DSP_COOKEDMODE, &tmp); // No error checking with this call
  optind++;

  modem_out_fd = open(argv[optind], O_RDWR);
  if (modem_out_fd < 0)
    {
      perror (argv[optind]);
      exit (-1);
    }
  tmp=0;
  ioctl(modem_out_fd, SNDCTL_DSP_COOKEDMODE, &tmp); // No error checking with this call
  optind++;

  if (argc > optind)
      phone_number = argv[optind];

  assert ( ioctl (modem_in_fd,  SNDCTL_DSP_CHANNELS, &channels) >= 0 );
  assert ( ioctl (modem_out_fd, SNDCTL_DSP_CHANNELS, &channels) >= 0 );
  assert ( ioctl (dev_dsp_fd, SNDCTL_DSP_CHANNELS, &channels) >= 0 );

  assert ( ioctl (modem_in_fd,  SNDCTL_DSP_SETFMT, &format) >= 0 );
  assert ( ioctl (modem_out_fd, SNDCTL_DSP_SETFMT, &format) >= 0 );
  assert ( ioctl (dev_dsp_fd, SNDCTL_DSP_SETFMT, &format) >= 0 );

  assert ( ioctl (modem_in_fd,  SNDCTL_DSP_SPEED, &srate) >= 0 );
  assert ( ioctl (modem_out_fd, SNDCTL_DSP_SPEED, &srate) >= 0 );
  assert ( ioctl (dev_dsp_fd, SNDCTL_DSP_SPEED, &srate) >= 0 );

  {
    int tmp;
    tmp = 0;
    assert ( ioctl (modem_in_fd,  SNDCTL_DSP_SETTRIGGER, &tmp) >= 0 );
    tmp = PCM_ENABLE_INPUT;
    assert ( ioctl (modem_in_fd,  SNDCTL_DSP_SETTRIGGER, &tmp) >= 0 );
    tmp = 0;
    assert ( ioctl (modem_out_fd, SNDCTL_DSP_SETTRIGGER, &tmp) >= 0 );
    tmp = PCM_ENABLE_OUTPUT;
    assert ( ioctl (modem_out_fd, SNDCTL_DSP_SETTRIGGER, &tmp) >= 0 );
    tmp = 0;
    assert ( ioctl (dev_dsp_fd, SNDCTL_DSP_SETTRIGGER, &tmp) >= 0 );
    tmp = PCM_ENABLE_INPUT | PCM_ENABLE_OUTPUT;
    assert ( ioctl (dev_dsp_fd, SNDCTL_DSP_SETTRIGGER, &tmp) >= 0 );
  }

  go_offhook ();

  if (phone_number[0] != '\0')
  {
      if (wait_dialtone () < 0)
        {
          printf("No dial tone.\n");
          exit (-1);
        }
      dial_phone_number(phone_number);
    }

  printf("Call in progress...\n");
  printf("Press Ctrl-C to quit.\n");

  {
    uint16_t buf[128];
    fd_set rfds;
    int retval;

    int max_fd = modem_in_fd;
    if (dev_dsp_fd > max_fd)
        max_fd = dev_dsp_fd;

    while (1)
         {
	   int dummy;
           FD_ZERO(&rfds);
           FD_SET(modem_in_fd, &rfds);
           FD_SET(dev_dsp_fd, &rfds);

           retval = select(max_fd+1, &rfds, NULL, NULL, NULL);
           if (retval == -1)
             {
               perror("select");
             }
           else if (retval)
             {
               if (FD_ISSET(modem_in_fd, &rfds))
                 {
                   modem_read(modem_in_fd, buf, sizeof(buf));
                   dummy=write(dev_dsp_fd, buf, sizeof(buf));
                 }
               if (FD_ISSET(dev_dsp_fd, &rfds))
                 {
                   dummy=read(dev_dsp_fd, buf, sizeof(buf));
                   modem_write(modem_out_fd, buf, sizeof(buf));
                 }
             }
         }
  }

  // return 0; /* NOT REACHED */
}

