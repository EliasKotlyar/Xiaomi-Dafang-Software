ccflags-y += -I/usr/lib/oss

ifneq ($(KERNELRELEASE),)

	obj-m := cuckoo.o cuckoo_pcm.o cuckoo_mixer.o

else

	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

install: default
	cp *.ko /lib/modules/`uname -r`/kernel/oss
	depmod -a

clean:
	rm -f *.o *.ko *.mod.c *.mod.o .*.cmd core core.* x y z
	rm -rf .tmp_versions Modules.symvers
