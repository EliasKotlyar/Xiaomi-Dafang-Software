#!/bin/sh

. ./.directories

if gawk '' >/dev/null
then
   TXT2MAN=$SRCDIR/setup/txt2man
else
   echo "No gawk found. Using lesser replacement" >&2
   cc -o txt2man origdir/setup/txt2man.c
   TXT2MAN=./txt2man
fi

[ -z "$LD" ] && LD=ld

rm -rf prototype

mkdir prototype
mkdir prototype/etc
echo "OSSLIBDIR=$OSSLIBDIR" > prototype/etc/oss.conf
mkdir prototype/usr
mkdir prototype/usr/bin
mkdir prototype/usr/share
mkdir prototype/usr/share/man
mkdir prototype/usr/share/man/man1
mkdir prototype/usr/share/man/man7
mkdir prototype/usr/share/man/man8
mkdir prototype/usr/sbin
mkdir -p prototype/$OSSLIBDIR
mkdir prototype/$OSSLIBDIR/etc
mkdir prototype/$OSSLIBDIR/save
mkdir prototype/$OSSLIBDIR/conf.tmpl
mkdir prototype/$OSSLIBDIR/lib
mkdir prototype/$OSSLIBDIR/modules.regparm
mkdir prototype/$OSSLIBDIR/modules.noregparm
mkdir prototype/$OSSLIBDIR/objects.regparm
mkdir prototype/$OSSLIBDIR/objects.noregparm
mkdir prototype/$OSSLIBDIR/include
mkdir prototype/$OSSLIBDIR/include/sys
mkdir prototype/$OSSLIBDIR/include/internals
mkdir prototype/$OSSLIBDIR/build

chmod 700 prototype/$OSSLIBDIR/modules.*
chmod 700 prototype/$OSSLIBDIR/objects.*
chmod 700 prototype/$OSSLIBDIR/build
chmod 700 prototype/$OSSLIBDIR/save

if test -f regparm && test "`cat regparm` " = "1 "
then
  MODULES=modules.regparm
  OBJECTS=objects.regparm
else
  MODULES=modules.noregparm
  OBJECTS=objects.noregparm
fi

cp .version prototype/$OSSLIBDIR/version.dat

if test "`uname -m` " != "arm "
then
	if ! test -f regparm
	then
	  echo Error: ./regparm is missing
	  exit 1
	fi
	cp regparm prototype/$OSSLIBDIR/build
fi

# Regenerating the config file templates
rm -f /tmp/confgen
if ! cc -o /tmp/confgen ./setup/Linux/confgen.c
then
	echo Building confgen failed
	exit 1
fi

