CFLAGS=-I../include

PROGRAMS=snoopy mixgen readtimings

all: $(PROGRAMS)

snoopy: snoopy.c ../kernel/drv/oss_hdaudio/hdaudio.h
	$(CC) -o snoopy snoopy.c -I../kernel/drv/oss_hdaudio $(CFLAGS)

mixgen: mixgen.c ../kernel/drv/oss_hdaudio/hdaudio.h
	$(CC) -o mixgen mixgen.c -I../kernel/drv/oss_hdaudio $(CFLAGS)

mixgen2: mixgen2.c ../kernel/drv/oss_hdaudio/hdaudio.h
	$(CC) -o mixgen2 mixgen2.c -I../kernel/drv/oss_hdaudio $(CFLAGS)

clean:
	rm -f $(PROGRAMS) core core.* *.core *~ x y z *.o
