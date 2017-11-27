
obj-$(CONFIG_VIDEO_TX_ISP) += fate.o

$(obj)/fate.o:$(obj)/lib.a

$(obj)/lib.a:$(obj)/libapical_isp.hex
	cp $(obj)/libapical_isp.hex $(obj)/lib.a
