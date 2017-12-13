# Makefile for Open Sound System module oss_cmi878x

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
OBJECTS=$(TMPDIR)/oss_cmi878x.o
TARGETS=$(MODDIR)/oss_cmi878x $(MODDIR)/oss_cmi878x.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_cmi878x.c

all: $(MODDIR)/oss_cmi878x.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_cmi878x.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_cmi878x.o $(OBJECTS)


$(TMPDIR)/oss_cmi878x.o:	oss_cmi878x.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_cmi878x.c -o $(TMPDIR)/oss_cmi878x.o

clean: clean_local

