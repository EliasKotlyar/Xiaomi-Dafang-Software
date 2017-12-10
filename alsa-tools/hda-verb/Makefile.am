MYNAME = hda-verb
AUTOMAKE_OPTIONS = 1.3 foreign
bin_PROGRAMS = hda-verb
hda_verb_SOURCES = hda-verb.c
noinst_HEADERS = hda_hwdep.h

EXTRA_DIST = gitcompile README ChangeLog

alsa-dist: distdir
	@rm -rf ../distdir/$(MYNAME)
	@mkdir -p ../distdir/$(MYNAME)
	@cp -RLpv $(distdir)/* ../distdir/$(MYNAME)
	@rm -rf $(distdir)
