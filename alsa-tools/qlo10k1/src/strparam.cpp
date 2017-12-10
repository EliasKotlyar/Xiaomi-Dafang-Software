/*
 *  qlo10k1 - GUI frontend for ld10k1
 *
 *  Copyright (c) 2004 by Peter Zubaj
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
 
#include <stdlib.h>
#include <lo10k1/lo10k1.h>

#include "strglobal.h"
#include "strparam.h"
#include "ld10k1file.h"

class CardParamPrivate
{
public:
	liblo10k1_connection_t card_conn;
	
	liblo10k1_connection_t *conn()
	{
		return &card_conn;
	}
};

CardParam::CardParam(void)
{
	prv = new CardParamPrivate();
	liblo10k1_connection_init(prv->conn());
	Structure = NULL;
}

CardParam::~CardParam(void)
{
	disconnect();
	if (Structure)
		delete Structure;
	delete prv;
}

StrGlobal *CardParam::getStructure()
{
	Structure = new StrGlobal(this);
	return Structure;
}

int CardParam::disconnect(void)
{
	if (liblo10k1_is_open(prv->conn()))
		return liblo10k1_disconnect(prv->conn());
	return 0;
}

int CardParam::isConnected(void)
{
	return liblo10k1_is_open(prv->conn());
}

int CardParam::clearDSP(void)
{
	return liblo10k1_dsp_init(prv->conn());
}

int CardParam::testConnection(void)
{
	int err = connect();
	if (err < 0)
		return err;
	disconnect();
	return 0;
}

int CardParam::connect(void)
{
	int err;

	if (liblo10k1_is_open(prv->conn()))
		disconnect();

	liblo10k1_param params;
	const char *tmp = NULL;

	tmp = CardSocket;

	params.wfc = 500; // FIXME - user selectable
	params.type = COMM_TYPE_LOCAL; // FIXME
	params.server = 0;
	params.name = (char *)tmp;
	params.port = 0;

	if ((err = liblo10k1_connect(&params, prv->conn())) < 0)
		return err;

	return liblo10k1_check_version(prv->conn());
}

int CardParam::load(LD10k1File *ld10k1file, int before, int *loaded, int *loaded_id)
{
	int err;
	liblo10k1_dsp_patch_t *patch = ld10k1file->getLD10k1Format();
	err = liblo10k1_patch_load(prv->conn(), patch, before, loaded, loaded_id);
	return err;
}

int CardParam::unload(int num)
{
	return liblo10k1_patch_unload(prv->conn(), num);
}

int CardParam::get(int patch_num, LD10k1File **ld10k1file)
{
	liblo10k1_dsp_patch_t *patch;
	int err;
	 
	if ((err = liblo10k1_patch_get(prv->conn(), patch_num, &patch)) < 0)
		return err;
		
	LD10k1File *tmp = new LD10k1File();
	tmp->setLD10k1Format(patch);
	*ld10k1file = tmp;
	
	return 0;
}

int CardParam::conAdd(bool multi, bool simple, int from_type, int from_patch, int from_io, int to_type,int to_patch, int to_io, int *id)
{
	return liblo10k1_con_add(prv->conn(), multi, simple, from_type, from_patch, from_io, to_type, to_patch, to_io, id);
}

int CardParam::conDel(int from_type, int from_patch, int from_io, int *id)
{
	return liblo10k1_con_del(prv->conn(), from_type, from_patch, from_io, id);
}

int CardParam::getPointsInfo(int **out, int *count)
{
	return liblo10k1_get_points_info(prv->conn(), out, count);
}

int CardParam::getPointInfo(int point_id, CardParamPointInfo *out)
{
	int err;
	
	liblo10k1_point_info_t pi;
	if ((err = liblo10k1_get_point_info(prv->conn(), point_id, &pi)) < 0)
		return err;
	
	out->id = pi.id;
	out->type = pi.type;
	out->io_idx = pi.io_idx;
	out->simple = pi.simple;
	out->multi = pi.multi;
	out->conn_count = pi.conn_count;
	
	for (int i = 0; i < POINTINFO_MAX_CONN_PER_POINT; i++)
	{
		out->io_type[i] = pi.io_type[i];
		out->patch[i] = pi.patch[i];
		out->io[i] = pi.io[i];
	}
	
	return 0;
}

QString CardParam::errorStr(int err)
{
	return QString(liblo10k1_error_str(err));
}

CardParam::CardParam(CardParam *from)
{
	prv = new CardParamPrivate();
	liblo10k1_connection_init(prv->conn());
	CardName = from->CardName;
	CardSocket = from->CardSocket;
};

int CardParam::getFXCount(int *cnt)
{
	return liblo10k1_get_fx_count(prv->conn(), cnt);
}

int CardParam::getInputCount(int *cnt)
{
	return liblo10k1_get_in_count(prv->conn(), cnt);
}

int CardParam::getOutputCount(int *cnt)
{
	return liblo10k1_get_out_count(prv->conn(), cnt);
}

int CardParam::getFX(int i, QString &name)
{
	liblo10k1_get_io_t ld_tmp;
	int err = 0;
	
	if ((err = liblo10k1_get_fx(prv->conn(), i, &ld_tmp)) < 0)
		return err;
	name = ld_tmp.name;
	return 0;
}

int CardParam::getInput(int i, QString &name)
{
	liblo10k1_get_io_t ld_tmp;
	int err = 0;
	
	if ((err = liblo10k1_get_in(prv->conn(), i, &ld_tmp)) < 0)
		return err;
	name = ld_tmp.name;
	return 0;
}

int CardParam::getOutput(int i, QString &name)
{
	liblo10k1_get_io_t ld_tmp;
	int err = 0;
	
	if ((err = liblo10k1_get_out(prv->conn(), i, &ld_tmp)) < 0)
		return err;
	name = ld_tmp.name;
	return 0;
}

int CardParam::getPInputCount(int pnum, int *cnt)
{
	return liblo10k1_get_pin_count(prv->conn(), pnum, cnt);
}

int CardParam::getPOutputCount(int pnum, int *cnt)
{
	return liblo10k1_get_pout_count(prv->conn(), pnum, cnt);
}

int CardParam::getPInput(int pnum, int i, QString &name)
{
	liblo10k1_get_io_t ld_tmp;
	int err = 0;
	
	if ((err = liblo10k1_get_pin(prv->conn(), pnum, i, &ld_tmp)) < 0)
		return err;
	name = ld_tmp.name;
	return 0;
}

int CardParam::getPOutput(int pnum, int i, QString &name)
{
	liblo10k1_get_io_t ld_tmp;
	int err = 0;
	
	if ((err = liblo10k1_get_pout(prv->conn(), pnum, i, &ld_tmp)) < 0)
		return err;
	name = ld_tmp.name;
	return 0;
}

int CardParam::getPatchesInfo(CardParamPatchInfo ***pi, int *cnt)
{
	liblo10k1_patches_info_t *ld_patches;
	int err;
	
	CardParamPatchInfo **tmppi = NULL;
	*pi = NULL;
	
	if ((err = liblo10k1_get_patches_info(prv->conn(), &ld_patches, cnt)) < 0)
		return err;
		
	tmppi = new CardParamPatchInfo *[*cnt];
	
	for (int i = 0; i < *cnt; i++)
	{
		tmppi[i] = new CardParamPatchInfo();
		tmppi[i]->patch_num = ld_patches[i].patch_num;
		tmppi[i]->id = ld_patches[i].id;
		tmppi[i]->patch_name = ld_patches[i].patch_name;
	}
	
	free(ld_patches);
	*pi = tmppi;
	return 0;
}

int CardParam::getDspConfig(LD10k1DspFile **dc)
{
	liblo10k1_file_dsp_setup_t *ds = NULL;
	int err;
	
	if ((err = liblo10k1lf_get_dsp_config(prv->conn(), &ds)) < 0)
		return err;
	
	LD10k1DspFile *ldc = new LD10k1DspFile();
	ldc->setLD10k1DspFormat(ds);
	*dc = ldc;
	return 0;
}
	
int CardParam::putDspConfig(LD10k1DspFile *dc)
{
	return liblo10k1lf_put_dsp_config(prv->conn(), dc->getLD10k1DspFormat());
}
