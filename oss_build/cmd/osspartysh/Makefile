# Makefile for Open Sound System module osspartysh

CC=cc
HOSTCC=cc
CPLUSPLUS=g++ -fno-rtti -fno-exceptions -I.
LDFLAGS=-lutil
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
OBJECTS=$(TMPDIR)/osspartysh.o
TARGETS=$(BINDIR)/osspartysh
DEPDIR=$(BINDIR)/
CSOURCES=osspartysh.c

all: $(TARGETS) 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

osspartysh:	$(BINDIR)/osspartysh

$(BINDIR)/osspartysh:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/osspartysh $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/osspartysh.o:	osspartysh.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) osspartysh.c -o $(TMPDIR)/osspartysh.o

clean: clean_local

