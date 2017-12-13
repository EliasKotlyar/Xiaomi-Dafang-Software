# Makefile for Open Sound System module vmixctl

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
OBJECTS=$(TMPDIR)/vmixctl.o
TARGETS=$(SBINDIR)/vmixctl
DEPDIR=$(SBINDIR)/
CSOURCES=vmixctl.c

all: $(SBINDIR)/vmixctl 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

vmixctl:	$(SBINDIR)/vmixctl

$(SBINDIR)/vmixctl:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(SBINDIR)/vmixctl $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/vmixctl.o:	vmixctl.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) vmixctl.c -o $(TMPDIR)/vmixctl.o

clean: clean_local

