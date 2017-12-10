#!/bin/sh

. ./.directories

#BEOS_SYSTEM=beos/system
# to install as user addons
BEOS_SYSTEM=home/config

DRVPREFIX=oss_

rm -rf prototype

mkdir prototype
#mkdir prototype/etc
#echo "OSSLIBDIR=$OSSLIBDIR" > prototype/etc/oss.conf

TXT2MAN=$SRCDIR/setup/txt2man

if gawk '' 2>/dev/null
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

mkdir -p prototype/$BEOS_SYSTEM/add-ons/media
mkdir -p prototype/$BEOS_SYSTEM/add-ons/kernel/media
#hack for now
#mkdir -p prototype/$BEOS_SYSTEM/add-ons/kernel/media/oss
mkdir -p prototype/$BEOS_SYSTEM/add-ons/kernel/drivers/bin
mkdir -p prototype/$BEOS_SYSTEM/add-ons/kernel/drivers/dev/audio/multi
mkdir -p prototype/$BEOS_SYSTEM/add-ons/kernel/drivers/dev/audio/oss
#hack for now
mkdir -p prototype/$BEOS_SYSTEM/add-ons/kernel/drivers/dev/oss
ln -s ../../bin/${DRVPREFIX}loader prototype/$BEOS_SYSTEM/add-ons/kernel/drivers/dev/oss/
ln -s ../bin/${DRVPREFIX}loader prototype/$BEOS_SYSTEM/add-ons/kernel/drivers/dev/
#hack: install bins for now
mkdir -p prototype/home/config/bin
mkdir -p prototype/home/config/settings/kernel/drivers
mkdir -p prototype/home/Desktop

#cp $SRCDIR/include/soundcard.h prototype/usr/include/sys

#cp .version prototype/$OSSLIBDIR/version.dat

#cp -R $SRCDIR/include/* prototype/$OSSLIBDIR/include/sys/
#cp $SRCDIR/kernel/framework/include/midiparser.h prototype/$OSSLIBDIR/include/

