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
 
#include <qpainter.h>

#include "structure_patch.h"
#include "structure_link.h"

QString StrPatch::name(void)
{
	return patchName;
}

void StrPatch::setName(QString pn)
{
	patchName = pn;
}

StrPatch::StrPatch(int pn, int id, QString name) :
	RSItemBaseWithType(RSItemBaseWithType::Patch)
{
	patchName = name;
	patchNum = pn;
	patchId = id;
	patchOrder = 0;
	
	count[0] = 0;
	count[1] = 0;
	
	int i, j;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 32; j++)
			ios[i][j] = NULL;
}

int StrPatch::num(void)
{
	return patchNum;
}

int StrPatch::id(void)
{
	return patchId;
}

int StrPatch::order(void)
{
	return patchOrder;
}

void StrPatch::setOrder(int o)
{
	patchOrder = o;
}

int StrPatch::getMaxIOIdx(bool out)
{
	return 32;
}

RSItemIO *StrPatch::getIO(bool out, int idx)
{
	if (idx >= 32)
		return NULL;
	else
		return ios[out][idx];
}

void StrPatch::setIO(bool out, int idx, RSItemIO *io)
{
	if (idx >= 32)
		return;
	else
	{
		if (ios[out][idx])
			delete ios[out][idx];
		ios[out][idx] = io;
	}
	getIOCount(&(count[0]), &(count[1]));	
}

StrPatch::~StrPatch()
{
	int i, j;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 32; j++)
			if (ios[i][j])
			{
				RSItemIO *tmp = ios[i][j];
				delete ios[i][j];
				ios[i][j] = NULL;
				tmp->connectTo(NULL);
			}
}

void StrPatch::getIOCount(int *in, int *out)
{
	// get count of inputs and outputs
	int i, j;
	int count[2];
	for (i = 0; i < 2; i++)
	{
		count[i] = 0;
		for (j = 0; j < 32; j++)
			if (ios[i][j])
				count[i]++;
	}
	*in = count[0];
	*out = count[1];
}

#define PATCH_IO_WIDTH 8
#define PATCH_IO_HEIGHT 8

#define PATCH_WIDTH 70

#define PATCH_SELECT_WH 7

void StrPatch::calcSize()
{
	// get count of inputs and outputs
	int max_count;
	if (count[0] > count[1])
		max_count = count[0];
	else
		max_count = count[1];
	
	setSize(PATCH_WIDTH, (max_count * 2 + 2) * PATCH_IO_HEIGHT + PATCH_SELECT_WH * 2);
	
	RSItemBase::calcSize();
}

void StrPatch::draw(DrawingParams *dp)
{
	int xp, yp, wp, hp;
	xp = (int)(x() * dp->zoomKoef);
	yp = (int)(y() * dp->zoomKoef);
	wp = (int)(width() * dp->zoomKoef);
	hp = (int)(height() * dp->zoomKoef);
	
	dp->painter->setPen(QPen(BOX_COLOR, 1));
	dp->painter->drawRect(xp, yp, wp, hp);
	
	if (flagSelected())
	{
		// top left
		dp->painter->fillRect(
			xp,
			yp,
			(int)(PATCH_SELECT_WH * dp->zoomKoef),
			(int)(PATCH_SELECT_WH * dp->zoomKoef),
			QBrush(BOX_SELECTED_COLOR));
		// top right
		dp->painter->fillRect(
			xp + wp - (int)(PATCH_SELECT_WH * dp->zoomKoef),
			yp,
			(int)(PATCH_SELECT_WH * dp->zoomKoef),
			(int)(PATCH_SELECT_WH * dp->zoomKoef),
			QBrush(BOX_SELECTED_COLOR));
		// bottom left
		dp->painter->fillRect(
			xp,
			yp + hp - (int)(PATCH_SELECT_WH * dp->zoomKoef),
			(int)(PATCH_SELECT_WH * dp->zoomKoef),
			(int)(PATCH_SELECT_WH * dp->zoomKoef),
			QBrush(BOX_SELECTED_COLOR));
			
		// bottom right
		dp->painter->fillRect(
			xp + wp - (int)(PATCH_SELECT_WH * dp->zoomKoef),
			yp + hp - (int)(PATCH_SELECT_WH * dp->zoomKoef),
			(int)(PATCH_SELECT_WH * dp->zoomKoef),
			(int)(PATCH_SELECT_WH * dp->zoomKoef),
			QBrush(BOX_SELECTED_COLOR));
	}
	
	int i;
	
	float tmpy = PATCH_IO_HEIGHT + PATCH_SELECT_WH;
	
	float stepy = height() - 2 * (PATCH_IO_HEIGHT + PATCH_SELECT_WH) - PATCH_IO_HEIGHT * count[0];
	
	if (count[0] > 1)
		stepy = stepy / (count[0] - 1);
	
	for (i = 0; i < count[0]; i++)
	{
		dp->painter->setPen(QPen(BOX_COLOR, 1));
		dp->painter->drawRect(
			(int)(x() * dp->zoomKoef),
			(int)((y() + tmpy ) * dp->zoomKoef),
			(int)(PATCH_IO_WIDTH * dp->zoomKoef),
			(int)(PATCH_IO_HEIGHT * dp->zoomKoef)
			);
		tmpy += stepy + PATCH_IO_HEIGHT;
	}
	
	tmpy = PATCH_IO_HEIGHT + PATCH_SELECT_WH;
	
	stepy = height() - 2 * (PATCH_IO_HEIGHT + PATCH_SELECT_WH) - PATCH_IO_HEIGHT * count[1];
		
	if (count[1] > 1)
		stepy = stepy / (count[1] - 1);
	
	
	for (i = 0; i < count[1]; i++)
	{
		dp->painter->setPen(QPen(BOX_COLOR, 1));
		dp->painter->drawRect(
			(int)((x() + width() - PATCH_IO_WIDTH) * dp->zoomKoef),
			(int)((y() + tmpy) * dp->zoomKoef),
			(int)(PATCH_IO_WIDTH * dp->zoomKoef),
			(int)(PATCH_IO_HEIGHT * dp->zoomKoef));
		tmpy += stepy + PATCH_IO_HEIGHT;
	}
	
	QString pname = QString("%1\n%2").arg(num()).arg(name());
	
	dp->painter->setPen(QPen(TEXT_COLOR, 1));
	dp->painter->drawText(
		(int)((x() + 2 * PATCH_IO_WIDTH) * dp->zoomKoef),
		(int)((y() + PATCH_IO_HEIGHT) * dp->zoomKoef),
		(int)((width() - 4 * PATCH_IO_WIDTH) * dp->zoomKoef),
		(int)((height() - 2 * PATCH_IO_HEIGHT) * dp->zoomKoef),
		Qt::AlignLeft | Qt::WordBreak,
		pname);
}

