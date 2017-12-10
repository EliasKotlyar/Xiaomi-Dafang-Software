#!/bin/bash
if test `whoami` != "root"
then
  echo "You must be super-user or logged in as root to uninstall OSS..."
  exit 0
fi

echo "Uninstalling OSS...."
echo "Running soundoff...."
/usr/sbin/soundoff

echo Uninstalling OSS modules
cp -f $OSSLIBDIR/etc/installed_drivers /tmp/installed_drivers
(cd $OSSLIBDIR;rm -rf etc/installed_drivers etc/legacy_devices logs conf)

# Remove the drivers - preremove will copy installed_drivers to /tmp
for n in `ls $OSSLIBDIR/modules`
do
 if [ -d /etc/conf/pack.d/$n ]; then
    /etc/conf/bin/idinstall -P oss -d $n > /dev/null 2>&1
    rm -f $OSSLIBDIR/modules/$n/install.log
 fi
done

echo "Removing OSS Files in MANIFEST"
cd /
for i in `cat /usr/lib/oss/MANIFEST`
do
# echo "Removing file $i"
rm -f $i
done

echo "Removing /usr/lib/oss directory"
rm -rf /usr/lib/oss

echo "OSS Uninstalled. However you may need reboot the system."
