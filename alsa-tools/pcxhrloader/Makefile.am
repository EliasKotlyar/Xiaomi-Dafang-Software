# # Process this file with automake to produce Makefile.in.
AUTOMAKE_OPTIONS = 1.3 foreign

MYNAME = pcxhrloader

AM_CFLAGS = -DDATAPATH=\"$(datadir)/alsa/firmware/$(MYNAME)\"

bin_PROGRAMS = pcxhrloader

pcxhrloader_SOURCES = pcxhrloader.c

EXTRA_DIST = gitcompile depcomp

alsa-dist: distdir
	@rm -rf ../distdir/$(MYNAME)
	@mkdir -p ../distdir/$(MYNAME)
	@cp -RLpv $(distdir)/* ../distdir/$(MYNAME)
	@rm -rf $(distdir)
