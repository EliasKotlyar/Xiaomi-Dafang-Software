# Makefile for Open Sound System module ossdetect

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
OBJECTS=$(TMPDIR)/ossdetect.o
TARGETS=$(SBINDIR)/ossdetect
DEPDIR=$(SBINDIR)/
CSOURCES=ossdetect.c

all: $(SBINDIR)/ossdetect 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

ossdetect:	$(SBINDIR)/ossdetect

$(SBINDIR)/ossdetect:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(SBINDIR)/ossdetect $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/ossdetect.o:	ossdetect.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossdetect.c -o $(TMPDIR)/ossdetect.o

clean: clean_local

