SUBDIRS = effects

EXTRA_DIST = init_audigy.in init_audigy_eq10.in init_live.in
bin_SCRIPTS = init_audigy init_audigy_eq10 init_live

do_subst = sed -e 's,[@]datadir[@],$(datadir),g' \
 -e 's,[@]PERL[@],$(PERL),g' \
 -e 's,[@]PACKAGE[@],$(PACKAGE),g' \
 -e 's,[@]VERSION[@],$(VERSION),g' \
 -e 's,[@]bindir[@],$(bindir),g'

init_live: init_live.in
	$(do_subst) $(srcdir)/init_live.in > init_live

init_audigy: init_audigy.in
	$(do_subst) $(srcdir)/init_audigy.in > init_audigy

init_audigy_eq10: init_audigy_eq10.in
	$(do_subst) $(srcdir)/init_audigy_eq10.in > init_audigy_eq10

CLEANFILES = init_live init_audigy init_audigy_eq10