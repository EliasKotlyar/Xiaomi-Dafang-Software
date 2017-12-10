AC_INIT(qlo10k1, 0.1.2p1)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE([enable])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)
AC_PROG_CXX
AC_PROG_LD
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_C_BIGENDIAN
gw_CHECK_QT

dnl Check ld10k1 availability
AC_ARG_WITH(ld10k1,
  [  --with-ld10k1  local|system  Specify the ld10k1 installation (default=local)],
  ld10k1="$withval",
  ld10k1="local")

if test "$ld10k1" = "system"; then
  AM_PATH_LD10K1(0.1.8)
else
  if test "$ld10k1" = "local"; then
    ld10k1="../ld10k1"
  fi
  test -d "$ld10k1" || AC_MSG_ERROR([Cannot find ld10k1 directory $ld10k1])
  rm -f ld10k1
  ln -s $ld10k1 ld10k1
  rm -rf include
  mkdir include
  ln -s ../ld10k1/include include/lo10k1
  LD10K1_CFLAGS="-I../include"
  LD10K1_LIBS="-L$ld10k1/src -llo10k1"
  AC_SUBST(LD10K1_CFLAGS)
  AC_SUBST(LD10K1_LIBS)
fi

AC_OUTPUT(Makefile
	src/Makefile)
