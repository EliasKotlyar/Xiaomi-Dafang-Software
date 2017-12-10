#!/bin/sh
if test `whoami` != "root"
then
  echo "You must be super-user or logged in as root to uninstall OSS..."
  exit 0
fi

if test -f /etc/oss.conf
then
  . /etc/oss.conf
else
  OSSLIBDIR=/usr/lib/oss
fi

echo "Uninstalling OSS...."
echo "Running soundoff...."
/usr/sbin/soundoff
echo "Restoring previously install sound drivers..."
sh "$OSSLIBDIR"/scripts/restore_drv.sh
echo "Removing OSS Files in MANIFEST"
cd /
for i in `cat "$OSSLIBDIR"/MANIFEST`
do
# echo "Removing file $i"
rm -f $i
done

echo "Removing $OSSLIBDIR directory"
rm -rf "$OSSLIBDIR"

echo "OSS Uninstalled. However you may need to reboot the system."
