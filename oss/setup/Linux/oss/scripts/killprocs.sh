#!/bin/sh
PROCS="`fuser /dev/mixer* /dev/dsp* /dev/audio* /dev/sequencer /dev/music /dev/midi* 2>/dev/null`"

if test "$PROCS " = " "
then
   exit 0
fi

for pid in $PROCS
do
	#ps ax|grep "^ *$pid "
	echo killing $pid
	kill $pid
done

sleep 2
exit 0
