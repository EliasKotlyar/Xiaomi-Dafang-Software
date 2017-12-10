# # Process this file with automake to produce Makefile.in.
AUTOMAKE_OPTIONS = 1.3 foreign

bin_PROGRAMS = cspctl
man_MANS = cspctl.1

cspctl_SOURCES = cspctl.c

EXTRA_DIST = gitcompile README COPYING cspctl.1 depcomp

alsa-dist: distdir
	@rm -rf ../distdir/sb16_csp
	@mkdir -p ../distdir/sb16_csp
	@cp -RLpv $(distdir)/* ../distdir/sb16_csp
	@rm -rf $(distdir)
