/*
 *  EMU10k1 loader
 *
 *  Copyright (c) 2003,2004 by Peter Zubaj
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

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <alsa/asoundlib.h>
#include "version.h"
#include "comm.h"
#include "ld10k1_fnc.h"
#include "ld10k1_error.h"
#include "ld10k1_debug.h"

#include "liblo10k1.h"
#include "liblo10k1ef.h"
#include "liblo10k1lf.h"

char comm_pipe[256];
liblo10k1_connection_t conn;

static void error(const char *fmt,...)
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
		"  -h, --help           this help\n"
		"  -p, --pipe_name      connect to this, default = /tmp/.ld10k1_port\n"
		"  -l, --list           dump lkoaded patch\n"
		"  -i, --info           print some info\n"
		"  -s, --setup          setup DSP\n"
		"  -a, --add            load patch\n"
		"  -d, --del            unload patch\n"
		"  -q, --conadd         connect 2 patches\n"
		"  -w, --condel         delete connection\n"
		"      --debug          print debug information\n"
		"  -n, --defionames     define default in/out names for loaded patch\n"
		"      --ctrl           modify control parameters for loaded patch\n"
		"      --patch_name     load patch with this name\n"
		"      --where          insert patch before\n"
		"      --renam          rename patch, input, output, fx, patch input, patch output\n"
		"      --dump           dump DSP setup to file, can by loaded by dl10k1\n"
		"      --host           lo10k1 uses network socket instead of named socked (host,port)\n"
		"  -P, --path           include path\n"
		"      --store          store DSP setup\n"
		"      --restore        restore DSP setup\n"
		, command);
}

typedef struct tag_path_info {
	char *path;
	struct tag_path_info *next;
} path_t;

path_t *first_path;
path_t *last_path;

static void add_path(char *path)
{
	path_t *path_info = malloc(sizeof(path_t));

	path_info->path = strdup(path);
	path_info->next = NULL;

	if (last_path)
		last_path->next = path_info;

	last_path = path_info;

	if (!first_path)
		first_path = path_info;
}

static void add_paths(char *paths)
{
	char *str = strdup(paths);
	char *path = strtok(str, ":");
	
	while (path) {
		add_path(path);

		path = strtok(NULL, ":");
	}

	free (str);
}

static void free_all_paths()
{
	path_t *path_info = first_path;
	path_t *path_info_n = NULL;

	while (path_info) {
		path_info_n = path_info->next;
		free(path_info);
		path_info = path_info_n;
	}
}

static liblo10k1_emu_patch_t *try_patch(char *file_name)
{
	int en;
	
	liblo10k1_emu_patch_t *p = NULL;
	if ((en = liblo10k1_emu_load_patch(file_name, &p)) < 0)
		return NULL;

	return p;
}

static liblo10k1_emu_patch_t *open_patch(char *file_name)
{
	liblo10k1_emu_patch_t *patch;
	path_t *path_info = first_path;

	patch = try_patch(file_name);
	
	if (patch)
		return patch;

	while (path_info) {
		char path[256]; /* FIXME */

		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path)-1, "%s/%s", 
			 path_info->path, file_name);
		
		patch = try_patch(path);

		if (patch) {
			return patch;
		}

		snprintf(path, sizeof(path)-1, "%s/%s.emu10k1", 
			 path_info->path, file_name);

		patch = try_patch(path);

		if (patch) {
			return patch;
		}

		path_info = path_info->next;
	}

	return NULL;
}

static int load_patch(char *file_name, liblo10k1_emu_patch_t **p)
{
	liblo10k1_emu_patch_t *patch;
	
	if (!(patch = open_patch(file_name))) {
		error("unable to load patch %s", file_name);
		return 1;
	}
	
	*p = patch;
	return 0;
}

static char get_str(char **str, char *out, int maxlen, char *sep, int isnum)
{
	char ch = **str;
	char *tmpsep;
	int len = 0;
	int found = 0;
	
	*out = '\0';
	
	if (ch == '\0')
		return ch;
	
	len = 0;
	while (**str && len < maxlen) {
		found = 0;
		ch = **str;
		for (tmpsep = sep; *tmpsep; tmpsep++) {
			if (ch == *tmpsep) {
				found = 1;
				break;
			}
		}
		
		if (found)
			break;
 		if (isnum && !isdigit(ch))
			break;
		
		*out++ = *(*str)++;
			len++;
	}
	
	*out = '\0';
	return ch;
}

