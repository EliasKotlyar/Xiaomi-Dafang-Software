	name "FXBUS"
	include "emu_constants.asm"

	;; From alsa driver pci/emu10k1/emufx.c _volume_add
	
pcm_l	io
pcm_r	io
pcm_lr	io
pcm_rr	io
midi_l	io
midi_r	io
pcm_c	io
pcm_lf	io
spdif_l io
spdif_r io

	;; Process FX Buses

	macints pcm_l, C_0, pcm_l, C_4
	macints pcm_r, C_0, pcm_r, C_4
	macints pcm_lr, C_0, pcm_lr, C_4
	macints pcm_rr, C_0, pcm_rr, C_4
	macints midi_l, C_0, midi_l, C_4
	macints midi_r, C_0, midi_r, C_4
	macints pcm_c, C_0, pcm_c, C_4
	macints pcm_lf, C_0, pcm_lf, C_4
	macints spdif_l, C_0, spdif_l, C_4
	macints spdif_r, C_0, spdif_r, C_4

	end
