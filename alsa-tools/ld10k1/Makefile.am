SUBDIRS = setup doc patch dump contrib include src

aclocaldir=$(datadir)/aclocal
aclocal_DATA=ld10k1.m4

sbin_SCRIPTS = ld10k1d

do_subst = sed -e 's,[@]sbindir[@],$(sbindir),g' \
 -e 's,[@]binarydir[@],$(bindir),g' \
 -e 's,[@]sysconfdir[@],$(sysconfdir),g'
 
ld10k1d : ld10k1d.in
	$(do_subst) $(srcdir)/ld10k1d.in > ld10k1d

CLEANFILES = ld10k1d

noinst_HEADERS =

EXTRA_DIST = gitcompile ld10k1.m4 AUTHORS COPYING COPYING.LIB ChangeLog \
	     INSTALL README TODO ld10k1d.in

alsa-dist: distdir
	@rm -rf ../distdir/ld10k1
	@mkdir -p ../distdir/ld10k1
	@cp -RLpv $(distdir)/* ../distdir/ld10k1
	@rm -rf $(distdir)
