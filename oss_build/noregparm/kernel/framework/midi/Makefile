# Makefile for Open Sound System module midi

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
OBJECTS=$(OBJDIR)/oss_default_timer.o $(OBJDIR)/oss_midi_core.o $(OBJDIR)/oss_midi_mapper.o $(OBJDIR)/oss_midi_parser.o $(OBJDIR)/oss_midi_queue.o $(OBJDIR)/oss_midi_timers.o
TARGETS=midi
DEPDIR=
CSOURCES=oss_default_timer.c oss_midi_core.c oss_midi_mapper.c oss_midi_parser.c oss_midi_queue.c oss_midi_timers.c

all: $(AUTOGEN_SOURCES) objects 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(OBJDIR)/oss_default_timer.o:	oss_default_timer.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_default_timer.c -o $(OBJDIR)/oss_default_timer.o

$(OBJDIR)/oss_midi_core.o:	oss_midi_core.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_midi_core.c -o $(OBJDIR)/oss_midi_core.o

$(OBJDIR)/oss_midi_mapper.o:	oss_midi_mapper.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_midi_mapper.c -o $(OBJDIR)/oss_midi_mapper.o

$(OBJDIR)/oss_midi_parser.o:	oss_midi_parser.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_midi_parser.c -o $(OBJDIR)/oss_midi_parser.o

$(OBJDIR)/oss_midi_queue.o:	oss_midi_queue.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_midi_queue.c -o $(OBJDIR)/oss_midi_queue.o

$(OBJDIR)/oss_midi_timers.o:	oss_midi_timers.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_midi_timers.c -o $(OBJDIR)/oss_midi_timers.o

clean: clean_local

