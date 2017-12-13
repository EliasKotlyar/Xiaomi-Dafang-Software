#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
#CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
#export CC=${CROSS_COMPILE}gcc
#export LD=${CROSS_COMPILE}ld
#export CFLAGS=""
#export CPPFLAGS="-muclibc -O2"
#export LDFLAGS="-muclibc -O2"

cd oss/
OSSPARAM=$(pwd)/configure

cd ..
rm -r oss_build
mkdir oss_build
cd oss_build
$OSSPARAM