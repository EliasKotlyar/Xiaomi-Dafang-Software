# Makefile for Open Sound System module ossinfo


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
OBJECTS=$(TMPDIR)/ossinfo.o
TARGETS=$(BINDIR)/ossinfo
DEPDIR=$(BINDIR)/
CSOURCES=ossinfo.c

all: $(TARGETS) 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

ossinfo:	$(BINDIR)/ossinfo

$(BINDIR)/ossinfo:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/ossinfo $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/ossinfo.o:	ossinfo.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossinfo.c -o $(TMPDIR)/ossinfo.o

clean: clean_local

