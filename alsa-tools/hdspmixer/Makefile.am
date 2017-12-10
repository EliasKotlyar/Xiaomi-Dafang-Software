EXTRA_DIST = gitcompile hdspmixer.desktop depcomp
SUBDIRS = src pixmaps

desktopdir = $(DESKTOP_DIR)
desktop_DATA = hdspmixer.desktop

alsa-dist: distdir
	@rm -rf ../distdir/hdspmixer
	@mkdir -p ../distdir/hdspmixer
	@cp -RLpv $(distdir)/* ../distdir/hdspmixer
	@rm -rf $(distdir)