static int transfer_patch(int udin, char *ctrl_opt, liblo10k1_emu_patch_t *ep, liblo10k1_dsp_patch_t **p)
{
	int i, j, k;
	
	char ctrl_from_concate[16][32][MAX_NAME_LEN]; /* max 32 ctrl to 1 and max 16 of this*/
	char ctrl_to_concate[16][MAX_NAME_LEN]; 
	int ctrl_to_concate_count;
	int ctrl_from_count[16];
	
	char ctrl_visible[16][MAX_NAME_LEN];
	char ctrl_visible_max[16];
	unsigned char ctrl_visible_count;

	char ctrl_translate[16][MAX_NAME_LEN];
	char ctrl_translate_type[16];
	unsigned char ctrl_translate_count;
	
	char ctrl_index[16][MAX_NAME_LEN];
	int ctrl_index_val[16];
	unsigned char ctrl_index_count;

	char ctrl_values[16][MAX_NAME_LEN];
	int ctrl_values_val[16][32];
	unsigned char ctrl_values_cnt[16];
	unsigned char ctrl_values_count;

	char *tmp_str;
	char *tmp_num;
	char tmp_num_str[20];
	
	char sep;

	liblo10k1_ctl_transform_t *tctl;
	int ctl_idx;
	
	liblo10k1_dsp_patch_t *np = NULL;
	

	ctrl_to_concate_count = 0;
	ctrl_visible_count = 0;
	ctrl_translate_count = 0;
	ctrl_index_count = 0;
	ctrl_values_count = 0;
	for (i = 0; i < 16; i++) {
		ctrl_from_count[i] = 0;
		ctrl_to_concate[i][0] = '\0';
		ctrl_visible[i][0] = '\0';
		ctrl_visible_max[i] = 1;
		ctrl_translate[i][0] = '\0';
		ctrl_translate_type[i] = 1;
		
		ctrl_index[i][0] = '\0';
		ctrl_index_val[i] = -1;

		ctrl_values[i][0] = '\0';
		ctrl_values_cnt[i] = 0;
		for (j = 0; j < 32; j++) {
			ctrl_from_concate[i][j][0] = '\0';
			ctrl_values_val[i][j] = 0;
		}
	}

	/* parse ctrl opt */

	/* TODO - check for name boundary */
	while (ctrl_opt && *ctrl_opt) {
		switch (*ctrl_opt++) {
			case 'c':
				if (*ctrl_opt++ != '-') {
					error("wrong ctrl option format (c) - waiting -");
					return 1;
				}
				while (1) {
					tmp_str = ctrl_from_concate[ctrl_to_concate_count][ctrl_from_count[ctrl_to_concate_count]];
					sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ",:", 0);
					if (strlen(ctrl_from_concate[ctrl_to_concate_count][ctrl_from_count[ctrl_to_concate_count]]) == 0) {
						error("wrong ctrl option format (c) - wrong source ctrl name");
						return 1;
					}
					ctrl_from_count[ctrl_to_concate_count]++;

					if (sep == ':') {
						ctrl_opt++;
						break;
					}
					if (sep != ',') {
						error("wrong ctrl option format (c) - wrong separator - waiting , %c", sep);
						return 1;
					}

					ctrl_opt++;
				}

				tmp_str = ctrl_to_concate[ctrl_to_concate_count];
				/* next is new ctrl name */
				sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ",", 0);
				if (strlen(ctrl_to_concate[ctrl_to_concate_count]) == 0) {
					error("wrong ctrl option format (c) - wrong target ctrl name");
					return 1;
				}
				ctrl_to_concate_count++;
				break;
			case 'v':
				if (*ctrl_opt++ != '-') {
					error("wrong ctrl option format (v) - waiting -");
					return 1;
				}
				while (1)
				{
					tmp_str = ctrl_visible[ctrl_visible_count];
					sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ":", 0);
					if (strlen(ctrl_visible[ctrl_visible_count]) == 0) {
						error("wrong ctrl option format (v) - wrong ctrl name");
						return 1;
					}

					if (sep == ':') {
						ctrl_opt++;
						break;
					}
					ctrl_opt++;
				}

				tmp_num = tmp_num_str;
				/* next is new ctrl name */
				sep = get_str(&ctrl_opt, tmp_num, 10, ",", 1);
				if (strlen(tmp_num_str) == 0) {
					error("wrong ctrl option format (v) - wrong vcount count");
					return 1;
				}
				ctrl_visible_max[ctrl_visible_count] = atoi(tmp_num_str);
				ctrl_visible_count++;
				break;
			case 't':
				if (*ctrl_opt++ != '-') {
					error("wrong ctrl option format (t) - waiting -");
					return 1;
				}
				while (1)
				{
					tmp_str = ctrl_translate[ctrl_translate_count];
					sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ":", 0);
					if (strlen(ctrl_translate[ctrl_translate_count]) == 0) {
						error("wrong ctrl option format (t) - wrong ctrl name");
						return 1;
					}

					if (sep == ':') {
						ctrl_opt++;
						break;
					}
					ctrl_opt++;
				}

				tmp_num = tmp_num_str;
				/* next is new ctrl translate */
				sep = get_str(&ctrl_opt, tmp_num, 10, ",", 1);
				if (strlen(tmp_num_str) == 0) {
					error("wrong ctrl option format (t) - wrong translation function num");
					return 1;
				}
				ctrl_translate_type[ctrl_translate_count] = atoi(tmp_num_str);
				ctrl_translate_count++;
				break;
			case 'i':
				if (*ctrl_opt++ != '-') {
					error("wrong ctrl option format (i) - waiting -");
					return 1;
				}
				while (1)
				{
					tmp_str = ctrl_index[ctrl_index_count];
					sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ":", 0);
					if (strlen(ctrl_index[ctrl_index_count]) == 0) {
						error("wrong ctrl option format (i) - wrong ctrl name");
						return 1;
					}

					if (sep == ':') {
						ctrl_opt++;
						break;
					}
					ctrl_opt++;
				}

				tmp_num = tmp_num_str;
				/* next is new ctrl index */
				sep = get_str(&ctrl_opt, tmp_num, 10, ",", 1);
				if (strlen(tmp_num_str) == 0) {
					error("wrong ctrl option format (i) - wrong index num");
					return 1;
				}
				ctrl_index_val[ctrl_index_count] = atoi(tmp_num_str);
				ctrl_index_count++;
				break;
			case 's':
				if (*ctrl_opt++ != '-') {
					error("wrong ctrl option format (s) - waiting -");
					return 1;
				}
				while (1)
				{
					tmp_str = ctrl_values[ctrl_values_count];
					sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ":", 0);
					if (strlen(ctrl_values[ctrl_values_count]) == 0) {
						error("wrong ctrl option format (s) - wrong ctrl name");
						return 1;
					}

					if (sep == ':') {
						ctrl_opt++;
						break;
					}
					ctrl_opt++;
				}

				/* next is new ctrl name */
				do {
					tmp_num = tmp_num_str;

					sep = get_str(&ctrl_opt, tmp_num, 10, ",#", 1);
					if (strlen(tmp_num_str) == 0) {
						error("wrong ctrl option format (s) - wrong value");
						return 1;
					}
					ctrl_values_val[ctrl_values_count][ctrl_values_cnt[ctrl_values_count]] = atoi(tmp_num_str);
					ctrl_values_cnt[ctrl_values_count]++;
					if (sep != '#')
						break;
					ctrl_opt++;
				} while (1);
				ctrl_values_count++;
				break;
			default:
				error("wrong ctrl option format - unknown subfunction");
				return 1;
		}
		if (*ctrl_opt) {
			if (*ctrl_opt != ',') {
				error("wrong ctrl option format - wrong separator beetwen subfunctions");
				return 1;
			} else
				*ctrl_opt++;
		}
	}
	
	tctl = (liblo10k1_ctl_transform_t *)malloc(sizeof(liblo10k1_ctl_transform_t) * ctrl_to_concate_count);
	memset(tctl, 0, sizeof(liblo10k1_ctl_transform_t) * ctrl_to_concate_count);
	
	for (i = 0; i < ctrl_to_concate_count; i++) {
		/* find all controls for this ctl */
		for (k = 0; k < ctrl_from_count[i]; k++) {
			for (j = 0; j < ep->ctl_count; j++) {
				if (strcmp(ctrl_from_concate[i][k], ep->ctls[j].ctl_name) == 0) {
					tctl[i].emu_ctls[tctl[i].emu_ctl_count++] = j;
					break;
				}
			}
		}
		strcpy(tctl[i].ctl_name, ctrl_to_concate[i]);
	}
	
	if (liblo10k1_emu_transform_patch(ep,  tctl, ctrl_to_concate_count, &np) < 0)
	{
		error("error on liblo10k1_emu_transform_patch");
		return 1;
	}
	
	free(tctl);
	
	for (i = 0; i < ctrl_visible_count; i++) {
		ctl_idx = liblo10k1_patch_find_ctl_by_name(np, ctrl_visible[i]);
		if (ctl_idx < 0)
			goto err;
		if (liblo10k1_patch_ctl_set_vcount(np, ctl_idx, ctrl_visible_max[i]) < 0)
			goto err;
	}

	for (i = 0; i < ctrl_translate_count; i++) {
		ctl_idx = liblo10k1_patch_find_ctl_by_name(np, ctrl_translate[i]);
		if (ctl_idx < 0)
			goto err;
		if (liblo10k1_patch_ctl_set_trans(np, ctl_idx, ctrl_translate_type[i]) < 0)
			goto err;
	}
	
	for (i = 0; i < ctrl_index_count; i++) {
		ctl_idx = liblo10k1_patch_find_ctl_by_name(np, ctrl_index[i]);
		if (ctl_idx < 0)
			goto err;
		if (liblo10k1_patch_ctl_set_index(np, ctl_idx, ctrl_index_val[i]) < 0)
			goto err;
	}

	for (i = 0; i < ctrl_values_count; i++) {
		ctl_idx = liblo10k1_patch_find_ctl_by_name(np, ctrl_values[i]);
		if (ctl_idx < 0)
			goto err;
		for (j = 0; j < ctrl_values_cnt[i]; j++) {
			if (liblo10k1_patch_ctl_set_value(np, ctl_idx, j, ctrl_values_val[i][j]) < 0)
				goto err;
		}
	}
	
	*p = np;
	return 0;
