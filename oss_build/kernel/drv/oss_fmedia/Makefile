# Makefile for Open Sound System module oss_fmedia

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
OBJECTS=$(TMPDIR)/oss_fmedia.o
TARGETS=$(MODDIR)/oss_fmedia $(MODDIR)/oss_fmedia.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_fmedia.c

all: $(MODDIR)/oss_fmedia.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_fmedia.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_fmedia.o $(OBJECTS)


$(TMPDIR)/oss_fmedia.o:	oss_fmedia.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_fmedia.c -o $(TMPDIR)/oss_fmedia.o

clean: clean_local

