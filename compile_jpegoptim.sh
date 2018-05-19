#!/usr/bin/env bash

#First start compile_jpeg.sh !!

#choose between turbo or regular jpglib
export JPEGPATH=$(pwd)/libjpeg-turbo-1.5.3/
#export JPEGPATH=$(pwd)/jpeg-9c/

TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export CFLAGS="-muclibc -O3 -I ${JPEGPATH}"
export CPPFLAGS="-muclibc -O3"
export LDFLAGS="-muclibc -O3"

echo ${JPEGPATH}
if [ ! -d jpegoptim/.git ]
then
    git clone https://github.com/tjko/jpegoptim.git
fi

cd jpegoptim 
./configure --host=mips-linux-gnu --prefix=$(pwd)/_Install --with-libjpeg=${JPEGPATH}/_Install/lib
sed -e 's/-ljpeg/\$\{JPEGPATH\}\/_Install\/lib\/libjpeg.a/' -i Makefile
#LIBS      = -lm /home/osboxes/SRC/Xiaomi-Dafang-Software/jpeg-9c/_Install/lib/libjpeg.a

make
make install

