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
 
#ifndef STRPARAM_H
#define STRPARAM_H

#include <qstring.h>

class StrGlobal;
class LD10k1File;
class LD10k1DspFile;
class CardParamPrivate;

#define POINTINFO_MAX_CONN_PER_POINT 15
class CardParamPointInfo
{
public:
	int id;
	int type;
	int io_idx;
	int simple;
	int multi;
	unsigned int conn_count;
	int io_type[POINTINFO_MAX_CONN_PER_POINT];
	int patch[POINTINFO_MAX_CONN_PER_POINT];
	int io[POINTINFO_MAX_CONN_PER_POINT];
};

class CardParamPatchInfo
{
public:
	int patch_num;
	int id;
	QString patch_name;
};

class CardParam
{
	CardParamPrivate *prv;
public:
	QString CardName;
	QString CardSocket;
	
	StrGlobal *Structure;

	CardParam(void);
	CardParam(CardParam *from);
	~CardParam(void);
	StrGlobal *getStructure();

	int connect(void);
	int disconnect(void);
	int isConnected(void);
	int testConnection(void);
	int clearDSP(void);

	int load(LD10k1File *ld10k1file, int before, int *loaded, int *loaded_id);
	int unload(int num);
	int get(int patch_num, LD10k1File **ld10k1file);
	int getPointsInfo(int **out, int *count);
	int getPointInfo(int point_id, CardParamPointInfo *out);

	int conAdd(bool multi, bool simple, int from_type, int from_patch, int from_io, int to_type, int to_patch, int to_io, int *id);
	int conDel(int from_type, int from_patch, int from_io, int *id);
	QString errorStr(int err);
	
	int getFXCount(int *cnt);
	int getInputCount(int *cnt);
	int getOutputCount(int *cnt);
	
	int getFX(int i, QString &name);
	int getInput(int i, QString &name);
	int getOutput(int i, QString &name);
	
	int getPInputCount(int pnum, int *cnt);
	int getPOutputCount(int pnum, int *cnt);
	int getPInput(int pnum, int i, QString &name);
	int getPOutput(int pnum, int i, QString &name);
	
	int getPatchesInfo(CardParamPatchInfo ***pi, int *cnt);
	
	int getDspConfig(LD10k1DspFile **dc);
	int putDspConfig(LD10k1DspFile *dc);
};

#endif // STRPARAM_H
