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

snd_config_t *snd_config = NULL;

const char *
snd_strerror (int errnum)
{
  if (errnum < 0)
    errnum = -errnum;

  return strerror (errnum);;
}

/**
 * \brief Dumps the contents of a configuration node or tree.
 * \param config Handle to the (root) configuration node.
 * \param out Output handle.
 * \return Zero if successful, otherwise a negative error code.
 */
int
snd_config_save (snd_config_t * config, snd_output_t * out)
{
  dbg_printf ("snd_config_save()\n");

  return 0;
}

/**
 * \brief Searches for a node in a configuration tree.
 * \param config Handle to the root of the configuration (sub)tree to search.
 * \param key Search key: one or more node keys, separated with dots.
 * \param result The function puts the handle to the node found at the address
 *               specified by \p result.
 * \return Zero if successful, otherwise a negative error code.
 */
int
snd_config_search (snd_config_t * config, const char *key,
		   snd_config_t ** result)
{
  dbg_printf ("snd_config_search()\n");

  return 0;
}

/**
 * \brief Updates #snd_config by rereading the global configuration files (if needed).
 * \return A non-negative value if successful, otherwise a negative error code. * \retval 0 No action is needed.
 * \retval 1 The configuration tree has been rebuilt.
 *
 * The global configuration files are specified in the environment variable
 * \c ALSA_CONFIG_PATH. If this is not set, the default value is
 * "/usr/share/alsa/alsa.conf".
 *
 * \warning If the configuration tree is reread, all string pointers and
 * configuration node handles previously obtained from this tree become invalid.
 */
int
snd_config_update (void)
{
  dbg_printf ("snd_config_update()\n");

  return 0;
}

/**
 * \brief Frees the global configuration tree in #snd_config.
 * \return Zero if successful, otherwise a negative error code.
 */
int
snd_config_update_free_global (void)
{
  dbg_printf ("snd_config_update_free_global()\n");

  return 0;
}

/**
 * \brief Sets the error handler.
 * \param handler The pointer to the new error handler function.
 *
 * This function sets a new error handler, or (if \c handler is \c NULL)
 * the default one which prints the error messages to \c stderr.
 */
int
snd_lib_error_set_handler (snd_lib_error_handler_t handler)
{
  dbg_printf ("snd_lib_error_set_handler()\n");
  return 0;
}
