CROSS_COMPILE ?= mips-linux-gnu-

ifeq (${ISVP_ENV_KERNEL_DIR}, )
	ISVP_ENV_KERNEL_DIR = ../../../kernel
endif

KDIR := ${ISVP_ENV_KERNEL_DIR}

DEFS =
DEFS += -DCONFIG_PWM0
#DEFS += -DCONFIG_PWM1
#DEFS += -DCONFIG_PWM2
#DEFS += -DCONFIG_PWM3
#DEFS += -DCONFIG_PWM4
#DEFS += -DCONFIG_PWM5
#DEFS += -DCONFIG_PWM6
#DEFS += -DCONFIG_PWM7
EXTRA_CFLAGS += $(DEFS)

all: modules

.PHONY: modules clean

PWM_HAL_NAME := sample_pwm_hal
PWM_CORE_NAME := sample_pwm_core
$(PWM_HAL_NAME)-objs := pwm_hal.o
$(PWM_CORE_NAME)-objs := pwm_core.o
obj-m := $(PWM_HAL_NAME).o $(PWM_CORE_NAME).o

modules:
	@$(MAKE) -C $(KDIR)  M=$(shell pwd) $@

clean:
	@rm -rf *.o *~ .depend .*.cmd  *.mod.c .tmp_versions *.ko *.symvers modules.order
