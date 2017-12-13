# Makefile for Open Sound System module oss_userdev

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
OBJECTS=$(TMPDIR)/oss_userdev.o $(TMPDIR)/oss_userdev_devicepair.o
TARGETS=$(MODDIR)/oss_userdev $(MODDIR)/oss_userdev.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_userdev.c oss_userdev_devicepair.c

all: $(MODDIR)/oss_userdev.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_userdev.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_userdev.o $(OBJECTS)


$(TMPDIR)/oss_userdev.o:	oss_userdev.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_userdev.c -o $(TMPDIR)/oss_userdev.o

$(TMPDIR)/oss_userdev_devicepair.o:	oss_userdev_devicepair.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_userdev_devicepair.c -o $(TMPDIR)/oss_userdev_devicepair.o

clean: clean_local

