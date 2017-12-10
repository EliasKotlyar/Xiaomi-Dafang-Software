/*
 * Purpose: Application to local playback buffer resampling routine for vmix
 *
 * This file is almost the same than playmix.inc but uses simple
 * linear interpolation algorithm for sample rate conversion.
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
double vol, step;
double tmp;

/*
 * Initial setup
 */
step = (double) portc->rate / (double) mixer->play_engine.rate;

#if 1
/*
 * TODO:
 * For some reason "trivial" conversion rations such as 4:1, 2:1, 1:2 and 1:4
 * don't work properly. As a workaround we add a tiny error to break the
 * evil ratio. Need to find out why the algorithm doesn't work with such ratios.
 */
	if (step==4.0)step=4.0001;
	if (step==2.0)step=2.0001;
	if (step==0.5)step=0.5001;
	if (step==0.25)step=0.25001;
#endif

frame_size = sizeof (*inp);

inmax = dmap->bytes_in_use / frame_size / portc->channels;
inptr = (int)(portc->play_dma_pointer_src / frame_size / portc->channels);

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
    double ip = inptr + (ch%used_channels);
    double vu = portc->vu[ch % 2];
    float *chbuf = eng->chbufs[ch+portc->play_choffs];

    inp = (BUFFER_TYPE) dmap->dmabuf;
    inp += ch;

    i = portc->volume[ch%2];
    vol = vmix_db_table[i / 5];

    vu = vu / 255.0;

    for (i = 0; i < nsamples; i++)
      {
	double frag, tmp_next;
	int ip_next;

#if 0 && defined(SINE_DEBUG)
	if (ch > 1)
	  tmp = 0.0;
	else
	  tmp = sine_table[sine_phase[ch]];
	sine_phase[ch] = (sine_phase[ch] + 1) % SINE_SIZE;
#else
	/*
	 * Convert the sample to right endianess. 
	 */
	tmp = VMIX_BYTESWAP (inp[((int) ip) * portc->channels]);

	/* perform linear interpolation */
	ip_next = (((int) ip) + 1) % inmax;
	tmp_next = VMIX_BYTESWAP (inp[ip_next * portc->channels]);
	frag = ip - (int) ip;	/* Pointer fraction */
	tmp += (tmp_next - tmp) * frag;
	tmp /= 2.0;

	tmp = tmp * range;
#endif
	tmp = tmp * vol;

	*chbuf++ += tmp;
	ip = ip + step;
	if (ip > inmax)
	  ip -= inmax;

	/* VU meter */
	if (tmp < 0.0)
	  tmp = -tmp;
	if (tmp > vu)
	  vu = tmp;
      }

    if (ch < 2)
      {				/* Save left/right VU meters */
	vu = vu * 255.0;
	portc->vu[ch] = (int)vu;
      }
  }

/*
 * Finally save the state variables
 */
tmp = (nsamples * frame_size * portc->channels);

tmp *= step;

portc->play_dma_pointer_src = portc->play_dma_pointer_src + tmp;

if (portc->play_dma_pointer_src > dmap->bytes_in_use)
  portc->play_dma_pointer_src -= dmap->bytes_in_use;

portc->play_dma_pointer = (int)portc->play_dma_pointer_src;

frame_size *= portc->channels;

portc->play_dma_pointer = (portc->play_dma_pointer / frame_size) * frame_size;

