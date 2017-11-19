#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export CFLAGS=""
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

rm bftpd
cd bftpd/
make clean
./configure --host=mips-linux --enable-debug
make
