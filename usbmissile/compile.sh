#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/../mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CROSS_COMPILE=${CROSS_COMPILE}
export CC=${CROSS_COMPILE}gcc
export CPLUSPLUS=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld

LIBUSB_PATH=${PWD}/../libusb-compat-0.1.5/_install/
export CFLAGS="-muclibc -O2 -I$LIBUSB_PATH/include/ -c -O2 -Wall"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2 -L$LIBUSB_PATH/lib -lusb"
make clean
make