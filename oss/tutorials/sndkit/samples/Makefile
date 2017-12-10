#
# Note! In most systems soundcard.h located under <sys/> or <linux/> is for
#       obsolete OSS 3.x version and these utilities will fail to compile with
#       it. For safety reasons utilities located in this directory have been 
#	changed to include just <soundcard.h> instead of <sys/soundcard.h>.
#	
#	The correct version of soundcard.h is located in
#       /usr/lib/oss/include/sys. However some operating systems have OSS4
#	included in the base system and /usr/include/sys/soundcard.h may be
#	correct.
#
#	You can use the OSSLIBDIR variable defined in /etc/oss.conf. If this
#	file exists then include it in the Makefile. You can use
#       -I(OSSLIBDIR)/include/sys

CFLAGS=-I../../../include

# Note2! Not all programs are included in TARGET. The missing programs are
#        used to demonstrate OSS features that are not recommended.

TARGETS=audiolevel dsp_geterror_demo fulldup midi midiin mixer_applet mixext mmap_duplex mmap_test playtgt recsrc singen

all: $(TARGETS)

clean:
	rm -f $(TARGETS) *.o core core.* *.core x y z *~
