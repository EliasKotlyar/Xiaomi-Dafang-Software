/*
 * snoopy.c
 *
 * Purpose: Unsupported and undocumented diagnostic tool for hdaudio devices.
 *
 * This utility is an in-house tool used for examining capabilities and
 * implementation details of HD audio codec chips. Only the first codec
 * attached to the HD audio controller will be shown.
 *
 * You need to set the hdaudio_snoopy config option to 1 in hdaudio.conf.
 *
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <soundcard.h>

typedef int sound_os_info, mixer_create_controls_t, oss_device_t;

#include <hdaudio.h>

#define ioctl_arg int
#define mixer_ext_init_fn int
#include <hdaudio_codec.h>

#undef corb_read
#undef corb_write

int fd;
void *mixer = NULL;
int trace = 0;

int cad = -1;

int
corb_write (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	    unsigned int verb, unsigned int parm)
{
  unsigned int tmp;

  tmp = (cad << 28) | (d << 27) | (nid << 20) | (verb << 8) | parm;
  if (trace)
    printf ("WRITE %08x\n", tmp);

  if (ioctl (fd, HDA_IOCTL_WRITE, &tmp) == -1)
    {
      /* perror("HDA_IOCTL_WRITE"); */
      return 0;
    }

  return 1;
}

static int
corb_read (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	   unsigned int verb, unsigned int parm, unsigned int *upper,
	   unsigned int *lower)
{
  unsigned int tmp;

  tmp = (cad << 28) | (d << 27) | (nid << 20) | (verb << 8) | parm;

  if (ioctl (fd, HDA_IOCTL_READ, &tmp) == -1)
    {
      /* perror("HDA_IOCTL_READ"); */
      if (errno == EINVAL)
	{
	  fprintf (stderr, "hdaudio_snoopy mode is not available\n");
	  exit (-1);
	}
      return 0;
    }

  *upper = tmp;
  *lower = 0;
  if (trace)
    printf ("READ %08x\n", tmp);

  return 1;
}