err:
	if (np)
		liblo10k1_patch_free(np);
	return 1;
}

static int transfer_native_patch(liblo10k1_dsp_patch_t *p, char *ctrl_opt)
{
	unsigned char ctrl_values_count;

	char tmp_name_from_str[MAX_NAME_LEN];
	char tmp_name_to_str[MAX_NAME_LEN];
	char *tmp_str;
	char *tmp_num;
	char tmp_num_str[20];
	
	char sep;

	int ctl_idx;
		
	/* parse ctrl opt */

	/* TODO - check for name boundary */
	while (ctrl_opt && *ctrl_opt) {
		switch (*ctrl_opt++) {
			case 'r':
				if (*ctrl_opt++ != '-') {
					error("wrong ctrl option format (r) - waiting -");
					return 1;
				}
				while (1) {
					tmp_str = tmp_name_from_str;
					sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ",:", 0);
					if (strlen(tmp_name_from_str) == 0) {
						error("wrong ctrl option format (r) - wrong source ctrl name");
						return 1;
					}
					
					if (sep == ':') {
						ctrl_opt++;
						break;
					}
					
					ctrl_opt++;
				}

				tmp_str = tmp_name_to_str;
				/* next is new ctrl name */
				sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ",", 0);
				if (strlen(tmp_name_to_str) == 0) {
					error("wrong ctrl option format (r) - wrong target ctrl name");
					return 1;
				}
				
				ctl_idx = liblo10k1_patch_find_ctl_by_name(p, tmp_name_from_str);
				if (ctl_idx < 0) {
					error("unknown ctrl name");
					return 1;
				}
				strcpy(p->ctl[ctl_idx].name, tmp_name_to_str);
				break;
			case 'i':
				if (*ctrl_opt++ != '-') {
					error("wrong ctrl option format (i) - waiting -");
					return 1;
				}
				while (1)
				{
					tmp_str = tmp_name_from_str;
					sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ":", 0);
					if (strlen(tmp_name_from_str) == 0) {
						error("wrong ctrl option format (i) - wrong ctrl name");
						return 1;
					}

					if (sep == ':') {
						ctrl_opt++;
						break;
					}
					ctrl_opt++;
				}

				tmp_num = tmp_num_str;
				/* next is new ctrl index */
				sep = get_str(&ctrl_opt, tmp_num, 10, ",", 1);
				if (strlen(tmp_num_str) == 0) {
					error("wrong ctrl option format (i) - wrong index num");
					return 1;
				}
				ctl_idx = liblo10k1_patch_find_ctl_by_name(p, tmp_name_from_str);
				if (ctl_idx < 0) {
					error("unknown ctrl name");
					return 1;
				}
				if (liblo10k1_patch_ctl_set_index(p, ctl_idx, atoi(tmp_num_str)) < 0) {
					error("can not set ctrl index");
					return 1;
				}
				break;
			case 's':
				if (*ctrl_opt++ != '-') {
					error("wrong ctrl option format (s) - waiting -");
					return 1;
				}
				while (1)
				{
					tmp_str = tmp_name_from_str;
					sep = get_str(&ctrl_opt, tmp_str, MAX_NAME_LEN, ":", 0);
					if (strlen(tmp_name_from_str) == 0) {
						error("wrong ctrl option format (s) - wrong ctrl name");
						return 1;
					}

					if (sep == ':') {
						ctrl_opt++;
						break;
					}
					ctrl_opt++;
				}
				ctl_idx = liblo10k1_patch_find_ctl_by_name(p, tmp_name_from_str);
				if (ctl_idx < 0){
					error("unknown ctrl name");
					return 1;
				}
				
				/* next is value */
				ctrl_values_count = 0;
				do {
					tmp_num = tmp_num_str;

					sep = get_str(&ctrl_opt, tmp_num, 10, ",#", 1);
					if (strlen(tmp_num_str) == 0) {
						error("wrong ctrl option format (s) - wrong value");
						return 1;
					}
					if (liblo10k1_patch_ctl_set_value(p, ctl_idx, ctrl_values_count, atoi(tmp_num_str)) < 0){
						error("can not set ctrl value");
						return 1;
					}
					if (sep != '#')
						break;
					ctrl_opt++;
					ctrl_values_count++;
				} while (1);
				break;
			default:
				error("wrong ctrl option format - unknown subfunction");
				return 1;
		}
		if (*ctrl_opt) {
			if (*ctrl_opt != ',') {
				error("wrong ctrl option format - wrong separator beetwen subfunctions");
				return 1;
			} else
				*ctrl_opt++;
		}
	}
	
	return 0;
}

