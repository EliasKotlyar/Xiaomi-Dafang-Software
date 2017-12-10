
. ./.directories

OSSLIBDIR=/usr/lib/oss

if test "`which gawk|grep 'no gawk in '` " = " " 2>/dev/null
then
   TXT2MAN=$SRCDIR/setup/txt2man
else
   cc -o txt2man origdir/setup/txt2man.c
   TXT2MAN=./txt2man
fi

if test "`uname -p`" = "sparc"
then
	KERNEL32=sparc
	KERNEL64=sparcv9
	MACH=sun4u
else
	case `uname -r` in
	"5.8")
		KERNEL32=i386
		KERNEL64=NULL
		;;
	"5.9")
		KERNEL32=i386
		KERNEL64=NULL
		;;
	*)
		KERNEL32=i386
		KERNEL64=amd64
		;;
	esac
	MACH=i86pc
fi

if test "`uname -r`" = "5.9" || test "`uname -r`" = "5.8"
then
	AMSRC=amsrc1
	OSFLAGS=-DSOL9
else
	AMSRC=amsrc2
fi

# Re-create the prototype directory
rm -rf prototype

mkdir prototype
mkdir prototype/kernel
mkdir prototype/kernel/drv
mkdir prototype/kernel/drv/$KERNEL64
mkdir prototype/kernel/misc
mkdir prototype/kernel/misc/$KERNEL64

mkdir prototype/etc
mkdir prototype/etc/oss
mkdir prototype/etc/init.d
mkdir prototype/usr
mkdir prototype/usr/bin
mkdir prototype/usr/sbin
mkdir prototype/usr/man
mkdir prototype/usr/man/man1
mkdir prototype/usr/man/man1m
mkdir prototype/usr/man/man7d
mkdir prototype/usr/include
mkdir prototype/usr/include/sys
mkdir prototype/usr/include/oss
mkdir prototype/usr/lib
mkdir prototype/usr/lib/oss

cat > prototype/usr/lib/oss/README <<EOF
This directory is not used any more. Configuration files for OSS are
located under /etc/oss. The soundon.log file is located in
/var/log/soundon.log.
EOF

cp $KERNEL32/.version prototype/etc/oss/version.dat

# Copy the files to their right place

