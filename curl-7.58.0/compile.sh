TOOLCHAIN=$(pwd)/../mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
echo "Using the following Toolchain $TOOLCHAIN"
make clean
export CC=${CROSS_COMPILE}gcc
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

export OPENSSLDIR=$(pwd)/../libressl-2.6.4/_install


./configure --host=mips-linux-gnu --prefix=${PWD}/_install
make -j16
make install

