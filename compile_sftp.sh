TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
OPENSSL_DIR=$(pwd)/libressl/_install
ZLIB_DIR=$(pwd)/zlib/_install/
echo "Using the following Toolchain $TOOLCHAIN"
echo "Using the following OPENSSL: $OPENSSL_DIR"
echo "Using the following ZLIB: $ZLIB_DIR"


export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"



cd openssh
autoreconf
./configure --prefix=${PWD}/_install --host=mips-linux-gnu --with-ssl-dir=${OPENSSL_DIR} --with-zlib=${ZLIB_DIR} --disable-etc-default-login --with-pid-dir=/tmp  --with-privsep-user=root --disable-strip
make sftp-server