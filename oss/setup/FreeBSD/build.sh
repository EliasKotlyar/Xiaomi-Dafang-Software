#!/bin/sh

. ./.directories

if which gawk >/dev/null
then
   TXT2MAN=$SRCDIR/setup/txt2man
else
   echo "No gawk found. Using lesser replacement" >&2
   cc -o txt2man origdir/setup/txt2man.c
   TXT2MAN=./txt2man
fi

rm -rf prototype

mkdir prototype
mkdir prototype/etc
mkdir prototype/etc/rc.d
mkdir prototype/usr
mkdir prototype/usr/bin
mkdir prototype/usr/sbin
mkdir -p prototype/$OSSLIBDIR
mkdir prototype/$OSSLIBDIR/etc
mkdir prototype/$OSSLIBDIR/lib
mkdir prototype/$OSSLIBDIR/include
mkdir prototype/$OSSLIBDIR/include/internals
mkdir prototype/$OSSLIBDIR/include/sys
mkdir prototype/$OSSLIBDIR/modules
mkdir prototype/$OSSLIBDIR/objects
mkdir prototype/usr/share
mkdir prototype/usr/share/man
mkdir prototype/usr/share/man/man1
mkdir prototype/usr/share/man/man7
mkdir prototype/usr/share/man/man8
mkdir prototype/$OSSLIBDIR/conf

echo "OSSLIBDIR=$OSSLIBDIR" > prototype/etc/oss.conf

# Regenerating the config file templates
rm -f /tmp/confgen
if ! cc -o /tmp/confgen ./setup/FreeBSD/confgen.c
then
        echo Building confgen failed
        exit 1
fi

if ! /tmp/confgen prototype/$OSSLIBDIR/conf $OSSLIBDIR/conf kernel/drv/* kernel/nonfree/drv/*
then
        echo Running confgen failed
        exit 1
fi

rm -f /tmp/confgen

cp -r $SRCDIR/setup/FreeBSD/oss/* prototype/$OSSLIBDIR/
cp $SRCDIR/kernel/OS/FreeBSD/wrapper/bsddefs.h prototype/$OSSLIBDIR/build/

cp $SRCDIR/include/*.h prototype/$OSSLIBDIR/include/sys/
cp $SRCDIR/lib/libOSSlib/midiparser.h prototype/$OSSLIBDIR/include/
cp kernel/framework/include/timestamp.h kernel/framework/include/local_config.h $SRCDIR/kernel/framework/include/*_core.h $SRCDIR/kernel/framework/include/ossddk/*.h prototype/$OSSLIBDIR/include/internals
cp kernel/framework/include/ossddk/oss_limits.h prototype/$OSSLIBDIR/include/internals

ld -r -o prototype/$OSSLIBDIR/build/osscore.lib target/objects/*.o

rm -f devlist.txt

for n in target/modules/*.o
do
	N=`basename $n .o`
echo Check devices for $N
  	grep "^$N[ 	]" ./devices.list >> devlist.txt
done

(cd target/bin; rm -f ossrecord; ln -s ossplay ossrecord)
cp target/modules/*.o prototype/$OSSLIBDIR/objects
cp target/build/*.c prototype/$OSSLIBDIR/build/
cp target/bin/* prototype/usr/bin/
cp target/sbin/* prototype/usr/sbin/
cp $SRCDIR/setup/FreeBSD/sbin/* prototype/usr/sbin/
cp $SRCDIR/setup/FreeBSD/etc/rc.d/oss prototype/etc/rc.d
cp lib/libOSSlib/libOSSlib.so prototype/$OSSLIBDIR/lib

cp devlist.txt prototype/$OSSLIBDIR/etc/devices.list

if test -d kernel/nonfree
then
	rm -f $SRCDIR/devlists/FreeBSD
	cp devlist.txt $SRCDIR/devlists/FreeBSD
fi

# Generate Man pages for commands
for i in target/bin/*
do
CMD=`basename $i`
$TXT2MAN -t "$CMD" -v "User Commands" -s 1 cmd/$CMD/$CMD.man | gzip -9 > prototype/usr/share/man/man1/$CMD.1.gz
echo done $CMD
done

for i in target/sbin/*
do
  CMD=`basename $i`
  if test -f cmd/$CMD/$CMD.man
  then
	$TXT2MAN -t "$CMD" -v "System Administration Commands" -s 8 cmd/$CMD/$CMD.man | gzip -9 > prototype/usr/share/man/man8/$CMD.8.gz
	echo done $CMD
  fi
done

for i in $SRCDIR/misc/man1m/*.man
do
        N=`basename $i .man`
        $TXT2MAN -t "$CMD" -v "OSS System Administration Commands" -s 1 $i | gzip -9 > prototype/usr/share/man/man1/$N.1.gz
done

$TXT2MAN -t "ossdetect" -v "User Commands" -s 8 os_cmd/FreeBSD/ossdetect/ossdetect.man | gzip -9 > prototype/usr/share/man/man8/ossdetect.8.gz
echo done ossdetect

for n in target/modules/*.o
do
	N=`basename $n .o`
	ld -r -o prototype/$OSSLIBDIR/$MODULES/$N.o $n
	echo Check devices for $N
  	grep "^$N[ 	]" ./devices.list >> devlist.txt

	rm -f /tmp/ossman.txt

	if test -f $SRCDIR/kernel/drv/$N/$N.man
	then
	  sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/drv/$N/$N.man > /tmp/ossman.txt
	  $TXT2MAN -t "$CMD" -v "OSS Devices" -s 7 /tmp/ossman.txt|gzip -9 > prototype/usr/share/man/man7/$N.7.gz
	else
		if test -f $SRCDIR/kernel/nonfree/drv/$N/$N.man
		then
	  		sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/nonfree/drv/$N/$N.man > /tmp/ossman.txt
	  		$TXT2MAN -t "$CMD" -v "OSS Devices" -s 7 $SRCDIR/kernel/nonfree/drv/$N/$N.man|gzip -9 > prototype/usr/share/man/man7/$N.7.gz
		fi
	fi
done

sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/drv/osscore/osscore.man > /tmp/ossman.txt
$TXT2MAN -t "osscore" -v "OSS Devices" -s 7 /tmp/ossman.txt|gzip -9 > prototype/usr/share/man/man7/osscore.7.gz
rm -f /tmp/ossman.txt

cp .version prototype/$OSSLIBDIR/version.dat

# Licensing stuff
if test -f $SRCDIR/4front-private/osslic.c
then
	cc -o prototype/usr/sbin/osslic -Isetup -Ikernel/nonfree/include -Ikernel/framework/include -Iinclude -Ikernel/OS/FreeBSD -I$SRCDIR $SRCDIR/4front-private/osslic.c
	strip prototype/usr/sbin/osslic

        BITS=3 # Default to 32 bit ELF format
        if test "`uname -m` " = "amd64 "
        then
           BITS=6 # Use 64 bit ELF format
        fi

	prototype/usr/sbin/osslic -q -u -$BITS./prototype/$OSSLIBDIR/build/osscore.lib
	
fi

if test -f 4front-private/ossupdate.c
then
  #ossupdate
  cc -I. 4front-private/ossupdate.c -s -o prototype/usr/sbin/ossupdate
fi

sh $SRCDIR/setup/build_common.sh $SRCDIR $OSSLIBDIR

chmod 700 prototype/usr/sbin/*
chmod 755 prototype/usr/bin/*
chmod 700 prototype/$OSSLIBDIR

(cd prototype;ls usr/sbin/* usr/bin/* etc/* usr/share/man/man*/*) > prototype/$OSSLIBDIR/sysfiles.list

exit 0
