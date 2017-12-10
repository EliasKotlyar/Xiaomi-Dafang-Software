/*
 *  EMU10k1 dump loader
 *
 *  Copyright (c) 2003,2004 by Peter Zubaj
 *
 *   Hwdep usage based on sb16_csp
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/* TODO - kontrola dat, ktore nahravam */

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <alsa/sound/emu10k1.h>

#include <getopt.h>
#include <bitops.h>

#include "ld10k1_dump_file.h"

#define DL10K1_SIGNATURE "DUMP Image (dl10k1)"
int card = 0;
snd_hwdep_t *handle;
const char *card_proc_id;

void error(const char *fmt,...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
}

static void help(char *command)
{
	fprintf(stderr,
		"Usage: %s [-options]\n"
		"\nAvailable options:\n"
		"  -h, --help        this help\n"
		"  -c, --card        select card number, default = 0\n"
		"  -d, --dump        file with dump\n"
		, command);
}

int driver_set_tram_size(int tram_size)
{
	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_TRAM_SETUP, &tram_size) < 0) {
		error("unable to setup tram");
		return 1;
	}
	return 0;
}

void free_code_struct(emu10k1_fx8010_code_t *code)
{
	if (code->gpr_map)
		free(code->gpr_map);
	if (code->tram_data_map)
		free(code->tram_data_map);
	if (code->tram_addr_map)
		free(code->tram_addr_map);
	if (code->code)
		free(code->code);
}

int alloc_code_struct(emu10k1_fx8010_code_t *code)
{
	/* alloc code structure */
	code->gpr_map = NULL;
	code->tram_data_map = NULL;
	code->tram_addr_map = NULL;
	code->code = NULL;
	
	code->gpr_map = (uint32_t *)malloc(sizeof(uint32_t) * 0x200);
	if (!code->gpr_map)
		goto err;
	memset(code->gpr_map, 0, sizeof(uint32_t) * 0x200);

	code->tram_data_map = (uint32_t *)malloc(sizeof(uint32_t) * 0x100);
	if (!code->tram_data_map)
		goto err;
	memset(code->tram_data_map, 0, sizeof(uint32_t) * 0x100);

	code->tram_addr_map = (uint32_t *)malloc(sizeof(uint32_t) * 0x100);
	if (!code->tram_addr_map)
		goto err;
	memset(code->tram_addr_map, 0, sizeof(uint32_t) * 0x100);

	code->code = (uint32_t *)malloc(sizeof(uint32_t) * 1024 * 2);
	if (!code->code)
		goto err;
	memset(code->code, 0, sizeof(uint32_t) * 1024 * 2);

        return 0;
err:
	free_code_struct(code);
	return -1;
}

