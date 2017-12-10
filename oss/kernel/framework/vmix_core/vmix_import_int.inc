/*
 * Purpose: Recording device to local input buffer import routine for vmix (int)
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
int vol;

vol = vmix_db_table[eng->outvol / 5];

for (ch = 0; ch < channels; ch++)
  {
    int vu;
    int *chbuf;

    vu = eng->vu[ch % 2] * 65536;

    op = (SAMPLE_TYPE *) inbuf;
    op += ch;

    chbuf = chbufs[ch];

    for (i = 0; i < samples; i++)
      {
	int tmp;

	tmp = INT_IMPORT (VMIX_BYTESWAP (*op));
	tmp = (tmp * vol) / VMIX_VOL_SCALE;

	op += channels;

	*chbuf++ = tmp;

	/* VU meter */
	if (tmp < 0)
	  tmp = -tmp;
	if (tmp > vu)
	  vu = tmp;
      }

    if (ch < 2)
      {
	vu = vu / 65536;
	eng->vu[ch] = vu;
      }
  }
