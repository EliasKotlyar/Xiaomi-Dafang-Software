#!/bin/sh
MAKE=$4

case `uname` in

"Linux")
	$MAKE libOSSlib.so
	;;

"FreeBSD")
	$MAKE libOSSlib.so
	;;

"OSF1")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"NetBSD")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"OpenBSD")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"BSD/OS")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"AIX")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"HP-UX")
	$MAKE libOSSlib.a CC=$2 CFLAGS=$3
	;;

"LynxOS")
	$MAKE AROPTS=rcs libOSSlib.a CFLAGS=""
	;;

"UNIX_SV")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"UnixWare")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"OpenUNIX")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"SCO_SV")
	$MAKE libOSSlib.a INSTALLLIB="$1" CFLAGS=""
	;;

"SunOS")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"PowerMAX_OS")
	$MAKE libOSSlib.a CFLAGS=""
	;;
"DragonFly")
	$MAKE libOSSlib.a CFLAGS=""
	;;

"BeOS"|"Haiku")
	$MAKE libOSSlib.a CFLAGS=""
	;;

*)
      echo Can\'t recognize your operating system '('`uname`')'.
      echo;echo
      echo use $MAKE libOSSlib.so or $MAKE libOSSlib.a to build OSSlib
      exit 0

esac

exit 0