static int list_patch(char *file_name)
{
	int err, i, j;
	liblo10k1_emu_patch_t *p;

	err = load_patch(file_name, &p);
	if (err)
		return err;

 	/* and now print */
	printf("Patch name : %s\n", p->patch_name);
	printf("IN:\n");
	for (i = 0; i < p->in_count; i++)
		printf("%03d: %08x\n", i, p->ins[i]);
	printf("OUT:\n");
	for (i = 0; i < p->out_count; i++)
		printf("%03d: %08x\n", i, p->outs[i]);
	
	printf("DYN:\n");
	for (i = 0; i < p->dyn_count; i++)
		printf("%03d: %08x\n", i, p->dyns[i]);
	
	printf("STA:\n");
	for (i = 0; i < p->sta_count; i++)
		printf("%03d: %08x  %08x\n", i, p->stas[i].sc, p->stas[i].sc_val);

	printf("CTRL:\n");
	for (i = 0; i < p->ctl_count; i++)
		printf("%03d: %08x   %08x  %08x  %08x   %s\n", i, p->ctls[i].ctl, p->ctls[i].ctl_val, p->ctls[i].ctl_val_min, p->ctls[i].ctl_val_max, p->ctls[i].ctl_name);
	
	printf("CON:\n");
	for (i = 0; i < p->con_count; i++)
		printf("%03d: %08x  %08x\n", i, p->cons[i].sc, p->cons[i].sc_val);
	
	printf("TRAM LOOKUP:\n");
	for (i = 0; i < p->tram_lookup_count; i++) {
		printf("%03d: %08x\n", i, p->tram_lookups[i].size);
		for (j = 0; j < p->tram_lookups[i].read_line_count; j++)
			printf("  %03d: %c  %03d  %08x  %08x\n", i, 'R', j, p->tram_lookups[i].read_lines[j].line,p->tram_lookups[i].read_lines[j].line_size);
		for (j = 0; j < p->tram_lookups[i].write_line_count; j++)
			printf("  %03d: %c  %03d  %08x  %08x\n", i, 'W', j, p->tram_lookups[i].write_lines[j].line,p->tram_lookups[i].write_lines[j].line_size);
	}
	
	printf("TRAM DELAY:\n");
	for (i = 0; i < p->tram_delay_count; i++) {
		printf("%03d: %08x\n", i, p->tram_delays[i].size);
		for (j = 0; j < p->tram_delays[i].read_line_count; j++)
			printf("  %03d: %c  %03d  %08x  %08x\n", i, 'R', j, p->tram_delays[i].read_lines[j].line,p->tram_delays[i].read_lines[j].line_size);
		for (j = 0; j < p->tram_delays[i].write_line_count; j++)
			printf("  %03d: %c  %03d  %08x  %08x\n", i, 'W', j, p->tram_delays[i].write_lines[j].line,p->tram_delays[i].write_lines[j].line_size);
	}
		
	printf("INSTR:\n");
	for (i = 0; i < p->instr_count; i++)
		printf("%03d: %08x  %08x  %08x  %08x  %08x\n", i, p->instrs[i].op, p->instrs[i].arg[0], p->instrs[i].arg[1], p->instrs[i].arg[2], p->instrs[i].arg[3]);
	return 0;
}

static int add_patch(char *file_name, int udin, char *ctrl_opt, char *opt_patch_name, int where)
{
	int err;
	liblo10k1_emu_patch_t *ep;
	liblo10k1_dsp_patch_t *p;
	
	err = load_patch(file_name, &ep);
	if (err)
		return err;

	err = transfer_patch(udin, ctrl_opt, ep, &p);
	if (err) {
		error("unable to transfer patch");
		return err;
	}
	
	if (opt_patch_name) {
		strncpy(p->patch_name, opt_patch_name, MAX_NAME_LEN - 1);
		p->patch_name[MAX_NAME_LEN - 1] = '\0';
	}
		
	if ((err = liblo10k1_patch_load(&conn, p, where, NULL, NULL)) < 0) {
		error("unable to load patch (ld10k1 error:%s)", liblo10k1_error_str(err));
		return err;
	}

	return 0;
}

static int load_dsp_patch(char *file_name, char *ctrl_opt, char *opt_patch_name, int where)
{
	int err;
	liblo10k1_dsp_patch_t *p;
	liblo10k1_file_info_t *fi;
	
	fi = NULL;
	
	if ((err = liblo10k1lf_load_dsp_patch(&p, file_name, &fi)) < 0) {
		error("unable to load dsp patch (ld10k1 error:%s)", liblo10k1_error_str(err));
		goto err;
	}
	
	err = transfer_native_patch(p, ctrl_opt);
	if (err)
		goto err;
	
	if (opt_patch_name) {
		strncpy(p->patch_name, opt_patch_name, MAX_NAME_LEN - 1);
		p->patch_name[MAX_NAME_LEN - 1] = '\0';
	}
		
	if ((err = liblo10k1_patch_load(&conn, p, where, NULL, NULL)) < 0) {
		error("unable to load dsp patch (ld10k1 error:%s)", liblo10k1_error_str(err));
		return err;
	}

	liblo10k1lf_file_info_free(fi);
	liblo10k1_patch_free(p);
	return 0;
err:
	if (fi)
		liblo10k1lf_file_info_free(fi);
	if (p)
		liblo10k1_patch_free(p);
	return 1;
}


static int save_dsp_patch(char *file_name, int pn)
{
	int err;
 	
	liblo10k1_dsp_patch_t *p;
	liblo10k1_file_info_t *fi;
	
	if (pn < 0) {
		error("wrong patch num");
		return 1;
	}
	
	fi = liblo10k1lf_file_info_alloc();
	if (!fi) {
		error("no mem");
		goto err;
	}
		
	if ((err = liblo10k1_patch_get(&conn, pn, &p)) < 0) {
		error("unable to get dsp patch (ld10k1 error:%s)", liblo10k1_error_str(err));
		goto err;
	}
	
	fi->creater = strdup("lo10k1 - emu10k1/emu10k2 effect loader for alsa");
	
	if ((err = liblo10k1lf_save_dsp_patch(p, file_name, fi)) < 0) {
		error("unable to save dsp patch (ld10k1 error:%s)", liblo10k1_error_str(err));
		goto err;
	}
	
	liblo10k1lf_file_info_free(fi);
	liblo10k1_patch_free(p);
	return 0;
err:
	if (fi)
		liblo10k1lf_file_info_free(fi);
	if (p)
		liblo10k1_patch_free(p);
	return 1;
}

