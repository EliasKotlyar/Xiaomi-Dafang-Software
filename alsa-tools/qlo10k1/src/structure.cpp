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
 
#include "structure.h"
#include "structure_patch.h"
#include "structure_io.h"
#include "structure_link.h"
#include "ld10k1file.h"

RSItemIO::RSItemIO(RSItemBase *o, bool out, int i, QString d)
{
	output = out;
	idx = i;
	desc = d;
	owner = o;
	connectedTo = NULL;
}

RSItemIO::~RSItemIO()
{
	if (connectedTo)
	{
		int r = connectedTo->findRoute(this);
		if (r >= 0)
			connectedTo->setRoutePoint(r, NULL);
	}
}

bool RSItemIO::isOutput()
{
	return output;
}
	
void RSItemIO::setOutput(bool o)
{
	output = o;
}

int RSItemIO::getIdx()
{
	return idx;
}
	
void RSItemIO::setIdx(int i)
{
	idx = i;
}

QString RSItemIO::getDesc()
{
	return desc;
}
	
void RSItemIO::setDesc(QString d)
{
	desc = d;
}
	
RSItemBase *RSItemIO::getOwner()
{
	return owner;
}
	
void RSItemIO::connectTo(StrLink *to)
{
	if (connectedTo)
	{
		int r = connectedTo->findRoute(this);
		if (r >= 0)
			connectedTo->setRoutePoint(r, NULL);
	}
	connectedTo = to;
}
	
StrLink *RSItemIO::getConnectedTo()
{
	return connectedTo;
}




DrawingParams::DrawingParams(float zoom, QWidget *dt, QPaintEvent *pe, QPainter *p)
{
	zoomKoef = (float)zoom;
	drawTo = dt;
	paintEvent = pe;
	painter = p;
}
	
RSItemBase::RSItemBase()
{
	fNew = false;
	fUsed = false;
	fChanged = false;
	fSelected = false;
}
	
int RSItemBase::x()
{
	return position.x();
}

int RSItemBase::y()
{
	return position.y();
}

void RSItemBase::setPosition(int x, int y)
{
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	position.setX(x);
	position.setY(y);
}
	
int RSItemBase::width()
{
	return size.width();
}

int RSItemBase::height()
{
	return size.height();
}

int RSItemBase::minWidth()
{
	return minSize.width();
}

int RSItemBase::minHeight()
{
	return minSize.height();
}

void RSItemBase::setSize(int w, int h)
{
	if (w > minSize.width())
		size.setWidth(w);
	if (h > minSize.height())
		size.setHeight(h);
}

void RSItemBase::moveRel(int dx, int dy)
{
	setPosition(x() + dx, y() + dy);
}

void RSItemBase::moveHandleRel(int num, int dx, int dy)
{
}

void RSItemBase::updateContents(QScrollView *sv, float zoom)
{
	sv->updateContents((int)(x() * zoom),
		(int)(y() * zoom),
		(int)(width() * zoom),
		(int)(height() * zoom));
}

bool RSItemBase::containsPoint(int xp, int yp)
{
	return false;
}
	
RSItemIO *RSItemBase::getIOAtPoint(int xp, int yp)
{
	return NULL;
}


bool RSItemBase::containsHandlePoint(int xp, int yp)
{
	return false;
}

RSItemBase::RSItemHandle RSItemBase::getHandlePoint(int xp, int yp)
{
	return RSItemBase::None;
}

int RSItemBase::getHandlePointNum(int xp, int yp)
{
	return -1;
}

bool RSItemBase::flagUsed()
{
	return fUsed;
}

void RSItemBase::setFlagUsed(bool u)
{
	fUsed = u;
}

bool RSItemBase::flagNew()
{
	return fNew;
}

void RSItemBase::setFlagNew(bool n)
{
	fNew = n;
}

bool RSItemBase::flagChanged()
{
	return fNew;
}

void RSItemBase::setFlagChanged(bool c)
{
	fChanged = c;
}

bool RSItemBase::flagSelected()
{
	return fSelected;
}

void RSItemBase::setFlagSelected(bool s)
{
	fSelected = s;
}

QString RSItemBase::getDesc()
{
	return "";
}

void RSItemBase::draw(DrawingParams *dp)
{
}

void RSItemBase::calcSize()
{
	minSize = size;
}

	


RSItemBaseWithType::RSItemBaseWithType(ItemType type)
{
	iType = type;
}

RSItemBaseWithType::ItemType RSItemBaseWithType::type()
{
	return iType;
}

int RSItemBaseWithType::getMaxIOIdx(bool out)
{
	return 0;
}

RSItemIO *RSItemBaseWithType::getIO(bool out, int idx)
{
	return NULL;
}

void RSItemBaseWithType::getIOPoint(RSItemIO *item, int *xp, int *yp)
{
}

RSItemBaseWithType::~RSItemBaseWithType()
{
}
