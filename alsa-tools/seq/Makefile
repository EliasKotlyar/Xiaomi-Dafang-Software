SUBDIRS = sbiload
TOP = ..

all:
	@for i in $(SUBDIRS); do cd $$i; echo $$i; ./gitcompile $(GITCOMPILE_ARGS) || exit 1; cd ..; $(MAKE) -C $$i; done

configure:
	@for i in $(SUBDIRS); do cd $$i; echo $$i; ./configure $(CONFIGURE_ARGS) || exit 1; cd ..; $(MAKE) -C $$i; done

install:
	@for i in $(SUBDIRS); do $(MAKE) -C $$i DESTDIR=$(DESTDIR) install || exit 1; done

alsa-dist:
	@for i in $(SUBDIRS); do $(MAKE) -C $$i alsa-dist || exit 1; done
	@cp gitcompile Makefile $(TOP)/distdir/seq

clean:
	@for i in $(SUBDIRS); do $(MAKE) -C $$i clean || exit 1; done