void debug_print(char *str)
{
	printf("%s", str);
}

static int debug(int deb)
{
	int err;
	
	if ((err = liblo10k1_debug(&conn, deb, debug_print)) < 0) {
		error("unable to debug (ld10k1 error:%s)", liblo10k1_error_str(err));
		return err;
	}

	return 0;
}

static int del_patch(char *file_name)
{
	int err;

	if ((err = liblo10k1_patch_unload(&conn, atoi(file_name))) < 0 ) {
		error("unable to del patch (ld10k1 error:%s)", liblo10k1_error_str(err));
		return err;
	}

	return 0;
}

static int setup_dsp()
{
	int err;

	if ((err = liblo10k1_dsp_init(&conn)) < 0) {
		error("unable to setup DSP (ld10k1 error:%s)", liblo10k1_error_str(err));
		return err;
	}
	
	return 0;
}

static int is_num(char *str)
{
	int i;

	for (i = 0; i < strlen(str); i++)
		if (!isdigit(str[i]))
			return 0;
	return 1;
}

typedef struct
{
	char type;
	int patch;
	int io;
} conn_info_t;

char *parse_connect_sym(char *con_str, char *sym, int *len, int max_len)
{
	*len = 0;
	while (*len < max_len && *con_str && *con_str != ',' && *con_str != ')') {
		*sym++ = *con_str++;
		(*len)++;
	}

	*sym++ = '\0';
	if (*len == 0)
		return NULL;
	return con_str;
}

char *parse_simple_params(char *con_str, char io_type, int pn, conn_info_t *con_info, int *con_info_count, int max_con_info_count)
{
	char con_arg[255];
	int con_arg_len = 0;
	int io_idx;

	while(1) {
		if (*con_info_count >= max_con_info_count)
			return NULL;
		if (!(con_str = parse_connect_sym(con_str, con_arg, &con_arg_len, sizeof(con_arg) - 1)))
			return NULL;
		if (*con_str != ')' && *con_str != ',')
			return NULL; /* wrong format */

		con_info[*con_info_count].type = io_type;

		if (is_num(con_arg)) {
			/* input number */
			if (io_type == 'A' || io_type == 'B') {
				con_info[*con_info_count].patch = pn;
				con_info[(*con_info_count)++].io = atoi(con_arg);
			} else {
				con_info[*con_info_count].patch = -1;
				con_info[(*con_info_count)++].io = atoi(con_arg);
			}
		} else {
			/* input name */
			switch (io_type) {
				case 'A':
					if (liblo10k1_find_patch_in(&conn, pn, con_arg, &io_idx) < 0)
						return NULL;
					con_info[*con_info_count].patch = pn;
					con_info[(*con_info_count)++].io = io_idx;
					break;
				case 'B':
					if (liblo10k1_find_patch_out(&conn, pn, con_arg, &io_idx) < 0)
						return NULL;
					con_info[*con_info_count].patch = pn;
					con_info[(*con_info_count)++].io = io_idx;
					break;
				case 'F':
					if (liblo10k1_find_fx(&conn, con_arg, &io_idx) < 0)
						return NULL;
					con_info[*con_info_count].patch = -1;
					con_info[(*con_info_count)++].io = io_idx;
					break;
				case 'I':
					if (liblo10k1_find_in(&conn, con_arg, &io_idx) < 0)
						return NULL;
					con_info[*con_info_count].patch = -1;
					con_info[(*con_info_count)++].io = io_idx;
					break;
				case 'O':
					if (liblo10k1_find_out(&conn, con_arg, &io_idx) < 0)
						return NULL;
					con_info[*con_info_count].patch = -1;
					con_info[(*con_info_count)++].io = io_idx;
					break;
			}
		}

		if (*con_str != ',')
			break;
		con_str++;
	}
	return con_str;
}

char *parse_patch_params(char *con_str, char io_type, conn_info_t *con_info, int *con_info_count, int max_con_info_count)
{
	char con_arg[255];
	int con_arg_len = 0;
	int i;
	int patch_num = -1;
	int io_count = 0;

	if (!(con_str = parse_connect_sym(con_str, con_arg, &con_arg_len, sizeof(con_arg) - 1)))
		return NULL;
	if (*con_str != ')' && *con_str != ',')
		return NULL;

	if (is_num(con_arg))
		/* patch number */
		patch_num = atoi(con_arg);
	else
		/* patch name - find patch */
		if (liblo10k1_find_patch(&conn, con_arg, &patch_num) < 0)
			return NULL;

	/* argumenty */
	if (*con_str == ',') {
		con_str++;
		if (*con_info_count >= max_con_info_count)
			return NULL;
		if (!(con_str = parse_simple_params(con_str, io_type, patch_num, con_info, con_info_count, max_con_info_count)))
			return NULL;
	} else {
		/* add all patch inputs or outputs */
		if (io_type == 'A') {
			/* get all inputs */
			if (liblo10k1_get_pin_count(&conn, patch_num, &io_count) < 0)
				return NULL;
		} else {
			/* get all outputs */
			if (liblo10k1_get_pout_count(&conn, patch_num, &io_count) < 0)
				return NULL;
		}

		i = 0;
		while (i < io_count) {
			if (*con_info_count >= max_con_info_count) {
				return NULL;
			}
			con_info[*con_info_count].type = io_type;
			con_info[*con_info_count].patch = patch_num;
			con_info[(*con_info_count)++].io = i;
			i++;
		}
	}
	return con_str;
}

