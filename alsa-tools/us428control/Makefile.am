# # Process this file with automake to produce Makefile.in.
AUTOMAKE_OPTIONS = 1.3 foreign

bin_PROGRAMS = us428control

us428control_SOURCES = us428control.cc Cus428State.cc Cus428_ctls.cc Cus428Midi.cc
noinst_HEADERS = usbus428ctldefs.h Cus428State.h Cus428_ctls.h Cus428Midi.h

EXTRA_DIST = gitcompile depcomp

alsa-dist: distdir
	@rm -rf ../distdir/us428control
	@mkdir -p ../distdir/us428control
	@cp -RLpv $(distdir)/* ../distdir/us428control
	@rm -rf $(distdir)
