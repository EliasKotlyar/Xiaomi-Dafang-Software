# Makefile for Open Sound System module oss_envy24ht

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
OBJECTS=$(TMPDIR)/envy24ht_ac97.o $(TMPDIR)/envy24ht_ap192.o $(TMPDIR)/envy24ht_aureon.o $(TMPDIR)/envy24ht_julia.o $(TMPDIR)/envy24ht_revo51.o $(TMPDIR)/envy24ht_revo71.o $(TMPDIR)/oss_envy24ht.o
TARGETS=$(MODDIR)/oss_envy24ht $(MODDIR)/oss_envy24ht.o
DEPDIR=$(TMPDIR)
CSOURCES=envy24ht_ac97.c envy24ht_ap192.c envy24ht_aureon.c envy24ht_julia.c envy24ht_revo51.c envy24ht_revo71.c oss_envy24ht.c

all: $(MODDIR)/oss_envy24ht.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_envy24ht.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_envy24ht.o $(OBJECTS)


$(TMPDIR)/envy24ht_ac97.o:	envy24ht_ac97.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24ht_ac97.c -o $(TMPDIR)/envy24ht_ac97.o

$(TMPDIR)/envy24ht_ap192.o:	envy24ht_ap192.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24ht_ap192.c -o $(TMPDIR)/envy24ht_ap192.o

$(TMPDIR)/envy24ht_aureon.o:	envy24ht_aureon.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24ht_aureon.c -o $(TMPDIR)/envy24ht_aureon.o

$(TMPDIR)/envy24ht_julia.o:	envy24ht_julia.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24ht_julia.c -o $(TMPDIR)/envy24ht_julia.o

$(TMPDIR)/envy24ht_revo51.o:	envy24ht_revo51.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24ht_revo51.c -o $(TMPDIR)/envy24ht_revo51.o

$(TMPDIR)/envy24ht_revo71.o:	envy24ht_revo71.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) envy24ht_revo71.c -o $(TMPDIR)/envy24ht_revo71.o

$(TMPDIR)/oss_envy24ht.o:	oss_envy24ht.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_envy24ht.c -o $(TMPDIR)/oss_envy24ht.o

clean: clean_local