int parse_connect(int add, char *con_str, int *multi, int *simple, conn_info_t **con_info, int *con_info_count, int max_con_info_count)
{
	char con[10];
	int con_len;

	int ft = 0;
	while (1) {
		con_len = 0;
		for(;*con_str && *con_str != '('; con_str++) {
			if (con_len >= sizeof(con) - 1)
				return 1;/* ERROR */
			con[con_len++] = *con_str;
		}
		con[con_len++] = '\0';

		if (*con_str != '(')
			return 1;/* ERROR */
		con_str++;

		if (ft && strcmp(con,"FX") == 0) {
			if (!(con_str = parse_simple_params(con_str, 'F', -1, con_info[ft], &(con_info_count[ft]), max_con_info_count)))
				return 1;/* ERROR */
		} else if (ft && strcmp(con,"IN") == 0) {
			if (!(con_str = parse_simple_params(con_str, 'I',  -1, con_info[ft], &(con_info_count[ft]), max_con_info_count)))
				return 1;/* ERROR */
		} else if (ft && strcmp(con,"OUT") == 0) {
			if (!(con_str = parse_simple_params(con_str, 'O', -1, con_info[ft], &(con_info_count[ft]), max_con_info_count)))
				return 1;/* ERROR */
		} else if (strcmp(con,"PIN") == 0) {
			if (!(con_str = parse_patch_params(con_str, 'A', con_info[ft], &(con_info_count[ft]), max_con_info_count)))
				return 1;/* ERROR */
		} else if (strcmp(con,"POUT") == 0) {
			if (!(con_str = parse_patch_params(con_str, 'B', con_info[ft], &(con_info_count[ft]), max_con_info_count)))
				return 1;/* ERROR */
		} else
			return 1;/* ERROR */

		con_str++;

		if (ft && !*con_str)
			return 0; /* OK */
		if (!add) {
			if (!*con_str)
				return 0;
			else
				return 1;
		}
		
		if (add && !ft && (*con_str == '=' || *con_str == '>' || *con_str == ':')) {
			ft++;
			if (*con_str == '=') {
				*multi = 0;
				*simple = 0;
			} else if (*con_str == ':') {
				*multi = 0;
				*simple = 1;
			} else
				*multi = 1;

		} else if (add &&*con_str != '+')
			return 1;/* ERROR */
		/* process next */
		con_str++;
	}
}

int parse_rename(char *con_str, char *io_type, int *pn, int *io, char **new_name)
{
	char con[10];
	int con_len = 0;

	char con_arg1[255];
	int con_arg_len1 = 0;
	int is_arg_num1 = 0;
	int arg_num1 = -1;

	char con_arg2[255];
	int con_arg_len2 = 0;
	int is_arg_num2 = 0;
	int arg_num2 = -1;

	for(;*con_str && *con_str != '('; con_str++) {
		if (con_len >= sizeof(con) - 1)
			return 1;/* ERROR */
		con[con_len++] = *con_str;
	}
	con[con_len++] = '\0';

	if (*con_str != '(')
		return 1;/* ERROR */
	con_str++;

	*io_type = '\0';
	if (strcmp(con,"FX") == 0)
		*io_type = 'F';
	else if (strcmp(con,"IN") == 0)
		*io_type = 'I';
	else if (strcmp(con,"OUT") == 0)
		*io_type = 'O';
	else if (strcmp(con,"PIN") == 0)
		*io_type = 'A';
	else if (strcmp(con,"POUT") == 0)
		*io_type = 'B';
	else if (strcmp(con,"PATCH") == 0)
		*io_type = 'P';
	else
		return 1;/* ERROR */

	if (!(con_str = parse_connect_sym(con_str, con_arg1, &con_arg_len1, sizeof(con_arg1) - 1)))
			return 1;

	if ((is_arg_num1 = is_num(con_arg1)))
		arg_num1 = atoi(con_arg1);

	if (*io_type == 'A' || *io_type == 'B') {
		/* two arguments */
		if (*con_str != ',')
			return 1; /* ERROR */
		con_str++;
		if (!(con_str = parse_connect_sym(con_str, con_arg2, &con_arg_len2, sizeof(con_arg2) - 1)))
			return 1;

		if ((is_arg_num2 = is_num(con_arg2)))
			arg_num2 = atoi(con_arg2);
	}

	if (*con_str != ')')
		return 1; /* ERROR */

	switch (*io_type) {
		case 'A':
			if (!is_arg_num1)
				if (liblo10k1_find_patch(&conn, con_arg1, &arg_num1) < 0)
					return 1;
			if (!is_arg_num2)
				if (liblo10k1_find_patch_in(&conn, arg_num1, con_arg2, &arg_num2) < 0)
					return 1;
			break;
		case 'B':
			if (!is_arg_num1)
				if (liblo10k1_find_patch(&conn, con_arg1, &arg_num1) < 0)
					return 1;
			if (!is_arg_num2)
				if (liblo10k1_find_patch_out(&conn, arg_num1, con_arg2, &arg_num2) < 0)
					return 1;
			break;
		case 'F':
			if (!is_arg_num1)
				if (liblo10k1_find_fx(&conn, con_arg1, &arg_num1) < 0)
					return 1;
			break;
		case 'I':
			if (!is_arg_num1)
				if (liblo10k1_find_in(&conn, con_arg1, &arg_num1) < 0)
					return 1;
			break;
		case 'O':
			if (!is_arg_num1)
				if (liblo10k1_find_out(&conn, con_arg1, &arg_num1) < 0)
					return 1;
			break;
		case 'P':
			if (!is_arg_num1)
				if (liblo10k1_find_patch(&conn, con_arg1, &arg_num1) < 0)
					return 1;
			break;
	}

	con_str++;
	if (*con_str != '=')
		return 1; /* ERROR */
	con_str++;

	if (*io_type == 'A' || *io_type == 'B' || *io_type == 'P') {
		*pn = arg_num1;
		*io = arg_num2;
	} else {
		*io = arg_num1;
	}

	*new_name = con_str;
	return 0;
}

static int con_add(char *file_name)
{
	int err, i;
	int multi = 0;
	int simple = 0;

	conn_info_t con_infof[32];
	conn_info_t con_infot[32];
	conn_info_t *con_info[2] = {con_infof, con_infot};

	int con_info_count[2] = {0, 0};

	if (parse_connect(1, file_name, &multi, &simple, con_info, con_info_count, 32)) {
		error("wrong parameter - connection string");
		return 1;
	}

	if (con_info_count[0] != con_info_count[1]) {
		error("wrong parameter - connection string from <> to");
		return 1;
	}

	if (!con_info_count[0]) {
		error("wrong parameter - connection string");
		return 1;
	}

	for (i = 0; i < con_info_count[0]; i++) {
		if ((err = liblo10k1_con_add(&conn, multi, simple,
			con_infof[i].type, con_infof[i].patch, con_infof[i].io,
			con_infot[i].type, con_infot[i].patch, con_infot[i].io,
			NULL)) < 0) {
			error("unable to connect  (ld10k1 error:%s)", liblo10k1_error_str(err));
			return err;
		}
	}

	return 0;
}

