
obj-y += misc.o regulator.o sound.o

 obj-$(CONFIG_FB_JZ4780_LCDC0)              += lcd.o
 obj-$(CONFIG_MMC_JZ4775)        += mmc.o
 obj-$(CONFIG_BCM4330_RFKILL)	 += bt.o
ifdef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
obj-y += pm_slcd.o
else
obj-y += pm.o
endif
