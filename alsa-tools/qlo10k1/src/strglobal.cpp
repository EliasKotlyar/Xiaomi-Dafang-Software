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
#include "strglobal.h"
#include "strparam.h"
#include "structure.h"
#include "structure_io.h"
#include "structure_link.h"
#include "structure_patch.h"

#include "ld10k1file.h"

StrGlobal::StrGlobal(CardParam *card)
{
	Card = card;
	Card->Structure = this;
		
	// registers
	Inputs.setAutoDelete(TRUE); // the list owns the objects
	Outputs.setAutoDelete(TRUE);
	FXs.setAutoDelete(TRUE);
	Links.setAutoDelete(TRUE);

	// patches
	Patches.setAutoDelete(TRUE); // the list owns the objects
}

StrGlobal::~StrGlobal(void)
{
	Card = NULL;
}

void StrGlobal::clear(void)
{
	Links.clear();
	Inputs.clear();
	Outputs.clear();
	FXs.clear();
	Patches.clear();
	UsedItems.clear();
}

void StrGlobal::clearFlags(void)
{
	StrFX *fx = NULL;
	for(fx = FXs.first(); fx; fx = FXs.next())
        {
		fx->setFlagUsed(false);
		fx->setFlagNew(false);
		fx->setFlagChanged(false);
	}
	
	StrInput *in = NULL;
	for(in = Inputs.first(); in; in = Inputs.next())
        {
		in->setFlagUsed(false);
		in->setFlagNew(false);
		in->setFlagChanged(false);
	}
	
	StrOutput *out = NULL;
	for(out = Outputs.first(); out; out = Outputs.next())
        {
		out->setFlagUsed(false);
		out->setFlagNew(false);
		out->setFlagChanged(false);
	}
	
	StrPatch *p = NULL;
	for(p = Patches.first(); p; p = Patches.next())
        {
		p->setFlagUsed(false);
		p->setFlagNew(false);
		p->setFlagChanged(false);
	}
	
	StrLink *l = NULL;
	for(l = Links.first(); l; l = Links.next())
        {
		l->setFlagUsed(false);
		l->setFlagNew(false);
		l->setFlagChanged(false);
	}
}

