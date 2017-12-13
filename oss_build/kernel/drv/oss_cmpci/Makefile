# Makefile for Open Sound System module oss_cmpci

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
OBJECTS=$(TMPDIR)/oss_cmpci.o
TARGETS=$(MODDIR)/oss_cmpci $(MODDIR)/oss_cmpci.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_cmpci.c

all: $(MODDIR)/oss_cmpci.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_cmpci.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_cmpci.o $(OBJECTS)


$(TMPDIR)/oss_cmpci.o:	oss_cmpci.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_cmpci.c -o $(TMPDIR)/oss_cmpci.o

clean: clean_local

