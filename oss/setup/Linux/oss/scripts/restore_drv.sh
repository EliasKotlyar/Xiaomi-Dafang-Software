#!/bin/sh
if test -f /etc/oss.conf
then
  . /etc/oss.conf
else
  OSSLIBDIR=/usr/lib/oss
fi

/usr/sbin/soundoff

rm -rf /lib/modules/`uname -r`/kernel/oss

if test -x /sbin/chkconfig 
then /sbin/chkconfig oss off > /dev/null 2>&1
else
  if test -x /sbin/update-rc.d
  then /usr/sbin/update-rc.d -f oss remove > /dev/null 2>&1
  fi
fi

rm -f /etc/init.d/oss

if ! test -d /lib/modules/`uname -r`/kernel/sound
then
   if test -f /lib/modules/`uname -r`/sound-preoss.tar.bz2
   then
     (cd /lib/modules/`uname -r`; tar xfj sound-preoss.tar.bz2)
     /sbin/depmod -a
   fi
fi

rm -f /lib/modules/`uname -r`/sound-preoss.tar.bz2

if test -f $OSSLIBDIR/sysfiles.list
then
	rm -f `cat $OSSLIBDIR/sysfiles.list`
fi

if test -f $OSSLIBDIR/save/alsadevs.tar.bz2
then
  (cd /;tar xfj $OSSLIBDIR/save/alsadevs.tar.bz2)
fi

if test -f $OSSLIBDIR/save/alsarc/tar.bz2
then
  (cd /;tar xfj $OSSLIBDIR/save/alsarc/tar.bz2)
fi

rm -f /dev/dsp* /dev/midi* /dev/mixer* /dev/sndstat

/sbin/ldconfig

if test -x /sbin/chkconfig
then
  /sbin/chkconfig alsasound on > /dev/null 2>&1
else
  if test -x /usr/sbin/update-rc.d
  then
    /usr/sbin/update-rc.d alsa-utils defaults > /dev/null 2>&1
  fi
fi

exit 0
