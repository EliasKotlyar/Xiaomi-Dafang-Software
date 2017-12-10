CC=cc -O
SRCDIR=../../..
INCLUDE=-I$(SRCDIR)/include
LDFLAGS=
INSTALLDIR=/usr/local/bin
INSTALLLIB=/usr/lib/oss
CFLAGS=$(INCLUDE) -DUSERLAND

all: asm10k ld10k emu10k.bin1 emu10k.bin2 emu10k1_dsp.h emu10k2_dsp.h

install: ld10k emu10k.bin1 emu10k.bin2
		strip asm10k
		rm -f $(INSTALLDIR)/ld10k $(INSTALLDIR)/emu10k.bin1 $(INSTALLDIR)/emu10k.bin2
		cp  ld10k $(INSTALLDIR)
		cp  emu10k.bin1 $(INSTALLLIB)
		cp  emu10k.bin2 $(INSTALLLIB)
		cp  *.map /tmp

load: ld10k emu10k.bin1
		cp -f *.map /tmp
		./ld10k emu10k.bin1 -a

aload: ld10k emu10k.bin2
		cp -f *.map /tmp
		./ld10k emu10k.bin2 -a

testload: ld10k test.bin2
		./ld10k test.bin2 -a

ld10k: ld10k.c
		$(CC) $(CFLAGS) $(INCLUDE) -o ld10k ld10k.c
		strip ld10k

asm10k: asm10k.c
		$(CC) $(CFLAGS) $(INCLUDE) -o asm10k asm10k.c

emu10k.bin1: emu10k.dsp asm10k equalizer.mac emu10k1.mac
		./emuasm1 emu10k.dsp

emu10k.bin2: emu10k.dsp asm10k equalizer.mac emu10k2.mac
		./emuasm2 emu10k.dsp

test.bin2: test.dsp asm10k equalizer.mac emu10k2.mac
		./emuasm2 test.dsp

emu10k1_dsp.h:	emu10k.bin1 mkheader
		./mkheader emu10k1_dsp < emu10k.bin1 > emu10k1_dsp.h

emu10k2_dsp.h:	emu10k.bin2 mkheader
		./mkheader emu10k2_dsp < emu10k.bin2 > emu10k2_dsp.h

clean:
		rm -f *.o asm10k ld10k writegpr *.bin *.bin1 *.bin2 core x y z testgen *.map mkheader *_dsp.h
