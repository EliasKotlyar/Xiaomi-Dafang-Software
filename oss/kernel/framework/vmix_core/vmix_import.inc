#ifdef CONFIG_OSS_VMIX_FLOAT
/*
 * Purpose: Recording device to local input buffer import routine for vmix
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

int i, ch;
float vol;

vol = vmix_db_table[eng->outvol / 5];

for (ch = 0; ch < channels; ch++)
  {
    float vu;
    float *chbuf;

    vu = eng->vu[ch % 2];
    vu = vu / 255.0;

    op = (SAMPLE_TYPE *) inbuf;
    op += ch;

    chbuf = chbufs[ch];

    for (i = 0; i < samples; i++)
      {
	float tmp;

#if 0 && defined(SINE_DEBUG)
	/* Generate internal sine wave test signal */
	tmp = 0;
	if (ch < 2)
	  {
	    tmp = sine_table[sine_phase[ch]];
	    sine_phase[ch] = (sine_phase[ch] + 1) % SINE_SIZE;
	  }
#else
	tmp = VMIX_BYTESWAP (*op);
	tmp /= SAMPLE_RANGE;
	tmp *= vol;

	if (tmp < -1.0)
	  tmp = -1.0;
	else if (tmp > 1.0)
	  tmp = 1.0;

#endif
	op += channels;

	*chbuf++ = tmp;

	/* VU meter */
	if (tmp < 0.0)
	  tmp = -tmp;
	if (tmp > vu)
	  vu = tmp;
      }

    if (ch < 2)
      {
	vu = vu * 255.0;
	eng->vu[ch] = (int)vu;
      }
  }
#else
#include "vmix_import_int.inc"
#endif
