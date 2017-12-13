# Makefile for Open Sound System module ossmix


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
OBJECTS=$(TMPDIR)/ossmix.o
TARGETS=$(BINDIR)/ossmix
DEPDIR=$(BINDIR)/
CSOURCES=ossmix.c

all: $(TARGETS) 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

ossmix:	$(BINDIR)/ossmix

$(BINDIR)/ossmix:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/ossmix $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/ossmix.o:	ossmix.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossmix.c -o $(TMPDIR)/ossmix.o

clean: clean_local

