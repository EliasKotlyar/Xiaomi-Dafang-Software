CROSS_COMPILE ?= mips-linux-gnu-

ifeq (${ISVP_ENV_KERNEL_DIR}, )
	ISVP_ENV_KERNEL_DIR = $(PWD)/../../../kernel
endif

KDIR := ${ISVP_ENV_KERNEL_DIR}
MODULE_NAME := tx-isp

all: modules

.PHONY: modules clean

EXTRA_CFLAGS += -I$(PWD)/include

$(MODULE_NAME)-objs := tx-isp-vic.o tx-isp-videobuf.o tx-isp-video-in.o tx-isp-frame-channel.o tx-isp-csi.o tx-isp-interrupt.o tx-isp-debug.o tx-isp-device.o \
			apical-isp/tx-isp-core-tuning.o apical-isp/tx-isp-core.o \
			apical-isp/apical_dynamic_calibrations.o apical-isp/apical_static_calibrations.o \
			apical-isp/apical_calibrations.o apical-isp/apical_calibrations_init.o apical-isp/tx-isp-load-parameters.o \
			apical-isp/apical_custom_initialization.o apical-isp/ApicalDefault.o \
			apical-isp/system_isp_io.o apical-isp/log.o apical-isp/system_io.o \
			apical-isp/system_i2c.o apical-isp/system_spi.o apical-isp/system_timer.o apical-isp/system_uart.o \
			apical-isp/system_semaphore.o apical-isp/sensor_drv.o apical-isp/system_chardev.o

$(MODULE_NAME)-objs += libapical_isp.a

obj-m := $(MODULE_NAME).o

modules:
	@$(MAKE) -C $(KDIR) M=$(shell pwd) $@

clean:
	@find . -name '*.o' -o -name '*~' -o -name '.depend' -o -name '.*.cmd' \
		-o -name '*.mod.c' -o -name '.tmp_versions' -o -name '*.ko' \
		-o -name '*.symvers' -o -name 'modules.order' | xargs rm -rf
