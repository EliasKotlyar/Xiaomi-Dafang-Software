# Makefile for Open Sound System module lib

CC=cc
HOSTCC=cc
CPLUSPLUS=g++ -fno-rtti -fno-exceptions -I.
OSFLAGS=-Wall -DOSS_LITTLE_ENDIAN
OS=Linux
ARCH=x86_64
TOPDIR=..
OBJDIR=$(TOPDIR)/target/objects
TMPDIR=.
MODDIR=$(TOPDIR)/target/modules
BINDIR=$(TOPDIR)/target/bin
LIBDIR=$(TOPDIR)/target/lib
SBINDIR=$(TOPDIR)/target/sbin
OSSLIBDIR="/usr/lib/oss"
THISOS=kernel/OS/Linux
CFLAGS+=-O
SUBDIRS=libOSSlib libossmix
TARGETS=lib
DEPDIR=

all: subdirs 
lint: lint_subdirs 

dep: dep_subdirs 

include $(TOPDIR)/make.defs

lib:	$(BINDIR)/lib

$(BINDIR)/lib:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/lib $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


clean: clean_local clean_subdirs

