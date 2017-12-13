# Makefile for Open Sound System module ossphone


HOSTCC=cc
CPLUSPLUS=g++ -fno-rtti -fno-exceptions -I.
LDFLAGS=-lm
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
OBJECTS=$(TMPDIR)/ossphone.o
TARGETS=$(BINDIR)/ossphone
DEPDIR=$(BINDIR)/
CSOURCES=ossphone.c

all: $(TARGETS) 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

ossphone:	$(BINDIR)/ossphone

$(BINDIR)/ossphone:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/ossphone $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


$(TMPDIR)/ossphone.o:	ossphone.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossphone.c -o $(TMPDIR)/ossphone.o

clean: clean_local