int driver_init_dsp(int audigy)
{
	int i;
	emu10k1_fx8010_code_t code;
	emu10k1_fx8010_control_gpr_t *ctrl;
	emu10k1_ctl_elem_id_t *ids;
	emu10k1_fx8010_pcm_t ipcm;
	unsigned int *iptr;

	if (alloc_code_struct(&code) < 0) {
		error("no mem");
		return 1;
	}

	/* get count of controls */
	code.gpr_list_control_count = 0;
	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_CODE_PEEK, &code) < 0) {
		error("unable to peek code");
		free_code_struct(&code);
		return 1;
	}

	ctrl = (emu10k1_fx8010_control_gpr_t *)malloc(sizeof(emu10k1_fx8010_control_gpr_t) * code.gpr_list_control_total);
	if (!ctrl) {
		error("no mem");
		free_code_struct(&code);
		return 1;
	}

	code.gpr_list_control_count = code.gpr_list_control_total;
	code.gpr_list_controls = ctrl;

	for (i = 0; i < sizeof(code.gpr_valid) / sizeof(unsigned long); i++)
		code.gpr_valid[i] = 0x0;
	for (i = 0; i < sizeof(code.tram_valid) / sizeof(unsigned long); i++)
		code.tram_valid[i] = 0x0;
	for (i = 0; i < sizeof(code.code_valid) / sizeof(unsigned long); i++)
		code.code_valid[i] = 0x0;;

	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_CODE_PEEK, &code) < 0) {
		error("unable to peek code");
		free_code_struct(&code);
		free(ctrl);
		return 1;
	}

	
	/* new name */
	strcpy(code.name, DL10K1_SIGNATURE);
	for (i = 0; i < sizeof(code.gpr_valid) / sizeof(unsigned long); i++)
		code.gpr_valid[i] = ~0;

	for (i = 0; i < sizeof(code.gpr_valid) * 8; i++)
		code.gpr_map[i] = 0;

	ids = (emu10k1_ctl_elem_id_t *)malloc(sizeof(emu10k1_ctl_elem_id_t) * code.gpr_list_control_total);
	if (!ids) {
		free_code_struct(&code);
		free(ctrl);
		error("no mem");
		return 1;
	}

	code.gpr_del_control_count = code.gpr_list_control_total;
	if (code.gpr_del_control_count) {
		for (i = 0; i < code.gpr_del_control_count; i++) {
			memcpy(&(ids[i]), &(ctrl[i].id), sizeof(emu10k1_ctl_elem_id_t));
		}
	}

	free(ctrl);

	code.gpr_del_controls = ids;
	code.gpr_list_control_count = 0;
	code.gpr_add_control_count = 0;
	code.gpr_list_control_count = 0;

	for (i = 0; i < sizeof(code.tram_valid) / sizeof(unsigned long); i++)
		code.tram_valid[i] = ~0;
	for (i = 0; i < sizeof(code.code_valid) / sizeof(unsigned long); i++)
		code.code_valid[i] = ~0;

	for (i = 0; i < sizeof(code.tram_valid) * 8; i++) {
		code.tram_addr_map[i] = 0;
		code.tram_data_map[i] = 0;
	}

	for (iptr = code.code, i = 0; i < sizeof(code.code_valid) * 8; i++, iptr += 2)
		if (audigy) {
			*iptr = ((0xcf & 0x7ff) << 12) | (0xc0 & 0x7ff);
			*(iptr + 1) = ((0x0f & 0x0f) << 24) | ((0xc0 & 0x7ff) << 12) | (0xc0 & 0x7ff);
		} else {
			*iptr = ((0x40 & 0x3ff) << 10) | (0x40 & 0x3ff);
			*(iptr + 1) = ((0x06 & 0x0f) << 20) | ((0x40 & 0x3ff) << 10) | (0x40 & 0x3ff);
		}

	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_CODE_POKE, &code) < 0) {
		error("unable to poke code");
		free_code_struct(&code);
		free(ids);
		return 1;
	}

	free(ids);

	/* delete tram pcm dsp part */
	if (!audigy) {
		for (i = 0; i < EMU10K1_FX8010_PCM_COUNT; i++) {
			ipcm.substream = i;
			ipcm.channels = 0;
			if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_PCM_POKE, &ipcm) < 0) {
				error("unable to poke code");
				free_code_struct(&code);
				return 1;
			}
		}
	}
	return 0;
}

