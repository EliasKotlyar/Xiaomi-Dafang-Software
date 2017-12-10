SUBDIRS = src

EXTRA_DIST = gitcompile COPYING configure.ac README NEWS AUTHORS TODO \
             INSTALL ChangeLog

alsa-dist: distdir
	@rm -rf ../distdir/qlo10k1
	@mkdir -p ../distdir/qlo10k1
	@cp -RLpv $(distdir)/* ../distdir/qlo10k1
	@rm -rf $(distdir)
