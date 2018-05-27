
#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export CXX=${CROSS_COMPILE}g++
export LD=${CROSS_COMPILE}ld
export CFLAGS="-muclibc -O3"
export CPPFLAGS="-muclibc -O3"
export LDFLAGS="-muclibc -O3"


if [ ! -d libpcre/.git ]
then
    wget  wget https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz
    tar xvfz pcre-8.42.tar.gz
    rm pcre-8.42.tar.gz
fi

cd pcre-8.42 
./configure --host=mips-linux-gnu --prefix=$(pwd)/_install  --enable-static   --disable-shared
make clean
make
make install
