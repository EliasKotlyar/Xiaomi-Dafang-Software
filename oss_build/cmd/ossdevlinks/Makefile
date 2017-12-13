# Makefile for Open Sound System module ossdevlinks

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
OBJECTS=$(TMPDIR)/ossdevlinks.o
TARGETS=$(SBINDIR)/ossdevlinks
DEPDIR=$(SBINDIR)/
CSOURCES=ossdevlinks.c

all: $(SBINDIR)/ossdevlinks 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

ossdevlinks:	$(SBINDIR)/ossdevlinks

$(SBINDIR)/ossdevlinks:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(SBINDIR)/ossdevlinks $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/ossdevlinks.o:	ossdevlinks.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossdevlinks.c -o $(TMPDIR)/ossdevlinks.o

clean: clean_local