static void
dump_verbs (int ix, int ntype, int clen, unsigned int wcaps)
{
  int i;
  unsigned int a, b, sel, state, fmt, spd, pstate, conv, sdi;
  unsigned int unsol, pin, sense, eapd, beep, def, stripe;

  static const int bits[8] = { 8, 16, 20, 24, 32 };
  static const char *vrefs[8] =
    { "Hi-Z", "50%", "Ground (0V)", "Reserved", "80%", "100%", "Reserved",
    "Reserved"
  };

  if (ntype == NT_ADC || ntype == NT_SELECT || ntype == NT_PIN)
    if (corb_read (mixer, cad, ix, 0, GET_SELECTOR, 0, &sel, &b))
      printf ("\tConnection select (701) = %08x\n", sel);

  if (ntype == NT_ADC || ntype == NT_DAC || ntype == NT_SELECT
      || ntype == NT_PIN)
    if (corb_read (mixer, cad, ix, 0, GET_PROCESSING_STATE, 0, &state, &b))
      printf ("\tProcessing state (703) = %08x\n", state);

  if (ntype == NT_ADC || ntype == NT_DAC)
    if (corb_read (mixer, cad, ix, 0, GET_CONVERTER_FORMAT, 0, &fmt, &b))
      {
	printf ("\tConverter format (00A) = %04x\n", fmt);
	printf ("\t\tNon-PCM %d\n", (fmt >> 15) & 0x01);
	printf ("\t\tSR base 44.1 kHz %d\n", (fmt >> 14) & 0x01);
	printf ("\t\tSR multiplier %d\n", ((fmt >> 11) & 0x07) + 1);
	printf ("\t\tSR divider %d\n", ((fmt >> 8) & 0x07) + 1);
	printf ("\t\tBits %d\n", bits[(fmt >> 4) & 0x07]);
	printf ("\t\tChannels %d\n", (fmt & 0x0f) + 1);
      }

  if (ntype == NT_ADC || ntype == NT_DAC)
    if (corb_read (mixer, cad, ix, 0, GET_CONVERTER, 0, &conv, &b))
      {
	printf ("\tConverter control (F06) %02x\n", conv);
	printf ("\t\tStream %d\n", (conv >> 4) & 0xf);
	printf ("\t\tChannel %d\n", (conv >> 0) & 0xf);
      }

  if (ntype == NT_PIN)
    {
      if (corb_read (mixer, cad, ix, 0, GET_PINCTL, 0, &pin, &b))
	{
	  printf ("\tPin widget control (F07) %02x\n", pin);
	  printf ("\t\tOutput amp enable %d\n", (pin >> 7) & 1);
	  printf ("\t\tOutput enable %d\n", (pin >> 6) & 1);
	  printf ("\t\tInput enable %d\n", (pin >> 5) & 1);
	  printf ("\t\tVRef enable %d %s\n", pin & 7, vrefs[pin & 7]);
	}

      corb_write (mixer, cad, ix, 0, TRIGGER_PIN_SENSE, 0);
      usleep (100 * 1000);

      if (corb_read (mixer, cad, ix, 0, GET_PIN_SENSE, 0, &sense, &b))
	{
	  printf ("\tPin sense (F09) %08x\n", sense);
	  printf ("\t\tPresence detect %d\n", (sense >> 31) & 1);
	  printf ("\t\tImpedance sense bits %08x\n", sense & ~0x80000000);
	}

      if (corb_read (mixer, cad, ix, 0, GET_CONFIG_DEFAULT, 0, &def, &b))
	{
	  int v;

	  printf ("\tConfiguration default (F1C) %08x\n", def);

	  /* Port connectivity */
	  v = (def >> 30) & 3;
	  printf ("\t\tPort connectivity %d ", v);
	  switch (v)
	    {
	    case 0:
	      printf ("Jack (1/8\", ATAPI, etc)");
	      break;
	    case 1:
	      printf ("No physical connection for port");
	      break;
	    case 2:
	      printf
		("A fixed function device (integrated speaker, mic, etc)");
	      break;
	    case 3:
	      printf ("Both integrated device and a jack is connected");
	      break;
	    }
	  printf ("\n");

	  /* Location */
	  v = (def >> 24) & 0x3f;
	  printf ("\t\tLocation %02x ", v);
	  switch (v >> 4)
	    {
	    case 0:
	      printf ("External or primary chassis ");
	      break;
	    case 1:
	      printf ("Internal ");
	      break;
	    case 2:
	      printf ("Separate chassis ");
	      break;
	    case 3:
	      printf ("Other ");
	      break;
	    }
	  switch (v & 0xf)
	    {
	    case 0:
	      printf ("N/A");
	      break;
	    case 1:
	      printf ("rear");
	      break;
	    case 2:
	      printf ("front");
	      break;
	    case 3:
	      printf ("left");
	      break;
	    case 4:
	      printf ("right");
	      break;
	    case 5:
	      printf ("top");
	      break;
	    case 6:
	      printf ("bottom");
	      break;
	    case 7:
	      printf ("special");
	      break;
	    case 8:
	      printf ("special");
	      break;
	    case 9:
	      printf ("special");
	      break;
	    default:
	      printf ("reserved");
	      break;
	    }
	  printf ("\n");

	  /* Default device */
	  v = (def >> 20) & 0xf;
	  printf ("\t\tDefault device %d ", v);
	  switch (v)
	    {
	    case 0x0:
	      printf ("Line out");
	      break;
	    case 0x1:
	      printf ("Speaker");
	      break;
	    case 0x2:
	      printf ("Headphone out");
	      break;
	    case 0x3:
	      printf ("CD");
	      break;
	    case 0x4:
	      printf ("SPDIF out");
	      break;
	    case 0x5:
	      printf ("Digital other out");
	      break;
	    case 0x6:
	      printf ("Modem line side");
	      break;
	    case 0x7:
	      printf ("Modem handset side");
	      break;
	    case 0x8:
	      printf ("Line in");
	      break;
	    case 0x9:
	      printf ("AUX");
	      break;
	    case 0xa:
	      printf ("Mic in");
	      break;
	    case 0xb:
	      printf ("Telephony");
	      break;
	    case 0xc:
	      printf ("SPDIF in");
	      break;
	    case 0xd:
	      printf ("Digital other in");
	      break;
	    case 0xe:
	      printf ("Reserved");
	      break;
	    case 0xf:
	      printf ("Other");
	      break;
	    }
	  printf ("\n");

	  /* Cpnnection type */
	  v = (def >> 16) & 0x0f;
	  printf ("\t\tConnection type %x ", v);
	  switch (v)
	    {
	    case 0x0:
	      printf ("Unknown");
	      break;
	    case 0x1:
	      printf ("1/8\" stereo/mono");
	      break;
	    case 0x2:
	      printf ("1/4\" stereo/mono");
	      break;
	    case 0x3:
	      printf ("ATAPI internal");
	      break;
	    case 0x4:
	      printf ("RCA");
	      break;
	    case 0x5:
	      printf ("Optical");
	      break;
	    case 0x6:
	      printf ("Other digital");
	      break;
	    case 0x7:
	      printf ("Other analog");
	      break;
	    case 0x8:
	      printf ("Multichannel analog (DIN)");
	      break;
	    case 0x9:
	      printf ("XLR/professional");
	      break;
	    case 0xa:
	      printf ("RJ-11 (modem)");
	      break;
	    case 0xb:
	      printf ("Combination");
	      break;
	    case 0xc:
	      printf ("Reserved");
	      break;
	    case 0xd:
	      printf ("Reserved");
	      break;
	    case 0xe:
	      printf ("Reserved");
	      break;
	    case 0xf:
	      printf ("Other");
	      break;
	    }
	  printf ("\n");

	  /* Color */
	  v = (def >> 12) & 0xf;
	  printf ("\t\tColor %d ", v);
	  switch (v)
	    {
	    case 0x0:
	      printf ("Unknown");
	      break;
	    case 0x1:
	      printf ("Black");
	      break;
	    case 0x2:
	      printf ("Grey");
	      break;
	    case 0x3:
	      printf ("Blue");
	      break;
	    case 0x4:
	      printf ("Green");
	      break;
	    case 0x5:
	      printf ("Red");
	      break;
	    case 0x6:
	      printf ("Orange");
	      break;
	    case 0x7:
	      printf ("Yellow");
	      break;
	    case 0x8:
	      printf ("Purple");
	      break;
	    case 0x9:
	      printf ("Pink");
	      break;
	    case 0xa:
	      printf ("Reserved");
	      break;
	    case 0xb:
	      printf ("Reserved");
	      break;
	    case 0xc:
	      printf ("Reserved");
	      break;
	    case 0xd:
	      printf ("Reserved");
	      break;
	    case 0xe:
	      printf ("White");
	      break;
	    case 0xf:
	      printf ("Other");
	      break;
	    }
	  printf ("\n");

	  /* Mixc */
	  v = (def >> 8) & 0xf;
	  printf ("\t\tMisc 0x%x ", v);
	  if (v & 0x1)
	    printf ("JackDetectOverride ");
	  if (v & 0x2)
	    printf ("reserved1 ");
	  if (v & 0x4)
	    printf ("reserved2 ");
	  if (v & 0x8)
	    printf ("reserved3 ");
	  printf ("\n");

	  /* Default association */
	  v = (def >> 4) & 0xf;
	  printf ("\t\tDefault association %d ", v);
	  printf ("\n");

	  /* Sequence */
	  v = (def >> 0) & 0xf;
	  printf ("\t\tSequence %d ", v);
	  printf ("\n");
	}
    }

  if (corb_read (mixer, cad, ix, 0, GET_EAPD, 0, &eapd, &b))
    {
      printf ("\tEAPD/BTL enable (F0C) %08x\n", eapd);
      printf ("\t\tBTL %d\n", (eapd >> 0) & 1);
      printf ("\t\tEAPD %d\n", (eapd >> 1) & 1);
      printf ("\t\tL/R swap %d\n", (eapd >> 2) & 1);
    }

  if (ntype == NT_ADC)
    if (corb_read (mixer, cad, ix, 0, GET_SDI_SELECT, 0, &sdi, &b))
      {
	printf ("\tSDI Select (F04) %d\n", sdi & 0x0f);
      }

  if (ntype == NT_DAC)
    if (corb_read (mixer, cad, ix, 0, GET_STRIPE_CONTROL, 0, &stripe, &b))
      {
	printf ("\tStripe control (F24) %d\n", stripe);
      }


  if (ntype == NT_BEEP)
    if (corb_read (mixer, cad, ix, 0, GET_BEEP, 0, &beep, &b))
      {
	beep &= 0xff;

	if (beep == 0)
	  printf ("\tBeep OFF\n");
	else
	  printf ("Beep %d Hz\n", 48000 / beep);
      }

  if (wcaps & (1 << 7))
    if (corb_read (mixer, cad, ix, 0, GET_UNSOLICITED, 0, &unsol, &b))
      {
	printf ("\tUnsolicited response (F08) %02x\n", unsol);
	printf ("\t\tEnabled %d\n", (unsol >> 7) & 1);
	printf ("\t\tTag %d\n", unsol & 0x7f);
      }

  if (ntype == NT_ADC || ntype == NT_DAC)
    if (wcaps & (1 << 9))	/* Digital capable */
      if (corb_read (mixer, cad, ix, 0, GET_SPDIF_CONTROL, 0, &spd, &b))
	{
	  printf ("\tS/PDIF converter control (F0D) %08x\n", spd);
	  printf ("\t\tCategory code (CC) %02x\n", (spd >> 8) & 0x3f);
	  printf ("\t\tGeneration level (L) %d\n", (spd >> 7) & 1);
	  printf ("\t\tPRO %d\n", (spd >> 6) & 1);
	  printf ("\t\t/AUDIO %d\n", (spd >> 5) & 1);
	  printf ("\t\tCOPY %d\n", (spd >> 4) & 1);
	  printf ("\t\tPreemphasis (PRE) %d\n", (spd >> 3) & 1);
	  printf ("\t\tValidity config (VCFG) %d\n", (spd >> 2) & 1);
	  printf ("\t\tValidity FLAG (V) %d\n", (spd >> 1) & 1);
	  printf ("\t\tDigital Enable (DigEn)  %d\n", (spd >> 0) & 1);
	}

  if (corb_read (mixer, cad, ix, 0, GET_POWER_STATE, 0, &pstate, &b))
    printf ("\tPower state (F05) D%d\n", pstate & 3);


  if (wcaps & (1 << 2))		/* Output amp present */
    {
      if (corb_read (mixer, cad, ix, 0, GET_GAIN (1, 0), 0, &a, &b))
	printf ("\tLeft output gain %02x\n", a);
      else
	printf ("\tLeft output gain read error\n");
      if (corb_read (mixer, cad, ix, 0, GET_GAIN (1, 1), 0, &a, &b))
	printf ("\tRight output gain %02x\n", a);
      else
	printf ("\tRight output gain read error\n");
    }

  if (wcaps & (1 << 1))		/* Input amp(s) present */
    for (i = 0; i < clen; i++)
      {
	if (corb_read (mixer, cad, ix, 0, GET_GAIN (0, 0), 0, &a, &b))
	  printf ("\tInput gain %2d %02x", i, a);
	else
	  printf ("\tInput gain %2d - Read error\n");
	if (corb_read (mixer, cad, ix, 0, GET_GAIN (0, 1), 0, &a, &b))
	  printf (", %02x", a);
	else
	  printf (", Read error");
	printf ("\n");
      }
}