cp $SRCDIR/include/soundcard.h prototype/usr/include/sys
cp $SRCDIR/include/oss_userdev_exports.h prototype/usr/include/oss
(cd $KERNEL32/target/bin; rm -f ossrecord; ln -s ossplay ossrecord)
cp $KERNEL32/target/bin/* prototype/usr/bin
cp $KERNEL32/target/sbin/* prototype/usr/sbin
cp $KERNEL32/setup/SunOS/sbin/* prototype/usr/sbin
cp origdir/setup/SunOS/S89oss prototype/etc/init.d/oss
chmod 500 prototype/usr/sbin/*
echo "autosave_mixer yes" > prototype/etc/oss/userdefs
#echo "usbif,class1" > prototype/etc/oss/forceload.conf
rm -f devlist.txt

# Create the driver modules (for 64 bit)

if test "`ls $KERNEL64/target/modules/*.o 2>/dev/null` " != " "
then
	# osscommon
	if ld -64 -dy -r -Nmisc/usba -o prototype/kernel/misc/$KERNEL64/osscommon $KERNEL64/target/objects/*.o
	then
		$TXT2MAN -v "OSS Devices" -s 7 $KERNEL64/kernel/drv/osscore/osscore.man > prototype/usr/man/man7d/osscore.7d
	else
		exit 1
	fi

	rm -f fpsupport.o

	# Man pages
	for n in $SRCDIR/misc/man7/*.man
	do
		N=`basename $n .man`
		$TXT2MAN -t "$CMD" -v "OSS Devices" -s 7 $n > prototype/usr/man/man7d/$N.7d
	done

	for n in $SRCDIR/misc/man1m/*.man
	do
		N=`basename $n .man`
		$TXT2MAN -t "$CMD" -v "OSS System Administration Commands" -s 1 $n > prototype/usr/man/man1m/$N.1m
	done

	# Other modules for 64bit kernel
	for n in $KERNEL64/target/modules/*.o
	do
  		N=`basename $n .o`
  		if ld -64 -dy -r -Nmisc/osscommon -o prototype/kernel/drv/$KERNEL64/$N $n 
  		then
   			OK=1
		else
   			exit 1
		fi

  		if test $KERNEL64 = "sparcv9"
		then
			grep "^$N[ 	]" $KERNEL64/devices.list >> devlist.txt
  		fi
	done

	# USB Module
	if test -f $KERNEL64/target/modules/oss_usb.o
	then
		if ld -64 -dy -r -Nmisc/osscommon -Nmisc/usba -o prototype/kernel/drv/$KERNEL64/oss_usb $KERNEL64/target/modules/oss_usb.o
 		then
  			OK=1
 		else
  			exit 1
 		fi
 	fi

	# SADA compatibility module
	if test -r $KERNEL64/target/modules/oss_sadasupport.o
	then
		if ld -64 -dy -r -Nmisc/osscommon -Nmisc/$AMSRC -Nmisc/audiosup -Nmisc/mixer -o prototype/kernel/drv/$KERNEL64/oss_sadasupport $KERNEL64/target/modules/oss_sadasupport.o
		then
  			OK=1
		else
			exit 1
		fi
	else
		OK=1
	fi
fi # 64 bit modules done

# Handle 32 bit modules

if test "`ls $KERNEL32/target/modules/*.o 2>/dev/null` " != " "
then

	# osscommon

	if ld -dy -r -Nmisc/usba -o prototype/kernel/misc/osscommon $KERNEL32/target/objects/*.o
	then
		$TXT2MAN -v "OSS Devices" -s 7 $KERNEL32/kernel/drv/osscore/osscore.man > prototype/usr/man/man7d/osscore.7d
	else
		exit 1
	fi

	rm -f fpsupport.o __xtol.o

	# Other modules for 32bit kernel
	for n in $KERNEL32/target/modules/*.o
	do
		N=`basename $n .o`
		if ld -dy -r -Nmisc/osscommon -o prototype/kernel/drv/$N $n
		then
			OK=1
		else
			exit 1
		fi
		grep "^$N[ 	]" $KERNEL32/devices.list >> devlist.txt
	done

	# USB Modules
	if test -f $KERNEL32/target/modules/oss_usb.o
	then
		if ld -dy -r -Nmisc/osscommon -Nmisc/usba -o prototype/kernel/drv/oss_usb $KERNEL32/target/modules/oss_usb.o
		then
			OK=1
		else
			exit 1
		fi
	fi
	
	# SADA Compatibility
	if test -r $KERNEL32/target/modules/oss_sadasupport.o
	then
		if ld -dy -r -Nmisc/osscommon -Nmisc/$AMSRC -Nmisc/audiosup -Nmisc/mixer -o prototype/kernel/drv/oss_sadasupport $KERNEL32/target/modules/oss_sadasupport.o
		then
			OK=1
		else
			exit 1
		fi
	else
		OK=1
	fi

fi # 32 bit modules done

if test "$KERNEL64 " = "sparcv9 "
then
	# Drop SB Live! from the list of supported devices for Sparc
	rm -f tmplist
	mv devlist.txt tmplist
	grep -v "Sound Blaster Live" < tmplist|sort|uniq >devlist.txt
	rm -f tmplist
fi

cp devlist.txt prototype/etc/oss/devices.list

if test -d $KERNEL32/kernel/nonfree
then
	cp -f devlist.txt $KERNEL32/origdir/devlists/Solaris-`uname -p`
fi

# Generate the config files
rm -f confgen
cc -o confgen -DTXT2MAN=\"$TXT2MAN\" $OSFLAGS $KERNEL32/setup/SunOS/confgen.c

./confgen prototype/kernel/drv \\/kernel\\/drv $KERNEL32/kernel/drv/* $KERNEL32/kernel/nonfree/drv/* $KERNEL32/kernel/framework/*
rm -f confgen

# Generate Man pages for user commands
for i in $KERNEL32/target/bin/*
do
	CMD=`basename $i`
	$TXT2MAN -t "$CMD" -v "OSS User Commands" -s 1 $KERNEL32/cmd/$CMD/$CMD.man > prototype/usr/man/man1/$CMD.1
	echo done $CMD
done

# Generate Man pages for system commands
for i in $KERNEL32/target/sbin/*
do
	CMD=`basename $i`
	if test -f $KERNEL32/cmd/$CMD/$CMD.man
	then
		$TXT2MAN -t "$CMD" -v "OSS System Administration Commands" -s 1m $KERNEL32/cmd/$CMD/$CMD.man > prototype/usr/man/man1m/$CMD.1m
		echo done $CMD
	fi
done

# Generate pages for Maintenance Commands 
rm -f prototype/usr/man/man1m/ossdetect.1m
$TXT2MAN -t "ossdetect" -v "OSS User Commands" -s 1m $KERNEL32/os_cmd/SunOS/ossdetect/ossdetect.man > prototype/usr/man/man1m/ossdetect.1m
echo done ossdetect

# Licensing stuff
if test -f $KERNEL32/4front-private/osslic.c
then
	cc $OSFLAGS -o prototype/usr/sbin/osslic -I$KERNEL32/setup -I$KERNEL32/kernel/nonfree/include -I$KERNEL32/kernel/framework/include -I$KERNEL32/include -I$KERNEL32/kernel/OS/SunOS $KERNEL32/4front-private/osslic.c
	strip prototype/usr/sbin/osslic
	
	if test -f prototype/kernel/misc/osscommon
	then 
		prototype/usr/sbin/osslic -q -u -3prototype/kernel/misc/osscommon
	fi
	
	if test -f prototype/kernel/misc/$KERNEL64/osscommon
	then
		prototype/usr/sbin/osslic -q -u -6prototype/kernel/misc/$KERNEL64/osscommon
	fi
fi

if test -f $KERNEL32/4front-private/ossupdate.c
then
  # ossupdate
  cc -I$KERNEL32 $KERNEL32/4front-private/ossupdate.c -s -o prototype/usr/sbin/ossupdate -lsocket -lnsl
fi

sh $SRCDIR/setup/build_common.sh $SRCDIR $OSSLIBDIR

rm -f $OSSLIBDIR/oss/

exit 0
