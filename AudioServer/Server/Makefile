ROOT=../../
CROSS_COMPILE=${ROOT}/mips-gcc472-glibc216-64bit/bin/mips-linux-gnu-
CC = $(CROSS_COMPILE)gcc
CPLUSPLUS = ${ROOT}/mips-gcc472-glibc216-64bit/bin/mips-linux-gnu-g++
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar cr
STRIP = $(CROSS_COMPILE)strip
CFLAGS = $(INCLUDES) -O2 -Wall -march=mips32r2 -I.  -I ../libwebsockets_compil/ -I ../libwebsockets_compil/include -I../../zlib/_install/include/ -I../../libressl/_install/include/ -I.
CFLAGS += -muclibc
LDFLAG += -muclibc
LIBS += ../libwebsockets_compil/lib/libwebsockets.a ../../zlib/_install/lib/libz.so ../../libressl/_install/lib/libssl.a ../../libressl/_install/lib/libcrypto.a
LDFLAG += -Wl,-gc-sections

all: audioserver	

audioserver: server.o protocol.o play.o
	$(CPLUSPLUS) $(LDFLAG) -o $@ $^ $(LIBS) -lpthread -lm -lrt


%.o:%.c 
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o *~

distclean: clean
	rm -f $(SAMPLES)