int StrGlobal::loadFromLD(void)
{
	clearFlags();

	// load registers
	int cnt;
	int err = 0;
	int i;
	
	QString ioname;
	
	// FXs
	if ((err = Card->getFXCount(&cnt)) < 0)
		return err;
	
	for (i = 0; i < cnt; i++)
	{
		if ((err = Card->getFX(i, ioname)) < 0)
			return err;
		
		StrFX *fx = findFXByNum(i);
		if (fx)
		{
			if (fx->name() != ioname)
			{
				fx->setName(ioname);
				fx->setFlagChanged(true);
			}
			// Don't make unconnected IO/FX disappear.
			fx->setFlagUsed(true);
		}
		else
		{
			fx = new StrFX(i, ioname);
			fx->setFlagNew(true);
			FXs.append(fx);
		}
	}

	if ((err = Card->getInputCount(&cnt)) < 0)
		return err;
	
	for (i = 0; i < cnt; i++)
	{
		if ((err = Card->getInput(i, ioname)) < 0)
			return err;
		
		StrInput *in = findInputByNum(i);
		if (in)
		{
			if (in->name() != ioname)
			{
				in->setName(ioname);
				in->setFlagChanged(true);
			}
			in->setFlagUsed(true);
		}
		else
		{
			in = new StrInput(i, ioname);
			in->setFlagNew(true);
			Inputs.append(in);
		}
	}

	if ((err = Card->getOutputCount(&cnt)) < 0)
		return err;
	
	for (i = 0; i < cnt; i++)
	{
		if ((err = Card->getOutput(i, ioname)) < 0)
			return err;
		
		StrOutput *out = findOutputByNum(i);
		if (out)
		{
			if (out->name() != ioname)
			{
				out->setName(ioname);
				out->setFlagChanged(true);
			}
			out->setFlagUsed(true);
		}
		else
		{
			out = new StrOutput(i, ioname);
			out->setFlagNew(true);
			Outputs.append(out);
		}
	}

	// load patches and patch registers
	CardParamPatchInfo **pi;
	
	
	if ((err = Card->getPatchesInfo(&pi, &cnt)) < 0)
		return err;
	// copy registers to internal structure
	for (i = 0; i < cnt; i++)
	{
		StrPatch *patch = NULL;
		
		if ((err = actualizePatch(pi[i]->id, pi[i]->patch_num, pi[i]->patch_name, &patch)) < 0)
		{
			for (i = 0; i < cnt; i++)
				delete pi[i];
			delete pi;
			return err;
		}
		patch->setOrder(i);
	}
	
	for (i = 0; i < cnt; i++)
		delete pi[i];
	delete pi;
	
	int *ld_tmplids = NULL;
	if ((err = Card->getPointsInfo(&ld_tmplids, &cnt)) < 0)
		return err;
	
	for (i = 0; i < cnt; i++)
	{
		StrLink *link = NULL;
		
		if ((err = actualizeLink(ld_tmplids[i], &link)) < 0)
		{
			free(ld_tmplids);
			return err;
		}
	}

	free(ld_tmplids);
	
	// copy used items
	StrPatch *patch;
	
	UsedItems.clear();
		
	for(patch = Patches.first(); patch; patch = Patches.next() )
	{
		if (patch->flagUsed())
			UsedItems.append(patch);
	}
	
	// remove unused
	for(patch = Patches.last(); patch; patch = Patches.prev() )
	{
		if (!patch->flagUsed())
			Patches.remove(patch);
	}
	
	QPtrListIterator <StrInput> itin(Inputs);
	StrInput *in;
	while ((in = itin.current()) != 0 ) {
		++itin;
		if (in->flagUsed())
			UsedItems.append(in);
		else
			Inputs.remove(in);
	}
	
	QPtrListIterator <StrOutput> itout(Outputs);
	StrOutput *out;
	while ((out = itout.current()) != 0 ) {
		++itout;
		if (out->flagUsed())
			UsedItems.append(out);
		else
			Outputs.remove(out);
	}
	
	QPtrListIterator <StrFX> itfx(FXs);
	StrFX *fx;
	while ((fx = itfx.current()) != 0 ) {
		++itfx;
		if (fx->flagUsed())
			UsedItems.append(fx);
		else
			FXs.remove(fx);
	}
	
	QPtrListIterator <StrLink> itlnk(Links);
	StrLink *lnk;
	while ((lnk = itlnk.current()) != 0 ) {
		++itlnk;
		if (lnk->flagUsed())
			UsedItems.append(lnk);
		else
			Links.remove(lnk);
	}
	
	updatePatchesOrder();
	
	AutoArange(&UsedItems);
	
	QPtrListIterator <StrLink> itlnk1(Links);
	while ((lnk = itlnk1.current()) != 0 ) {
		++itlnk1;
		lnk->calcSize();
	}
	return 0;
}

int StrGlobal::getPio(int pnum, bool out, int idx, QString &name)
{
	int err;
	
	if (out)
		err = Card->getPOutput(pnum, idx, name);
	else
		err = Card->getPInput(pnum, idx, name);
	return err;
}