static int con_del(char *file_name)
{
	conn_info_t con_info[32];
	int err, i;

	conn_info_t *con_info_p = con_info;

	int con_info_count = 0;

	if (parse_connect(0, file_name, NULL, NULL, &con_info_p, &con_info_count, 32)) {
		error("wrong parameter - disconnection string");
		return 1;
	}

	if (!con_info_count) {
		error("wrong parameter - disconnection string");
		return 1;
	}

	for (i = 0; i < con_info_count; i++) {
		if ((err = liblo10k1_con_del(&conn, con_info[i].type, con_info[i].patch, con_info[i].io, NULL)) < 0) {
			error("unable to connect  (ld10k1 error:%s)", liblo10k1_error_str(err));
			return err;
		}
	}
	
	return 0;
}

static int rename_arg(char *arg_name)
{
	char io_type = '\0';
	int pn = -1;
	int io = -1;
	char *new_name = NULL;

	if (parse_rename(arg_name, &io_type, &pn, &io, &new_name)) {
		error("wrong parameter for rename");
		return 1;
	}

	switch (io_type) {
		case 'A':
			if (liblo10k1_rename_patch_in(&conn, pn, io, new_name) < 0) {
				error("couldn't rename patch in");
				return 1;
			}
			break;
		case 'B':
			if (liblo10k1_rename_patch_out(&conn, pn, io, new_name) < 0) {
				error("couldn't rename patch out");
				return 1;
			}
			break;
		case 'F':
			if (liblo10k1_rename_fx(&conn, io, new_name) < 0) {
				error("couldn't rename fx");
				return 1;
			}
			break;
		case 'I':
			if (liblo10k1_rename_in(&conn, io, new_name) < 0) {
				error("couldn't rename in");
				return 1;
			}
			break;
		case 'O':
			if (liblo10k1_rename_out(&conn, io, new_name) < 0) {
				error("couldn't rename out");
				return 1;
			}
			break;
		case 'P':
			if (liblo10k1_rename_patch(&conn, pn, new_name) < 0) {
				error("couldn't rename patch");
				return 1;
			}
			break;
	}

	return 0;
}

static int dump(char *file_name)
{
	int err;
	void *dump = NULL;
	int size = 0;

	FILE *dump_file = NULL;

	if ((err = liblo10k1_dump(&conn, &dump, &size)) < 0 ) {
		error("unable to dump (ld10k1 error:%s)", liblo10k1_error_str(err));
		return err;
	}

	dump_file = fopen(file_name, "w");
	if (!dump_file) {
		free(dump);
		error("unable to open dump");
		return 1;
	}

	if (fwrite(dump, 1, size, dump_file) < size) {
		free(dump);
		error("unable to write dump");
		return 1;
	}

	free(dump);
	fclose(dump_file);

	return 0;
}

static int store_dsp(char *file_name)
{
	int err;
 	
	liblo10k1_file_dsp_setup_t *setup;
	liblo10k1_file_info_t *fi;
	
	fi = liblo10k1lf_file_info_alloc();
	if (!fi) {
		error("no mem");
		goto err;
	}
		
	if ((err = liblo10k1lf_get_dsp_config(&conn, &setup)) < 0) {
		error("unable to get dsp config (ld10k1 error:%s)", liblo10k1_error_str(err));
		goto err;
	}
	
	fi->creater = strdup("lo10k1 - emu10k1/emu10k2 effect loader for alsa");
	
	if ((err = liblo10k1lf_save_dsp_config(setup, file_name, fi)) < 0) {
		error("unable to store dsp config (ld10k1 error:%s)", liblo10k1_error_str(err));
		goto err;
	}
	
	liblo10k1lf_file_info_free(fi);
	liblo10k1lf_dsp_config_free(setup);
	return 0;
err:
	if (fi)
		liblo10k1lf_file_info_free(fi);
	return 1;
}

