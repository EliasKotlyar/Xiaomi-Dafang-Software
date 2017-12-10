#CC=gcc
#CFLAGS=-O6 -funroll-loops -Wall
CC=cc
CFLAGS=-O -Wall

all:		vplay vrec

vplay:		vplay.c fmtheaders.h
		$(CC) $(CFLAGS) vplay.c -o vplay
		strip vplay

vrec:		vplay
		ln -s vplay vrec
clean:
		rm -f vplay vrec *.o core

install:	vplay
		cp vplay /usr/local/bin
		rm -f /usr/local/bin/vrec
		ln -s /usr/local/bin/vplay /usr/local/bin/vrec