(cd target/bin; rm -f ossrecord; ln -s ossplay ossrecord)
cp -f target/bin/* prototype/home/config/bin
cp -f target/sbin/* prototype/home/config/bin

#cp -R $SRCDIR/oss/* prototype/$OSSLIBDIR

# generate driver_settings file from the .params on stdin.
function gensettings () {
	awk 'BEGIN { print "# Open Sound System configuration file" ; print "" }
/^int/ { split($2, option, "[=;]") }
/^ \*\// { print "#" option[1] "	" option[2] ; print "" }
/^ \* / { print "# " substr($0, 4) }'
}


# set version info and mime type on a binary
function setvermime () {
	local longver="`cat .version`"
	local shortver="${longver%% *}"
	local appver="${shortver:0:1} ${shortver:2:1} 0 b ${shortver##*[a-z]}"
	#local lic="`cat .license`"
	local copyright="`echo -n -e '\302\251'` 2007 4Front"
	setversion "$1" -app $appver -short $shortver -long "$longver $copyright"
	mimeset -f "$1"
}

#ld -r -o prototype/$OSSLIBDIR/modules/osscore/Driver.o target/objects/*.o $SRCDIR/setup/SCO_SV/*.o $FPSUPPORT

#core=prototype/$BEOS_SYSTEM/add-ons/kernel/media/oss/${DRVPREFIX}core
#must match internal module name...
core=prototype/$BEOS_SYSTEM/add-ons/kernel/media/oss
#gcc -o $drv target/objects/*.o -nostdlib /boot/develop/lib/x86/_KERNEL_ || exit 1

# no midi yet
rm target/modules/oss_midiloop.o

# try to build all in a single bin for now...
# driver_beos.o shouldn' be in, oh well...
# R5 has symbols like __ucmpdi2 but not Haiku, so use libgcc
gcc -o $core target/objects/*.o target/modules/*.o -nostdlib -lgcc /boot/develop/lib/x86/_KERNEL_ || exit 1
setvermime $core

# except the loader driver...
# using the same bin works in BeOS but not in Haiku.
drv=prototype/$BEOS_SYSTEM/add-ons/kernel/drivers/bin/${DRVPREFIX}loader
gcc -o $drv target/objects/driver_beos.o -nostdlib /boot/develop/lib/x86/_KERNEL_ || exit 1
setvermime $drv

rm -f devlist.txt

# generate driver settings
settingspath=prototype/home/config/settings/kernel/drivers
gensettings < kernel/framework/ac97/.params > $settingspath/oss_core
gensettings < kernel/drv/osscore/.params >> $settingspath/oss_core
for n in target/modules/*.o
do
	N=`basename $n .o`
	test -e kernel/drv/$N/.params && gensettings < kernel/drv/$N/.params > $settingspath/$N
	echo Check devices for $N
	grep "^$N[ 	]" ./devices.list >> devlist.txt
done

#echo "Copying media node addon, make sure it's up to date! (cd lib/opensound.media_addon && make)"
#cp ../oss-*-gpl/lib/opensound.media_addon/obj.x86/opensound.media_addon prototype/$BEOS_SYSTEM/add-ons/media/
#copyattr -d ../oss-*-gpl/lib/opensound.media_addon/OpenSound_README.txt prototype/home/Desktop/
echo "make sure the opensound media addon is installed and up to date!"
echo "(cd lib/opensound.media_addon && make)"
echo "The addon is distributed as part of Haiku (www.haiku-os.org) source"

#grep '^int' $SRCDIR/kernel/framework/osscore/options.c > prototype/$OSSLIBDIR/modules/osscore/Space.c

#sed 's/.*	//' <  devlist.txt|sort|uniq >$SRCDIR/devlists/OSR6
if test -d kernel/nonfree
then
	cp devlist.txt $SRCDIR/devlists/BeOS
fi

exit 0
##########

for n in target/modules/*.o
do
	N=`basename $n .o`
	#mkdir prototype/$OSSLIBDIR/modules/$N
	#cp target/build/$N/* prototype/$OSSLIBDIR/modules/$N
	#ld -r -o prototype/$OSSLIBDIR/modules/$N/Driver.o $n

	#drv=prototype/$BEOS_SYSTEM/add-ons/kernel/drivers/bin/${DRVPREFIX}$N
	#gcc -o $drv $n -nostdlib /boot/develop/lib/x86/_KERNEL_ || exit 1
	#longver="`cat .version`"
	#shortver="${longver%% *}"
	#appver="${shortver:0:1} ${shortver:0:1} 0 b ${shortver##*[a-z]}"
	#lic="`cat .license`"
	#copyright="`echo -n -e '\302\251'` 2007 4Front"
	#setversion $drv -app $appver -short $shortver -long "$longver $copyright $lic"
	#mimeset -f $drv

# Now copy the man pages
#	if test -f $SRCDIR/kernel/drv/$N/$N.man
#        then
#	     sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf/:g" < $SRCDIR/kernel/drv/$N/$N.man > /tmp/ossman.tmp
#             $TXT2MAN -t "$N" -v "Devices" -s 7d /tmp/ossman.tmp > prototype/usr/man/man7/$N.7
#	fi

#        if test -f $SRCDIR/kernel/nonfree/drv/$N/$N.man
#	then
#	     sed "s:CONFIGFILEPATH:$OSSLIBDIR/conf/:g" < $SRCDIR/kernel/nonfree/drv/$N/$N.man > /tmp/ossman.tmp
#	     $TXT2MAN -t "$N" -v "Devices" -s 7d /tmp/ossman.tmp > prototype/usr/man/man7/$N.7
#	fi

done

#cp devlist.txt prototype/$OSSLIBDIR/etc/devices.list














# Generate Man pages for commands
#for i in target/bin/*
#do
#CMD=`basename $i`
#$TXT2MAN -t "$CMD" -v "User Commands" -s 1 cmd/$CMD/$CMD.man > prototype/usr/man/man1/$CMD.1
#echo done $CMD
#done

#for i in target/sbin/*
#do
#  CMD=`basename $i`
#  if test -f cmd/$CMD/$CMD.man
#  then
#	$TXT2MAN -t "$CMD" -v "System Administration Commands" -s 8 cmd/$CMD/$CMD.man > prototype/usr/man/man8/$CMD.8
#	echo done $CMD
#  fi
#done

#rm -f prototype/usr/man/man8/ossdetect.8
#$TXT2MAN -t "ossdetect" -v "User Commands" -s 8 os_cmd/SCO_SV/ossdetect/ossdetect.man > prototype/usr/man/man8/ossdetect.8
#echo done ossdetect

## Licensing stuff
#if test -f $SRCDIR/4front-private/osslic.c
#then
#	cc -o prototype/usr/sbin/osslic -Isetup -Ikernel/nonfree/include -Ikernel/framework/include -Iinclude -Ikernel/OS/SCO_SV -I$SRCDIR $SRCDIR/4front-private/osslic.c
#	strip prototype/usr/sbin/osslic
#	
#	prototype/usr/sbin/osslic -q -u -3prototype/$OSSLIBDIR/modules/osscore/Driver.o
#	
#fi

#if test -f 4front-private/ossupdate.c
#then
#  # ossupdate
#  cc -I. 4front-private/ossupdate.c -s -o prototype/usr/sbin/ossupdate -lsocket -lbind
#fi

sh $SRCDIR/setup/build_common.sh $SRCDIR $OSSLIBDIR

#chmod 700 prototype/usr/sbin/*
#chmod 755 prototype/usr/bin/*

#cp setup/SCO_SV/S89oss prototype/$OSSLIBDIR/etc
#chmod 744 prototype/$OSSLIBDIR/etc/S89oss

exit 0