int StrGlobal::actualizePatch(int pid, int pnum, QString pname, StrPatch **out)
{
	QString ioname;
	int z, j;
	int err;
		
	int cnt1[2];
	
	if ((err = Card->getPInputCount(pnum, &(cnt1[0]))) < 0)
		return err;
	if ((err = Card->getPOutputCount(pnum, &(cnt1[1]))) < 0)
		return err;
		
	StrPatch *patch = findPatchById(pid);
		
	if (patch)
	{
		// check changes
		if (patch->name() != pname)
		{
			patch->setName(pname);
			patch->setFlagChanged(true);
		}
		
		for (z = 0; z < 2; z++)
		{
			for (j = 0; j < cnt1[z]; j++)
			{
				if ((err = getPio(pnum, z, j, ioname)) < 0)
					return err;
				
				RSItemIO *io = patch->getIO(z, j);
				if (io->getDesc() != ioname)
				{
					io->setDesc(ioname);
					patch->setFlagChanged(true);
				}
			}
		}
	}
	else
	{
		patch = new StrPatch(pnum, pid, pname);
		Patches.append(patch);

		for (z = 0; z < 2; z++)
		{
			for (j = 0; j < cnt1[z]; j++)
			{
				if ((err = getPio(pnum, z, j, ioname)) < 0)
					return err;
					
				RSItemIO *io = new RSItemIO(patch, z, j, ioname);
				patch->setIO(z, j, io);
			}
		}
		
		patch->setFlagNew(true);
	}
	
	patch->setFlagUsed(true);
	
	*out = patch;
	return 0;
}

StrPatch *StrGlobal::findPatchByNum(int num)
{
	StrPatch *patch;
		
	for(patch = Patches.first(); patch; patch = Patches.next() )
	{
		if (patch->num() == num)
			return patch;
	}
	
	return NULL;
}

StrPatch *StrGlobal::findPatchById(int id)
{
	StrPatch *patch;
		
	for(patch = Patches.first(); patch; patch = Patches.next() )
	{
		if (patch->id() == id)
			return patch;
	}
	
	return NULL;
}

StrFX *StrGlobal::findFXByNum(int num)
{
	StrFX *fx;
		
	for(fx = FXs.first(); fx; fx = FXs.next() )
	{
		if (fx->num() == num)
			return fx;
	}
	
	return NULL;
}

StrInput *StrGlobal::findInputByNum(int num)
{
	StrInput *in;
		
	for(in = Inputs.first(); in; in = Inputs.next() )
	{
		if (in->num() == num)
			return in;
	}
	
	return NULL;
}

StrOutput *StrGlobal::findOutputByNum(int num)
{
	StrOutput *out;
		
	for(out = Outputs.first(); out; out = Outputs.next() )
	{
		if (out->num() == num)
			return out;
	}
	
	return NULL;
}

StrLink *StrGlobal::findLinkById(int id)
{
	StrLink *l;
		
	for(l = Links.first(); l; l = Links.next() )
	{
		if (l->id() == id)
			return l;
	}
	
	return NULL;
}

int StrGlobal::conAdd(bool multi, bool simple, RSItemIO *from_io, RSItemIO *to_io, int *id)
{
	int ftype = 0;
	int ttype = 0;
	int type = 0;
	int fp = 0;
	int tp = 0;
	int p = 0;
	int fio = 0;
	int tio = 0;
	int io = 0;
	
	RSItemIO *tmp;
	
	RSItemBaseWithType *owner;

	for (int i = 0; i < 2; i++)
	{
		if (!i)
			tmp = from_io;
		else
			tmp = to_io;
			
		owner = (RSItemBaseWithType *)tmp->getOwner();
		
		p = -1;
		io = -1;
				
		switch (owner->type())
		{
			case RSItemBaseWithType::In:
				type = CON_IO_IN;
				io = ((StrInput *)owner)->num();
				break;
			case RSItemBaseWithType::Out:
				type = CON_IO_OUT;
				io = ((StrOutput *)owner)->num();
				break;
			case RSItemBaseWithType::FX:
				type = CON_IO_FX;
				io = ((StrFX *)owner)->num();
				break;
			case RSItemBaseWithType::Patch:
				if (!tmp->isOutput())
					type = CON_IO_PIN;
				else
					type = CON_IO_POUT;
				io = tmp->getIdx();
					
				p = ((StrPatch *)owner)->num();
				break;
			default:
				break;
		}
		
		if (!i)
		{
			ftype = type;
			fio = io;
			fp = p;
		}
		else
		{
			ttype = type;
			tio = io;
			tp = p;
		}
	}

	return Card->conAdd(multi, simple, ftype, fp, fio, ttype, tp, tio, id);
	// FIXME - uprava strglobal - urobene inde
}

