# # Process this file with automake to produce Makefile.in.
AUTOMAKE_OPTIONS = 1.3 foreign

bin_PROGRAMS = sscape_ctl

sscape_ctl_SOURCES = sscape_ctl.c

EXTRA_DIST = gitcompile depcomp

alsa-dist: distdir
	@rm -rf ../distdir/sscape_ctl
	@mkdir -p ../distdir/sscape_ctl
	@cp -RLpv $(distdir)/* ../distdir/sscape_ctl
	@rm -rf $(distdir)
