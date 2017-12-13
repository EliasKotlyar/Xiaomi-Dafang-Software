# Makefile for Open Sound System module os_cmd

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
SUBDIRS=Linux
TARGETS=os_cmd
DEPDIR=

all: subdirs 
lint: lint_subdirs 

dep: dep_subdirs 

include $(TOPDIR)/make.defs

os_cmd:	$(BINDIR)/os_cmd

$(BINDIR)/os_cmd:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/os_cmd $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


clean: clean_local clean_subdirs