int dump_load(int audigy, char *file_name)
{
	struct stat dump_stat;
	void *dump_data, *ptr;
	FILE *dump_file;
	emu10k1_fx8010_control_gpr_t *ctrl = NULL;
	ld10k1_ctl_dump_t *fctrl = NULL;
	unsigned int *fgpr = NULL;
	ld10k1_tram_dump_t *ftram = NULL;
	ld10k1_instr_dump_t *finstr = NULL;
	int i, j;
	unsigned int vaddr, addr;
	int op;
	
	unsigned int *iptr;

	emu10k1_fx8010_code_t code;

	ld10k1_dump_t *header = NULL;

	/* first load patch to mem */
	if (stat(file_name, &dump_stat)) {
		error("unable to load patch %s", file_name);
		return 1;
	}

	/* minimal dump len is size of header */
	if (dump_stat.st_size < sizeof(ld10k1_dump_t)) {
		error("unable to load dump %s (wrong file size)", file_name);
		return 1;
	}


	dump_data = malloc(dump_stat.st_size);
	if (!dump_data) {
		error("no mem");
		return 1;
	}

	dump_file = fopen(file_name, "r");
	if (!dump_file) {
		error("unable to open file %s", file_name);
		goto err;
	}

	if (fread(dump_data, dump_stat.st_size, 1, dump_file) != 1) {
		error("unable to read data from file %s", file_name);
		goto err;
	} else
		fclose(dump_file);

	/* signature check */

	header = (ld10k1_dump_t *)dump_data;
	if (strncmp(header->signature, "LD10K1 DUMP 001", 16) != 0) {
		error("wrong dump file %s (wrong signature)", file_name);
		goto err;
	}
	
	/*printf("Size header%d\nctc %d %d\ngpr %d %d\ntram %d %d\ninstr %d %d\n", sizeof(ld10k1_dump_t),
		header->ctl_count, sizeof(ld10k1_ctl_dump_t),
		header->gpr_count, sizeof(unsigned int),
		header->tram_count, sizeof(ld10k1_tram_dump_t),
		header->instr_count, sizeof(ld10k1_instr_dump_t));*/

	/*check size */
	if (sizeof(ld10k1_dump_t) +
		header->ctl_count * sizeof(ld10k1_ctl_dump_t) +
		header->gpr_count * sizeof(unsigned int) +
		header->tram_count * sizeof(ld10k1_tram_dump_t) +
		header->instr_count * sizeof(ld10k1_instr_dump_t) != dump_stat.st_size)
		goto err;

	/* check dump type */
	if (header->dump_type == DUMP_TYPE_LIVE && audigy) {
		error("can't load dump from Live to Audigy");
		goto err1;
	} else if ((header->dump_type == DUMP_TYPE_AUDIGY_OLD ||
			header->dump_type == DUMP_TYPE_AUDIGY) &&
			!audigy) {
		error("can't load dump from Audigy to Live");
		goto err1;
	} else if (header->dump_type == DUMP_TYPE_AUDIGY_OLD) {
		error("can't load dump from Audigy (not patched drivers) to Audigy (current drivers)");
		goto err1;
	}

	ptr = dump_data;
	ptr += sizeof(ld10k1_dump_t);

	ctrl = (emu10k1_fx8010_control_gpr_t *)malloc(sizeof(emu10k1_fx8010_control_gpr_t) * header->ctl_count);
	if (!ctrl) {
		error("no mem");
		goto err1;
	}

	if (alloc_code_struct(&code) < 0) {
		error("no mem");
		return 1;
	}
	

	strcpy(code.name, DL10K1_SIGNATURE);

	/* copy ctls */
	fctrl = (ld10k1_ctl_dump_t *)ptr;
	memset(ctrl, 0, sizeof(emu10k1_fx8010_control_gpr_t) * header->ctl_count);
	for (i = 0; i < header->ctl_count; i++) {
		strcpy(ctrl[i].id.name, fctrl[i].name);
		ctrl[i].id.iface = EMU10K1_CTL_ELEM_IFACE_MIXER;
		ctrl[i].id.index = fctrl[i].index;
		ctrl[i].vcount = fctrl[i].vcount;
		ctrl[i].count = fctrl[i].count;
		for (j = 0; j < 32; j++) {
			ctrl[i].gpr[j] = fctrl[i].gpr_idx[j];
			ctrl[i].value[j] = fctrl[i].value[j];
		}
		ctrl[i].min = fctrl[i].min;
		ctrl[i].max = fctrl[i].max;
		ctrl[i].translation = fctrl[i].translation;
	}
	code.gpr_add_control_count = header->ctl_count;
	code.gpr_add_controls = ctrl;

	code.gpr_del_control_count = 0;
	code.gpr_del_controls = NULL;

	code.gpr_list_control_count = 0;
	code.gpr_list_controls = NULL;

	/* copy gprs */
	ptr += sizeof(ld10k1_ctl_dump_t) * header->ctl_count;
	fgpr = (unsigned int *)ptr;

	for (i = 0; i < sizeof(code.gpr_valid) / sizeof(unsigned long); i++)
		code.gpr_valid[i] = ~0;

	for (i = 0; i < header->gpr_count; i++)
		code.gpr_map[i] = fgpr[i];

	ptr += sizeof(unsigned int) * header->gpr_count;
	ftram = (ld10k1_tram_dump_t *)ptr;
	/* tram addr + data */
	for (i = 0; i < header->tram_count; i++) {
		addr = ftram[i].addr;
		vaddr = addr & 0xFFFFF;
		op = ftram[i].type;

		set_bit(i, code.tram_valid);
		switch(op) {
			case DUMP_TRAM_READ:
				if (audigy)
					vaddr = vaddr | 0x2 << 20;
				else
					vaddr = vaddr | TANKMEMADDRREG_READ | TANKMEMADDRREG_ALIGN;
				break;
			case DUMP_TRAM_WRITE:
				if (audigy)
					vaddr = vaddr | 0x6 << 20;
				else
					vaddr = vaddr | TANKMEMADDRREG_WRITE | TANKMEMADDRREG_ALIGN;
				break;
			case DUMP_TRAM_NULL:
			default:
				vaddr = 0;
				break;
		}

		code.tram_addr_map[i] = vaddr;
		code.tram_data_map[i] = ftram[i].data;
	}

	ptr += sizeof(ld10k1_tram_dump_t) * header->tram_count;
	finstr = (ld10k1_instr_dump_t *)ptr;
	for (iptr = code.code, i = 0; i < header->instr_count; i++, iptr += 2) {
		set_bit(i, code.code_valid);
		if (finstr[i].used) {
			if (audigy) {
				*iptr = ((finstr[i].arg[2] & 0x7ff) << 12) | (finstr[i].arg[3] & 0x7ff);
				*(iptr + 1) = ((finstr[i].op & 0x0f) << 24) | ((finstr[i].arg[0] & 0x7ff) << 12) | (finstr[i].arg[1] & 0x7ff);
			} else {
				if (i < 0x200) {
					*iptr = ((finstr[i].arg[2] & 0x3ff) << 10) | (finstr[i].arg[3] & 0x3ff);
					*(iptr + 1) = ((finstr[i].op & 0x0f) << 20) | ((finstr[i].arg[0] & 0x3ff) << 10) | (finstr[i].arg[1] & 0x3ff);
				}
			}
		} else {
			if (audigy) {
				*iptr = ((0xcf & 0x7ff) << 12) | (0xc0 & 0x7ff);
				*(iptr + 1) = ((0x0f & 0x0f) << 24) | ((0xc0 & 0x7ff) << 12) | (0xc0 & 0x7ff);
			} else {
				if (i < 0x200) {
					*iptr = ((0x40 & 0x3ff) << 10) | (0x40 & 0x3ff);
					*(iptr + 1) = ((0x06 & 0x0f) << 20) | ((0x40 & 0x3ff) << 10) | (0x40 & 0x3ff);
				}
			}
		}
	}

	if (header->dump_type != DUMP_TYPE_AUDIGY_OLD && 
		driver_set_tram_size(header->tram_size))
		goto err1;

	if (driver_init_dsp(audigy))
		goto err1;

	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_CODE_POKE, &code) < 0) {
		error("unable to poke code");
		goto err1;
	}

	if (dump_data)
		free(dump_data);

	if (ctrl)
		free(ctrl);

	return 0;

