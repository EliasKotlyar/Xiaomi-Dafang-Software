# Makefile for Open Sound System module oss_hdaudio

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
OBJECTS=$(TMPDIR)/hdaudio_abit_AA8.o $(TMPDIR)/hdaudio_asus_P4B_E.o $(TMPDIR)/hdaudio_asus_m9.o $(TMPDIR)/hdaudio_codec.o $(TMPDIR)/hdaudio_eeepc.o $(TMPDIR)/hdaudio_ferrari5k.o $(TMPDIR)/hdaudio_generic.o $(TMPDIR)/hdaudio_gpio_handlers.o $(TMPDIR)/hdaudio_scaleoP.o $(TMPDIR)/hdaudio_si3055.o $(TMPDIR)/hdaudio_thinkpad_r61.o $(TMPDIR)/hdaudio_vaio_vgn.o $(TMPDIR)/oss_hdaudio.o
TARGETS=$(MODDIR)/oss_hdaudio $(MODDIR)/oss_hdaudio.o
DEPDIR=$(TMPDIR)
CSOURCES=hdaudio_abit_AA8.c hdaudio_asus_P4B_E.c hdaudio_asus_m9.c hdaudio_codec.c hdaudio_eeepc.c hdaudio_ferrari5k.c hdaudio_generic.c hdaudio_gpio_handlers.c hdaudio_scaleoP.c hdaudio_si3055.c hdaudio_thinkpad_r61.c hdaudio_vaio_vgn.c oss_hdaudio.c

all: $(MODDIR)/oss_hdaudio.o 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

$(MODDIR)/oss_hdaudio.o:	$(OBJECTS)
	$(LD) $(LDARCH) -r -o $(MODDIR)/oss_hdaudio.o $(OBJECTS)


$(TMPDIR)/hdaudio_abit_AA8.o:	hdaudio_abit_AA8.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_abit_AA8.c -o $(TMPDIR)/hdaudio_abit_AA8.o

$(TMPDIR)/hdaudio_asus_P4B_E.o:	hdaudio_asus_P4B_E.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_asus_P4B_E.c -o $(TMPDIR)/hdaudio_asus_P4B_E.o

$(TMPDIR)/hdaudio_asus_m9.o:	hdaudio_asus_m9.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_asus_m9.c -o $(TMPDIR)/hdaudio_asus_m9.o

$(TMPDIR)/hdaudio_codec.o:	hdaudio_codec.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_codec.c -o $(TMPDIR)/hdaudio_codec.o

$(TMPDIR)/hdaudio_eeepc.o:	hdaudio_eeepc.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_eeepc.c -o $(TMPDIR)/hdaudio_eeepc.o

$(TMPDIR)/hdaudio_ferrari5k.o:	hdaudio_ferrari5k.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_ferrari5k.c -o $(TMPDIR)/hdaudio_ferrari5k.o

$(TMPDIR)/hdaudio_generic.o:	hdaudio_generic.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_generic.c -o $(TMPDIR)/hdaudio_generic.o

$(TMPDIR)/hdaudio_gpio_handlers.o:	hdaudio_gpio_handlers.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_gpio_handlers.c -o $(TMPDIR)/hdaudio_gpio_handlers.o

$(TMPDIR)/hdaudio_scaleoP.o:	hdaudio_scaleoP.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_scaleoP.c -o $(TMPDIR)/hdaudio_scaleoP.o

$(TMPDIR)/hdaudio_si3055.o:	hdaudio_si3055.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_si3055.c -o $(TMPDIR)/hdaudio_si3055.o

$(TMPDIR)/hdaudio_thinkpad_r61.o:	hdaudio_thinkpad_r61.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_thinkpad_r61.c -o $(TMPDIR)/hdaudio_thinkpad_r61.o

$(TMPDIR)/hdaudio_vaio_vgn.o:	hdaudio_vaio_vgn.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) hdaudio_vaio_vgn.c -o $(TMPDIR)/hdaudio_vaio_vgn.o

$(TMPDIR)/oss_hdaudio.o:	oss_hdaudio.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) oss_hdaudio.c -o $(TMPDIR)/oss_hdaudio.o

clean: clean_local

