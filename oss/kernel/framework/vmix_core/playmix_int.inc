/*
 * Purpose: Application to local playback buffer import routine for vmix (int)
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

/*
 * Mixing function for virtual devices
 */

vmix_mixer_t *mixer = portc->mixer;
vmix_engine_t *eng = &mixer->play_engine;

dmap_t *dmap = audio_engines[portc->audio_dev]->dmap_out;

int frame_size;

int inptr, inmax;
int used_channels;
int i, ch, nch;
int vol;

/*
 * Initial setup
 */

frame_size = sizeof (*inp);

inmax = dmap->bytes_in_use / frame_size;
inptr = portc->play_dma_pointer / frame_size;

inp = (BUFFER_TYPE) dmap->dmabuf;

used_channels = portc->channels;
if (used_channels > eng->channels)
  used_channels = eng->channels;

/* ignored_channels = portc->channels - used_channels; */

/*
 * Handle mono playback by playing the mono stream twice (for left and right ch)
 */
nch=used_channels;
if (nch<2)nch=2;

/*
 * Do the mixing
 */
for (ch = 0; ch < nch; ch++)
  {
    int ip = inptr + (ch%used_channels);
    int vu = portc->vu[ch % 2];
    int *chbuf = eng->chbufs[ch+portc->play_choffs];

    i = portc->volume[ch%2];
    vol = vmix_db_table[i / 5];

    vu = vu * 65536;

    for (i = 0; i < nsamples; i++)
      {
	int tmp;

	/*
	 * Convert the sample to right endianess. 
	 */
	tmp = INT_OUTMIX(VMIX_BYTESWAP (inp[ip]));
	tmp = (tmp * vol) / VMIX_VOL_SCALE;

	*chbuf++ += tmp;
	ip = (ip + portc->channels) % inmax;

	/* VU meter */
	if (tmp < 0)
	  tmp = -tmp;
	if (tmp > vu)
	  vu = tmp;
      }

    if (ch < 2)
      {				/* Save left/right VU meters */
	vu = vu  / 65536;
	portc->vu[ch] = vu;
      }
  }

/*
 * Finally save the state variables
 */

portc->play_dma_pointer =
  (portc->play_dma_pointer +
   nsamples * frame_size * portc->channels) % dmap->bytes_in_use;
