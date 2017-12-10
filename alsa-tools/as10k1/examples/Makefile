TARGETS = blank.emu10k1 chorus.emu10k1 delay.emu10k1 eq2.emu10k1 eq5.emu10k1 \
	  fir.emu10k1 flanger.emu10k1 sine.emu10k1 tremolo.emu10k1 vibrato.emu10k1 \
	  vol_ctrl.emu10k1

%.emu10k1: %.asm
	../as10k1 $<

all:

dsp: $(TARGETS)

clean:
	rm -f *~ *.emu10k1

distclean: clean

install:

distdir:
	@echo > /dev/null
