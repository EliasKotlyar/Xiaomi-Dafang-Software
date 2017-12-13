# Makefile for Open Sound System module oss_envy24

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
OBJECTS=$(TMPDIR)/envy24_1010lt.o $(TMPDIR)/envy24_6fire.o $(TMPDIR)/envy24_default.o $(TMPDIR)/envy24_direct.o $(TMPDIR)/envy24_ews88d.o $(TMPDIR)/envy24_tdif.o $(TMPDIR)/oss_envy24.o
TARGETS=$(MODDIR)/oss_envy24 $(MODDIR)/oss_envy24.o
DEPDIR=$(TMPDIR)
CSOURCES=envy24_1010lt.c envy24_6fire.c envy24_default.c envy24_direct.c envy24_ews88d.c envy24_tdif.c oss_envy24.c

all: $(MODDIR)/oss_envy24.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_envy24.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_envy24.o $(OBJECTS)


$(TMPDIR)/envy24_1010lt.o:	envy24_1010lt.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24_1010lt.c -o $(TMPDIR)/envy24_1010lt.o

$(TMPDIR)/envy24_6fire.o:	envy24_6fire.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24_6fire.c -o $(TMPDIR)/envy24_6fire.o

$(TMPDIR)/envy24_default.o:	envy24_default.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24_default.c -o $(TMPDIR)/envy24_default.o

$(TMPDIR)/envy24_direct.o:	envy24_direct.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24_direct.c -o $(TMPDIR)/envy24_direct.o

$(TMPDIR)/envy24_ews88d.o:	envy24_ews88d.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24_ews88d.c -o $(TMPDIR)/envy24_ews88d.o

$(TMPDIR)/envy24_tdif.o:	envy24_tdif.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24_tdif.c -o $(TMPDIR)/envy24_tdif.o

$(TMPDIR)/oss_envy24.o:	oss_envy24.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_envy24.c -o $(TMPDIR)/oss_envy24.o

clean: clean_local

