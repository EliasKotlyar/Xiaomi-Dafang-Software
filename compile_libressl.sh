TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-

export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

cd libressl/
./autogen.sh
./configure --prefix=${PWD}/_install --host=mips-linux-gnu --with-pic
make -j4
make install