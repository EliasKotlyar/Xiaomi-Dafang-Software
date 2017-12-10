#!/bin/sh
if test "`uname -s`" = "UnixWare"
then
  OS=unixware
else
  OS=osr6
fi
# Force both OS versions
OS=unixware_osr6

ARCH=i386

PKG=oss

VERSION=`cat ./.version`
BUILDID=`cat ./buildid.dat`
PKGVERSION=$VERSION-$BUILDID
PKGFILE=$PKG-$OS-$PKGVERSION-$ARCH.pkg
TOPDIR=`pwd`
# Setup the scripts
echo "CLASSES=none drvcfg" > ./setup/pkginfo
echo "BASEDIR=/" >> ./setup/pkginfo
echo "PKG=$PKG" >> ./setup/pkginfo
echo "NAME=Open Sound System" >> ./setup/pkginfo
echo "VERSION=$PKGVERSION" >> ./setup/pkginfo
echo "CATEGORY=driver" >> ./setup/pkginfo
echo "DESC=Open Sound System for SCO $OS" >> ./setup/pkginfo
echo "ARCH=$ARCH" >> ./setup/pkginfo
echo "VENDOR=4Front Technologies" >> ./setup/pkginfo
echo "HOTLINE=+1 (310) 202 8530" >> ./setup/pkginfo
echo "EMAIL=support@opensound.com" >> ./setup/pkginfo

echo "i pkginfo=./setup/pkginfo"         > /tmp/$$
if test -f .date
then
  # Open Source Version
  echo "i copyright=./Copying"                 >>/tmp/$$
else
  # Retail version
  echo "i copyright=./setup/SCO_EULA"        >>/tmp/$$
fi
echo "i depend=./setup/pkgdepend"        >> /tmp/$$
echo "i postinstall=./setup/postinstall" >> /tmp/$$
echo "i preremove=./setup/preremove"     >> /tmp/$$
echo "i postremove=./setup/postremove"   >> /tmp/$$
echo "i request=./setup/request"         >> /tmp/$$
echo "i i.drvcfg=./setup/i.drvcfg"       >> /tmp/$$
echo "i r.drvcfg=./setup/r.drvcfg"       >> /tmp/$$

# now get a list of all the files and directories
(cd prototype; find . -type f -print |pkgproto  >> /tmp/$$)
(cd prototype; find . -type l -print |pkgproto  >> /tmp/$$)
(cd prototype; find usr/lib/oss -type d -print | pkgproto >> /tmp/$$)
# Change the owner/group for the files
OLDOG="`id -un` `id -gn`"
cp /tmp/$$ /tmp/$$.o
cat /tmp/$$.o | sed -e "s/${OLDOG}/root sys/g" > /tmp/$$

# For some files, change the file type of .conf files to editable and set
# class to drvcfg. For architecture specific files, just set the class.
EXCEPTIONLIST='Space.c|userdefs|.uw7|.osr6'
/bin/grep -v -E -e $EXCEPTIONLIST /tmp/$$ > proto
EXCEPTIONLIST='Space.c|userdefs|.uw7$|.osr6$'
/bin/grep -E -e $EXCEPTIONLIST /tmp/$$ |sed -e 's/f none/e drvcfg/g' >> proto
EXCEPTIONLIST='.uw7'
/bin/grep -E -e $EXCEPTIONLIST /tmp/$$ |sed -e 's/f none/f uw7/g' >> proto
EXCEPTIONLIST='.osr6'
/bin/grep -E -e $EXCEPTIONLIST /tmp/$$ |sed -e 's/f none/f osr6/g' >> proto

# Remove the temp file.
echo "removing /tmp/$$"; rm -f /tmp/$$

#now create the package.
pkgmk -o -d /tmp -r $TOPDIR/prototype -a $ARCH -f proto
echo package file is $PKGFILE
pkgtrans -s /tmp $PKGFILE $PKG
echo package file is $PKGFILE
mv /tmp/$PKGFILE $TOPDIR
