SUBDIRS = examples

bin_PROGRAMS = as10k1
as10k1_SOURCES = as10k1.c parse.c assemble.c macro.c
noinst_HEADERS = as10k1.h list.h parse.h proto.h types.h
EXTRA_DIST = gitcompile output.doc examples/Makefile examples/*.asm depcomp
AM_CFLAGS = -fno-strict-aliasing
AUTOMAKE_OPTIONS = foreign

dsp:
	$(MAKE) -C examples dsp

alsa-dist: distdir
	@rm -rf ../distdir/as10k1
	@mkdir -p ../distdir/as10k1
	@cp -RLpv $(distdir)/* ../distdir/as10k1
	@rm -rf $(distdir)
