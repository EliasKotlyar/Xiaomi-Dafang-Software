OBJECTS=ddksample.o ddksample_audio.o ddksample_mixer.o ddksample_misc.o
CFLAGS=-D_KERNEL

all: ddksample

ddksample:	$(OBJECTS)
		ld -dy -r -Ndrv/osscore $(OBJECTS) -o ddksample
#		ld -64 -dy -r -Ndrv/osscore $(OBJECTS) -o ddksample	# For 64 bit kernels

install:	ddksample
		cp ddksample /kernel/drv/ddksample
		cp ddksample.conf /kernel/drv/ddksample.conf
		-rem_drv ddksample
		add_drv ddksample
		ossdetect -d

clean:
		rm -f ddksample *.o x y z core *~
