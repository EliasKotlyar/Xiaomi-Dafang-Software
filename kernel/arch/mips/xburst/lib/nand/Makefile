
obj-$(CONFIG_NAND) += fate.o

$(obj)/fate.o:$(obj)/lib.a

$(obj)/lib.a:$(obj)/libnm.hex
	cp $(obj)/libnm.hex $(obj)/lib.a