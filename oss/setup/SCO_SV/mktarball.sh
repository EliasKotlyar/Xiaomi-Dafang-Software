#!/bin/sh

. ./.directories

VERSION=`sh showversion.sh`
RELEASE=`cat buildid.dat`
if test "`uname -s` " = "UnixWare "
then
  OSSNAME=oss-unixware
else
  OSSNAME=oss-osr6
fi

PKGNAME=$OSSNAME-$VERSION-$RELEASE-`uname -m`

echo building $PKGNAME.tar.Z
cp ./setup/SCO_SV/removeoss.sh prototype/$OSSLIBDIR/scripts
(cd prototype; find . -type f -print) > prototype/$OSSLIBDIR/MANIFEST
(cd prototype; tar cf - . ) | compress > $PKGNAME.tar.Z

if test -f 4front-private/export_package.sh
then
  sh 4front-private/export_package.sh $PKGNAME.tar.Z . `sh showversion.sh` /tmp `uname -m`
fi

exit 0