static int restore_dsp(char *file_name)
{
	int err;
 	
	liblo10k1_file_dsp_setup_t *setup;
	liblo10k1_file_info_t *fi;
	
	fi = NULL;
	
	if ((err = liblo10k1lf_load_dsp_config(&setup, file_name, &fi)) < 0) {
		error("unable to restore dsp config (ld10k1 error:%s)", liblo10k1_error_str(err));
		goto err;
	}
	
	if ((err = liblo10k1lf_put_dsp_config(&conn, setup)) < 0) {
		error("unable to put dsp config (ld10k1 error:%s)", liblo10k1_error_str(err));
		goto err;
	}
	
	liblo10k1lf_file_info_free(fi);
	liblo10k1lf_dsp_config_free(setup);
	return 0;
err:
	if (fi)
		liblo10k1lf_file_info_free(fi);
	return 1;
}
int main(int argc, char *argv[])
{
	int  c;

	int opt_list;
	int opt_setup;
	int opt_info;
	int opt_add;
	int opt_del;
	int opt_con_add;
	int opt_con_del;
	int opt_debug;
	char *opt_list_patch;
	int opt_use_default_io_names;
	char *opt_ctrl;
	char *opt_patch_name;
	char *opt_new_name;
	int opt_where;
	int option_index = 0;
	char *opt_dump_name;
	char *opt_host;
	char *tmp = NULL;
	
	int opt_store;
	int opt_restore;
	char *opt_store_restore_file;
	
	int opt_load_patch;
	int opt_save_patch;
	
	unsigned int opt_wait_for_conn;

	liblo10k1_param params;

	int err = 0;
	
 	static struct option long_options[] = {
				{"pipe_name", 1, 0, 'p'},
    				{"list", 1, 0, 'l'},
				{"info", 0, 0, 'i'},
				{"add", 1, 0, 'a'},
				{"del", 1, 0, 'd'},
				{"conadd", 1, 0, 'q'},
				{"condel", 1, 0, 'w'},
				{"debug", 1, 0, 0},
				{"defionames", 0, 0, 'n'},
				{"ctrl", 1, 0, 0},
				{"patch_name", 1, 0, 0},
				{"where", 1, 0, 0},
				{"setup", 1, 0, 's'},
				{"renam", 1, 0, 0},
				{"dump", 1, 0, 0},
				{"host", 1, 0, 0},
				{"path", 1, 0, 'P'},
				{"store", 1, 0, 0},
				{"restore", 1, 0, 0},
				{"load_patch", 1, 0, 0},
				{"save_patch", 1, 0, 0},
				{"wait", 1, 0, 0},
				{0, 0, 0, 0}
	};

	opt_list = 0;
	opt_add = 0;
	opt_del = 0;
	opt_list_patch = NULL;
	opt_info = 0;
	opt_con_add = 0;
	opt_con_del = 0;
	opt_debug = 0;
	opt_use_default_io_names = 0;
	opt_ctrl = NULL;
	opt_patch_name = NULL;
	opt_new_name = NULL;
	opt_where = -1;
	opt_setup = 0;
	opt_dump_name = NULL;
	opt_host = NULL;
	
	opt_store = 0;
	opt_restore = 0;
	opt_store_restore_file = NULL;
	
	opt_load_patch = 0;
	opt_save_patch = 0;
	
	opt_wait_for_conn = 500;

	strcpy(comm_pipe,"/tmp/.ld10k1_port");

	if (argc > 1 && !strcmp(argv[1], "--help")) {
		help(argv[0]);
		return 0;
	}

	first_path = NULL;
#ifdef EFFECTSDIR
	add_paths(EFFECTSDIR);
#endif

	while ((c = getopt_long(argc, argv, "hil:p:a:d:q:w:nsh:P:",
	        long_options, &option_index)) != EOF) {
		switch (c) {
		case 0:
			if (strcmp(long_options[option_index].name, "debug") == 0)
				opt_debug = atoi(optarg);
			else if (strcmp(long_options[option_index].name, "ctrl") == 0)
				opt_ctrl = optarg;
			else if (strcmp(long_options[option_index].name, "patch_name") == 0)
				opt_patch_name = optarg;
			else if (strcmp(long_options[option_index].name, "where") == 0)
				opt_where = atoi(optarg);
			else if (strcmp(long_options[option_index].name, "renam") == 0)
				opt_new_name = optarg;
			else if (strcmp(long_options[option_index].name, "dump") == 0)
				opt_dump_name = optarg;
			else if (strcmp(long_options[option_index].name, "host") == 0)
				opt_host = optarg;
			else if (strcmp(long_options[option_index].name, "wait") == 0) {
				opt_wait_for_conn = atoi(optarg);
				if (opt_wait_for_conn < 0)
					opt_wait_for_conn = 0;
				else if (opt_wait_for_conn > 500)
					opt_wait_for_conn = 500;
			}
			else if (strcmp(long_options[option_index].name, "store") == 0) {
				opt_store = 1;
				opt_store_restore_file = optarg;
			} else if (strcmp(long_options[option_index].name, "restore") == 0) {
				opt_restore = 1;
				opt_store_restore_file = optarg;
			} else if (strcmp(long_options[option_index].name, "load_patch") == 0) {
				opt_load_patch = 1;
				opt_store_restore_file = optarg;
			} else if (strcmp(long_options[option_index].name, "save_patch") == 0) {
				opt_save_patch = 1;
				opt_store_restore_file = optarg;
			}
			break;
		case 'h':
			help(argv[0]);
			return 0;
		case 'l':
			opt_list = 1;
			opt_list_patch = optarg;
			break;
		case 'p':
			strcpy(comm_pipe, optarg);
			break;
		case 'a':
			opt_add = 1;
			opt_list_patch = optarg;
			break;
		case 'd':
			opt_del = 1;
			opt_list_patch = optarg;
			break;
		case 'i':
			opt_info = 1;
			break;
		case 'q':
			opt_con_add = 1;
			opt_list_patch = optarg;
			break;
		case 'w':
			opt_con_del = 1;
			opt_list_patch = optarg;
			break;
		case 'n':
			opt_use_default_io_names = 1;
			break;
		case 's':
			opt_setup = 1;
			break;
		case 'P':
			add_path(optarg);
			break;
		case '?':
			break;
		default:
			error("unknown option %c", c);
			return 1;
		}
	}

	params.wfc = opt_wait_for_conn;
	if (opt_host) {
		params.type = COMM_TYPE_IP;
		params.name = strtok(opt_host, ":");
		if (!params.name)
			error("wrong hostname");
		tmp = strtok(NULL, ":");
		if (!tmp)
			error("wrong port");
		params.port = atoi(tmp);
	} else {
		params.type = COMM_TYPE_LOCAL;
		params.name = comm_pipe;
	}

	params.server = 0;

	while (1) {
		if ((err = liblo10k1_connect(&params, &conn))) {
			error("unable to connect ld10k1");
			break;
		}
		
		if ((err = liblo10k1_check_version(&conn))) {
			error("Wrong ld10k1 version");
			break;
		}
		
		if (opt_store || opt_restore) {
			if (opt_store) {
				if ((err = store_dsp(opt_store_restore_file)))
					break;
			} else {
				if ((err = restore_dsp(opt_store_restore_file)))
					break;
			}
		} else {
			if (opt_setup)
				if ((err = setup_dsp()))
					break;
			if (opt_list)
				if ((err = list_patch(opt_list_patch)))
					break;
	
			if (opt_add)
				if ((err = add_patch(opt_list_patch, opt_use_default_io_names, opt_ctrl, opt_patch_name, opt_where)))
					break;
			
			if (opt_load_patch)
				if ((err = load_dsp_patch(opt_store_restore_file, opt_ctrl, opt_patch_name, opt_where)))
					break;
					
			if (opt_save_patch)
				if ((err = save_dsp_patch(opt_store_restore_file, opt_where)))
					break;
	
			if (opt_del)
				if ((err = del_patch(opt_list_patch)))
					break;
	
			if (opt_con_add)
				if ((err = con_add(opt_list_patch)))
					break;
	
			if (opt_con_del)
				if ((err = con_del(opt_list_patch)))
					break;
	
			if (opt_debug)
				if ((err = debug(opt_debug)))
					break;
	
			if (opt_new_name)
				if ((err = rename_arg(opt_new_name)))
					break;
	
			if (opt_dump_name)
				if ((err = dump(opt_dump_name)))
					break;
		}
		break;
	}	

	if (liblo10k1_is_open(&conn)) {
		/*send_msg(conn_num, FNC_CLOSE_CONN, NULL, 0);
		free_comm(conn_num);*/
		liblo10k1_disconnect(&conn);
	}
	
	free_all_paths();

	return err;
}