bool StrPatch::containsPoint(int xp, int yp)
{
	return QRect(x(), y(), width(), height()).contains(xp, yp);
}

RSItemIO *StrPatch::getIOAtPoint(int xp, int yp)
{
	QRect r;
	
	for (int j = 0; j < 2; j++)
		for (int i = 0; i < count[j]; i++)
		{
			if (ios[j][i])
			{
				getIORect(j, i, r);
				if (r.contains(xp, yp))
					return ios[j][i];
			}
		}
	return NULL;
}

void StrPatch::getIORect(bool out, int idx, QRect &r)
{
	float tmpy = PATCH_IO_HEIGHT + PATCH_SELECT_WH;
	
	if (!out)
	{
		float stepy = height() - 2 * (PATCH_IO_HEIGHT + PATCH_SELECT_WH) - PATCH_IO_HEIGHT * count[0];
	
		if (count[0] > 1)
			stepy = stepy / (count[0] - 1);
		
		r.setRect(x(), (int)(y() + tmpy + idx * (stepy + PATCH_IO_HEIGHT)),
			PATCH_IO_WIDTH, PATCH_IO_HEIGHT);
	}
	else
	{
		float stepy = height() - 2 * (PATCH_IO_HEIGHT + PATCH_SELECT_WH) - PATCH_IO_HEIGHT * count[1];
			
		if (count[1] > 1)
			stepy = stepy / (count[1] - 1);
		
		r.setRect(x() + width() - PATCH_IO_WIDTH, (int)(y() + tmpy + idx * (stepy + PATCH_IO_HEIGHT)),
			PATCH_IO_WIDTH,	PATCH_IO_HEIGHT);
	}
}

bool StrPatch::containsHandlePoint(int xp, int yp)
{
	return getHandlePoint(xp, yp) != RSItemBase::None;
}

RSItemBase::RSItemHandle StrPatch::getHandlePoint(int xp, int yp)
{
	// top left
	if (QRect(x(), y(), PATCH_SELECT_WH, PATCH_SELECT_WH).contains(xp, yp))
		return RSItemBase::TopLeft;
	// top right
	if (QRect(x() + width() - PATCH_SELECT_WH, y(), PATCH_SELECT_WH, PATCH_SELECT_WH).contains(xp, yp))
		return RSItemBase::TopRight;
	// bottom left
	if (QRect(x(), y() + height() - PATCH_SELECT_WH, PATCH_SELECT_WH, PATCH_SELECT_WH).contains(xp, yp))
		return RSItemBase::BottomLeft;
	// bottom right
	if (QRect(x() + width() - PATCH_SELECT_WH, y() + height() - PATCH_SELECT_WH, PATCH_SELECT_WH, PATCH_SELECT_WH).contains(xp, yp))
		return RSItemBase::BottomRight;
	return RSItemBase::None;
}

void StrPatch::getIOPoint(RSItemIO *item, int *xp, int *yp)
{
	// for io it is in center
	if (item->isOutput())
	{
		float tmpy = PATCH_IO_HEIGHT + PATCH_SELECT_WH;
	
		float stepy = height() - 2 * (PATCH_IO_HEIGHT + PATCH_SELECT_WH) - PATCH_IO_HEIGHT * count[1];
		
		if (count[1] > 1)
			stepy = stepy / (count[1] - 1);
		
		*xp = x() + width() - PATCH_IO_WIDTH + PATCH_IO_WIDTH / 2;
		*yp = (int)(y() + tmpy + (stepy + PATCH_IO_HEIGHT) * item->getIdx() + PATCH_IO_HEIGHT / 2);
		return;
	}
	else
	{
		float tmpy = PATCH_IO_HEIGHT + PATCH_SELECT_WH;
	
		float stepy = height() - 2 * (PATCH_IO_HEIGHT + PATCH_SELECT_WH) - PATCH_IO_HEIGHT * count[0];
		
		if (count[0] > 1)
			stepy = stepy / (count[0] - 1);
		
		*xp = x() + PATCH_IO_WIDTH / 2;
		*yp = (int)(y() + tmpy + (stepy + PATCH_IO_HEIGHT)* item->getIdx() + PATCH_IO_HEIGHT / 2);
		return;
	}
}

void StrPatch::updateContents(QScrollView *sv, float zoom)
{
	int i;
	int j;
	
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 32; j++)
			if (ios[i][j])
			{
				StrLink *l = ios[i][j]->getConnectedTo();
				if (l)
					l->updateOneSegment(sv, l->findRoute(ios[i][j]), zoom);
			}
	}
		
	RSItemBaseWithType::updateContents(sv, zoom);
}
