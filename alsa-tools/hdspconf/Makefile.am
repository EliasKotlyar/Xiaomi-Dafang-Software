EXTRA_DIST = gitcompile hdspconf.desktop depcomp
SUBDIRS = src pixmaps

desktopdir = $(DESKTOP_DIR)
desktop_DATA = hdspconf.desktop

alsa-dist: distdir
	@rm -rf ../distdir/hdspconf
	@mkdir -p ../distdir/hdspconf
	@cp -RLpv $(distdir)/* ../distdir/hdspconf
	@rm -rf $(distdir)
