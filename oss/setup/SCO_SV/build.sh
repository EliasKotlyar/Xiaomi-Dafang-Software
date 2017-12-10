#!/bin/sh

. ./.directories

rm -rf prototype

mkdir prototype
mkdir prototype/etc
echo "OSSLIBDIR=$OSSLIBDIR" > prototype/etc/oss.conf

TXT2MAN=$SRCDIR/setup/txt2man

if test ! -f /usr/local/bin/gawk
then
  # No gawk installed. Use the simple txt2man program instead of
  # the fully featured shell script which depends on gawk.

  rm -f txt2man
  
  cc -o txt2man $SRCDIR/setup/txt2man.c
  
  if test -f txt2man
  then
    TXT2MAN=./txt2man
  fi
fi

mkdir prototype/usr
mkdir prototype/usr/lib
mkdir prototype/usr/bin
mkdir prototype/usr/man
mkdir prototype/usr/man/man1
mkdir prototype/usr/man/man7
mkdir prototype/usr/man/man8
mkdir prototype/usr/sbin
mkdir prototype/$OSSLIBDIR
mkdir prototype/$OSSLIBDIR/etc
echo "autosave_mixer yes" > prototype/$OSSLIBDIR/etc/userdefs
mkdir prototype/$OSSLIBDIR/lib
mkdir prototype/$OSSLIBDIR/logs
mkdir prototype/$OSSLIBDIR/modules
mkdir prototype/$OSSLIBDIR/modules/osscore
mkdir prototype/$OSSLIBDIR/include
mkdir prototype/$OSSLIBDIR/include/sys
mkdir prototype/$OSSLIBDIR/conf
mkdir prototype/usr/include
mkdir prototype/usr/include/sys

chmod 700 prototype/$OSSLIBDIR/modules

cp $SRCDIR/include/soundcard.h prototype/usr/include/sys

cp .version prototype/$OSSLIBDIR/version.dat

cp -R $SRCDIR/include/* prototype/$OSSLIBDIR/include/sys/
cp $SRCDIR/kernel/framework/include/midiparser.h prototype/$OSSLIBDIR/include/

(cd target/bin; rm -f ossrecord; ln -s ossplay ossrecord)
cp -f target/bin/* prototype/usr/bin
cp -f target/sbin/* prototype/usr/sbin
cp -f $SRCDIR/setup/SCO_SV/sbin/* prototype/usr/sbin
cp -R $SRCDIR/setup/SCO_SV/oss prototype/usr/lib

FPSUPPORT=

if test -f $SRCDIR/setup/SCO_SV/fpsupport.s && as $SRCDIR/setup/SCO_SV/fpsupport.s
then
  FPSUPPORT=fpsupport.o
fi

ld -r -o prototype/$OSSLIBDIR/modules/osscore/Driver.o target/objects/*.o $SRCDIR/setup/SCO_SV/*.o $FPSUPPORT

grep '^int' $SRCDIR/kernel/framework/osscore/oss_core_options.c > prototype/$OSSLIBDIR/modules/osscore/Space.c

rm -f devlist.txt

for n in target/modules/*.o
do
	N=`basename $n .o`
	mkdir prototype/$OSSLIBDIR/modules/$N
	cp target/build/$N/* prototype/$OSSLIBDIR/modules/$N
	ld -r -o prototype/$OSSLIBDIR/modules/$N/Driver.o $n

# Now copy the man pages
	if test -f $SRCDIR/kernel/drv/$N/$N.man
        then
	     sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/drv/$N/$N.man > /tmp/ossman.tmp
             $TXT2MAN -t "$N" -v "Devices" -s 7d /tmp/ossman.tmp > prototype/usr/man/man7/$N.7
	fi

        if test -f $SRCDIR/kernel/nonfree/drv/$N/$N.man
	then
	     sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/nonfree/drv/$N/$N.man > /tmp/ossman.tmp
	     $TXT2MAN -t "$N" -v "Devices" -s 7d /tmp/ossman.tmp > prototype/usr/man/man7/$N.7
	fi

echo Check devices for $N
  	grep "^$N[ 	]" ./devices.list >> devlist.txt
done

sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/drv/osscore/osscore.man > /tmp/ossman.tmp
$TXT2MAN -t "osscore" -v "Devices" -s 7 /tmp/ossman.tmp > prototype/usr/man/man7/osscore.7
rm -f /tmp/ossman.tmp

#if cp lib/libOSSlib/libOSSlib.a prototype/$OSSLIBDIR/lib
#then
#  ok=1
#else
#  exit 1
#fi

cp devlist.txt prototype/$OSSLIBDIR/etc/devices.list

if test -d kernel/nonfree
then
	sed 's/.*	//' <  devlist.txt|sort|uniq >$SRCDIR/devlists/OSR6
	#cp devlist.txt $SRCDIR/devlists/SCO
fi

# Generate Man pages for commands
for i in target/bin/*
do
CMD=`basename $i`
$TXT2MAN -t "$CMD" -v "User Commands" -s 1 cmd/$CMD/$CMD.man > prototype/usr/man/man1/$CMD.1
echo done $CMD
done

for i in target/sbin/*
do
  CMD=`basename $i`
  if test -f cmd/$CMD/$CMD.man
  then
	$TXT2MAN -t "$CMD" -v "System Administration Commands" -s 8 cmd/$CMD/$CMD.man > prototype/usr/man/man8/$CMD.8
	echo done $CMD
  fi
done

for i in $SRCDIR/misc/man1m/*.man
do
	N=`basename $i .man`
	$SRCDIR/setup/txt2man -t "$CMD" -v "OSS System Administration Commands" -s 1 $i > prototype/usr/man/man1/$N.1
done

rm -f prototype/usr/man/man8/ossdetect.8
$TXT2MAN -t "ossdetect" -v "User Commands" -s 8 os_cmd/SCO_SV/ossdetect/ossdetect.man > prototype/usr/man/man8/ossdetect.8
echo done ossdetect

# Licensing stuff
if test -f $SRCDIR/4front-private/osslic.c
then
	cc -o prototype/usr/sbin/osslic -Isetup -Ikernel/nonfree/include -Ikernel/framework/include -Iinclude -Ikernel/OS/SCO_SV -I$SRCDIR $SRCDIR/4front-private/osslic.c
	strip prototype/usr/sbin/osslic
	
	prototype/usr/sbin/osslic -q -u -3prototype/$OSSLIBDIR/modules/osscore/Driver.o
	
fi

if test -f 4front-private/ossupdate.c
then
  # ossupdate
  cc -I. 4front-private/ossupdate.c -s -o prototype/usr/sbin/ossupdate -lsocket -lnsl
fi

sh $SRCDIR/setup/build_common.sh $SRCDIR $OSSLIBDIR

chmod 700 prototype/usr/sbin/*
chmod 755 prototype/usr/bin/*

cp setup/SCO_SV/S89oss prototype/$OSSLIBDIR/etc
chmod 744 prototype/$OSSLIBDIR/etc/S89oss

exit 0
