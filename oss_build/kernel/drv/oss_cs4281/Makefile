# Makefile for Open Sound System module oss_cs4281

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
OBJECTS=$(TMPDIR)/oss_cs4281.o
TARGETS=$(MODDIR)/oss_cs4281 $(MODDIR)/oss_cs4281.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_cs4281.c

all: $(MODDIR)/oss_cs4281.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_cs4281.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_cs4281.o $(OBJECTS)


$(TMPDIR)/oss_cs4281.o:	oss_cs4281.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_cs4281.c -o $(TMPDIR)/oss_cs4281.o

clean: clean_local

