TOOLCHAIN=$(pwd)/../mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
echo "Using the following Toolchain $TOOLCHAIN"
make clean

export CPP=${CROSS_COMPILE}g++
export AR=${CROSS_COMPILE}ar
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export LINK=${CROSS_COMPILE}ld
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

export OPENSSLDIR=$(pwd)/../libressl/_install
export CURLDIR=$(pwd)/../curl-7.58.0/_install
export NO_R_TO_GCC_LINKER=Yes
export V=1
export NO_GETTEXT=1
export ZLIB_PATH=$(pwd)/../zlib/_install
export EXPATDIR=$(pwd)/../libexpat/expat/_install

export HOME=${PWD}/_install
export DEFAULT_PAGER="/system/sdcard/bin/busybox less"


make configure
./configure --host=mips-linux-gnu --prefix=${PWD}/_install
make -j16
make install