err:
	error("wrong dump file format %s", file_name);
err1:
	free_code_struct(&code);
	if (dump_data)
		free(dump_data);
	if (ctrl)
		free(ctrl);

	return 1;
}

int main(int argc, char *argv[])
{
	int dev;
	int c;
	int err;
	int audigy;
	
	int opt_help = 0;
	char *opt_dump_file = NULL;

	char card_id[32];
	snd_ctl_t *ctl_handle;
	snd_ctl_card_info_t *card_info;
	snd_hwdep_info_t *hwdep_info;

	char name[16];

	snd_ctl_card_info_alloca(&card_info);
	snd_hwdep_info_alloca(&hwdep_info);

	static struct option long_options[] = {
				   {"help", 0, 0, 'h'},
				   {"card", 1, 0, 'c'},
				   {"dump", 1, 0, 'd'},
				   {0, 0, 0, 0}
               };

	int option_index = 0;
	while ((c = getopt_long(argc, argv, "hc:d:",
	        long_options, &option_index)) != EOF) {
		switch (c) {
/* 		case 0: */
/* 			break; */
		case 'h':
			opt_help = 1;
			break;
		case 'd':
			opt_dump_file = optarg;
			break;
		case 'c':
			card = snd_card_get_index(optarg);
			if (card < 0 || card > 31) {
				error("wrong -c argument '%s'\n", optarg);
				return 1;
			}
			break;
		default:
			return 1;
		}
	}

	if (opt_help) {
		help(argv[0]);
		return 0;
	}

	if (!opt_dump_file) {
		error("dump file not specified");
		return 1;
	}

	if (getuid() != 0 )
	{
		error("You are not running dl10k1 as root.");
		return 1;
	}
	
	/* Get control handle for selected card */
	sprintf(card_id, "hw:%i", card);
	if ((err = snd_ctl_open(&ctl_handle, card_id, 0)) < 0) {
		error("control open (%s): %s", card_id, snd_strerror(err));
		return 1;
	}

	/* Read control hardware info from card */
	if ((err = snd_ctl_card_info(ctl_handle, card_info)) < 0) {
		error("control hardware info (%s): %s", card_id, snd_strerror(err));
		exit(1);
	}

	if (!(card_proc_id = snd_ctl_card_info_get_id (card_info))) {
		error("card id (%s): %s", card_id, snd_strerror(err));
		exit(1);
	}


	/* EMU10k1/EMU10k2 chip is present only on SB Live, Audigy, Audigy 2, E-mu APS cards */
	if (strcmp(snd_ctl_card_info_get_driver(card_info), "EMU10K1") != 0 &&
	    strcmp(snd_ctl_card_info_get_driver(card_info), "Audigy") != 0 &&
		strcmp(snd_ctl_card_info_get_driver(card_info), "Audigy2") != 0 &&
		strcmp(snd_ctl_card_info_get_driver(card_info), "E-mu APS") != 0) {
		error("not a EMU10K1/EMU10K2 based card");
		exit(1);
	}

	if (strcmp(snd_ctl_card_info_get_driver(card_info), "Audigy") == 0 ||
		strcmp(snd_ctl_card_info_get_driver(card_info), "Audigy2") == 0)
		audigy = 1;
	else
		audigy = 0;

	/* find EMU10k1 hardware dependant device and execute command */
	dev = -1;
	err = 1;
	while (1) {
		if (snd_ctl_hwdep_next_device(ctl_handle, &dev) < 0)
			error("hwdep next device (%s): %s", card_id, snd_strerror(err));
		if (dev < 0)
			break;
		snd_hwdep_info_set_device(hwdep_info, dev);
		if (snd_ctl_hwdep_info(ctl_handle, hwdep_info) < 0) {
			if (err != -ENOENT)
				error("control hwdep info (%s): %s", card_id, snd_strerror(err));
			continue;
		}
		if (snd_hwdep_info_get_iface(hwdep_info) == SND_HWDEP_IFACE_EMU10K1) {
			sprintf(name, "hw:%i,%i", card, dev);

			/* open EMU10k1 hwdep device */
			if ((err = snd_hwdep_open(&handle, name, O_WRONLY)) < 0) {
				error("EMU10k1 open (%i-%i): %s", card, dev, snd_strerror(err));
				exit(1);
			}
			
			err = dump_load(audigy, opt_dump_file);

			snd_hwdep_close(handle);

			break;
		}
	}

	snd_ctl_close(ctl_handle);

	return 0;
}
