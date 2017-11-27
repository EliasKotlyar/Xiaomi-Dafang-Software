#!/bin/sh
  mipsel-linux-gcc image_enh_test.c -lm -o image_enh_test
 scp image_enh_test boliu@192.168.8.2:/home1/nfsroot/boliu/hyli/nfsroot/