/*
 * Purpose: Local output buffer to device export routine for vmix (int)
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
int vol;

#define RANGE_MAX	((1<<24)-1)
#define RANGE_MIN	-(1<<24)

vol = vmix_db_table[eng->outvol / 5];

for (ch = 0; ch < channels; ch++)
  {
    int vu;
    int *chbuf;

    och = eng->channel_order[ch];
    op = (SAMPLE_TYPE *) outbuf;
    op += och;

    chbuf = chbufs[ch];

    vu = eng->vu[och % 2];
    vu = vu * 65536;

    for (i = 0; i < samples; i++)
      {
	int tmp;

	tmp = *chbuf++;

	tmp = (tmp * vol) / VMIX_VOL_SCALE;

/*
 * Check for clipping. Decrease volume if necessary.
 */
	if (tmp<RANGE_MIN)
           {
	   	tmp=RANGE_MIN;
	        eng->outvol -= 1;
		vol /= 2;
	   }
	else
	   if (tmp>RANGE_MAX)
	      {
	   	tmp=RANGE_MAX;
	        eng->outvol -= 1;
		vol /= 2;
	      }
	
	*op = VMIX_BYTESWAP(INT_EXPORT(tmp));
	op += channels;

	/* VU meter */
	if (tmp < 0)
	  tmp = -tmp;
	if (tmp > vu)
	  vu = tmp;
      }

    if (och < 2)
      {
	vu = vu / 65536;
	eng->vu[och] = vu;
      }
  }
