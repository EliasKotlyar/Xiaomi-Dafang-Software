include /etc/oss.conf

BINDIR=/usr/local/bin

OSSINCLUDES=-I$(OSSLIBDIR)/include/sys -I$(OSSLIBDIR)/include -L/usr/lib/oss/lib
CFLAGS=-lm -O6

all: softsynth softsynth_gtk

softsynth: softsynth.c
	   $(CC) $(CFLAGS) $(OSSINCLUDES) softsynth.c -o softsynth -lOSSlib

softsynth_gtk1: softsynth_gtk.c
	   -$(CC) $(CFLAGS) `gtk-config --cflags --libs` $(OSSINCLUDES) softsynth_gtk.c -o softsynth_gtk -lOSSlib

softsynth_gtk: softsynth_gtk.c
	   -$(CC) $(CFLAGS) `pkg-config gtk+-2.0 --cflags --libs` $(OSSINCLUDES) softsynth_gtk.c -o softsynth_gtk -lOSSlib

install: all
	-cp softsynth softsynth_gtk $(BINDIR)

clean:
	rm -f softsynth softsynth_gtk *.o x y z *~ core core.*
