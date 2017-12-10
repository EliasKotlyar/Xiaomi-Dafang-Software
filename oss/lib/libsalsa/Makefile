LOBJECTS=main.lo output.lo misc.lo pcm.lo mix.lo seq.lo seqmid.lo rawmidi.lo seq_output.lo timer.lo stubs.lo seq_input.lo

OSSLIBDIR=/usr/lib/oss

CFLAGS=-O -I../../include -I../libOSSlib
LIBTOOL=libtool --tag=CC

all: libsalsa.la

*.lo:		local.h 

main.lo:	main.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c main.c

pcm.lo:	pcm.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c pcm.c 

misc.lo:	misc.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c misc.c

stubs.lo:	stubs.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c stubs.c

mix.lo:	mix.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c mix.c

output.lo:	output.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c output.c

seq.lo:	seq.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c seq.c 

timer.lo:	timer.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c timer.c 

seqmid.lo:	seqmid.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c seqmid.c 

seq_input.lo:	seq_input.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c seq_input.c 

seq_output.lo:	seq_output.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c seq_output.c 

rawmidi.lo:	rawmidi.c
		$(LIBTOOL) --mode=compile ${CC} $(CFLAGS) -c rawmidi.c 

libsalsa.la:   $(LOBJECTS)
		$(LIBTOOL) --mode=link ${CC} -rpath /usr/lib -Wl,--version-script=Versions -version-info 2:0:0 -o libsalsa.la $(LOBJECTS) -L../libOSSlib -lOSSlib

install:	libsalsa.la
		$(LIBTOOL) cp libsalsa.la /usr/lib
		(cd /usr/lib;ln -sf libsalsa.so.2.0.0 libasound.so)
		(cd /usr/lib;ln -sf libsalsa.so.2.0.0 libasound.so.2)

dep:

clean:
		rm -f core core.* *.o *.a *.lo *.so *.loT *.la x y z *.s
		rm -rf .libs