int StrGlobal::conDel(RSItemIO *from_io, int *id)
{
	int io, p;
	int type = 0;
	
	RSItemBaseWithType *owner;

	owner = (RSItemBaseWithType *)from_io->getOwner();
		
	p = -1;
	io = -1;
				
	switch (owner->type())
	{
		// only patch valid
		case RSItemBaseWithType::Patch:
			if (!from_io->isOutput())
				type = CON_IO_PIN;
			else
				type = CON_IO_POUT;
			io = from_io->getIdx();
				
			p = ((StrPatch *)owner)->num();
			break;
		default:
			break;
	}
		
	return Card->conDel(type, p, io, id);
}

int StrGlobal::load(LD10k1File *ld10k1file, StrPatch *before, StrPatch **loaded)
{
	int bef = -1;
	int loade, loade_id;
	int retval;
	
	if (before)
		bef = before->num();
	if ((retval = Card->load(ld10k1file, bef, &loade, &loade_id)) < 0)
		return retval;
		
	StrPatch *p = NULL;
	
	if ((retval = actualizePatch(loade_id, loade, ld10k1file->getPatchName(), &p)) < 0)
		return retval;
		
	UsedItems.append(p);
	updatePatchesOrder();
	AutoArange(&UsedItems);
	*loaded = p;
	return retval;
}

int StrGlobal::unload(StrPatch *p)
{
	int retval;
	if ((retval = Card->unload(p->num())) < 0)
		return retval;
		
	// FIXME - odmaz aj prepojenia
	UsedItems.remove(p);
	Patches.remove(p);
	updatePatchesOrder();
	return retval;
}

int StrGlobal::clearDSP(void)
{
	int retval;
		
	if ((retval = Card->clearDSP()) < 0)
		return retval;
		
	clear();
	return 0;
}

void StrGlobal::destroyLink(StrLink *l)
{
	UsedItems.remove(l);
	l->disconnectAll();
	
	// delete not needed - autodelete
	Links.remove(l);
}

int StrGlobal::disconnectFromLink(RSItemIO *io)
{
	StrLink *link = io->getConnectedTo();
	if (link)
	{
		int idx = link->findRoute(io);
		int conn_id;
		int err;
		if((err = conDel(io, &conn_id)) < 0)
			return err;
		link->setRoutePoint(idx, NULL);
		if (conn_id < 0 || !link->isValid())
		{
			destroyLink(link);
			return 0;
		}
		link->calcSize();
	}
	return 0;
}

