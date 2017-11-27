
SRCFILES=wakeup.c

all: clean wakeup

wakeup: $(SRCFILES)
	mips-linux-gnu-gcc -o $@ $(SRCFILES) --static

clean:
	rm -rf wakeup
