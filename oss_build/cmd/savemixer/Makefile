# Makefile for Open Sound System module savemixer

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
OBJECTS=$(TMPDIR)/savemixer.o
TARGETS=$(SBINDIR)/savemixer
DEPDIR=$(SBINDIR)/
CSOURCES=savemixer.c

all: $(SBINDIR)/savemixer 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

savemixer:	$(SBINDIR)/savemixer

$(SBINDIR)/savemixer:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(SBINDIR)/savemixer $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/savemixer.o:	savemixer.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) savemixer.c -o $(TMPDIR)/savemixer.o

clean: clean_local

