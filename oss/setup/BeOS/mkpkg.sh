#!/bin/sh

VERSION=`sh showversion.sh`
RELEASE=`cat buildid.dat`
OSSNAME=oss-beos
PKGNAME=$OSSNAME-$VERSION-$RELEASE

echo building $PKGNAME.zip
(cd prototype; zip -ry9 ../$PKGNAME.zip *)

#if test -f 4front-private/export_package.sh
#then
#  sh 4front-private/export_package.sh $PKGNAME.zip . `sh showversion.sh` /tmp `uname -i`-26
#fi

exit 0
