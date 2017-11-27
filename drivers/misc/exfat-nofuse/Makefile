#
# Makefile for Linux FAT12/FAT16/FAT32(VFAT)/FAT64(ExFAT) filesystem driver.
#

ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-$(CONFIG_EXFAT_FS) += exfat.o

exfat-objs := exfat_core.o exfat_super.o exfat_api.o exfat_blkdev.o exfat_cache.o \
			   exfat_data.o exfat_bitmap.o exfat_nls.o exfat_oal.o exfat_upcase.o

else
# external module build

EXTRA_FLAGS += -I$(PWD)

#
# KDIR is a path to a directory containing kernel source.
# It can be specified on the command line passed to make to enable the module to
# be built and installed for a kernel other than the one currently running.
# By default it is the path to the symbolic link created when
# the current kernel's modules were installed, but
# any valid path to the directory in which the target kernel's source is located
# can be provided on the command line.
#
ifeq (${ISVP_ENV_KERNEL_DIR}, )
	ISVP_ENV_KERNEL_DIR = ../../../kernel
endif
KDIR	:= ${ISVP_ENV_KERNEL_DIR}
PWD	:= $(shell pwd)
KREL	:= $(shell cd ${KDIR} && make -s kernelrelease)
PWD	:= $(shell pwd)

export CONFIG_EXFAT_FS := m

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

help:
	$(MAKE) -C $(KDIR) M=$(PWD) help

endif

.PHONY : all clean install uninstall
