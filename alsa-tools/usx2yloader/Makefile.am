# # Process this file with automake to produce Makefile.in.
AUTOMAKE_OPTIONS = 1.3 foreign

MYNAME = usx2yloader

AM_CFLAGS = -DDATAPATH=\"$(datadir)/alsa/firmware/$(MYNAME)\"

bin_PROGRAMS = usx2yloader

usx2yloader_SOURCES = usx2yloader.c

EXTRA_DIST = gitcompile depcomp tascam_fw.usermap tascam_fw.in tascam_fpga.in

hotplug_files = tascam_fw.usermap tascam_fw tascam_fpga
hotplugdir = /etc/hotplug/usb
hotplug_SCRIPTS = $(hotplug_files)


alsa-dist: distdir
	@rm -rf ../distdir/$(MYNAME)
	@mkdir -p ../distdir/$(MYNAME)
	@cp -RLpv $(distdir)/* ../distdir/$(MYNAME)
	@rm -rf $(distdir)
