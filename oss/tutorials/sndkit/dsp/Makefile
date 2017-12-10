#CC	= gcc
#CFLAGS	= -O6 -m486 -funroll-loops -Wall
CFLAGS	= -O -I../../../include
#LD	= gcc
LD	= cc
#LDFLAGS	= -s -N
LDFLAGS	= -s 

INSTALLDIR	= /usr/local/bin

.c.o:
#	$(CC) -c $(CFLAGS) -o $*.o $<
	$(CC) -c $(CFLAGS) $<

all: srec

install: all
	cp srec $(INSTALLDIR)
	ln -sf $(INSTALLDIR)/srec $(INSTALLDIR)/splay
	chown root $(INSTALLDIR)/splay $(INSTALLDIR)/srec
	chmod 755 $(INSTALLDIR)/splay $(INSTALLDIR)/srec
	cd str;make install

srec: recplay.o help.o
	$(LD) $(LDFLAGS) recplay.o help.o -o srec

clean:
	rm -f $(OBJS) *.o srec a.out core
	cd str;make clean

recplay.o: recplay.c
