export CC=mipsel-linux-gnu-gcc
export CFLAGS="-DDEBUG_TRACE -lcrypt"
export LIBS="-lcrypt"
rm dropbearmulti
cd dropbear-2017.75/
make clean
./configure --host=mipsel-linux --disable-zlib
make PROGRAMS="dropbear dbclient scp dropbearkey dropbearconvert" MULTI=1 STATIC=1 SCPPROGRESS=1
mv dropbearmulti ../dropbearmulti
cd ..