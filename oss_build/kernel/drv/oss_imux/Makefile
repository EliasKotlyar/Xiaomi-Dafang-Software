# Makefile for Open Sound System module oss_imux

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
CFLAGS += -O
OBJECTS=$(TMPDIR)/oss_imux.o
TARGETS=$(MODDIR)/oss_imux $(MODDIR)/oss_imux.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_imux.c

all: $(MODDIR)/oss_imux.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_imux.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_imux.o $(OBJECTS)


$(TMPDIR)/oss_imux.o:	oss_imux.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_imux.c -o $(TMPDIR)/oss_imux.o

clean: clean_local

