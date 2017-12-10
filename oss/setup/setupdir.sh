#!/bin/sh -e

if test "$CONFIGURE " != "YES "
then
	echo
	echo Error: Wrong usage
	echo
	echo You must run `dirname $0`/configure instead of $0
	exit 1
fi

[ -z "$CC" ] && CC=cc

echo srcdir=$SRCDIR

BLDDIR=`pwd`
OS=`uname -s`

if test "$TARGETOS " != " "
then
  OS=$TARGETOS
fi

# Use the same source directories for SCO UnixWare and SCO OSR
if test "$OS " = "UnixWare "
then
	OS=SCO_SV
fi

# Use the same source directories for Haiku and BeOS
if test "$OS " = "Haiku "
then
	OS=BeOS
fi

# Use Linux24 as the OS name for Linux 2.4.x
if test "$OS " = "Linux "
then
  if test "`uname -r | cut -d '.' -f 1-2` " = "2.4 "
  then
	OS=Linux24
  fi
fi

# pkg-config seems to crash in some systems so disable core dumps
ulimit -c 0 >/dev/null 2>&1

if pkg-config gtk+-2.0 --cflags > /dev/null 2>&1
then
  HAVE_GTK=y
  GTK2=1
  export HAVE_GTK GTK2
else
    if gtk-config --cflags > /dev/null 2>&1
    then
  	HAVE_GTK=y
  	GTK1=1
  	export HAVE_GTK GTK1
    fi
fi

if test "`ls .` " != " " && test "`ls .` " != ".makefile "
then
	echo Error: Current directory must be empty
	exit 1
fi

if test -f $SRCDIR/setup/setupdir.sh
then
	echo Source directory is $SRCDIR
	echo Build directory is $BLDDIR
else
	echo Error: Invalid source directory $SRCDIR
	exit 2
fi

# Copy the ".devices" files for all drivers to devices.list
cat `find $SRCDIR/kernel/drv -name .devices`|grep -v '^#' > devices.list

if test -d $SRCDIR/kernel/nonfree/drv
then
  cat `find $SRCDIR/kernel/nonfree/drv -name .devices`|grep -v '^#' >> devices.list
fi

echo BLDDIR=$BLDDIR > .directories
echo SRCDIR=$SRCDIR >> .directories
echo OSSLIBDIR=$OSSLIBDIR >> .directories

$CC $LDFLAGS -o dirsetup $SRCDIR/setup/dirsetup.c

# Make sure the latest soundcard.h version is installed in the system
#rm /usr/include/sys/soundcard.h
#cp $SRCDIR/include/soundcard.h /usr/include/sys/soundcard.h

if ./dirsetup "$SRCDIR" "$BLDDIR" $* $COPY_OPTIONS
then
	echo Build tree created OK
else
	echo Cannot create the build tree
	rm -f dirsetup
	exit 3
fi

rm -f dirsetup

if test "$CLOSED_SOURCE " != "YES "
then
	rm -rf kernel/nonfree 4front-private
fi

if test "$ONLY_DRVS " != " "
then
	cd kernel/drv
	for drv in *
	do
		if echo $ONLY_DRVS | grep ",$drv" >/dev/null
		then
			:
		else
			touch "$drv"/.nomake
		fi
	done
	cd ../..

	if test "$CLOSED_SOURCE " = "YES "
	then
		cd kernel/nonfree/drv
		for drv in *
		do
			if echo $ONLY_DRVS | grep ",$drv" >/dev/null
			then
				:
			else
				touch "$drv"/.nomake
			fi
		done
		cd ../../../
	fi
fi

DTARGETOS=
if test "$TARGETOS " != " "
then
  DTARGETOS="-D$TARGETOS"
fi

$CC $LDFLAGS -D`uname -s` $DTARGETOS -o srcconf $SRCDIR/setup/srcconf.c

if ./srcconf $*
then
	echo Source configuration OK
else
	echo Source configuration failed
fi

if test "$CLOSED_SOURCE " != "YES " && test -d $SRCDIR/.hg
then
  HGID=`(cd $SRCDIR && hg tip|grep changeset) 2>/dev/null`
  
  if test "$HGID " != " "
  then
  	echo '#define' OSS_HG_INFO \"$HGID, \" \\ >> kernel/framework/include/local_config.h
  
  	HGID=`(cd $SRCDIR && hg tip|grep tag) 2>/dev/null`
  	echo '                   ' \"$HGID, \" \\ >> kernel/framework/include/local_config.h
  
  	HGID=`(cd $SRCDIR && hg tip|grep date) 2>/dev/null`
  	echo '                   ' \"$HGID, \" \\ >> kernel/framework/include/local_config.h
  
  	HGID=`(cd $SRCDIR && hg tip|grep summary) 2>/dev/null`
  	echo '                   ' \"$HGID\" >> kernel/framework/include/local_config.h
  fi

