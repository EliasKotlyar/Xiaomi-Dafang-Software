CC=../mips-gcc472-glibc216-64bit/bin/mips-linux-uclibc-gnu-gcc
CFLAGS=-std=c99 -muclibc -O2 -lrt

autonight: autonight.c
	$(CC) $(CFLAGS) -o $@ $<


.PHONY: clean
clean:
	rm -f autonight
