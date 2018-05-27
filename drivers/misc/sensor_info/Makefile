CROSS_COMPILE ?= mips-linux-uclibc-gnu-

TOOLCHAIN=../../../mips-gcc472-glibc216-64bit/bin


ifeq (${ISVP_ENV_KERNEL_DIR}, )
	#ISVP_ENV_KERNEL_DIR = ../../../kernel
	ISVP_ENV_KERNEL_DIR = /home/beihj/Ingenic-kernel
endif

KDIR := ${ISVP_ENV_KERNEL_DIR}
MODULE_NAME := sinfo

all: modules sample_sinfo

.PHONY: modules clean

$(MODULE_NAME)-objs := sensor_info.o
obj-m := $(MODULE_NAME).o

modules:
	@$(MAKE) -C $(KDIR) M=$(shell pwd) $@
sample_sinfo:
	$(TOOLCHAIN)/mips-linux-gnu-gcc sample_sinfo.c -o sample_sinfo
clean:
	@rm -rf *.o *~ .depend .*.cmd  *.mod.c .tmp_versions *.ko *.symvers modules.order
	@rm -f sample_sinfo