static void
dump_node (int ix)
{
  int i;
  unsigned int a, b, gtype, gcaps, wcaps, sizes, fmts, pincaps;
  unsigned int inamp_caps, outamp_caps, clen, pstates, pcaps, gpio_count;
  unsigned int vkcaps;
  hda_name_t name;
  int first_node = 0, num_nodes = 0;
  char *s;
  int ntype = -1;

  memset (&name, 0, sizeof (name));

  name.cad = cad;
  name.wid = ix;
  ioctl (fd, HDA_IOCTL_NAME, &name);

  printf ("\n*** Widget 0x%02x (%d) name=%s\n", ix, ix, name.name);

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_GROUP_TYPE, &gtype, &b))
    if ((gtype & 0x1ff) != 0)
      {
	s = "Unknown";

	switch (gtype & 0xff)
	  {
	  case 0:
	    s = "Reserved";
	    break;
	  case 1:
	    s = "Audio function group";
	    break;
	  case 2:
	    s = "Vendor defined modem function group";
	    break;
	  }

	printf ("\tGroup type=%08x (%s), UnSol capable %d\n", gtype & 0xff, s,
		(gtype >> 8) & 1);
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_AUDIO_GROUP_CAPS, &gcaps, &b))
    if ((gcaps & 0xffff) != 0)
      {
	printf ("\tAudio group capabilities %08x\n", gcaps);
	printf ("\t\tBeep gen %d\n", (gcaps >> 16) & 1);
	printf ("\t\tInput delay %d\n", (gcaps >> 8) & 0xf);
	printf ("\t\tOutput delay %d\n", (gcaps) & 0xf);
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_WIDGET_CAPS, &wcaps, &b))
    if (wcaps != 0)
      {
	int type;

	printf ("\tAudio widget capabilities %08x\n", wcaps);

	type = (wcaps >> 20) & 0xf;
	s = "Unknown";

	switch (type)
	  {
	  case 0:
	    s = "Audio output converter (DAC)";
	    break;
	  case 1:
	    s = "Audio input converter (ADC)";
	    break;
	  case 2:
	    s = "Audio mixer";
	    break;
	  case 3:
	    s = "Audio selector";
	    break;
	  case 4:
	    s = "Pin complex";
	    break;
	  case 5:
	    s = "Power widget";
	    break;
	  case 6:
	    s = "Volume knob widget";
	    break;
	  case 7:
	    s = "Beep generator widget";
	    break;
	  case 0xf:
	    s = "Vendor defined";
	    break;
	  }

	ntype = type;

	printf ("\t\tType %x (%s)\n", type, s);
	printf ("\t\tDelay %d\n", (wcaps >> 16) & 0xf);
	printf ("\t\tL-R Swap %d\n", (wcaps >> 11) & 1);
	printf ("\t\tPowerControl %d\n", (wcaps >> 10) & 1);
	printf ("\t\tDigital %d\n", (wcaps >> 9) & 1);
	printf ("\t\tConnList %d\n", (wcaps >> 8) & 1);
	printf ("\t\tUnsol capable %d\n", (wcaps >> 7) & 1);
	printf ("\t\tProc Widget %d\n", (wcaps >> 6) & 1);
	printf ("\t\tStripe %d\n", (wcaps >> 5) & 1);
	printf ("\t\tFormat Override %d\n", (wcaps >> 3) & 1);
	printf ("\t\tAmp param Override %d\n", (wcaps >> 3) & 1);
	printf ("\t\tOut amp present %d\n", (wcaps >> 2) & 1);
	printf ("\t\tIn amp present %d\n", (wcaps >> 1) & 1);
	printf ("\t\tStereo %d\n", (wcaps >> 0) & 1);

      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_CONNLIST_LEN, &clen, &b))
    if (clen != 0)
      {
	unsigned int clist;
	int j;

	printf ("\tConnection list len %d (longform=%d): ", clen & 0x7f,
		clen >> 7);

	clen &= 0x7f;

	for (i = 0; i < clen; i += 4)
	  if (corb_read
	      (mixer, cad, ix, 0, GET_CONNECTION_LIST_ENTRY, i, &clist, &b))
	    for (j = 0; j < 4 && (i + j) < clen; j++)
	      {
		printf ("0x%x ", (clist >> (j * 8)) & 0xff);
	      }

	printf ("\n");
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_PIN_CAPS, &pincaps, &b))
    if (pincaps != 0)
      {
	printf ("\tPin capabilities\n");
	if (pincaps & (1 << 16))
	  printf ("\t\tEAPD capable\n");
	printf ("\t\tVref control %02x\n", (pincaps >> 8) & 0xff);
	if (pincaps & (1 << 6))
	  printf ("\t\tBalanced I/O pins\n");
	if (pincaps & (1 << 5))
	  printf ("\t\tInput capable\n");
	if (pincaps & (1 << 4))
	  printf ("\t\tOutput capable\n");
	if (pincaps & (1 << 3))
	  printf ("\t\tHeadphone drive capable\n");
	if (pincaps & (1 << 2))
	  printf ("\t\tPrecense detect capable\n");
	if (pincaps & (1 << 1))
	  printf ("\t\tTrigger required\n");
	if (pincaps & (1 << 0))
	  printf ("\t\tImpedance sense capable\n");
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_STREAM_FMTS, &fmts, &b))
    if (fmts != 0)
      {
	printf ("\tSupported stream formats %08x\n", fmts);
	if (fmts & 0x01)
	  printf ("\t\tPCM\n");
	if (fmts & 0x02)
	  printf ("\t\tFloat32\n");
	if (fmts & 0x04)
	  printf ("\t\tAC3\n");
      }

  if (corb_read (mixer, cad, ix, 0, GET_PARAMETER, HDA_PCM_SIZES, &sizes, &b))
    if (sizes != 0)
      {
	printf ("\tSupported sample sizes/rates %08x\n", sizes);

	if (sizes & (1 << 20))
	  printf ("\t\t32 bits\n");
	if (sizes & (1 << 19))
	  printf ("\t\t24 bits\n");
	if (sizes & (1 << 18))
	  printf ("\t\t20 bits\n");
	if (sizes & (1 << 17))
	  printf ("\t\t16 bits\n");
	if (sizes & (1 << 16))
	  printf ("\t\t8 bits\n");

	if (sizes & (1 << 11))
	  printf ("\t\t384 kHz\n");
	if (sizes & (1 << 10))
	  printf ("\t\t192 kHz\n");
	if (sizes & (1 << 9))
	  printf ("\t\t176.4 kHz\n");
	if (sizes & (1 << 8))
	  printf ("\t\t96 kHz\n");
	if (sizes & (1 << 7))
	  printf ("\t\t88.2 kHz\n");
	if (sizes & (1 << 6))
	  printf ("\t\t48 kHz\n");
	if (sizes & (1 << 5))
	  printf ("\t\t44.1 kHz\n");
	if (sizes & (1 << 4))
	  printf ("\t\t32 kHz\n");
	if (sizes & (1 << 3))
	  printf ("\t\t22.05 kHz\n");
	if (sizes & (1 << 2))
	  printf ("\t\t16 kHz\n");
	if (sizes & (1 << 1))
	  printf ("\t\t11.025 kHz\n");
	if (sizes & (1 << 0))
	  printf ("\t\t8 kHz\n");
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_INPUTAMP_CAPS, &inamp_caps, &b))
    if (inamp_caps != 0)
      {
	printf ("\tInput amp caps %08x\n", inamp_caps);
	printf ("\t\tMute Capable %d\n", (inamp_caps >> 31) & 0x1);
	printf ("\t\tStep size %d\n", (inamp_caps >> 16) & 0x7f);
	printf ("\t\tNum steps %d\n", (inamp_caps >> 8) & 0x7f);
	printf ("\t\tOffset %d\n", (inamp_caps >> 0) & 0x7f);
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_OUTPUTAMP_CAPS, &outamp_caps,
       &b))
    if (outamp_caps != 0)
      {
	printf ("\tOutput amp caps %08x\n", outamp_caps);
	printf ("\t\tMute Capable %d\n", (outamp_caps >> 31) & 0x1);
	printf ("\t\tStep size %d\n", (outamp_caps >> 16) & 0x7f);
	printf ("\t\tNum steps %d\n", (outamp_caps >> 8) & 0x7f);
	printf ("\t\tOffset %d\n", (outamp_caps >> 0) & 0x7f);
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_SUPPORTED_POWER_STATES, &pstates,
       &b))
    if (pstates != 0)
      {
	printf ("\tSupported power states %08x\n", pstates);
	if (pstates & (1 << 3))
	  printf ("\t\tD3\n");
	if (pstates & (1 << 2))
	  printf ("\t\tD2\n");
	if (pstates & (1 << 1))
	  printf ("\t\tD1\n");
	if (pstates & (1 << 0))
	  printf ("\t\tD0\n");
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_PROCESSING_CAPS, &pcaps, &b))
    if (pcaps != 0)
      {
	printf ("\tProcessing capabilities %08x\n", pcaps);
	printf ("\t\tNumCoeff %d\n", (pcaps >> 8) & 0xff);
	printf ("\t\tBening %d\n", pcaps & 1);
      }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_GPIO_COUNT, &gpio_count, &b))
    if (gpio_count != 0)
      {
	printf ("\tGPIO count %08x\n", gpio_count);
	printf ("\t\tGPIWake %d\n", (gpio_count >> 31) & 0x1);
	printf ("\t\tGPIUnsol %d\n", (gpio_count >> 30) & 0x1);
	printf ("\t\tNumGPIs %d\n", (gpio_count >> 16) & 0xff);
	printf ("\t\tNumGPOs %d\n", (gpio_count >> 8) & 0xff);
	printf ("\t\tNumGPIOs %d\n", (gpio_count >> 0) & 0xff);
      }

  if (ntype >= 0)
    dump_verbs (ix, ntype, clen, wcaps);
  else
    {
      if (corb_read (mixer, cad, ix, 0, GET_SUBSYSTEM_ID, 0, &a, &b))
	printf ("\tSubsystem ID %08x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPI_DATA, 0, &a, &b))
	printf ("\tGPI data %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPI_WAKE, 0, &a, &b))
	printf ("\tGPI wake mask %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPI_UNSOL, 0, &a, &b))
	printf ("\tGPI unsolicited mask %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPI_STICKY, 0, &a, &b))
	printf ("\tGPI sticky mask %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPO_DATA, 0, &a, &b))
	printf ("\tGPO data %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPIO_DATA, 0, &a, &b))
	printf ("\tGPIO data %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPIO_ENABLE, 0, &a, &b))
	printf ("\tGPIO enable %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPIO_DIR, 0, &a, &b))
	printf ("\tGPIO direction %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPIO_WKEN, 0, &a, &b))
	printf ("\tGPIO wake enable %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPIO_UNSOL, 0, &a, &b))
	printf ("\tGPIO unsolicited enable mask %02x\n", a);
      if (corb_read (mixer, cad, ix, 0, GET_GPIO_STICKY, 0, &a, &b))
	printf ("\tGPIO sticky mask %02x\n", a);
    }

  if (corb_read
      (mixer, cad, ix, 0, GET_PARAMETER, HDA_VOLUMEKNOB_CAPS, &vkcaps, &b))
    if ((vkcaps & 0xff) != 0)
      {
	printf ("\tVolume knob capabilities %08x\n", vkcaps);
      }

  if (corb_read (mixer, cad, ix, 0, GET_PARAMETER, HDA_NODE_COUNT, &a, &b))
    {
      first_node = (a >> 16) & 0xff;
      num_nodes = a & 0xff;
    }
  else
    num_nodes = 0;

  if (num_nodes > 0)
    {
      printf ("\tFirst node %02x, num nodes %d\n", first_node, num_nodes);

      for (i = first_node; i < first_node + num_nodes; i++)
	dump_node (i);
    }
}

int
main (int argc, char *argv[])
{
  unsigned int a, b;
  int first_node, num_nodes;
  int i;

  if ((fd = open ("/dev/oss/oss_hdaudio0/pcm0", O_RDWR | O_EXCL, 0)) == -1)
    {
      perror ("/dev/oss/oss_hdaudio0/pcm0");
      exit (-1);
    }

  if (argc > 1)
    cad = atoi (argv[1]);
  if (cad == -1)		/* Not given on command line so find it. */
    for (cad = 0; cad < 16; cad++)
      if (corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_VENDOR, &a, &b))
	break;

  printf ("Codec index is %d\n", cad);
  printf ("Codec vendor %04x:%04x\n", a >> 16, a & 0xffff);

  if (corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_REVISION, &a, &b))
    {
      printf ("HD codec revision %d.%d (%d.%d) (0x%08x)\n",
	      (a >> 20) & 0xf, (a >> 16) & 0xf, (a >> 8) & 0xff, a & 0xff, a);
    }
  else
    printf ("hdaudio: Can't get codec revision\n");

/*
 * Find out the primary group list
 */

  if (!corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_NODE_COUNT, &a, &b))
    exit(1);

  first_node = (a >> 16) & 0xff;
  num_nodes = a & 0xff;

  printf ("First node %02x, num nodes %d\n", first_node, num_nodes);

  for (i = first_node; i < first_node + num_nodes; i++)
    dump_node (i);

  exit (0);
}
