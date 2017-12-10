lo10k1includedir = $(includedir)/lo10k1
lo10k1include_HEADERS = version.h comm.h liblo10k1.h liblo10k1ef.h ld10k1_error.h ld10k1_fnc.h liblo10k1lf.h lo10k1.h

INCLUDES = -I$(top_srcdir)/include

#taken from alsa-lib - start
CLEANFILES = stamp-vh version.h lo10k1

version.h: stamp-vh
	@:

stamp-vh: $(top_builddir)/configure.ac
	@echo "/*" > ver.tmp
	@echo " *  version.h" >> ver.tmp
	@echo " */" >> ver.tmp
	@echo "" >> ver.tmp
	@echo "#define LD10K1_LIB_MAJOR		$(LD10K1_LIB_MAJOR) /**< major number of library version */" >> ver.tmp
	@echo "#define LD10K1_LIB_MINOR		$(LD10K1_LIB_MINOR) /**< minor number of library version */" >> ver.tmp
	@echo "#define LD10K1_LIB_SUBMINOR	$(LD10K1_LIB_SUBMINOR) /**< subminor number of library version */" >> ver.tmp
	@echo "/** library version */" >> ver.tmp
	@echo "#define LD10K1_LIB_VERSION		((LD10K1_LIB_MAJOR<<16)|\\" >> ver.tmp
	@echo "					 (LD10K1_LIB_MINOR<<8)|\\" >> ver.tmp
	@echo "				  	 LD10K1_LIB_SUBMINOR)" >> ver.tmp
	@echo "/** library version (string) */" >> ver.tmp
	@echo "#define LD10K1_LIB_VERSION_STR	\"$(LD10K1_LIB_VERSION)\"" >> ver.tmp
	@echo >> ver.tmp
	@cmp -s version.h ver.tmp \
          || (echo "Updating version.h"; \
              cp ver.tmp version.h; \
              echo timestamp > stamp-vh)
	-@rm -f ver.tmp

#taken from alsa-lib - end