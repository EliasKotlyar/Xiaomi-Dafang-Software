/*
 *  EMU10k1 loader
 *
 *  Copyright (c) 2003,2004 by Peter Zubaj
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation;  either version 2 of the License, or
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

#ifndef __LD10K1_FNC1_H
#define __LD10K1_FNC1_H

#include "comm.h"

extern ld10k1_dsp_mgr_t dsp_mgr;

int main_loop(comm_param *param, int audigy, const char *card_id, int tram_size, snd_ctl_t *ctlp);

int send_response_ok(int conn_num);
int send_response_err(int conn_num, int err);
int send_response_wd(int conn_num, void *data, int data_size);

#endif /* __LD10K1_FNC1_H */
