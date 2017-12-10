AM_CFLAGS = @RMEDIGICONTROL_CFLAGS@
bin_PROGRAMS = rmedigicontrol
rmedigicontrol_SOURCES = rmedigicontrol.c rmedigicontrol.h level.c loopback.c

rmedigicontrol_LDADD = @RMEDIGICONTROL_LIBS@

EXTRA_DIST = gitcompile depcomp configure.in-gtk1
AUTOMAKE_OPTIONS = foreign

alsa-dist: distdir
	@rm -rf ../distdir/rmedigicontrol
	@mkdir -p ../distdir/rmedigicontrol
	@cp -RLpv $(distdir)/* ../distdir/rmedigicontrol
	@rm -rf $(distdir)
