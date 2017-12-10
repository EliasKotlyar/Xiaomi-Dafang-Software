#!/bin/sh

if test "$USER " != "root "
then
  echo "You must be super-user or logged in as root to install OSS"
  exit 1
fi

if test "$1 " != " "
then
  OSSLIBDIR="$1"
else
  OSSLIBDIR="/usr/lib/oss"
fi

echo "Installing Open Sound System `cat "./$OSSLIBDIR/version.dat"`...."
echo "Copying files from ./etc and ./usr into /..."
tar -cpf - etc usr |(cd /; tar -xpf -)
echo "Running /usr/lib/oss/build/install script...."
if ! sh "/$OSSLIBDIR/build/install.sh"
then
  echo
  echo "ERROR: install.sh script failed"
  exit 0
fi
echo "OSS installation complete..."
echo
echo "Run /usr/sbin/soundon to start the drivers"
echo "Run /usr/bin/osstest to test the audio"
echo "Run /usr/bin/ossinfo to display status"
