#!/usr/bin/env bash

TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export CFLAGS="-muclibc -O2 -fomit-frame-pointer -DL_ENDIAN "
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"


cd oss_build/cmd/ossplay
make clean
make
