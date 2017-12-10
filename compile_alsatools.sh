#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/alsa-tools/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export CFLAGS=""
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

./configure --host=mips-linux --enable-debug
make
