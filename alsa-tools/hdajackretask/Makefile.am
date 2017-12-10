EXTRA_DIST = gitcompile README
AM_CFLAGS = @GTK3_CFLAGS@ -Wno-deprecated-declarations
bin_PROGRAMS = hdajackretask
man_MANS =
hdajackretask_SOURCES = main-gtk.c sysfs-pin-configs.c apply-changes.c 
hdajackretask_LDADD = @GTK3_LIBS@

noinst_HEADERS = apply-changes.h sysfs-pin-configs.h

BUILT_SOURCES = README.generated.h
nodist_hdajackretask_SOURCES = README.generated.h
CLEANFILES = README.generated.h

README.generated.h:
	sed 's/\\/\\\\/g;s/"/\\"/g;s/^/"/;s/$$/\\n"/;' README >$@

AUTOMAKE_OPTIONS = foreign

alsa-dist: distdir
	@rm -rf ../distdir/hdajackretask
	@mkdir -p ../distdir/hdajackretask
	@cp -RLpv $(distdir)/* ../distdir/hdajackretask
	@rm -rf $(distdir)

