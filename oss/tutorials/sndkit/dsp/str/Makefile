#CC	= gcc
#CFLAGS	= -O6 -m486 -funroll-loops -Wall
#LD	= gcc
#LDFLAGS	= -s -N
CC	= cc
CFLAGS	= -O
LD	= cc
LDFLAGS	= -s 

INSTALLDIR	= /usr/local/bin

OBJS	= str.o strplay.o ../help.o

.c.o:
#	$(CC) -c $(CFLAGS) -o $*.o $<
	$(CC) -c $(CFLAGS) $<

all: str

install: all
	cp str $(INSTALLDIR)
	chown root $(INSTALLDIR)/str
	chmod 755 $(INSTALLDIR)/str
	ln -f $(INSTALLDIR)/str $(INSTALLDIR)/str15
	ln -f $(INSTALLDIR)/str $(INSTALLDIR)/str32

str: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o str

clean:
	rm -f $(OBJS) str a.out core

str.o: str.c str.h
strplay.o: strplay.c str.h
