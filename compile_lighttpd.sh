#!/usr/bin/env bash
TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
export CC=${CROSS_COMPILE}gcc
export LD=${CROSS_COMPILE}ld
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

if [ ! -d lighttpd1.4/.git ]
then
   git clone https://github.com/lighttpd/lighttpd1.4.git
fi

cd lighttpd1.4/
./autogen.sh
LIGHTTPD_STATIC=yes CPPFLAGS=-DLIGHTTPD_STATIC ./configure --prefix=${PWD}/_install --host=mips-linux-gnu --without-mysql --without-zlib --without-bzip2 --disable-ipv6 --without-pcre --enable-static --disable-shared

echo "PLUGIN_INIT(mod_auth)" > src/plugin-static.h
echo "PLUGIN_INIT(mod_redirect)" >> src/plugin-static.h 
echo "PLUGIN_INIT(mod_rewrite)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_cgi)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_fastcgi)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_scgi)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_ssi)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_proxy)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_indexfile)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_dirlisting)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_staticfile)" >> src/plugin-static.h
echo "PLUGIN_INIT(mod_authn_file)" >> src/plugin-static.h

make -j4
make install

