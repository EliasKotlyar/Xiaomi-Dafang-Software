# Makefile for Open Sound System module ossplay


HOSTCC=cc
CPLUSPLUS=g++ -fno-rtti -fno-exceptions -I.
OSFLAGS=-Wall -DOSS_LITTLE_ENDIAN
OS=Linux
ARCH=x86_64
TOPDIR=../..
OBJDIR=$(TOPDIR)/target/objects
TMPDIR=.
MODDIR=$(TOPDIR)/target/modules
BINDIR=$(TOPDIR)/target/bin
LIBDIR=$(TOPDIR)/target/lib
SBINDIR=$(TOPDIR)/target/sbin
OSSLIBDIR="/usr/lib/oss"
THISOS=kernel/OS/Linux
CFLAGS+=-O
OBJECTS=$(TMPDIR)/ossplay.o $(TMPDIR)/ossplay_console.o $(TMPDIR)/ossplay_decode.o $(TMPDIR)/ossplay_parser.o $(TMPDIR)/ossplay_wparser.o
TARGETS=$(BINDIR)/ossplay
DEPDIR=$(BINDIR)/
CSOURCES=ossplay.c ossplay_console.c ossplay_decode.c ossplay_parser.c ossplay_wparser.c

all: $(TARGETS) 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

ossplay:	$(BINDIR)/ossplay

$(BINDIR)/ossplay:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/ossplay $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/ossplay.o:	ossplay.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossplay.c -o $(TMPDIR)/ossplay.o

$(TMPDIR)/ossplay_console.o:	ossplay_console.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossplay_console.c -o $(TMPDIR)/ossplay_console.o

$(TMPDIR)/ossplay_decode.o:	ossplay_decode.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossplay_decode.c -o $(TMPDIR)/ossplay_decode.o

$(TMPDIR)/ossplay_parser.o:	ossplay_parser.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossplay_parser.c -o $(TMPDIR)/ossplay_parser.o

$(TMPDIR)/ossplay_wparser.o:	ossplay_wparser.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossplay_wparser.c -o $(TMPDIR)/ossplay_wparser.o

clean: clean_local

