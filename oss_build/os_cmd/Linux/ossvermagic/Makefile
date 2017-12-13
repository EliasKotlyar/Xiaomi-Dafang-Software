# Makefile for Open Sound System module ossvermagic

CC=cc
HOSTCC=cc
CPLUSPLUS=g++ -fno-rtti -fno-exceptions -I.
OSFLAGS=-Wall -DOSS_LITTLE_ENDIAN
OS=Linux
ARCH=x86_64
TOPDIR=../../..
OBJDIR=$(TOPDIR)/target/objects
TMPDIR=.
MODDIR=$(TOPDIR)/target/modules
BINDIR=$(TOPDIR)/target/bin
LIBDIR=$(TOPDIR)/target/lib
SBINDIR=$(TOPDIR)/target/sbin
OSSLIBDIR="/usr/lib/oss"
THISOS=kernel/OS/Linux
CFLAGS+=-O
OBJECTS=$(TMPDIR)/ossvermagic.o
TARGETS=$(SBINDIR)/ossvermagic
DEPDIR=$(SBINDIR)/
CSOURCES=ossvermagic.c

all: $(SBINDIR)/ossvermagic 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

ossvermagic:	$(SBINDIR)/ossvermagic

$(SBINDIR)/ossvermagic:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(SBINDIR)/ossvermagic $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/ossvermagic.o:	ossvermagic.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossvermagic.c -o $(TMPDIR)/ossvermagic.o

clean: clean_local

