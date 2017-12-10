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
 
#ifndef STRUCTURE_PATCH_H
#define STRUCTURE_PATCH_H

#include "structure.h"
#include "custom_colors.h"

class StrPatch : public RSItemBaseWithType
{
	QString patchName;
	int patchNum;
	int patchId;
	int patchOrder;
	
	RSItemIO *ios[2][32];
	int count[2];
	
	void getIOCount(int *in, int *out);
public:
	QString name(void);
	void setName(QString pn);
	StrPatch(int pn, int id, QString name);
	int num(void);
	int id(void);
	
	int order(void);
	void setOrder(int o);
	
	void getIORect(bool out, int idx, QRect &r);
	
	virtual int getMaxIOIdx(bool out);
	virtual RSItemIO *getIO(bool out, int idx);
	virtual void setIO(bool out, int idx, RSItemIO *io);
	virtual ~StrPatch();
	virtual void calcSize();
	
	virtual void draw(DrawingParams *dp);
	
	virtual bool containsPoint(int xp, int yp);
	virtual RSItemIO *getIOAtPoint(int xp, int yp);
	virtual bool containsHandlePoint(int xp, int yp);	
	virtual RSItemHandle getHandlePoint(int xp, int yp);
	
	virtual void getIOPoint(RSItemIO *item, int *xp, int *yp);
	
	virtual void updateContents(QScrollView *sv, float zoom);
};

#endif // STRUCTURE_PATCH_H
