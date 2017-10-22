export CC=mipsel-linux-gnu-gcc
export CFLAGS="-DDEBUG_TRACE -lcrypt"
./configure --host=mipsel-linux --disable-zlib
make PROGRAMS="dropbear dbclient scp dropbearkey dropbearconvert" MULTI=1 STATIC=1 SCPPROGRESS=1
