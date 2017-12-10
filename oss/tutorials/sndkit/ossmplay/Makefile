CC = cc
CFLAGS = -g
LIBOBJS	= mlib.o 
INSTALLDIR=/usr/local/bin
INSTALLLIB=/usr/lib
.PREFIXES:	.o .c

# Uncomment the following lines if you want to use OSSlib.
# Look at the OSSlib subdirectory for more info about OSSlib.
# NOTE! OSS version 3.8-beta2 or later is required for OSSlib.
OSSDEFINES=-DOSSLIB
OSSLIBS=-lOSSlib

all : ossmplay

install: ossmplay
	cp ossmplay $(INSTALLDIR)/ossmplay

ossmplay : main.o $(LIBOBJS)
	$(CC) $(CFLAGS) -o ossmplay main.o $(LIBOBJS) -L$(INSTALLLIB) $(OSSLIBS)

main.o:	mlib.h main.c
	$(CC) $(CFLAGS) $(OSSDEFINES) -c main.c

mlib.o: mlib.h

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o ossmplay core