int StrGlobal::actualizeLink(int id, StrLink **out)
{
	int err;
	CardParamPointInfo point;
	
	err = Card->getPointInfo(id, &point);	
	if (err == LD10K1_ERR_UNKNOWN_POINT)
	{
		// FIXME - probably not exists - remove
		StrLink *delLink = findLinkById(id);
		if (delLink)
			destroyLink(delLink);
		
		*out = NULL;
		return 0;
	}
	if (err < 0)
		return err;
	
	RSItemIO *firstItem = NULL;
	StrLink::LinkType newType = StrLink::LinkFX;
	switch(point.type)
	{
		case CON_IO_FX:
			newType = StrLink::LinkFX;
			{
				StrFX *ft = findFXByNum(point.io_idx);
				if (ft)
					firstItem = ft->getIO(true, 0);
			}
			break;
		case CON_IO_IN:
			newType = StrLink::LinkIn;
			{
				StrInput *in = findInputByNum(point.io_idx);
				if (in)
					firstItem = in->getIO(true, 0);
			}
			break;
		case CON_IO_OUT:
			newType = StrLink::LinkOut;
			{
				StrOutput *out = findOutputByNum(point.io_idx);
				if (out)
					firstItem = out->getIO(false, 0);
			}
			break;
		case CON_IO_NORMAL:
			newType = StrLink::LinkNormal;
			break;
	}
	
	StrLink *link = findLinkById(id);
	if (link)
	{
		bool actualized[POINTINFO_MAX_CONN_PER_POINT];
		RSItemIO *newIOs[POINTINFO_MAX_CONN_PER_POINT];
		unsigned int j;
		unsigned int niosc = 0;
		
		for (j = 0; j < POINTINFO_MAX_CONN_PER_POINT; j++)
			actualized[j] = false;
		
		int r = 0;
		if (firstItem)
		{
			actualized[r] = true;
			((RSItemBaseWithType *)firstItem->getOwner())->setFlagUsed(true);
			r++;
		}
		
		// actualize current
		for (j = 0; j < point.conn_count; j++)
		{
			RSItemIO *io = findPatchIO(point.io_type[j], point.patch[j], point.io[j]);
			((RSItemBaseWithType *)io->getOwner())->setFlagUsed(true);
			
			// find io
			int rf = link->findRoute(io);
			if (rf >= 0)
			{
				actualized[rf] = true;
				link->setRoutePoint(rf, io);	
			}
			else
			{
				newIOs[niosc++] = io;
			}		
		}
		
		// delete old
		for (j = 0; j < POINTINFO_MAX_CONN_PER_POINT; j++)
		{
			if (!actualized[j])
			{
				link->setRoutePoint(j, NULL);
				link->clearRoutesPoints(j);
			}
		}
		
		// add new
		int l = 0;
		for (j = 0; j < niosc; j++)
		{
			for (; l < POINTINFO_MAX_CONN_PER_POINT; l++)
			{
				if (!link->getRoutePoint(l))
				{
					link->setRoutePoint(l, newIOs[j]);
					break;
				}
			}
		}
		
		link->setFlagChanged(true);
	}
	else
	{
		link = new StrLink(id, newType);
		int r = 0;
		if (firstItem)
		{
			link->setRoutePoint(r++, firstItem);
			((RSItemBaseWithType *)firstItem->getOwner())->setFlagUsed(true);
		}
			
		unsigned int j = 0;
		for (j = 0; j < point.conn_count; j++)
		{
			RSItemIO *io = findPatchIO(point.io_type[j], point.patch[j], point.io[j]);
			((RSItemBaseWithType *)io->getOwner())->setFlagUsed(true);
			link->setRoutePoint(r++, io);
		}
		link->setFlagNew(true);
		Links.append(link);
		UsedItems.append(link);
	}
	link->setFlagUsed(true);
	
	*out = link;
	return 0;
}

void StrGlobal::updatePatchesOrder(void)
{
	CardParamPatchInfo **pi;
	int i, cnt;
	int err;
	
	if ((err = Card->getPatchesInfo(&pi, &cnt)) < 0)
		return;
		
	for (i = 0; i < cnt; i++)
	{
		StrPatch *p = findPatchById(pi[i]->id);
		if (p)
			p->setOrder(i);
	}
	
	for (i = 0; i < cnt; i++)
		delete pi[i];
	delete pi;
}

RSItemIO *StrGlobal::findPatchIO(bool out, int pid, int ionum)
{
	if (pid < 0 || ionum < 0)
		return NULL;
	StrPatch *p = findPatchById(pid);
	if (!p)
		return NULL;
	
	return p->getIO(out, ionum);
}

int StrGlobal::deleteOneLink(StrLink *l)
{
	int err;
	
	for (int i = 0; i < l->getMaxRoute(); i++)
	{
		RSItemIO *io = l->getRoutePoint(i);
		if (io)
		{
			RSItemBaseWithType *owner = (RSItemBaseWithType *)io->getOwner();
			if (owner->type() == RSItemBaseWithType::Patch)
			{
				// only patch can be disconnected
				int conn_id;
				if((err = conDel(io, &conn_id)) < 0)
					return err;
				if (conn_id < 0)
				{
					destroyLink(l);
					return 0;
				}
					
			}
		}
	}
	return 0;
}

int StrGlobal::deleteOneFX(StrFX *fx)
{
	int err;
	
	RSItemIO *io = fx->getIO(true, 0);
	if (io)
	{
		StrLink *l = io->getConnectedTo();
		if (l)
		{
			if ((err = deleteOneLink(l)) < 0)
				return err;
		}
	}
	
	UsedItems.remove(fx);
	
	// delete not needed - autodelete
	FXs.remove(fx);
	
	return 0;
}

