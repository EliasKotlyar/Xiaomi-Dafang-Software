# Makefile for Open Sound System module ossmixd

CC=cc
HOSTCC=cc
CPLUSPLUS=g++ -fno-rtti -fno-exceptions -I.
LDFLAGS=-L../../target/lib -lossmix
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
OBJECTS=$(TMPDIR)/ossmixd.o
TARGETS=$(SBINDIR)/ossmixd
DEPDIR=$(SBINDIR)/
CSOURCES=ossmixd.c

all: $(SBINDIR)/ossmixd 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

ossmixd:	$(SBINDIR)/ossmixd

$(SBINDIR)/ossmixd:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(SBINDIR)/ossmixd $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/ossmixd.o:	ossmixd.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossmixd.c -o $(TMPDIR)/ossmixd.o

clean: clean_local

