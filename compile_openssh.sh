TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
OPENSSL_DIR=$(pwd)/openssl/_install
ZLIB_DIR=$(pwd)/zlib/_install/
echo "Using the following Toolchain $TOOLCHAIN"
echo "Using the following OPENSSL: $OPENSSL_DIR"
echo "Using the following ZLIB: $ZLIB_DIR"
cd openssh
make clean


./configure  --with-cflags="-muclibc"  --with-cppflags="-muclibc "  --with-ldflags="-muclibc " --prefix=${PWD}/_install --host=mips-linux --with-libs --with-zlib=${ZLIB_DIR} --with-ssl-dir=${OPENSSL_DIR} --disable-etc-default-login CC=${CROSS_COMPILE}gcc AR=${CROSS_COMPILE}ar
## Remove Strip here
#sed '/STRIP_OPT=-s/d' ./Makefile

#make
#make install

