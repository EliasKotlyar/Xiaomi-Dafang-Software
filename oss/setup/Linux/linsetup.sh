#!/bin/sh

if test "$CONFIGURE " != "YES "
then
	echo
	echo Error: Wrong usage
	echo
	echo You must run `dirname $0`/configure instead of $0
	exit 1
fi

if test "`ls .` " != " "
then
	echo Error: Current directory must be empty
	exit 1
fi

if test "`uname -m` " = "arm "
then
# ARM doesn't have regparm support

	echo Setting up for ARM architecture
	sh $SRCDIR/setup/setupdir.sh
	exit 0
fi

# Setup directory for REGPARM

unset NO_REGPARM
export USE_REGPARM=1

echo Setting up full REGPARM compiling environment

echo "SUBDIRS+=noregparm" > .makefile
sh $SRCDIR/setup/setupdir.sh

# Setup for NOREGPARM

mkdir noregparm

unset USE_REGPARM
export NO_REGPARM=1

echo > .nocopy
echo Setting up kernel-only NOREGPARM compiling environment
(cd noregparm;sh $SRCDIR/setup/setupdir.sh -K)
rm -f .nocopy

# Make sure both versions share the same timestamp.h file.
cp -f kernel/framework/include/timestamp.h noregparm/kernel/framework/include/timestamp.h

exit 0
