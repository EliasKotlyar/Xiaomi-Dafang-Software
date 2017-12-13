# Makefile for Open Sound System module drv

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
SUBDIRS=lynxone oss_ali5455 oss_atiaudio oss_audigyls oss_audioloop oss_audiopci oss_cmi878x oss_cmpci oss_cs4281 oss_cs461x oss_digi96 oss_emu10k1x oss_envy24 oss_envy24ht oss_fmedia oss_geode oss_hdaudio oss_ich oss_imux oss_madi oss_midiloop oss_midimix oss_sblive oss_sbpci oss_sbxfi oss_solo oss_trident oss_usb oss_userdev oss_via823x oss_via97 oss_ymf7xx
TARGETS=drv
DEPDIR=

all: subdirs 
lint: lint_subdirs 

dep: dep_subdirs 

include $(TOPDIR)/make.defs

clean: clean_local clean_subdirs