fi

if test ! "$OSS_CONFIG_OPTIONS " = " " && test ! "$OSS_CONFIG_OPTIONS " = "--include-closed-source "
then
	echo '#define OSS_CONFIG_OPTIONS '\"$OSS_CONFIG_OPTIONS\" >> kernel/framework/include/local_config.h
fi

if test ! -d target
then
	mkdir target
fi

mkdir target/bin
mkdir target/lib
mkdir target/sbin
mkdir target/modules
mkdir target/objects
mkdir target/tmpobjects

touch .depend

if date -u +%Y%m%d%H%M > build.id.new 2>/dev/null
then
	rm -f build.id
	mv build.id.new build.id
	echo "#define OSS_COMPILE_DATE \""`cat build.id`"\"" >> kernel/framework/include/timestamp.h
	echo "#define osdev_create osdev_create_`cat build.id`" >> kernel/framework/include/timestamp.h
	echo
	echo Build ID will become `cat build.id`
else
	echo Failed to create the build timestamp
	exit 254
fi

if test "$CLOSED_SOURCE " = "YES "
then
	echo "#define LICENSED_VERSION" >> kernel/framework/include/local_config.h
fi

if test -f $SRCDIR/buildid.dat
then
	echo '#define 'OSS_BUILD_ID \"`cat $SRCDIR/buildid.dat`\" > kernel/framework/include/buildid.h
else
	echo $SRCDIR/buildid.dat is missing
	exit 10
fi

if test ! -d kernel/nonfree || test -f $SRCDIR/kernel/nonfree/.nomake
then
	echo '#define __OPENOSS__' >> kernel/framework/include/buildid.h
	if test -f $SRCDIR/.license
	then
	   echo '#define OSS_LICENSE "'`cat $SRCDIR/.license`'"' >> kernel/framework/include/buildid.h
	else
	   echo '#define OSS_LICENSE "OSS_HG"' >> kernel/framework/include/buildid.h
	fi
fi

if test "$uOSS " = "1 "
then
	echo '#define uOSS' >> kernel/framework/include/buildid.h
fi

if test "$USE_REGPARM " = "1 "
then
	echo "1" > ./regparm
fi

if test "$NO_REGPARM " = "1 "
then
	echo "0" > ./regparm
fi

# Setup the link to the right os.h file for this operating system.
(cd kernel/framework/include;ln -s ../../../kernel/OS/$OS/os_*.h os.h)

$CC $LDFLAGS -o ossvers -I./kernel/framework/include  setup/ossvers.c
./ossvers > .version
rm ./ossvers

ln -s $SRCDIR origdir

if test -f setup/$OS/build_`uname -m`.sh
then
	ln -s setup/$OS/build_`uname -m`.sh build.sh
else
	if test -f setup/$OS/build.sh
	then
		ln -s setup/$OS/build.sh build.sh
	fi
fi

if test -f setup/$OS/make.local
then
  if test -f Makefile.php
  then
	echo >> Makefile.php
	echo include setup/$OS/make.local >> Makefile.php
  else
	echo >> Makefile
	echo include setup/$OS/make.local >> Makefile
  fi
fi
PHPMAKE=phpmake

if test ! -d phpmake && test -d ../phpmake
then
	PHPMAKE=../phpmake
fi

if test -f Makefile.php && test -d $PHPMAKE
then
	echo Running phpmake for all subdirectories - please wait

	if test "$PHPMAKE_LIBPATH " = " "
	then
	   PHPMAKE_LIBPATH="`pwd`/$PHPMAKE/"

	   export PHPMAKE_LIBPATH
	   echo PHPMAKE_LIBPATH not set - assuming $PHPMAKE_LIBPATH
	fi

	if test "$PHPMAKE_PROJECT " = " "
	then
	   echo PHPMAKE_PROJECT not set - cannot continue
	   exit 1
	fi

	phpmake

	(cd targetos && phpmake)

	make kernel/framework/include/ossddk/oss_limits.h	# Generate this file from PHh
else
	ln -s oss_limits.PHh kernel/framework/include/ossddk/oss_limits.h
fi 

make dep

echo Directory preparation complete.
echo Build ID will be `cat $SRCDIR/buildid.dat`

exit 0
