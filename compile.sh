export CC=mipsel-linux-gnu-gcc
export CFLAGS="-DDEBUG_TRACE -DFAKE_ROOT "
export LIBS=""
export LD=mipsel-linux-gnu-ld
export LDFLAGS="-L./ -l:libcrypto.so"
rm dropbearmulti
cd dropbear-2017.75/
make clean
./configure --host=mipsel-linux --disable-zlib
make PROGRAMS="dropbear dbclient scp dropbearkey dropbearconvert" MULTI=1 SCPPROGRESS=1
mv dropbearmulti ../dropbearmulti
cd ..