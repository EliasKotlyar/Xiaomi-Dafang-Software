#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/../mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CROSS_COMPILE=${CROSS_COMPILE}
export CC=${CROSS_COMPILE}gcc
export CPLUSPLUS=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld


LIBUSB_PATH=${PWD}/../libusb-1.0.21/_install
#CFLAGS=-I$LIBUSB_PATH/include/libusb-1.0


LIBCOMPAT_PATH=${PWD}/../libusb-compat-0.1.5/_install
CFLAGS=-I$LIBCOMPAT_PATH/include/
#LDFLAG=-L$LIBUSB_PATH/lib $LDFLAG

LDFLAG="-lusb -lusb-1.0 -L$LIBUSB_PATH/lib -L$LIBCOMPAT_PATH/lib "

export CFLAGS="-muclibc -O2  -c -O2 -Wall $CFLAGS"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2 $LDFLAG"

echo $CFLAGS
echo $LDFLAGS

make clean
make