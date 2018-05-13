#!/usr/bin/env bash
ROOT=$(pwd)/../
TOOLCHAIN=${ROOT}/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

if [ ! -d libwebsockets/.git ]
then
  git clone	https://github.com/warmcat/libwebsockets.git
fi

# First compile libwesocket
if [ ! -d libwebsockets_compil ]
then
   mkdir libwebsockets_compil
   cd libwebsockets_compil
   cmake ../libwebsockets -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_TOOLCHAIN_FILE=../cross-dafang.cmake -DLWS_WITHOUT_EXTENSIONS=0 -DLWS_WITH_SSL=1 -DLWS_HAVE_REALLOC=1 -DLWS_HAVE_TCP_USER_TIMEOUT=0 -DLWS_WITH_MINIMAL_EXAMPLES=1 -DLWS_ZLIB_LIBRARIES=${ROOT}/zlib/_install/lib/libz.so -DLWS_ZLIB_INCLUDE_DIRS=${ROOT}/zlib/_install/include/ -DLWS_OPENSSL_INCLUDE_DIRS=${ROOT}/libressl/_install/include/ -DLWS_OPENSSL_LIBRARIES="${ROOT}/libressl/_install/lib/libssl.a;${ROOT}/libressl/_install/lib/libcrypto.so"
fi

cd ../AudioServer/Server
make
