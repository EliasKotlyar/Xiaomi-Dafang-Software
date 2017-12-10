# # Process this file with automake to produce Makefile.in.
MYNAME = hdsploader

bin_PROGRAMS = hdsploader

AM_CFLAGS = -DDATAPATH=\"$(datadir)/alsa/firmware/$(MYNAME)\"

hdsploader_SOURCES = hdsploader.c

EXTRA_DIST = gitcompile depcomp

alsa-dist: distdir
	@rm -rf ../distdir/$(MYNAME)
	@mkdir -p ../distdir/$(MYNAME)
	@cp -RLpv $(distdir)/* ../distdir/$(MYNAME)
	@rm -rf $(distdir)
