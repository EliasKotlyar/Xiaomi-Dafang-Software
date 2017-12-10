AC_INIT(hdspmixer, 1.11)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE([enable])

AC_PROG_CXX
AC_PROG_MAKE_SET
AC_STDC_HEADERS
AC_PATH_X

AM_PATH_ALSA(1.0.0)

dnl desktop install
AC_ARG_WITH(desktop-dir,
  [ --with-desktop-dir  specify the path to install desktop file],
  [desktopdir="$withval"], [desktopdir=""])
if test -z "$desktopdir"; then
  desktopdir=$datadir/applications
fi
DESKTOP_DIR="$desktopdir"
AC_SUBST(DESKTOP_DIR)
	
AC_ARG_WITH(pixmap-dir,
  [ --with-pixmap-dir  specify the path to install desktop pixmap file],
  [pixmapdir="$withval"], [pixmapdir=""])
if test -z "$pixmapdir"; then
  pixmapdir=$datadir/pixmaps
fi
PIXMAP_DIR="$pixmapdir"
AC_SUBST(PIXMAP_DIR)

dnl library check
AC_PATH_PROG(FLTK_CONFIG, fltk-config, no)
if test "$FLTK_CONFIG" = "no" && ! test "`hostname -f`" = "alsa.alsa-project.org"; then
    AC_MSG_ERROR(fltk-config is required)
fi
FLTK_CXXFLAGS="`$FLTK_CONFIG --cxxflags` -fno-exceptions"
FLTK_LIBS="`$FLTK_CONFIG --ldflags`"

CXXFLAGS="$CXXFLAGS $ALSA_CFLAGS $FLTK_CXXFLAGS"
LIBS="$LIBS $ALSA_LIBS $FLTK_LIBS"

AC_OUTPUT(Makefile src/Makefile pixmaps/Makefile)
