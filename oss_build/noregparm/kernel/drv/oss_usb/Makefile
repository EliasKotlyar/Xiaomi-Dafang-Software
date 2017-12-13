# Makefile for Open Sound System module oss_usb

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
OBJECTS=$(TMPDIR)/oss_usb.o $(TMPDIR)/ossusb_audio.o $(TMPDIR)/ossusb_midi.o $(TMPDIR)/ossusb_midisport.o $(TMPDIR)/ossusb_ymhmidi.o
TARGETS=$(MODDIR)/oss_usb $(MODDIR)/oss_usb.o
DEPDIR=$(TMPDIR)
CSOURCES=oss_usb.c ossusb_audio.c ossusb_midi.c ossusb_midisport.c ossusb_ymhmidi.c

all: $(MODDIR)/oss_usb.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_usb.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_usb.o $(OBJECTS)


$(TMPDIR)/oss_usb.o:	oss_usb.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_usb.c -o $(TMPDIR)/oss_usb.o

$(TMPDIR)/ossusb_audio.o:	ossusb_audio.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossusb_audio.c -o $(TMPDIR)/ossusb_audio.o

$(TMPDIR)/ossusb_midi.o:	ossusb_midi.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossusb_midi.c -o $(TMPDIR)/ossusb_midi.o

$(TMPDIR)/ossusb_midisport.o:	ossusb_midisport.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossusb_midisport.c -o $(TMPDIR)/ossusb_midisport.o

$(TMPDIR)/ossusb_ymhmidi.o:	ossusb_ymhmidi.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) ossusb_ymhmidi.c -o $(TMPDIR)/ossusb_ymhmidi.o

clean: clean_local

