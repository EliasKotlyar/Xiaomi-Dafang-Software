#!/bin/sh

. ./.directories

VERSION=`sh showversion.sh`
RELEASE=`cat buildid.dat`
ARCH=`uname -m`
OSSNAME=oss-linux

RPMNAME=$OSSNAME-$VERSION
PKGNAME=$OSSNAME-$VERSION-$RELEASE.$ARCH
echo building $RPMNAME.rpm

rm -rf spec $RPMNAME
mkdir $RPMNAME
echo "Version: " $VERSION > spec
echo "Release: " $RELEASE >> spec
echo "Name: " $OSSNAME >> spec
cat setup/Linux/spec.tmpl | sed "s:OSSLIBDIR:\"$OSSLIBDIR\":g" >> spec
echo "%files" >> spec
(cd prototype; find . -type f -print | sed 's/^.//g' > /tmp/filelist)
cat /tmp/filelist >> spec
rm -rf /tmp/prototype
cp -af prototype /tmp
tar zcvf /tmp/oss $RPMNAME
rpmbuild -bb --buildroot=/tmp/prototype --define "_sourcedir /tmp" --define "_rpmdir ./" --define '_build_name_fmt %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm' spec
# Cleanup
rm -rf /tmp/oss /tmp/filelist $RPMNAME spec

if test -f 4front-private/export_package.sh
then
  sh 4front-private/export_package.sh $PKGNAME.rpm . `sh showversion.sh` /tmp `uname -i`-26
fi
