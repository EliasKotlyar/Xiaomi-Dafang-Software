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
 
#ifndef STRGLOBAL_H
#define STRGLOBAL_H

#include <qstring.h>
#include <qptrlist.h>

class StrGlobal;
class StrInput;
class StrOutput;
class StrFX;
class StrLink;
class StrPatch;
class RSItemBaseWithType;
class RSItemIO;
class CardParam;
class LD10k1File;
class LD10k1DspFile;

class StrGlobal
{
public:
	CardParam *Card;

	QPtrList <StrInput> Inputs;
	QPtrList <StrOutput> Outputs;
	QPtrList <StrFX> FXs;
	QPtrList <StrLink> Links;

	QPtrList <StrPatch> Patches;

	StrGlobal(CardParam *card);
	~StrGlobal(void);

	void clear(void);
	void clearFlags(void);
	int loadFromLD(void);
	StrPatch *findPatchByNum(int num);
	StrPatch *findPatchById(int id);
	StrFX *findFXByNum(int num);
	StrInput *findInputByNum(int num);
	StrOutput *findOutputByNum(int num);
	StrLink *findLinkById(int id);
	
	int load(LD10k1File *ld10k1file, StrPatch *before, StrPatch **loaded);
	int get(int patch_num, LD10k1File **dc);
	int unload(StrPatch *p);
	int conAdd(bool multi, bool simple, RSItemIO *from_io, RSItemIO *to_io, int *id);
	int conDel(RSItemIO *from_io, int *id);
	QString errorStr(int err);
	
	QPtrList <RSItemBaseWithType> UsedItems;
	
	int getPio(int pnum, bool out, int idx, QString &name);
	
	int actualizeLink(int id, StrLink **out);
	int actualizePatch(int pid, int pnum, QString pname, StrPatch **out);
	
	void destroyLink(StrLink *l);
	int disconnectFromLink(RSItemIO *io);
	
	RSItemIO *findPatchIO(bool out, int pid, int ionum);
	int clearDSP(void);
	
	static void AutoArange(QPtrList <RSItemBaseWithType> *items);
	
	void updatePatchesOrder(void);
	
	int deleteOneLink(StrLink *l);
	int deleteOneFX(StrFX *fx);
	int deleteOneIn(StrInput *in);
	int deleteOneOut(StrOutput *out);
	int deleteOnePatch(StrPatch *p);
	int deleteOneItem(RSItemBaseWithType *item);
	int deleteAllSelected();
	
	int getDspConfig(LD10k1DspFile **dc);
	int putDspConfig(LD10k1DspFile *dc);
};

class StrCardGlobal
{
public:
	QPtrList <CardParam> Cards;
	
	QString RepDirSystem;
	QString RepDirUser;
	
	StrCardGlobal();
};

#endif // STRGLOBAL_H
