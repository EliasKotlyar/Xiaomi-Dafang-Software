#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export CFLAGS="-DDEBUG_TRACE -DFAKE_ROOT "
export LIBS=""
export LD=${CROSS_COMPILE}ld
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

rm dropbearmulti
cd dropbear-2017.75/
make clean
./configure --host=mips-linux --disable-zlib
make PROGRAMS="dropbear dbclient scp dropbearkey dropbearconvert" MULTI=1 SCPPROGRESS=1
mv dropbearmulti ../dropbearmulti
cd ..