TARGETS=iosync multich16 multich32 musicin seltest2 spdif_in_debug sweepdown sweepup synctest ioctl_test

CFLAGS=-I../../../include

all: ${TARGETS}

sweepup:
	$(CC) ${CFLAGS} $(LDFLAGS) -lm -o sweepup sweepup.c 

clean:
	rm -f ${TARGETS} *.o x y z *~
