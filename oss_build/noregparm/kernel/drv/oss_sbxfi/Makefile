# Makefile for Open Sound System module oss_sbxfi

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
OBJECTS=$(TMPDIR)/oss_sbxfi.o $(TMPDIR)/sbxfi_hwaccess.o
TARGETS=$(MODDIR)/oss_sbxfi $(MODDIR)/oss_sbxfi.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_sbxfi.c sbxfi_hwaccess.c

all: $(MODDIR)/oss_sbxfi.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_sbxfi.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_sbxfi.o $(OBJECTS)


$(TMPDIR)/oss_sbxfi.o:	oss_sbxfi.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_sbxfi.c -o $(TMPDIR)/oss_sbxfi.o

$(TMPDIR)/sbxfi_hwaccess.o:	sbxfi_hwaccess.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) sbxfi_hwaccess.c -o $(TMPDIR)/sbxfi_hwaccess.o

clean: clean_local

