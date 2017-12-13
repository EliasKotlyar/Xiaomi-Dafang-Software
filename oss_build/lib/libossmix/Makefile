# Makefile for Open Sound System module libossmix

CC=cc
HOSTCC=cc
CPLUSPLUS=g++ -fno-rtti -fno-exceptions -I.
OSFLAGS=-Wall -DOSS_LITTLE_ENDIAN
OS=Linux
ARCH=x86_64
TOPDIR=../..
OBJDIR=$(TOPDIR)/target/objects
TMPDIR=.
MODDIR=$(TOPDIR)/target/modules
BINDIR=$(TOPDIR)/target/bin
LIBDIR=$(TOPDIR)/target/lib
SBINDIR=$(TOPDIR)/target/sbin
OSSLIBDIR="/usr/lib/oss"
THISOS=kernel/OS/Linux
CFLAGS+=-O
CFLAGS += -shared -fPIC
OBJECTS=$(TMPDIR)/libossmix_cache.o $(TMPDIR)/libossmix_local.o $(TMPDIR)/libossmix_main.o $(TMPDIR)/libossmix_tcp.o
TARGETS=libossmix
DEPDIR=
CSOURCES=libossmix_cache.c libossmix_local.c libossmix_main.c libossmix_tcp.c

all: $(LIBDIR)/libossmix.so 
lint: lint_sources 

dep: $(AUTOGEN_SOURCES) dep_local 

include $(TOPDIR)/make.defs

libossmix.so:	$(LIBDIR)/libossmix.so

$(LIBDIR)/libossmix.so:	$(OBJECTS)
	$(CC) $(LDFLAGS) -shared -fPIC -o $(LIBDIR)/libossmix.so $(OBJECTS)


$(TMPDIR)/libossmix_cache.o:	libossmix_cache.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) libossmix_cache.c -o $(TMPDIR)/libossmix_cache.o

$(TMPDIR)/libossmix_local.o:	libossmix_local.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) libossmix_local.c -o $(TMPDIR)/libossmix_local.o

$(TMPDIR)/libossmix_main.o:	libossmix_main.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) libossmix_main.c -o $(TMPDIR)/libossmix_main.o

$(TMPDIR)/libossmix_tcp.o:	libossmix_tcp.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) libossmix_tcp.c -o $(TMPDIR)/libossmix_tcp.o

clean: clean_local

