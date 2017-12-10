# # Process this file with automake to produce Makefile.in.
AUTOMAKE_OPTIONS = 1.3 foreign

bin_PROGRAMS = sbiload
#man_MANS = sbiload.1

AM_CFLAGS = -DPATCHDIR=\"$(datadir)/sounds/opl3\"

sbiload_SOURCES = sbiload.c

patchdir = $(datadir)/sounds/opl3
patch_DATA = std.o3 drums.o3 std.sb drums.sb

EXTRA_DIST = gitcompile README COPYING depcomp std.o3 drums.o3 std.sb drums.sb

alsa-dist: distdir
	@rm -rf ../../distdir/seq/sbiload
	@mkdir -p ../../distdir/seq/sbiload
	@cp -RLpv $(distdir)/* ../../distdir/seq/sbiload
	@rm -rf $(distdir)
