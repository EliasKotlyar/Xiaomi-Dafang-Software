/*
 * ddksample_audio.c -  OSS DDK sample driver - misc routines
 *
 * Description:
 * This file contains routines that are not related with the
 * OSS DDK interface. To understand how OSS DDK works you don't need to
 * pay any attention on these routines. They are here just to produce
 * some data for the mixer routines (ddksample_mixer.c).
 *
 * ddksample_misc.c emulates some arbitrary audio device. It does
 * volume scaling to the output data and computes the peak meters from the
 * result of the scaling.
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
 * Solaris DDI includes
 */
#include <sys/types.h>
#include <sys/modctl.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

/*
 * OSS specific includes
 */
#include <sys/soundcard.h>
#include <sys/ossddk/ossddk.h>

#include "ddksample.h"

/**************************************************/
void
ddksample_do_math (ddksample_portc * portc, void *buf, int len)
{
  int i;
  ddksample_devc *devc = ossddk_adev_get_devc (portc->dev);

  switch (portc->bits + portc->channels)
    {
    case 16 + 1:		/* 16 bits / mono */
      {
	int val;
	int peak = 0;
	short *p = buf;

	len /= sizeof (*p);

	for (i = 0; i < len; i++)
	  {
	    /* Do volume computations */

	    val = *p++ << 8;	/* Scale up to 24 bits */

	    val = (val * portc->left_volume) / DDKSAMPLE_MAX_VOL;

	    /*
	     * Note that legacy mixer volume max is always
	     * 100.
	     */
	    val = (val * devc->mainvol_left) / 100;

	    /*
	     * Now we have the sample value after volume control.
	     * This driver doesn't store this value anywhere but
	     * if necessary this functionality can be added here.

	     /*
	     * Next compute the peak value
	     */

	    if (val < 0)
	      val = -val;	/* Absolute value */

	    if (val > peak)
	      peak = val;
	  }

	if (peak > portc->left_peak)
	  portc->left_peak = peak;
	if (peak > portc->right_peak)
	  portc->right_peak = peak;
      }
      break;

    case 16 + 2:		/* 16 bits / stereo */
      {
	int val;
	int left_peak = 0, right_peak = 0;
	short *p = buf;

	len /= sizeof (*p);

	for (i = 0; i < len; i += 2)	/* Each stereo sa,ple pair */
	  {
	    /*
	     * Left channel
	     */

	    /* Do volume computations */

	    val = (*p++) << 8;	/* Scale up to 24 bits */

	    val = (val * portc->left_volume) / DDKSAMPLE_MAX_VOL;

	    /*
	     * Note that legacy mixer volume max is always
	     * 100.
	     */
	    val = (val * devc->mainvol_left) / 100;

	    /*
	     * Now we have the sample value after volume control.
	     * This driver doesn't store this value anywhere but
	     * if necessary this functionality can be added here.

	     /*
	     * Next compute the peak value
	     */

	    if (val < 0)
	      val = -val;	/* Absolute value */

	    if (val > left_peak)
	      left_peak = val;

	    /*
	     * Right channel
	     */

	    /* Do volume computations */

	    val = (*p++) << 8;	/* Scale up to 24 bits */

	    val = (val * portc->left_volume) / DDKSAMPLE_MAX_VOL;

	    /*
	     * Note that legacy mixer volume max is always
	     * 100.
	     */
	    val = (val * devc->mainvol_left) / 100;

	    /*
	     * Now we have the sample value after volume control.
	     * This driver doesn't store this value anywhere but
	     * if necessary this functionality can be added here.

	     /*
	     * Next compute the peak value
	     */

	    if (val < 0)
	      val = -val;	/* Absolute value */

	    if (val > right_peak)
	      right_peak = val;
	  }

	if (left_peak > portc->left_peak)
	  portc->left_peak = left_peak;
	if (right_peak > portc->right_peak)
	  portc->right_peak = right_peak;
      }
      break;
    }
}
