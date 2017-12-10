bin_PROGRAMS = lo10k1
sbin_PROGRAMS = ld10k1 dl10k1
ld10k1_SOURCES = ld10k1.c ld10k1_fnc.c ld10k1_fnc1.c ld10k1_debug.c \
	ld10k1_driver.c comm.c ld10k1_tram.c \
	ld10k1_dump.c ld10k1_mixer.c\
	ld10k1.h ld10k1_fnc_int.h ld10k1_fnc1.h ld10k1_debug.h \
	ld10k1_driver.h bitops.h ld10k1_tram.h \
	ld10k1_dump.h ld10k1_dump_file.h ld10k1_mixer.h
ld10k1_CFLAGS = $(AM_CFLAGS) $(ALSA_CFLAGS)
ld10k1_LDADD = $(ALSA_LIBS)

#liblo10k1_ladir = $(includedir)/lo10k1
lib_LTLIBRARIES = liblo10k1.la
liblo10k1_la_SOURCES = comm.c liblo10k1.c liblo10k1ef.c liblo10k1lf.c
#liblo10k1_la_HEADERS = comm.h liblo10k1.h liblo10k1ef.h ld10k1_error.h ld10k1_fnc.h liblo10k1lf.h
liblo10k1_la_CFLAGS = $(ALSA_CFLAGS)
liblo10k1_la_LIBADD = $(ALSA_LIBS)

lo10k1_SOURCES = lo10k1.c
lo10k1_CFLAGS = $(ALSA_CFLAGS) -DEFFECTSDIR='"$(effectsdir)"'
lo10k1_LDADD = liblo10k1.la

dl10k1_SOURCES = dl10k1.c ld10k1_dump_file.h
dl10k1_CFLAGS = $(ALSA_CFLAGS)
dl10k1_LDADD = $(ALSA_LIBS)

INCLUDES=-I$(top_srcdir)/include