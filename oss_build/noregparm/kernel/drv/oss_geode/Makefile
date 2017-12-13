# Makefile for Open Sound System module oss_geode

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
CFLAGS += -D_KERNEL
CFLAGS += -O3 -fno-common -mcmodel=kernel  -mno-red-zone -fno-pie  -fno-asynchronous-unwind-tables -ffreestanding -fno-stack-protector
OBJECTS=$(TMPDIR)/oss_geode.o
TARGETS=$(MODDIR)/oss_geode $(MODDIR)/oss_geode.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_geode.c

all: $(MODDIR)/oss_geode.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_geode.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_geode.o $(OBJECTS)


$(TMPDIR)/oss_geode.o:	oss_geode.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_geode.c -o $(TMPDIR)/oss_geode.o

clean: clean_local

