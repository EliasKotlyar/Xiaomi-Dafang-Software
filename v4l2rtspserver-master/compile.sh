#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/../mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CROSS_COMPILE=${CROSS_COMPILE}
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}g++
export CFLAGS="-muclibc -O2 "
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"
#make clean
#make all
rm -r CMakeFiles
cmake -DCMAKE_TOOLCHAIN_FILE="dafang.toolchain"
make VERBOSE=1
