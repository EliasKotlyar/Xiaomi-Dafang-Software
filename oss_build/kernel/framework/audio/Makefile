# Makefile for Open Sound System module audio

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
OBJECTS=$(OBJDIR)/oss_audio_core.o $(OBJDIR)/oss_audiofmt.o $(OBJDIR)/oss_grc3.o $(OBJDIR)/oss_spdif.o
TARGETS=audio
DEPDIR=
CSOURCES=oss_audio_core.c oss_audiofmt.c oss_grc3.c oss_spdif.c

all: $(AUTOGEN_SOURCES) objects 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(OBJDIR)/oss_audio_core.o:	oss_audio_core.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_audio_core.c -o $(OBJDIR)/oss_audio_core.o

$(OBJDIR)/oss_audiofmt.o:	oss_audiofmt.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_audiofmt.c -o $(OBJDIR)/oss_audiofmt.o

$(OBJDIR)/oss_grc3.o:	oss_grc3.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_grc3.c -o $(OBJDIR)/oss_grc3.o

$(OBJDIR)/oss_spdif.o:	oss_spdif.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_spdif.c -o $(OBJDIR)/oss_spdif.o

clean: clean_local

