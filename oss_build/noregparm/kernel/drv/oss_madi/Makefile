# Makefile for Open Sound System module oss_madi

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
OBJECTS=$(TMPDIR)/madi_mixer.o $(TMPDIR)/oss_madi.o
TARGETS=$(MODDIR)/oss_madi $(MODDIR)/oss_madi.o
DEPDIR=$(TMPDIR)
CSOURCES=madi_mixer.c oss_madi.c

all: $(MODDIR)/oss_madi.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_madi.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_madi.o $(OBJECTS)


$(TMPDIR)/madi_mixer.o:	madi_mixer.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) madi_mixer.c -o $(TMPDIR)/madi_mixer.o

$(TMPDIR)/oss_madi.o:	oss_madi.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_madi.c -o $(TMPDIR)/oss_madi.o

clean: clean_local

