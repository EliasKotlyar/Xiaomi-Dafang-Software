TOOLCHAIN=$(pwd)/../mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
echo "Using the following Toolchain $TOOLCHAIN"
make clean
export CC=${CROSS_COMPILE}gcc
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

./configure --host=mips-linux-gnu --disable-udev --prefix=${PWD}/_install
make libmp3lame.so
make install

