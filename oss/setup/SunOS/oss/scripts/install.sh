#!/bin/bash

echo "Setting up Open Sound System....please wait"

if test -f /etc/oss.conf
then
  . /etc/oss.conf
else
  OSSLIBDIR=/usr/lib/oss
fi

rm -f /etc/rc3.d/S80ossinstall # Remove temporary installer

# Remove previous sound drivers and make a backup of the removed
# /etc/driver_aliases lines.
rem_drv oss > /dev/null 2>&1 		# OSS 3.99.x and earlier
rem_drv ossaudios > /dev/null 2>&1	# Old name for sadasupport

for n in audiocs audioens audiots audio1575 audiovia823x audiohd audio810 audioixp usb_ac usb_as
do
	# Copy the /etc/driver_aliases entries for the SADA drivers to a backup
	# file so that the drivers could be restored later.
	grep "^$n " /etc/driver_aliases >> /usr/lib/oss/etc/driver_aliases.removed

	rem_drv $n > /dev/null 2>&1	# Remove the driver
done

# Make sure the driver aliases collection doesn't contain duplicate lines
sort /usr/lib/oss/etc/driver_aliases.removed|uniq > /usr/lib/oss/etc/driver_aliases.bak
mv /usr/lib/oss/etc/driver_aliases.bak /usr/lib/oss/etc/driver_aliases.removed
rm -f /usr/lib/oss/etc/driver_aliases.bak

# Remove previous OSS and SADA devices
rm -f /dev/sound/*
rm -f /dev/audio*
rm -f /dev/dsp*
rm -f /dev/mixer*
rm -f /dev/midi*
rm -f /dev/sndstat 
rm -f /dev/sequencer 
rm -f /dev/music

sync

rm -f /tmp/osspkg.tmp
grep -v type=oss_ /etc/devlink.tab > /tmp/osspkg.tmp
cat /tmp/osspkg.tmp > /etc/devlink.tab
cat >> /etc/devlink.tab <<EOF
type=oss_sysdev	\M0
type=oss_audio	oss/\M1/\M2
EOF

if test ! -f $OSSLIBDIR/etc/userdefs
then
  echo "autosave_mixer yes" > $OSSLIBDIR/etc/userdefs
fi

if /usr/xpg4/bin/grep -q 'install_imux yes' $OSSLIBDIR/etc/userdefs
then
	# Install the imux driver
	/usr/sbin/ossdetect -i
else
	/usr/sbin/ossdetect 
fi

/usr/sbin/devlinks
/usr/sbin/ossdevlinks

# Symlink the config files to $OSSLIBDIR/conf
#cd $OSSLIBDIR/conf
#ARCH=`uname -m`
#for n in `grep -l 'Open Sound System configuration file' /kernel/drv/*.conf`
#do
#  ln -s $n .
#done

#echo ""
#echo "Adding OSS startup scripts to /etc/rc3.d and /etc/init.d"
#rm -f /etc/rc3.d/S89oss /etc/rc3.d/S99oss /etc/init.d/oss

#cp $OSSLIBDIR/etc/S89oss /etc/init.d/oss
#chmod 744 /etc/init.d/oss

#ln -s ../init.d/oss /etc/rc3.d/S89oss
#ln -s ../init.d/oss /etc/rc3.d/K89oss

echo ""
echo ""
echo ""
echo Open Sound System installation complete
echo ""
echo You can use the osstest command to test audio playback in your system.
echo ""
echo It may be necessary to reboot the system before all devices get properly
echo detected by the system.

exit 0
