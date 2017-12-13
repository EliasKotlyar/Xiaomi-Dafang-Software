# Makefile for Open Sound System module cmd

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
SUBDIRS=ossdevlinks ossinfo ossmix ossmixd osspartysh ossphone ossplay osstest savemixer vmixctl
TARGETS=cmd
DEPDIR=

all: subdirs 
lint: lint_subdirs 

dep: dep_subdirs 

include $(TOPDIR)/make.defs

cmd:	$(BINDIR)/cmd

$(BINDIR)/cmd:	$(OBJECTS)
	$(CC) $(CFLAGS) -s -o $(BINDIR)/cmd $(OBJECTS) $(LIBRARIES) $(LDFLAGS) 


clean: clean_local clean_subdirs

