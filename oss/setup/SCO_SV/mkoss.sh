#!/usr/bin/ksh

BASEDIR=`pwd`
PKGDIR=$BASEDIR/combpkg
UW7DIR=$BASEDIR/oss-4.0-uw7.BUILD
OSR6DIR=$BASEDIR/oss-4.0-osr6.BUILD
COMBDIR=$BASEDIR/oss-4.0-comb
ARCHFILES="/usr/bin/ossxmix"
SETUPDIR=$COMBDIR/setup

# We must do this on a UW7 machine to redo the manual pages properly
if [ "`uname -s`" != "UnixWare" ]; then
	echo "Error: the combined build must be done on a UnixWare 7 machine!"
	exit 1
fi

# Make sure we have the built directories in place
if [ ! -d $UW7DIR ]; then
	echo "Error: you must have a build $UW7DIR directory installed!"
	exit 1
fi
if [ ! -d $OSR6DIR ]; then
	echo "Error: you must have a build $OSR6DIR directory installed!"
	exit 1
fi

#
# Recreate the combined directory
#
rm -rf $COMBDIR
mkdir $COMBDIR

# Copy the OSR6 files
cd $OSR6DIR
find prototype | cpio -pdumvL $COMBDIR

# Rename OSR6/copy UW7 files which are OS dependent
for file in $ARCHFILES
do
	mv $COMBDIR/prototype/$file $COMBDIR/prototype/$file.osr6
	cp $UW7DIR/prototype/$file $COMBDIR/prototype/$file.uw7
done

# Use the UW7 manual pages, which have the right format
rm -rf $COMBDIR/prototype/usr/man/*
cp -r $UW7DIR/prototype/usr/man/* $COMBDIR/prototype/usr/man

# Recreate the manual pages as plain installable text files
MANPATH=$COMBDIR/prototype/usr/man; export MANPATH
cd $MANPATH
find * -print | while read file
do
	mfile=${file#man[1-9]/}
	if [ "$file" != "$mfile" ]; then
		mfile=${mfile%\.[1-9]}
		man $mfile - > /dev/null
	else
		# Rename the directory the correct way
		sect=${file#man}
		mv $MANPATH/$file $MANPATH/man.$sect
	fi
done
rm -rf $MANPATH/man.[0-9]

# Copy the packaging files
rm -rf $SETUPDIR
mkdir $SETUPDIR
while read file
do
	cp $OSR6DIR/setup/SCO_SV/$file $SETUPDIR
done <<!EOF
	mkpkg.sh
	S89oss
	SCO_EULA
	i.drvcfg
	pkgdepend
	pkginfo
	postinstall
	postremove
	preremove
	r.drvcfg
!EOF

# Copy the new/modified packaging files
cd $PKGDIR
find * -print | cpio -pdumvL $SETUPDIR

# Create a version file
grep "define OSS_VERSION_ID" $OSR6DIR/kernel/framework/include/oss_version.h|sed 's/.*_ID "/v/'|sed 's/"//' > $COMBDIR/.version

# Copy the buildid file
cp $OSR6DIR/buildid.dat $COMBDIR

# Run the  mkpkg.sh to create the final package
cd $COMBDIR
./setup/mkpkg.sh
