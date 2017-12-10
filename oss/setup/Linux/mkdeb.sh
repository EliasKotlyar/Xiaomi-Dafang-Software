#!/bin/sh

. ./.directories

VERSION=`sh showversion.sh`
VERSION=${VERSION#v}
RELEASE=`cat buildid.dat`
OSSNAME="oss-linux"

# Chosing the right architecture
if test `uname -m` = "x86_64"; then ARCH=amd64
else ARCH=`uname -m|sed 's/^i[3-9]86/i386/'`
fi

DEBNAME=${OSSNAME}-v${VERSION}-${RELEASE}-${ARCH}

# Checking for known MD5 hashing programs
if type md5sum > /dev/null 2>&1; then MD5=MD5SUM
elif type openssl > /dev/null 2>&1; then MD5=OPENSSL
elif type md5 > /dev/null 2>&1; then MD5=MD5
elif type digest > /dev/null 2>&1; then MD5=DIGEST
else echo "There has been no MD5 creation utily found. deb archive creation will be aborted." && exit 1
fi

echo building $DEBNAME.deb


mkdir control 2>/dev/null
echo "2.0" > debian-binary
cat > control/control << END
Package: $OSSNAME
Version: ${VERSION}-${RELEASE}
Section: sound
Priority: optional
Architecture: $ARCH
Installed-Size: `du -ks prototype | awk '{print $1}'`
Build-Depends: build-essential sed gawk libtool libgtk2.0-dev
Depends: binutils, gcc, libc6, libgtk2.0-0, sed (>= 1.0.0)
Conflicts: libflashsupport
Provides: oss
Suggests: libsdl1.2debian-oss | libsdl1.2debian-all, libesd0, libwine-oss, libsox-fmt-oss, mpg123, gstreamer0.10-plugins-bad (>= 0.10.7), libasound2-plugins
Maintainer: 4Front Technologies <support@opensound.com>
Description: Open Sound System (http://www.opensound.com)
 OSS provides libraries and necessary drivers for practically all sound
  cards on the market including PnP and many PCI ones which enable you
  to play sound files, compose music, use MIDI (only included in the
  testing releases) and adjust your sound card using various user space
  programs.
END

# Copying the menu and copyright file to the right place, taking care that the md5sums generation will take place AFTER this step
mkdir -p prototype/usr/share/menu prototype/usr/share/doc/oss-linux
cp setup/Linux/menu.ex prototype/usr/share/menu/ossxmix
cp setup/Linux/copyright prototype/usr/share/doc/oss-linux/


# Create the MD5 sums file using the program we have found earlier
(cd prototype; find . -type f -exec sh ../setup/Linux/md5.sh "$MD5" "{}" \; > ../control/md5sums)

(cd prototype; find . -type f -print | sed 's/^.//g' | egrep "^/etc/" > ../control/conffiles)


# Removing older builds
rm -rf /tmp/prototype $DEBNAME.deb


cp -pRf prototype /tmp
cp setup/Linux/preinst setup/Linux/postinst setup/Linux/prerm setup/Linux/postrm control/
if test -e prototype/$OSSLIBDIR/lib/libsalsa.so*
then
  cp setup/Linux/shlibs control/
fi


# Correcting file and directory permissions required by lintian
chmod 0755 control/control

# Building control and data archives
(cd control; tar c * | gzip -9 > ../control.tar.gz)
(cd /tmp/prototype; tar c ./* | gzip -9 > data.tar.gz)
mv /tmp/prototype/data.tar.gz .


# Creating the actual archive
ar r $DEBNAME.deb debian-binary control.tar.gz data.tar.gz


# Cleanup
rm -rf /tmp/prototype control control.tar.gz data.tar.gz debian-binary


if test -f 4front-private/export_package.sh
then
  sh 4front-private/export_package.sh $OSSNAME*.deb . `sh showversion.sh` /tmp `uname -i`-26
fi
