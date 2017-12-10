#!/bin/sh
OSSVER=`sh showversion.sh`
BUILDID=`cat buildid.dat`
TOPDIR=`pwd`
SETUPDIR=$TOPDIR/setup/FreeBSD
PROTODIR=$TOPDIR/prototype
ARCH=`uname -m`
KERNEL=`uname -r | cut -s -d'.' -f1`
PKGNAME=oss-freebsd$KERNEL-$ARCH-$OSSVER-$BUILDID

(cd $PROTODIR; find . -type f -print  > $SETUPDIR/pkg-plist)

if test  $KERNEL = "10" 
then
(cd $PROTODIR; 
echo "name:  oss-freebsd$KERNEL-$ARCH" >> +MANIFEST
echo "origin: kernel/drivers" >> +MANIFEST
echo "version: $OSSVER-$BUILDID" >> +MANIFEST
echo "comment: Sound Drivers from 4Front Tech">>+MANIFEST
echo "www: http://www.opensound.com" >> +MANIFEST
echo "maintainer: dev@opensound.com" >> +MANIFEST
echo "prefix: /" >> +MANIFEST
echo "licenselogic: single" >> +MANIFEST
echo "licenses: [BSD]" >> +MANIFEST
echo "desc:" >> +MANIFEST
cat  $SETUPDIR/pkg-descr >> +MANIFEST
cp $SETUPDIR/pkg-postinstall +POST_INSTALL
cp $SETUPDIR/pkg-preremove +PRE_DEINSTALL
cp $SETUPDIR/pkg-postremove +POST_DEINSTALL
cp $SETUPDIR/pkg-plist plist
)
pkg create -m $PROTODIR -r $PROTODIR/ -p $PROTODIR/plist -o .

if test -f 4front-private/export_package.sh
then
  sh 4front-private/export_package.sh $PKGNAME.txz . `sh showversion.sh` /tmp `uname -m`
fi
else

(cd /; pkg_create -c $SETUPDIR/pkg-comment -d $SETUPDIR/pkg-descr -I $SETUPDIR/pkg-postinstall -k $SETUPDIR/pkg-preremove -K $SETUPDIR/pkg-postremove -f $SETUPDIR/pkg-plist -p / -S $PROTODIR -v $TOPDIR/$PKGNAME.tbz)

if test -f 4front-private/export_package.sh
then
  sh 4front-private/export_package.sh $PKGNAME.tbz . `sh showversion.sh` /tmp `uname -m`
fi
fi
