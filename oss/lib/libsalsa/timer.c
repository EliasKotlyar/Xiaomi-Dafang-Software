/*
 *  Copyright (c) 2004 by Hannu Savolainen < hannu@opensound.com>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License version 2.1 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <stdio.h>
#include "local.h"

struct _snd_timer
{
  int dummy;
};

struct _snd_timer_id
{
  int dummy;
};

struct _snd_timer_info
{
  int dummy;
};

/**
 * \brief Opens a new connection to the timer interface.
 * \param timer Returned handle (NULL if not wanted)
 * \param name ASCII identifier of the timer handle
 * \param mode Open mode
 * \return 0 on success otherwise a negative error code
 *
 * Opens a new connection to the timer interface specified with
 * an ASCII identifier and mode.
 */
int
snd_timer_open (snd_timer_t ** tmr, const char *name, int mode)
{
  snd_timer_t *timer;

  ALIB_INIT ();
  if (!alib_appcheck ())
    {
      dbg_printf ("snd_timer_open(%s, %x) refused,\n", name, mode);
      return -ENODEV;
    }

  timer = malloc (sizeof (*timer));

  dbg_printf ("snd_timer_open(name='%s', mode=%x)=%x\n", name, mode, timer);

  if (timer == NULL)
    return -ENOMEM;

  *tmr = timer;

  return 0;
}

/**
 * \brief close timer handle
 * \param timer timer handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified timer handle and frees all associated
 * resources.
 */
int
snd_timer_close (snd_timer_t * timer)
{
  dbg_printf ("snd_timer_close(%x)\n", timer);

  free (timer);
  return 0;
}

/**
 * \brief get size of the snd_timer_id_t structure in bytes
 * \return size of the snd_timer_id_t structure in bytes
 */
size_t
snd_timer_id_sizeof ()
{
  return sizeof (snd_timer_id_t);
}

/**
 * \brief get timer card
 * \param params pointer to #snd_timer_id_t structure
 * \return timer card number
 */
int
snd_timer_id_get_card (snd_timer_id_t * tid)
{
  dbg_printf ("snd_timer_id_get_card(tid=%x)\n", tid);

  return 0;			// TODO
}

/**
 * \brief get timer class
 * \param tid pointer to #snd_timer_id_t structure
 * \return timer class
 */
int
snd_timer_id_get_class (snd_timer_id_t * tid)
{
  dbg_printf ("snd_timer_id_get_class(tid=%x)\n", tid);

  return 0;			// TODO
}


/**
 * \brief get timer device
 * \param params pointer to #snd_timer_id_t structure
 * \return timer device number
 */
int
snd_timer_id_get_device (snd_timer_id_t * tid)
{
  dbg_printf ("snd_timer_id_get_device(tid=%x)\n", tid);

  return 0;			// TODO
}

/**
 * \brief get timer sub-class
 * \param params pointer to #snd_timer_id_t structure
 * \return timer sub-class
 */
int
snd_timer_id_get_sclass (snd_timer_id_t * tid)
{
  dbg_printf ("snd_timer_id_get_sclass(tid=%x)\n", tid);

  return 0;			// TODO
}

/**
 * \brief get timer subdevice
 * \param params pointer to #snd_timer_id_t structure
 * \return timer subdevice number
 */
int
snd_timer_id_get_subdevice (snd_timer_id_t * tid)
{
  dbg_printf ("snd_timer_id_get_subdevice(tid=%x)\n", tid);

  return 0;			// TODO
}

/**
 * \brief set timer card
 * \param tid pointer to #snd_timer_id_t structure
 * \param card card number
 */
void
snd_timer_id_set_card (snd_timer_id_t * tid, int card)
{
  dbg_printf ("snd_timer_id_set_card(tid=%x, card=%d)\n", tid, card);

  // TODO
}

/**
 * \brief set timer class
 * \param tid pointer to #snd_timer_id_t structure
 * \param dev_class class of timer device
 */
void
snd_timer_id_set_class (snd_timer_id_t * tid, int dev_class)
{
  dbg_printf ("snd_timer_id_set_class(tid=%x, dev_class=%d)\n", tid,
	      dev_class);
  // TODO
}

/**
 * \brief set timer device
 * \param tid pointer to #snd_timer_id_t structure
 * \param device device number
 */