int StrGlobal::deleteOneIn(StrInput *in)
{
	int err;
	
	RSItemIO *io = in->getIO(true, 0);
	if (io)
	{
		StrLink *l = io->getConnectedTo();
		if (l)
		{
			if ((err = deleteOneLink(l)) < 0)
				return err;
		}
	}
	
	UsedItems.remove(in);
	
	// delete not needed - autodelete
	Inputs.remove(in);
	
	return 0;
}

int StrGlobal::deleteOneOut(StrOutput *out)
{
	int err;
	
	RSItemIO *io = out->getIO(false, 0);
	if (io)
	{
		StrLink *l = io->getConnectedTo();
		if (l)
		{
			if ((err = deleteOneLink(l)) < 0)
				return err;
		}
	}
	
	UsedItems.remove(out);
	
	// delete not needed - autodelete
	Outputs.remove(out);
	
	return 0;
}

int StrGlobal::deleteOnePatch(StrPatch *p)
{
	int err;
	int z;
	int j;
	
	// store all conn ids
	
	QValueList <int> actIds;
					
	actIds.clear();
	
	for (z = 0; z < 2; z++)
	{
		for (j = 0; j < p->getMaxIOIdx(z); j++)
		{
			RSItemIO *io = p->getIO(z, j);
			if (io)
			{
				StrLink *l = io->getConnectedTo();
				if (l && actIds.findIndex(l->id()) < 0)
					actIds.append(l->id());
			}
		}
	}
	
	if ((err = unload(p)) < 0)
		return err;
		
	// actualize links
	for (unsigned int i = 0; i < actIds.count(); i++)
	{
		StrLink *link = NULL;
		int aid = actIds[i];
		if ((err = actualizeLink(aid, &link)) < 0)
			return err;
		
		if (link)
		{
			if (!link->isValid())
				destroyLink(link);
			else
				//wasn't error
				link->calcSize();
		}
	}
	
	return 0;
}

int StrGlobal::deleteOneItem(RSItemBaseWithType *item)
{
	switch (item->type())
	{
		case RSItemBaseWithType::In:
			return deleteOneIn((StrInput *)item);
		case RSItemBaseWithType::Out:
			return deleteOneOut((StrOutput *)item);
		case RSItemBaseWithType::FX:
			return deleteOneFX((StrFX *)item);
		case RSItemBaseWithType::Link:
			return deleteOneLink((StrLink *)item);
		case RSItemBaseWithType::Patch:
			return deleteOnePatch((StrPatch *)item);
		default:
			return 0;
	}
}

int StrGlobal::deleteAllSelected()
{
	int err;
	// through all selected
	RSItemBaseWithType *item;
	
	// first connections
	QPtrListIterator <RSItemBaseWithType> it1(UsedItems);
	while ((item = it1.current()) != 0 ) 
	{
		++it1;
		if (item->flagSelected() && item->type() == RSItemBaseWithType::Link)
		{
			if ((err = deleteOneLink((StrLink *)item)) < 0)
				return err;
		}
	}
	
	// everything others
	QPtrListIterator <RSItemBaseWithType> it2(UsedItems);
	while ((item = it2.current()) != 0 ) 
	{
		++it2;
		if (item->flagSelected())
		{
			if ((err = deleteOneItem(item)) < 0)
				return err;
		}
	}
	
	return 0;
}

QString StrGlobal::errorStr(int err)
{
	return Card->errorStr(err);
}

int StrGlobal::get(int patch_num, LD10k1File **dc)
{
	return Card->get(patch_num, dc);
}


int StrGlobal::getDspConfig(LD10k1DspFile **dc)
{
	return Card->getDspConfig(dc);
}

int StrGlobal::putDspConfig(LD10k1DspFile *dc)
{
	return Card->putDspConfig(dc);
}

StrCardGlobal::StrCardGlobal()
{
	// cards
	Cards.setAutoDelete( TRUE ); // the list owns the objects
}
