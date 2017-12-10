dnl Configure Paths for ld10k1 - stolen from ASLA (1.0.5a)
dnl Configure Paths for Alsa
dnl Some modifications by Richard Boulton <richard-alsa@tartarus.org>
dnl Christopher Lansdown <lansdoct@cs.alfred.edu>
dnl Jaroslav Kysela <perex@perex.cz>
dnl AM_PATH_LD10K1([MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for liblo10k1, and define LD10K1_CFLAGS and LD10K1_LIBS as appropriate.
dnl enables arguments --with-ld10k1-prefix=
dnl                   --with-ld10k1-enc-prefix=
dnl                   --disable-ld10k1test
dnl
dnl For backwards compatibility, if ACTION_IF_NOT_FOUND is not specified,
dnl and the ld10k1 libraries are not found, a fatal AC_MSG_ERROR() will result.
dnl
AC_DEFUN([AM_PATH_LD10K1],
[dnl Save the original CFLAGS, LDFLAGS, and LIBS
ld10k1_save_CFLAGS="$CFLAGS"
ld10k1_save_LDFLAGS="$LDFLAGS"
ld10k1_save_LIBS="$LIBS"
ld10k1_found=yes

dnl
dnl Get the cflags and libraries for ld10k1
dnl
AC_ARG_WITH(ld10k1-prefix,
[  --with-ld10k1-prefix=PFX  Prefix where ld10k1 library is installed(optional)],
[ld10k1_prefix="$withval"], [ld10k1_prefix=""])

AC_ARG_WITH(ld10k1-inc-prefix,
[  --with-ld10k1-inc-prefix=PFX  Prefix where include libraries are (optional)],
[ld10k1_inc_prefix="$withval"], [ld10k1_inc_prefix=""])

dnl FIXME: this is not yet implemented
AC_ARG_ENABLE(ld10k1test,
[  --disable-ld10k1test      Do not try to compile and run a test ld10k1 program],
[enable_ld10k1test="$enableval"],
[enable_ld10k1test=yes])

dnl Add any special include directories
AC_MSG_CHECKING(for ld10k1 CFLAGS)
if test "$ld10k1_inc_prefix" != "" ; then
	LD10K1_CFLAGS="$LD10K1_CFLAGS -I$ld10k1_inc_prefix"
	CFLAGS="$CFLAGS -I$ld10k1_inc_prefix"
fi
AC_MSG_RESULT($LD10K1_CFLAGS)

dnl add any special lib dirs
AC_MSG_CHECKING(for ld10l1 LDFLAGS)
if test "$ld10k1_prefix" != "" ; then
	LD10K1_LIBS="$LD10K1_LIBS -L$ld10k1_prefix"
	LDFLAGS="$LDFLAGS $LD10K1_LIBS"
fi

dnl add the ld10k1 library
LD10K1_LIBS="$LD10K1_LIBS -llo10k1"
dnl LIBS=`echo $LIBS | sed 's/-lm//'`
dnl LIBS=`echo $LIBS | sed 's/-ldl//'`
dnl LIBS=`echo $LIBS | sed 's/-lpthread//'`
LIBS=`echo $LIBS | sed 's/  //'`
LIBS="$LD10K1_LIBS $LIBS"
AC_MSG_RESULT($LD10K1_LIBS)

dnl Check for a working version of liblo10k1 that is of the right version.
min_ld10k1_version=ifelse([$1], ,0.1.5,$1)
AC_MSG_CHECKING(for liblo10k1 headers version >= $min_ld10k1_version)
no_ld10k1=""
    ld10k1_min_major_version=`echo $min_ld10k1_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    ld10k1_min_minor_version=`echo $min_ld10k1_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    ld10k1_min_micro_version=`echo $min_ld10k1_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

AC_LANG_SAVE
AC_LANG_C
AC_TRY_COMPILE([
#include <lo10k1/lo10k1.h>
], [
/* ensure backward compatibility */
#  if(LD10K1_LIB_MAJOR > $ld10k1_min_major_version)
  exit(0);
#  else
#    if(LD10K1_LIB_MAJOR < $ld10k1_min_major_version)
#       error not present
#    endif

#   if(LD10K1_LIB_MINOR > $ld10k1_min_minor_version)
  exit(0);
#   else
#     if(LD10K1_LIB_MINOR < $ld10k1_min_minor_version)
#          error not present
#      endif

#      if(LD10K1_LIB_SUBMINOR < $ld10k1_min_micro_version)
#        error not present
#      endif
#    endif
#  endif
exit(0);
],
  [AC_MSG_RESULT(found.)],
  [AC_MSG_RESULT(not present.)
   ifelse([$3], , [AC_MSG_ERROR(Sufficiently new version of liblo10k1 not found.)])
   ld10k1_found=no]
)
AC_LANG_RESTORE

dnl Now that we know that we have the right version, let's see if we have the library and not just the headers.
if test "x$enable_ld10k1test" = "xyes"; then
AC_CHECK_LIB([lo10k1], [liblo10k1_connection_init],,
	[ifelse([$3], , [AC_MSG_ERROR(No linkable liblo10k1 was found.)])
	 ld10k1_found=no]
)
fi

if test "x$ld10k1_found" = "xyes" ; then
   ifelse([$2], , :, [$2])
   LIBS=`echo $LIBS | sed 's/-llo10k1//g'`
   LIBS=`echo $LIBS | sed 's/  //'`
   LIBS="-llo10k1 $LIBS"
fi
if test "x$ld10k1_found" = "xno" ; then
   ifelse([$3], , :, [$3])
   CFLAGS="$ld10k1_save_CFLAGS"
   LDFLAGS="$ld10k1_save_LDFLAGS"
   LIBS="$ld10k1_save_LIBS"
   LD10K1_CFLAGS=""
   LD10K1_LIBS=""
fi

dnl That should be it.  Now just export out symbols:
AC_SUBST(LD10K1_CFLAGS)
AC_SUBST(LD10K1_LIBS)
])

