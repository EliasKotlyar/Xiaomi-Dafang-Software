/* Copyright (C) 2010
	Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _EGLIBC_DYNCONF_H_
#define _EGLIBC_DYNCONF_H_

#include <features.h>
#include <gnu/option-groups.h>

__BEGIN_DECLS

/* The dynamic version of BUFSIZ.  */
#define EGLIBC_CONF_BUFSIZ           0

/* The dynamic version of PTHREAD_STACK_MIN.  */
#define EGLIBC_CONF_THREAD_STACK_MIN 1

/* One more than the largest valid configuration name.  This value is
   subject to change in future versions of EGLIBC.  */
#define EGLIBC_CONF_MAX              2

#ifdef __OPTION_EGLIBC_DYNAMIC_CONF
# define EGLIBC_BUFSIZ (eglibc_getconf (EGLIBC_CONF_BUFSIZ))
# define EGLIBC_THREAD_STACK_MIN \
  (eglibc_getconf (EGLIBC_CONF_THREAD_STACK_MIN))
#else /* !defined (__OPTION_EGLIBC_DYNAMIC_CONF) */
# define EGLIBC_BUFSIZ (BUFSIZ)
# define EGLIBC_THREAD_STACK_MIN (PTHREAD_STACK_MIN)
#endif /* !defined (__OPTION_EGLIBC_DYNAMIC_CONF) */

/* Get the value of the dynamic configuration parameter __NAME.   */
extern long int eglibc_getconf (int __name);

/* Set the value of the dynamic configuration parameter __NAME to
   __VALUE.  */
extern int eglibc_setconf (int __name, long int __value);

__END_DECLS

#endif /* _EGLIBC_DYNCONF_H_ */
