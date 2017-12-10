#!/bin/sh

if test "$1 " = " "
then
	echo You need to give the Linux kernel directory as a argument.
	exit 1
fi

TARGETDIR="$1"

if ! test -d $TARGETDIR
then
	echo $TARGETDIR does not exist.
	exit 1
fi

echo
echo Kernel build directory is $TARGETDIR

if test -d $TARGETDIR/sound/oss4
then
  echo error: $TARGETDIR/sound/oss4 already exists. Remove it and try again.
  exit 1
fi

if ! test -d kernel/drv
then
  echo You need to run this script inside the OSS source directory
  exit 1
fi

echo Copying OSS files
if ! cp -R kernel $TARGETDIR/sound/oss4
then
  echo Failed to copy OSS files to $TARGETDIR/sound/oss4.
  exit 1
fi

(cd $TARGETDIR/sound/oss4;mv OS/Linux linux_os;rm -rf OS)
cp setup/Linux/oss/build/*.c $TARGETDIR/sound/oss4/linux_os
cp setup/Linux/oss/build/*.h $TARGETDIR/sound/oss4/linux_os
cp setup/Linux/oss/build/*.inc $TARGETDIR/sound/oss4/linux_os
cp setup/Linux/oss/build/osscore.c $TARGETDIR/sound/oss4/framework/osscore/

rm -rf $TARGETDIR/sound/oss4/drv/Makefile

# Remove some non-Linux drivers
rm -rf $TARGETDIR/sound/oss4/drv/oss_sadasupport
rm -rf $TARGETDIR/sound/oss4/drv/osscore
rm -rf $TARGETDIR/sound/oss4/drv/oss_audiocs
rm -rf $TARGETDIR/sound/oss4/nonfree

# Remove MIDI support that is not functional yet.
rm -rf $TARGETDIR/sound/oss4/framework/midi

# Remove all previous Makefiles that are bogus
find $TARGETDIR/sound/oss4 -name Makefile -exec rm {} ';'

# Remove the man files
find $TARGETDIR/sound/oss4 -name '*.man' -exec rm {} ';'

cat<<END_OF_CONFIG >$TARGETDIR/sound/oss4/Kconfig

config OSS_VMIX
	bool "Virtual audio mixer (vmix) suport"
	default y
	
config OSS_VMIX_FLOAT
	bool "Use floating point arithmetic for vmix computations"
	default y

config OSS_MIDI
	bool "MIDI support (partially functional)"
	default n

END_OF_CONFIG

echo Generating drivers
for n in $TARGETDIR/sound/oss4/drv/*
do
     N=`basename $n|tr 'a-z' 'A-Z'`
     BN=`basename $n`
  
     echo config $N				>>$TARGETDIR/sound/oss4/Kconfig
     echo "	tristate" \"`head -1 $n/.name`\">>$TARGETDIR/sound/oss4/Kconfig
     echo "	depends on" OSS4		>>$TARGETDIR/sound/oss4/Kconfig
     echo "	default y"			>>$TARGETDIR/sound/oss4/Kconfig
     echo					>>$TARGETDIR/sound/oss4/Kconfig

     mv $TARGETDIR/sound/oss4/drv/$BN/$BN.c $TARGETDIR/sound/oss4/drv/$BN/"$BN"_main.c
     cp target/build/$BN.c $TARGETDIR/sound/oss4/drv/$BN/"$BN"_stub.c

     echo obj-\$\(CONFIG_OSS_$N\)	+= drv\/$BN/ >> $TARGETDIR/sound/oss4/Makefile
     echo obj-\$\(CONFIG_OSS_$N\)	:= $BN.o > $TARGETDIR/sound/oss4/drv/$BN/Makefile
     echo 		>> $TARGETDIR/sound/oss4/drv/$BN/Makefile

     for fn in $TARGETDIR/sound/oss4/drv/$BN/*.c
     do
	fn=`basename $fn .c`.o
        echo $BN-objs += $fn		>> $TARGETDIR/sound/oss4/drv/$BN/Makefile
     done
done

echo Done

if ! grep -q OSS4 $TARGETDIR/sound/Kconfig
then
  
  echo
  echo NOTICE
  echo
  echo As the final step you need to copy/paste the following lines to
  echo $TARGETDIR/sound/Kconfig
  echo The right location is just above the last endmenu line.

  cat <<END_OF_CONFIG
----- cut here ---
menu "Open Sound System v4.x"

config OSS4
	tristate "Open Sound System v4.x support"
	depends on SOUND!=n
	depends on !SND
	depends on !SOUND_PRIME
	help
          This is version 4.x of Open Sound System. It replaces
	  the older (DEPRECATED) OSS version that has been included
	  in the kernel during past 10 years.

	  You need to disabele both ALSA and the older OSS version
	  before being able to compile OSSv4.

source "sound/oss4/Kconfig"
endmenu
----- cut here ---
END_OF_CONFIG

echo Finally append the following line to $TARGETDIR/sound/Makefile
echo 'obj-$(CONFIG_OSS4) += oss4/'
fi
exit 0
