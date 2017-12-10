#ifdef CONFIG_OSS_VMIX_FLOAT
/*
 * Purpose: Local output buffer to device export routine for vmix (FP version)
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

int i, ch, och;
double vol;

vol = vmix_db_table[eng->outvol / 5];

for (ch = 0; ch < channels; ch++)
  {
    double vu;
    float *chbuf;

    och = eng->channel_order[ch];
    op = (SAMPLE_TYPE *) outbuf;
    op += och;

    chbuf = chbufs[ch];

    vu = eng->vu[och % 2];
    vu = vu / 255.0;

    for (i = 0; i < samples; i++)
      {
	double tmp;

#if 0 && defined(SINE_DEBUG)
	tmp = sine_table[sine_phase[ch]];
	sine_phase[ch] = (sine_phase[ch] + 1 + ch / 2) % SINE_SIZE;
#else
	tmp = *chbuf++;
#endif

	tmp *= vol;

/*
 * Check for clipping. Decrease volume if necessary.
 */
	if (tmp < -1.0)
	  {
	    vol /= -tmp;
	    eng->outvol--;
	    tmp = -1.0;
	  }
	else if (tmp > 1.0)
	  {
	    vol /= tmp;
	    eng->outvol--;
	    tmp = 1.0;
	  }

	*op = VMIX_BYTESWAP ((SAMPLE_TYPE) (tmp * SAMPLE_RANGE));
	op += channels;

	/* VU meter */
	if (tmp < 0.0)
	  tmp = -tmp;
	if (tmp > vu)
	  vu = tmp;
      }

    if (och < 2)
      {
	vu = vu * 255.0;
	eng->vu[och] = (int)vu;
      }
  }
#else
#include "outexport_int.inc"
#endif
