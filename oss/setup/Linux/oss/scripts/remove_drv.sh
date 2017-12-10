#!/bin/sh
if test -f /etc/oss.conf
then
  . /etc/oss.conf
else
  OSSLIBDIR=/usr/lib/oss
fi

# This script wipes out the previously installed sound drivers
# from the system.

# Backup all kernel sound drivers (ALSA) and remove the kernel/sound
# directory from the system. Untar the backup package to return ALSA
# back in business.

if test -x /sbin/chkconfig
then
  /sbin/chkconfig alsasound off > /dev/null 2>&1
elif test -x /usr/sbin/update-rc.d
then
  /usr/sbin/update-rc.d -f alsa-utils remove > /dev/null 2>&1
elif test -x /usr/sbin/alsa
then
  /usr/sbin/alsa force-unload > /dev/null 2>&1
fi

if test -d /lib/modules/`uname -r`/kernel/sound
then
	if ! test -f /lib/modules/`uname -r`/sound-preoss.tar.bz2
	then
	   (cd /lib/modules/`uname -r`; tar cfj /lib/modules/`uname -r`/sound-preoss.tar.bz2 kernel/sound)
	fi

	rm -rf /lib/modules/`uname -r`/kernel/sound
	depmod -a
fi

# Kill all applications using ALSA or OSS/Free devices

# We have to use ugly replacement of fuser since this command got broken
# in some Linux recent distributions.

KILL=0

for n in /proc/[0-9]*
do
  PID=`basename $n`
  if test "`ls -l $n/fd/* 2>/dev/null|grep /dev/snd` " != " "
  then
	KILL=1
  fi

  if test "`ls -l $n/fd/* 2>/dev/null|grep /dev/mixer` " != " "
  then
	KILL=1
  fi
done

if ! test -d $OSSLIBDIR/save
then
	mkdir $OSSLIBDIR/save
fi

if test "$KILL " = "1 "
then
echo killing
  rm -f /dev/mixer.old
  mv /dev/mixer /dev/mixer.old 2>/dev/null
  #if test -d /dev/snd
  #then
	#(cd /;tar cfj $OSSLIBDIR/save/alsadevs.tar.bz2 dev/snd)
  #fi

  #mv /dev/snd /dev/snd.osssave
  #fuser -k -s /dev/mixer.old /dev/snd.osssave/*
fi

# Remove all loaded ALSA modules
SOUNDDEVS=

if test -f /dev/mixer.old
then
	SOUNDDEVS="$SOUNDDEVS /dev/mixer.old"
fi

if test -d /dev/snd.osssave
then
	SOUNDDEVS="$SOUNDDEVS /dev/snd.osssave/*"
fi

for timeout in 0 1 2 3 4 5 6 7 8 9 10 11
do
   if test "`cat /proc/modules|grep ^snd_|sed 's/ .*//'` " = " "
   then
      break
   fi

   if test $timeout -gt 10
   then
     	echo Cannot unload the ALSA modules. Apparently there is some
	echo application keeping them busy.
	echo Please reboot your system and try to start OSS again.
	ps ax
	lsmod
	cat /proc/devices
	cat /proc/interrupts
     	exit 1
   fi

   if test "$SOUNDDEVS " != " "
   then
      fuser -s -9 $SOUNDDEVS
   else
      echo Cannot find any processes using the conflicting sound driver
   fi

   for n in `cat /proc/modules|grep ^snd_|sed 's/ .*//'`
   do
	rmmod $n
	#rmmod $n >/dev/null 2>&1
   done

   sleep 1
done

rmmod snd > /dev/null 2>&1

# Remove soundcore
rmmod soundcore > /dev/null 2>&1

rm -f /dev/mixer.old

if cat /proc/devices|grep -q '^ *14 '
then

	echo There still appears to be another sound driver hanging around

	lsmod
	cat /proc/devices|grep '^ *14 '
	cat /proc/interrupts

	exit 1
fi

for n in /dev/sndstat /dev/mixer* /dev/dsp* /dev/midi* /dev/sequencer /dev/music
do
	if readlink $n >/dev/null 2>&1
	then # Symbolic link
	   if readlink $n | grep -q asound
	   then # Link to ALSA devices
		rm -f $n
	   fi
	fi
done

# Disable automatic startup of ALSA during system bootup

if test "`ls /etc/rc.d/rc*/*alsasound*` " != " " > /dev/null 2>&1
then
  (cd /;tar cfj $OSSLIBDIR/save/alsarc/tar.bz2 etc/rc.d/rc*/*alsasound*)
  rm -f /etc/rc.d/rc*/*alsasound*
fi > /dev/null 2>&1

exit 0
