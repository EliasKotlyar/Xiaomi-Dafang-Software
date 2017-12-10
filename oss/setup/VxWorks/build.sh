#!/bin/sh

rm -rf prototype 
mkdir prototype
mkdir prototype/oss-install

if test "$LD " = " "
then
  LD=ar
fi

$LD -r -o prototype/oss-install/osscore.o target/objects/*.o

cp target/modules/*.o prototype/oss-install/

if test "$AR " = " "
then
  AR=ar
fi

$AR rc prototype/oss-install/liboss.a prototype/oss-install/*.o

exit 0
