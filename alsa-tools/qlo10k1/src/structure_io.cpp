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

#include "structure_io.h"
#include "structure_link.h"

#define IO_WIDTH 70
#define IO_HEIGHT 20

StrIOBase::StrIOBase(RSItemBaseWithType::ItemType it) : RSItemBaseWithType(it)
{
}

void StrIOBase::calcSize()
{
	setSize(IO_WIDTH, IO_HEIGHT);
	
	RSItemBase::calcSize();
}

#define IO_SELECT_WH 7
#define IO_INNER_SPACE_X 7
#define IO_INNER_SPACE_Y 2

void StrIOBase::draw(DrawingParams *dp)
{
	int xp, yp, wp, hp;
	xp = (int)(x() * dp->zoomKoef);
	yp = (int)(y() * dp->zoomKoef);
	wp = (int)(width() * dp->zoomKoef);
	hp = (int)(height() * dp->zoomKoef);
	
	dp->painter->setPen(QPen(IO_BOX_COLOR, 1));
	dp->painter->drawRoundRect(xp, yp, wp, hp, 20, 50);
	
	if (flagSelected())
	{
		// top left
		dp->painter->fillRect(
			xp,
			yp,
			(int)(IO_SELECT_WH * dp->zoomKoef),
			(int)(IO_SELECT_WH * dp->zoomKoef),
			QBrush(BOX_SELECTED_COLOR));
		// top right
		dp->painter->fillRect(
			(int)(xp + wp - IO_SELECT_WH * dp->zoomKoef),
			yp,
			(int)(IO_SELECT_WH * dp->zoomKoef),
			(int)(IO_SELECT_WH * dp->zoomKoef),
			QBrush(BOX_SELECTED_COLOR));
		// bottom left
		dp->painter->fillRect(
			xp,
			(int)(yp + hp - IO_SELECT_WH * dp->zoomKoef),
			(int)(IO_SELECT_WH * dp->zoomKoef),
			(int)(IO_SELECT_WH * dp->zoomKoef),
			QBrush(BOX_SELECTED_COLOR));
			
		// bottom right
		dp->painter->fillRect(
			(int)(xp + wp - IO_SELECT_WH * dp->zoomKoef),
			(int)(yp + hp - IO_SELECT_WH * dp->zoomKoef),
			(int)(IO_SELECT_WH * dp->zoomKoef),
			(int)(IO_SELECT_WH * dp->zoomKoef),
			QBrush(BOX_SELECTED_COLOR));
	}
	
	dp->painter->setPen(IO_TEXT_PEN);
	dp->painter->drawText(
		(int)((x() + 2 * IO_INNER_SPACE_X + (width() / 100) * 10) * dp->zoomKoef),
		(int)((y() + IO_INNER_SPACE_Y + (height() / 100) * 10) * dp->zoomKoef),
		(int)((width() - 4 * IO_INNER_SPACE_X) * dp->zoomKoef),
		(int)((height() - 2 * IO_INNER_SPACE_Y) * dp->zoomKoef),
		Qt::AlignLeft | Qt::WordBreak,
		getIOName());
	dp->painter->setPen(DEF_PAINTING_PEN);
}

bool StrIOBase::containsPoint(int xp, int yp)
{
	// FIXME - round shape
	return QRect(x(), y(), width(), height()).contains(xp, yp);
}

bool StrIOBase::containsHandlePoint(int xp, int yp)
{
	return getHandlePoint(xp, yp) != RSItemBase::None;
}

RSItemBase::RSItemHandle StrIOBase::getHandlePoint(int xp, int yp)
{
	// top left
	if (QRect(x(), y(), IO_SELECT_WH, IO_SELECT_WH).contains(xp, yp))
		return RSItemBase::TopLeft;
	// top right
	if (QRect(x() + width() - IO_SELECT_WH, y(), IO_SELECT_WH, IO_SELECT_WH).contains(xp, yp))
		return RSItemBase::TopRight;
	// bottom left
	if (QRect(x(), y() + height() - IO_SELECT_WH, IO_SELECT_WH, IO_SELECT_WH).contains(xp, yp))
		return RSItemBase::BottomLeft;
	// bottom right
	if (QRect(x() + width() - IO_SELECT_WH, y() + height() - IO_SELECT_WH, IO_SELECT_WH, IO_SELECT_WH).contains(xp, yp))
		return RSItemBase::BottomRight;
	return RSItemBase::None;
}

void StrIOBase::getIOPoint(RSItemIO *item, int *xp, int *yp)
{
	// for io it is in center
	*xp = x() + width() / 2;
	*yp = y() + height() / 2;
}

void StrIOBase::updateContents(QScrollView *sv, float zoom)
{
	int i;
	
	for (i = 0; i < 2; i++)
	{
		RSItemIO *io = getIO(i, 0);
		if (io)
		{
			StrLink *l = io->getConnectedTo();
			if (l)
				l->updateOneSegment(sv, l->findRoute(io), zoom);
		}
	}
		
	RSItemBaseWithType::updateContents(sv, zoom);
}
