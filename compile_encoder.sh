#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-



cd libimp-samples/
make clean
make sample-Encoder-h264