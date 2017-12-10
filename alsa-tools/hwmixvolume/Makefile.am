# # Process this file with automake to procude Makefile.in.
bin_SCRIPTS = hwmixvolume
#man_MANS =
EXTRA_DIST = gitcompile hwmixvolume
AUTOMAKE_OPTIONS = foreign

alsa-dist: distdir
	@rm -rf ../distdir/hwmixvolume
	@mkdir -p ../distdir/hwmixvolume
	@cp -RLpv $(distdir)/* ../distdir/hwmixvolume
	@rm -rf $(distdir)
