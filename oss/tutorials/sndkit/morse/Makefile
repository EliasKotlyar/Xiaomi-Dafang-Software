INSTALLDIR=/usr/local/bin
TARGETS=morse morse2 morse3 testgen loadcw 
CFLAGS=-I../../../include
LDLAGS=

all: $(TARGETS)

morse2.o morse3.o morse.o testgen.o: charlist.h

morse: morse.o
		$(CC) $(LDFLAGS) -o morse morse.o -lm

morse2: morse2.o
		$(CC) $(LDFLAGS) -o morse2 morse2.o -lm

morse3: morse3.o
		$(CC) $(LDFLAGS) -o morse3 morse3.o -lm

testgen: testgen.o
		$(CC) $(LDFLAGS) -o testgen testgen.o -lm

install: $(TARGETS)
		cp -f $(TARGETS) randomcw Vvcw $(INSTALLDIR)
		chmod 755 $(INSTALLDIR)/randomcw

clean:
		rm -f *.o $(TARGETS) core core.* x y z *~
