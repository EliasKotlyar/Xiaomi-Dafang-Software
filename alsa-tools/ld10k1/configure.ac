AC_INIT(ld10k1, 0.1.8p1)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE([enable])

AC_CONFIG_HEADERS(config.h)
AC_PROG_CC
AC_PROG_LD
AC_PROG_INSTALL
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AM_PATH_ALSA(1.0.9, [], [])

effectsdir='$(pkgdatadir)/effects'
AC_SUBST(effectsdir)

AC_CHECK_PROG(AS10K1, as10k1, as10k1)
AM_CONDITIONAL(HAVE_AS10K1, test x"$AS10K1" != x)

#taken from alsa-lib
dnl Make a symlink for inclusion of lo10k1/xxx.h
if test ! -L include/lo10k1 ; then
  echo "Making a symlink include/lo10k1"
  rm -f include/lo10k1
  ln -sf . include/lo10k1
fi

#taken from aclocal.m4 for alsa-lib - start
AC_DEFUN([SAVE_LIBRARY_VERSION], [
AC_MSG_CHECKING(for library version)
LD10K1_LIB_VERSION=$VERSION
echo $VERSION > $srcdir/version
AC_DEFINE_UNQUOTED(VERSION, "$LD10K1_LIB_VERSION", [ld10k1 library version string])
AC_SUBST(LD10K1_LIB_VERSION)
LD10K1_LIB_MAJOR=`echo $VERSION | cut -d . -f 1`
AC_SUBST(LD10K1_LIB_MAJOR)
LD10K1_LIB_MINOR=`echo $VERSION | cut -d . -f 2`
AC_SUBST(LD10K1_LIB_MINOR)
LD10K1_LIB_SUBMINOR=`echo $VERSION | cut -d . -f 3 | sed -e 's/^\([[^[:alpha:]]]*\)\(.*\)$/\1/g'`
AC_SUBST(LD10K1_LIB_SUBMINOR)
AC_MSG_RESULT(major $LD10K1_LIB_MAJOR minor $LD10K1_LIB_MINOR subminor $LD10K1_LIB_SUBMINOR)
AC_SUBST(LD10K1_LIB_EXTRAVER)
])

SAVE_LIBRARY_VERSION
#taken from aclocal.m4 for alsa-lib - end

AC_OUTPUT(Makefile
	setup/Makefile
	setup/effects/Makefile
	doc/Makefile
	patch/Makefile
	dump/Makefile
	contrib/Makefile
	contrib/emu10k1MIDIEffects/Makefile
	include/Makefile
	src/Makefile)
