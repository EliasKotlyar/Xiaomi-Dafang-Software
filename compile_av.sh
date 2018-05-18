#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export CFLAGS="-muclibc -O3"
export CPPFLAGS="-muclibc -O3"
export LDFLAGS="-muclibc -O3"

if [ ! -d libav/.git ]
then
   git clone git://git.libav.org/libav.git
fi

cd libav/
./configure --arch=mips --target-os=linux --cross-prefix=${CROSS_COMPILE} --enable-cross-compile --disable-debug
make -j4
${CROSS_COMPILE}strip avconv
