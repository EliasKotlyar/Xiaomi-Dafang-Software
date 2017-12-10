EXTRA_DIST = \
	volBUS1.asm \
	3volBUS2.asm \
	emu_constants.asm \
	freberv.asm \
	gain_4.asm \
	michorusL.asm \
	michorusR.asm \
	volLR.asm \
	emu_constants.inc \
	fv-basstreble.inc \
	fv-controls.inc \
	fv-filters-reduced.inc \
	fv-filters.inc \
	fv-routes.inc \
	refl-big.inc \
	refl-default.inc \
	pontodo5

dist_effects_DATA = \
	3volBUS1.emu10k1 \
	3volBUS2.emu10k1 \
	freberv.emu10k1 \
	gain_4.emu10k1 \
	michorusL.emu10k1 \
	michorusR.emu10k1 \
	volLR.emu10k1

CLEANFILES = $(dist_effects_DATA)

SUFFIXEC = .asm .emu10k1

.asm.emu10k1:
	$(AS10K1) -o $@ $<
