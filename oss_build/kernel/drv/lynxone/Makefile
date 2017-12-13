# Makefile for Open Sound System module lynxone

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
OBJECTS=$(TMPDIR)/lynx_hal.o $(TMPDIR)/lynxone.o
TARGETS=$(MODDIR)/lynxone $(MODDIR)/lynxone.o
DEPDIR=$(TMPDIR)
CSOURCES=lynx_hal.c lynxone.c

all: $(MODDIR)/lynxone.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/lynxone.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/lynxone.o $(OBJECTS)


$(TMPDIR)/lynx_hal.o:	lynx_hal.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) lynx_hal.c -o $(TMPDIR)/lynx_hal.o

$(TMPDIR)/lynxone.o:	lynxone.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) lynxone.c -o $(TMPDIR)/lynxone.o

clean: clean_local

