
obj-$(CONFIG_NAND) += fate.o

$(obj)/fate.o:$(obj)/lib.a

$(obj)/lib.a:$(obj)/libops.hex
	cp $(obj)/libops.hex $(obj)/lib.a