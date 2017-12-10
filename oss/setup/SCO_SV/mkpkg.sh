#!/bin/sh
if test "`uname -s`" = "UnixWare"
then
  OS=unixware
else
  OS=osr6
fi

ARCH=i386

PKG=oss

VERSION=`sh showversion.sh`
BUILDID=`cat ./buildid.dat`
PKGVERSION=$VERSION-$BUILDID
PKGFILE=$PKG-$OS-$PKGVERSION-$ARCH.pkg
TOPDIR=`pwd`
# Setup the scripts
echo "CLASSES=none drvcfg" > ./setup/SCO_SV/pkginfo
echo "BASEDIR=/" >> ./setup/SCO_SV/pkginfo
echo "PKG=$PKG" >> ./setup/SCO_SV/pkginfo
echo "NAME=Open Sound System" >> ./setup/SCO_SV/pkginfo
echo "VERSION=$PKGVERSION" >> ./setup/SCO_SV/pkginfo
echo "CATEGORY=driver" >> ./setup/SCO_SV/pkginfo
echo "DESC=Open Sound System for SCO $OS" >> ./setup/SCO_SV/pkginfo
echo "ARCH=$ARCH" >> ./setup/SCO_SV/pkginfo
echo "VENDOR=4Front Technologies" >> ./setup/SCO_SV/pkginfo
echo "HOTLINE=+1 (310) 202 8530" >> ./setup/SCO_SV/pkginfo
echo "EMAIL=support@opensound.com" >> ./setup/SCO_SV/pkginfo

echo "i pkginfo=./setup/SCO_SV/pkginfo"         > /tmp/$$
if test -f .date
then
  # Open source version
  echo "i copyright=./COPYING"                  >>/tmp/$$
else
  # Retail version
  echo "i copyright=./EULA"                     >>/tmp/$$
fi
echo "i depend=./setup/SCO_SV/pkgdepend"        >> /tmp/$$
echo "i postinstall=./setup/SCO_SV/postinstall" >> /tmp/$$
echo "i preremove=./setup/SCO_SV/preremove"     >> /tmp/$$
echo "i postremove=./setup/SCO_SV/postremove"   >> /tmp/$$
echo "i i.drvcfg=./setup/SCO_SV/i.drvcfg"       >> /tmp/$$
echo "i r.drvcfg=./setup/SCO_SV/r.drvcfg"       >> /tmp/$$

# now get a list of all the files and directories
(cd prototype; find . -type f -print |pkgproto  >> /tmp/$$)
(cd prototype; find . -type l -print |pkgproto  >> /tmp/$$)
(cd prototype; find usr/lib/oss -type d -print | pkgproto >> /tmp/$$)

# now change the file type of .conf files to editable and set class to drvcfg
EXCEPTIONLIST='Space.c|userdefs'

/bin/grep -v -E -e $EXCEPTIONLIST /tmp/$$ > proto
/bin/grep -E -e $EXCEPTIONLIST /tmp/$$ |sed -e 's/f none/e drvcfg/g' >> proto

# Remove the temp file.
echo "removing /tmp/$$"; rm -f /tmp/$$

#now create the package.
pkgmk -o -d /tmp -r $TOPDIR/prototype -a $ARCH -f proto
echo package file is $PKGFILE
pkgtrans -s /tmp $PKGFILE $PKG
echo package file is $PKGFILE
mv /tmp/$PKGFILE $TOPDIR

if test -f 4front-private/export_package.sh
then
  if sh 4front-private/export_package.sh $PKGFILE . `sh showversion.sh` /tmp `uname -m`
  then
    exit 0
  else
    exit 1
  fi
fi
