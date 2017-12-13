# Makefile for Open Sound System module oss_emu10k1x

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
OBJECTS=$(TMPDIR)/oss_emu10k1x.o
TARGETS=$(MODDIR)/oss_emu10k1x $(MODDIR)/oss_emu10k1x.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_emu10k1x.c

all: $(MODDIR)/oss_emu10k1x.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_emu10k1x.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_emu10k1x.o $(OBJECTS)


$(TMPDIR)/oss_emu10k1x.o:	oss_emu10k1x.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_emu10k1x.c -o $(TMPDIR)/oss_emu10k1x.o

clean: clean_local

