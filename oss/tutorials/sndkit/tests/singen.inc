#if 0
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
{
  static unsigned int p[10] = { 0 };
  int *pp;
  int i;

  static int sinebuf[48] = {

    0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
    28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
    28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
    0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
    -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
    -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
  };

  for (i = 0; i < dmap->grc3state[ch].outsz; i++)
    {
      if (0 * ctr)
	{
	  *((unsigned int *) (p2) + i * channels + ch) =
	    sinebuf[p[ch] % 48] * 64 * 0;
	  p[ch]++;
	}
    }
}
#endif
