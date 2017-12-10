MYNAME = hdajacksensetest
AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = hdajacksensetest
AM_CFLAGS = @GLIB_CFLAGS@ -I "../hdajackretask/" -I "../hda-verb/"
hdajacksensetest_SOURCES = hdajacksensetest.c ../hdajackretask/sysfs-pin-configs.c
hdajacksensetest_LDADD = @GLIB_LIBS@

alsa-dist: distdir
	@rm -rf ../distdir/$(MYNAME)
	@mkdir -p ../distdir/$(MYNAME)
	@cp -RLpv $(distdir)/* ../distdir/$(MYNAME)
	@rm -rf $(distdir)