void
snd_timer_id_set_device (snd_timer_id_t * tid, int device)
{
  dbg_printf ("snd_timer_id_set_device(tid=%x, device=%d)\n", tid, device);

  // TODO
}

/**
 * \brief set timer sub-class
 * \param tid pointer to #snd_timer_id_t structure
 * \param dev_sclass sub-class of timer device
 */
void
snd_timer_id_set_sclass (snd_timer_id_t * tid, int dev_sclass)
{
  dbg_printf ("snd_timer_id_set_sclass(tid=%x, dev_sclass=%d)\n",
	      tid, dev_sclass);
  // TODO
}

/**
 * \brief set timer subdevice
 * \param tid pointer to #snd_timer_id_t structure
 * \param subdevice subdevice number
 */
void
snd_timer_id_set_subdevice (snd_timer_id_t * tid, int subdevice)
{
  dbg_printf ("snd_timer_id_set_subdevice(tid=%x, subdevice=%d)\n",
	      tid, subdevice);

  // TODO
}

/**
 * \brief get size of the snd_timer_info_t structure in bytes
 * \return size of the snd_timer_info_t structure in bytes
 */
size_t
snd_timer_info_sizeof ()
{
  return sizeof (snd_timer_info_t);
}

/**
 * \brief allocate a new snd_timer_info_t structure
 * \param ptr returned pointer
 * \return 0 on success otherwise a negative error code if fails
 *
 * Allocates a new snd_timer_info_t structure using the standard
 * malloc C library function.
 */
int
snd_timer_info_malloc (snd_timer_info_t ** info)
{
  *info = calloc (1, sizeof (snd_timer_info_t));
  dbg_printf ("snd_timer_info_malloc()=%x\n", *info);
  if (!*info)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees the snd_timer_info_t structure
 * \param info pointer to the snd_timer_info_t structure to free
 *
 * Frees the given snd_timer_info_t structure using the standard
 * free C library function.
 */
void
snd_timer_info_free (snd_timer_info_t * info)
{
  dbg_printf ("snd_timer_info_free(%x)\n", info);
  free (info);
}

/**
 * \brief get information about timer handle
 * \param timer timer handle
 * \param info pointer to a snd_timer_info_t structure to be filled
 * \return 0 on success otherwise a negative error code
 */
int
snd_timer_info (snd_timer_t * timer, snd_timer_info_t * info)
{
  dbg_printf ("snd_timer_info(timer=%x, info=%x)\n", timer, info);

  // TODO

  return 0;
}

/**
 * \brief get timer name
 * \param info pointer to #snd_timer_info_t structure
 * \return timer name
 */
const char *
snd_timer_info_get_name (snd_timer_info_t * info)
{
  dbg_printf ("snd_timer_info_get_name(info=%x)\n", info);

  return "OSS Timer";		// TODO
}


/**
 * \brief get timer resolution in us
 * \param info pointer to #snd_timer_info_t structure
 * \return timer resolution
 */
long
snd_timer_info_get_resolution (snd_timer_info_t * info)
{
  dbg_printf ("snd_timer_info_get_resolution(info=%x)\n", info);

  return 1000;			// TODO
}


static int
snd_timer_query_open_conf (snd_timer_query_t ** timer,
			   const char *name, snd_config_t * timer_root,
			   snd_config_t * timer_conf, int mode)
{
  dbg_printf
    ("snd_timer_query_open_conf(name='%s', root=%x, conf=%x, mode=%x)\n",
     name, timer_root, timer_conf, mode);
  ALIB_INIT ();
  if (!alib_appcheck ())
    return -ENODEV;

  return -EIO;
}

/**
 * \brief close timer query handle
 * \param timer timer handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified timer handle and frees all associated
 * resources.
 */
int
snd_timer_query_close (snd_timer_query_t * timer)
{
  dbg_printf ("snd_timer_query_close(timer=%x)\n", timer);

  return 0;
}

/**
 * \brief obtain the next timer identification
 * \param timer timer handle
 * \param tid timer identification
 * \return 0 on success otherwise a negative error code
 *
 * if tid->dev_class is -1, then the first device is returned
 * if result tid->dev_class is -1, no more devices are left
 */
int
snd_timer_query_next_device (snd_timer_query_t * timer, snd_timer_id_t * tid)
{
  dbg_printf ("snd_timer_query_next_device(timer=%x, tid=%x)\n", timer, tid);

  return -1;
}
