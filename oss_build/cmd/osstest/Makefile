# Makefile for Open Sound System module osstest

CC=cc
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
CFLAGS += -DSRC_SUPPORT
OBJECTS=$(TMPDIR)/help.o $(TMPDIR)/osstest.o $(TMPDIR)/wavedata.o
TARGETS=$(BINDIR)/osstest
DEPDIR=$(BINDIR)/
CSOURCES=help.c osstest.c wavedata.c

all: $(TARGETS) 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

osstest:	$(BINDIR)/osstest

$(BINDIR)/osstest:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/osstest $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/help.o:	help.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) help.c -o $(TMPDIR)/help.o

$(TMPDIR)/osstest.o:	osstest.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) osstest.c -o $(TMPDIR)/osstest.o

$(TMPDIR)/wavedata.o:	wavedata.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) wavedata.c -o $(TMPDIR)/wavedata.o

clean: clean_local