if ! /tmp/confgen prototype/$OSSLIBDIR/conf.tmpl $OSSLIBDIR/conf kernel/drv/* kernel/nonfree/drv/*
then
	echo Running confgen failed
	exit 1
fi

rm -f /tmp/confgen

cp $SRCDIR/include/*.h prototype/$OSSLIBDIR/include/sys/
cp $SRCDIR/kernel/framework/include/midiparser.h prototype/$OSSLIBDIR/include/
cp -f $SRCDIR/kernel/OS/Linux/wrapper/wrap.h prototype/$OSSLIBDIR/build/
cp -f $SRCDIR/kernel/framework/include/udi.h prototype/$OSSLIBDIR/build/
cp -a $SRCDIR/kernel/framework/include/*_core.h kernel/framework/include/local_config.h prototype/$OSSLIBDIR/include/internals
cp $SRCDIR/kernel/framework/include/ossddk/*.h prototype/$OSSLIBDIR/include/internals
cp kernel/framework/include/timestamp.h prototype/$OSSLIBDIR/include/internals
cp kernel/framework/include/ossddk/oss_limits.h prototype/$OSSLIBDIR/include/internals

cat > prototype/$OSSLIBDIR/include/internals/WARNING.txt << EOF
Caution: All header files included in this directory are there only because
         some parts of OSS may need to be re-compiled. It is not safe to use
         these files for any purposes because they will change between OSS
         versions/builds.
EOF

(cd target/bin; rm -f ossrecord; ln -s ossplay ossrecord)
cp -f target/build/* prototype/$OSSLIBDIR/build/
cp -f target/bin/* prototype/usr/bin
cp -f target/sbin/* prototype/usr/sbin

cp -a $SRCDIR/setup/Linux/oss/* prototype/$OSSLIBDIR/
cp -a $SRCDIR/setup/Linux/sbin prototype/usr/
chmod +x prototype/$OSSLIBDIR/scripts/*

if ! $LD -r -o prototype/$OSSLIBDIR/$OBJECTS/osscore.o target/objects/*.o
then
  echo Linking osscore failed!
  exit 1
fi

rm -f devlist.txt devices.list

for n in `find kernel/ -name .devices`
do
  cat $n >> devices.list
done

for n in target/modules/*.o
do
	N=`basename $n .o`
	$LD -r -o prototype/$OSSLIBDIR/$MODULES/$N.o $n
	echo Check devices for $N
  	grep "^$N[ 	]" ./devices.list >> devlist.txt

	rm -f /tmp/ossman.txt

	if test -f $SRCDIR/kernel/drv/$N/$N.man
	then
	  sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/drv/$N/$N.man > /tmp/ossman.txt
	  $TXT2MAN -t "$CMD" -v "OSS Devices" -s 7 /tmp/ossman.txt | gzip -9 > prototype/usr/share/man/man7/$N.7.gz
	else
		if test -f $SRCDIR/kernel/nonfree/drv/$N/$N.man
		then
	  		sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/nonfree/drv/$N/$N.man > /tmp/ossman.txt
	  		$TXT2MAN -t "$CMD" -v "OSS Devices" -s 7 $SRCDIR/kernel/nonfree/drv/$N/$N.man | gzip -9 > prototype/usr/share/man/man7/$N.7.gz
		fi
	fi
done

sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf:g" < $SRCDIR/kernel/drv/osscore/osscore.man > /tmp/ossman.txt
$TXT2MAN -t "osscore" -v "OSS Devices" -s 7 /tmp/ossman.txt | gzip -9 > prototype/usr/share/man/man7/osscore.7.gz
rm -f /tmp/ossman.txt

# Link the optional NOREGPARM modules
if test -d noregparm
then
   $LD -r -o prototype/$OSSLIBDIR/objects.noregparm/osscore.o noregparm/target/objects/*.o

   for n in noregparm/target/modules/*.o
   do
	N=`basename $n .o`
	$LD -r -o prototype/$OSSLIBDIR/modules.noregparm/$N.o $n
   done
fi

for n in $SRCDIR/misc/man7/*.man
do
	N=`basename $n .man`

	$TXT2MAN -t "$CMD" -v "OSS Devices" -s 7 $n | gzip -9 > prototype/usr/share/man/man7/$N.7.gz
done

for n in $SRCDIR/misc/man1m/*.man
do
	N=`basename $n .man`
	$TXT2MAN -t "$CMD" -v "OSS System Administration Commands" -s 1 $n | gzip -9 > prototype/usr/share/man/man1/$N.1.gz
done

if ! cp lib/libOSSlib/libOSSlib.so lib/libsalsa/.libs/libsalsa.so.2.0.0 prototype/$OSSLIBDIR/lib
then
  echo Warning: No libsalsa library compiled
fi

cp target/lib/* prototype/$OSSLIBDIR/lib

cp devlist.txt prototype/$OSSLIBDIR/etc/devices.list

if test -d kernel/nonfree
then
	cp devlist.txt $SRCDIR/devlists/Linux
fi

# Generate Man pages for commands
for i in target/bin/*
do
CMD=`basename $i`
$TXT2MAN -t "$CMD" -v "OSS User Commands" -s 1 cmd/$CMD/$CMD.man | gzip -9 > prototype/usr/share/man/man1/$CMD.1.gz
echo done $CMD
done

for i in target/sbin/*
do
  CMD=`basename $i`
  if test -f cmd/$CMD/$CMD.man
  then
	$TXT2MAN -t "$CMD" -v "OSS System Administration Commands" -s 8 cmd/$CMD/$CMD.man | gzip -9 > prototype/usr/share/man/man8/$CMD.8.gz
	echo done $CMD
  fi
done

$TXT2MAN -t "ossdetect" -v "User Commands" -s 8 os_cmd/Linux/ossdetect/ossdetect.man | gzip -9 > prototype/usr/share/man/man8/ossdetect.8.gz
echo done ossdetect

cp -f $SRCDIR/oss/lib/flashsupport.c prototype/$OSSLIBDIR/lib

# Licensing stuff
if test -f 4front-private/osslic.c
then
	cc -o prototype/usr/sbin/osslic -Isetup -Ikernel/nonfree/include -Ikernel/framework/include -Iinclude -Ikernel/OS/Linux -I$SRCDIR $SRCDIR/4front-private/osslic.c
	strip prototype/usr/sbin/osslic
	
	BITS=3 # Default to 32 bit ELF format
	if test "`uname -m` " = "x86_64 "
	then
	   BITS=6 # Use 64 bit ELF format
	fi
	prototype/usr/sbin/osslic -q -u -$BITS./prototype/$OSSLIBDIR/objects.regparm/osscore.o
	prototype/usr/sbin/osslic -q -u -$BITS./prototype/$OSSLIBDIR/objects.noregparm/osscore.o
	
fi

if test -f 4front-private/ossupdate.c
then
  #ossupdate
  cc -I. 4front-private/ossupdate.c -s -o prototype/usr/sbin/ossupdate
fi

sh $SRCDIR/setup/build_common.sh $SRCDIR $OSSLIBDIR

chmod 700 prototype/usr/sbin/*
chmod 755 prototype/usr/bin/*

(cd prototype;ls usr/sbin/* usr/bin/* etc/* usr/share/man/man*/*) > prototype/$OSSLIBDIR/sysfiles.list

exit 0
