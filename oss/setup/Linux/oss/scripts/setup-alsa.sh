#!/bin/sh

# This script will restgore selected ALSA modules that were earlier removed by
# remove_drv.sh

if test -d /proc/asound
then
# ALSA is already loaded
  exit 0
fi

if ! test -f /lib/modules/`uname -r`/sound-preoss.tar.bz2
then
  echo ALSA backup archive /lib/modules/`uname -r`/sound-preoss.tar.bz2 not found. Cannot continue.
  exit 1
fi

RESTORE=kernel/sound/soundcore.ko

for n in snd snd-pcm snd-timer snd-page-alloc
do
  RESTORE="$RESTORE kernel/sound/core/$n.ko kernel/sound/acore/$n.ko"
done

(cd /lib/modules/`uname -r` && tar xvfj sound-preoss.tar.bz2 $RESTORE)

if test -d /dev/snd.save && ! test -d /dev/snd
then
  mv /dev/snd.save /dev/snd
fi

exit 0
