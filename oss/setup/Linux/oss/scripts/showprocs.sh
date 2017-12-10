#!/bin/sh
PROCS="`fuser /dev/mixer* /dev/dsp* /dev/audio* /dev/sequencer /dev/music /dev/midi*|sed 's/.* //'|sort|uniq`"

if test "$PROCS " = " "
then
   exit 0
fi

if test "$1 " != "-q "
then
echo $PROCS
echo
echo "NOTICE!"
echo "======="
echo
echo There are some programs still using OSS devices. You may need to stop them
echo manually:
echo
fi

for pid in $PROCS
do
	ps ax|grep "^ *$pid "
done

echo

exit 0
