VERSION = 1.1.5
TOP = .
SUBDIRS = as10k1 envy24control hdsploader hdspconf hdspmixer \
	  mixartloader pcxhrloader rmedigicontrol sb16_csp seq sscape_ctl \
	  us428control usx2yloader vxloader echomixer ld10k1 qlo10k1 \
	  hwmixvolume hdajackretask hda-verb hdajacksensetest

all:
	@for i in $(SUBDIRS); do \
	  cd $(TOP)/$$i; echo $(TOP)/$$i; \
	  ./gitcompile $(GITCOMPILE_ARGS) || exit 1; \
	  cd ..; make -C $$i || exit 1; \
	done

configure:
	@for i in $(SUBDIRS); do \
	  cd $(TOP)/$$i; echo $(TOP)/$$i; \
	  if [ -x ./configure ]; then \
	    ./configure $(CONFIGURE_ARGS) || exit 1; \
	  else \
	    make CONFIGURE_ARGS="$CONFIGURE_ARGS" configure || exit 1; \
	  fi; \
	  cd ..; make -C $$i || exit 1; \
	done

install:
	@for i in $(SUBDIRS); do \
	  make -C $$i DESTDIR=$(DESTDIR) install || exit 1; \
	done

alsa-dist:
	@echo $(VERSION) >> $(TOP)/version
	@mkdir -p $(TOP)/distdir
	@for i in $(SUBDIRS); do \
	  cd $(TOP)/$$i; echo $(TOP)/$$i; \
	  ./gitcompile $(GITCOMPILE_ARGS) || exit 1; \
	  cd ..; make -C $$i alsa-dist || exit 1; \
	done
	@cp Makefile gitcompile distdir
	@mv distdir alsa-tools-$(VERSION)
	@tar --create --verbose --file=- alsa-tools-$(VERSION) \
	  | bzip2 -c -9 > alsa-tools-$(VERSION).tar.bz2
	@mv alsa-tools-$(VERSION) distdir

clean:
	rm -rf *~ distdir
	@for i in $(SUBDIRS); do make -C $$i clean || exit 1; done
