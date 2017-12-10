<?php
	include getenv("PHPMAKE_LIBPATH") . "library.php";

	phpmake_makefile_rules();
?>

# Makefile for Open Sound System module main

CC=cc -ffreestanding
HOSTCC=cc
CPLUSPLUS=g++ -fno-rtti -fno-exceptions -I.
OSFLAGS=-fno-common -fno-stack-protector -Wall -Werror -DOSS_LITTLE_ENDIAN
OS=Linux
ARCH=x86_64
TOPDIR=.
OBJDIR=$(TOPDIR)/target/objects
TMPDIR=.
MODDIR=$(TOPDIR)/target/modules
BINDIR=$(TOPDIR)/target/bin
LIBDIR=$(TOPDIR)/target/lib
SBINDIR=$(TOPDIR)/target/sbin
OSSLIBDIR="/usr/lib/oss"
THISOS=kernel/OS/Linux
CFLAGS=-O
SUBDIRS=lib cmd kernel os_cmd kernel/OS/Linux
TARGETS=main
DEPDIR=

include .makefile

all: subdirs 
lint: lint_subdirs 

dep: dep_subdirs 

include $(TOPDIR)/make.defs

clean: clean_local clean_subdirs


include setup/Linux/make.local
