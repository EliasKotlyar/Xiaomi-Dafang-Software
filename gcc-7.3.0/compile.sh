TOOLCHAIN=$(pwd)/../mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-

GCCDIR=$(pwd)
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

cd ..
mkdir gcc-build
cd gcc-build
#$GCCDIR/configure --prefix=${PWD}/_install --build=x86_64-linux-gnu --target=mips-linux-gnu --host=mips-linux-gnu --enable-checking=release --enable-languages=c,c++ --disable-multilib
#make clean
make all-gcc -j16
make install-gcc
#make install