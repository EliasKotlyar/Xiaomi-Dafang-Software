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

#include <math.h> 
#include <qpainter.h>

#include "structure_link.h"
#include "custom_colors.h"

StrLink::StrLink(int id, LinkType t) 
	: RSItemBaseWithType(RSItemBaseWithType::Link)
{
	linkId = id;
	type = t;
	
	for (int i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
		routes[i] = NULL;
		
	useMixPoint = false;
	
	setSize(0, 0);
	RSItemBaseWithType::calcSize();
}

StrLink::~StrLink()
{
	disconnectAll();
}

void StrLink::setId(int id)
{
	linkId = id;
}

int StrLink::id()
{
	return linkId;
}

void StrLink::setType(LinkType t)
{
	type = t;
}

StrLink::LinkType StrLink::getType()
{
	return type;
}

bool StrLink::isSimple()
{
	return simple;
}

void StrLink::setSimple(bool s)
{
	simple = s;
}

bool StrLink::isMulti()
{
	return multi;
}

void StrLink::setMulti(bool m)
{
	multi = m;
}

int StrLink::findRoute(RSItemIO *r)
{
	for (int i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i] == r)
			return i;
	}
	return -1;
}

RSItemIO *StrLink::getRoutePoint(int r)
{
	if (r >= POINTINFO_MAX_CONN_PER_POINT)
		return NULL;
	return routes[r];
}

void StrLink::setRoutePoint(int r, RSItemIO *ri)
{
	if (r >= POINTINFO_MAX_CONN_PER_POINT + 1)
		return;
	
	RSItemIO *tmp = routes[r];
	routes[r] = NULL;
	if (tmp)
		tmp->connectTo(NULL);
	
	routes[r] = ri;
	if (!ri)
		routesPoints[r].clear();
	if (ri)
		ri->connectTo(this);
}

void StrLink::clearRoutesPoints(int r)
{
	if (r >= POINTINFO_MAX_CONN_PER_POINT + 1)
		return;
	routesPoints[r].clear();
}

#define LINK_SELECT_WH 7

void StrLink::setMinMaxPoint(QPoint p, int *minx, int *maxx, int *miny, int *maxy)
{
	if (p.x() - LINK_SELECT_WH / 2 < *minx)
		*minx = p.x() - LINK_SELECT_WH / 2;
	if (p.x() + LINK_SELECT_WH > *maxx)
		*maxx = p.x() + LINK_SELECT_WH;
		
	if (p.y() - LINK_SELECT_WH / 2 < *miny)
		*miny = p.y() - LINK_SELECT_WH / 2;
	if (p.y() + LINK_SELECT_WH > *maxy)
		*maxy = p.y() + LINK_SELECT_WH;
}

void StrLink::setMinMax()
{
	int minx = 1000000;
	int maxx = -1;
	int miny = 1000000;
	int maxy = -1;
	
	int i, j;
	
	if (useMixPoint)
		setMinMaxPoint(mixPoint, &minx, &maxx, &miny, &maxy);
		
	for (i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			setMinMaxPoint(routesEndPoints[i], &minx, &maxx, &miny, &maxy);
			
			for (j = 0; j < (int)routesPoints[i].count(); j++)
			{
				QPoint tmpp = routesPoints[i][j];
				setMinMaxPoint(tmpp, &minx, &maxx, &miny, &maxy);
			}
		}
	}
	
	setPosition(minx, miny);
	setSize(maxx - minx, maxy - miny);	
}

void StrLink::calcSize()
{
	int i;
	int cnt = 0;
	
	int ioxp, ioyp;
	
	for (i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			cnt++;
			
			((RSItemBaseWithType *)routes[i]->getOwner())->getIOPoint(routes[i], &ioxp, &ioyp);
			routesEndPoints[i].setX(ioxp);
			routesEndPoints[i].setY(ioyp);
		}
	}
	
	setMinMax();
	
	if (cnt <= 2)
	{
		// not use mixpoint
		useMixPoint = false;
	}
	else
	{
		// use mixpoint
		if (!useMixPoint)
		{
			mixPoint.setX(x() + width() / 2);
			mixPoint.setY(y() + height() / 2);
			
			useMixPoint = true;
		}
	}
}

void StrLink::draw(DrawingParams *dp)
{
	int i, j;
	for (i = useMixPoint ? 0 : 1; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			QPoint fp = routesEndPoints[i];
			if (flagSelected())
				dp->painter->fillRect(
					(int)((fp.x() - LINK_SELECT_WH / 2) * dp->zoomKoef),
					(int)((fp.y() - LINK_SELECT_WH / 2) * dp->zoomKoef),
					(int)(LINK_SELECT_WH * dp->zoomKoef),
					(int)(LINK_SELECT_WH * dp->zoomKoef),
					QBrush(BOX_SELECTED_COLOR));
					
			for (j = 0; j < (int)routesPoints[i].count(); j++)
			{
				QPoint tmpp = routesPoints[i][j];
				if (flagSelected())
					dp->painter->fillRect(
						(int)((tmpp.x() - LINK_SELECT_WH / 2) * dp->zoomKoef),
						(int)((tmpp.y() - LINK_SELECT_WH / 2) * dp->zoomKoef),
						(int)(LINK_SELECT_WH * dp->zoomKoef),
						(int)(LINK_SELECT_WH * dp->zoomKoef),
						QBrush(BOX_SELECTED_COLOR));
						
				dp->painter->setPen(LINE_PEN);
				dp->painter->drawLine((int)(fp.x() * dp->zoomKoef),
					(int)(fp.y() * dp->zoomKoef),
					(int)(tmpp.x() * dp->zoomKoef),
					(int)(tmpp.y() * dp->zoomKoef));
				dp->painter->setPen(DEF_PAINTING_PEN);
				fp = tmpp;
			}
			
			if (useMixPoint)
			{
				dp->painter->setPen(LINE_PEN);
				dp->painter->drawLine((int)(fp.x() * dp->zoomKoef),
					(int)(fp.y() * dp->zoomKoef),
					(int)(mixPoint.x() * dp->zoomKoef),
					(int)(mixPoint.y() * dp->zoomKoef));
				dp->painter->setPen(DEF_PAINTING_PEN);
					
				if (flagSelected())
					dp->painter->fillRect(
						(int)((mixPoint.x() - LINK_SELECT_WH / 2) * dp->zoomKoef),
						(int)((mixPoint.y() - LINK_SELECT_WH / 2) * dp->zoomKoef),
						(int)(LINK_SELECT_WH * dp->zoomKoef),
						(int)(LINK_SELECT_WH * dp->zoomKoef),
						QBrush(BOX_SELECTED_COLOR));
			}
			else
			{
				dp->painter->setPen(LINE_PEN);
				dp->painter->drawLine((int)(fp.x() * dp->zoomKoef),
					(int)(fp.y() * dp->zoomKoef),
					(int)(routesEndPoints[0].x() * dp->zoomKoef),
					(int)(routesEndPoints[0].y() * dp->zoomKoef));
				dp->painter->setPen(DEF_PAINTING_PEN);
				
				if (flagSelected())
					dp->painter->fillRect(
						(int)((routesEndPoints[0].x() - LINK_SELECT_WH / 2) * dp->zoomKoef),
						(int)((routesEndPoints[0].y() - LINK_SELECT_WH / 2) * dp->zoomKoef),
						(int)(LINK_SELECT_WH * dp->zoomKoef),
						(int)(LINK_SELECT_WH * dp->zoomKoef),
						QBrush(BOX_SELECTED_COLOR));
			}			
		}
	}	
	
	if (useMixPoint)
	{
		dp->painter->drawEllipse((int)((mixPoint.x() - 1) * dp->zoomKoef),
					(int)((mixPoint.y() - 1) * dp->zoomKoef),
					(int)(3 * dp->zoomKoef),
					(int)(3 * dp->zoomKoef));
		dp->painter->drawEllipse((int)((mixPoint.x() - 2) * dp->zoomKoef),
					(int)((mixPoint.y() - 2) * dp->zoomKoef),
					(int)(5 * dp->zoomKoef),
					(int)(5 * dp->zoomKoef));
		dp->painter->drawEllipse((int)((mixPoint.x() - 3) * dp->zoomKoef),
					(int)((mixPoint.y() - 3) * dp->zoomKoef),
					(int)(7 * dp->zoomKoef),
					(int)(7 * dp->zoomKoef));
	}
}

void StrLink::moveRel(int dx, int dy)
{
	int i;
	unsigned int j;
	
	if (useMixPoint)
		moveRelPoint(&mixPoint, dx, dy);
		
	for (i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			for (j = 0; j < routesPoints[i].count(); j++)
			{
				QPoint tmpp = routesPoints[i][j];
				moveRelPoint(&tmpp, dx, dy);
				routesPoints[i][j] = tmpp;
			}
		}
	}
		
	calcSize();
}

void StrLink::moveRelPoint(QPoint *p, int dx, int dy)
{
	p->setX(p->x() + dx);
	p->setY(p->y() + dy);
	
	if (p->x() < 0)
		p->setX(0);
	if (p->y() < 0)
		p->setY(0);
}

void StrLink::updateOneSegment(QScrollView *sv, int r, float zoom)
{
	int ioxp;
	int ioyp;
	
	if (r < 0 || r >= POINTINFO_MAX_CONN_PER_POINT + 1)
		return; 
	
	if (routes[r])
	{
		((RSItemBaseWithType *)routes[r]->getOwner())->getIOPoint(routes[r], &ioxp, &ioyp);
		routesEndPoints[r].setX(ioxp);
		routesEndPoints[r].setY(ioyp);
		setMinMax();
		
		if (routesPoints[r].count())
		{
			QPoint rep = routesEndPoints[r];
			QPoint rp = routesPoints[r][0];
			
			QRect rect(QPoint(rep.x(), rep.y()), QPoint(rp.x(), rp.y()));
			rect = rect.normalize();
			rect.setX(rect.x() - LINK_SELECT_WH);
			rect.setY(rect.y() - LINK_SELECT_WH);
			rect.setWidth(rect.width() + 2 * LINK_SELECT_WH);
			rect.setHeight(rect.height() + 2 * LINK_SELECT_WH);
			sv->updateContents((int)(rect.x() * zoom),
				(int)(rect.y() * zoom),
				(int)(rect.width() * zoom),
				(int)(rect.height() * zoom));
		}
		else
		{
			if (useMixPoint)
			{
				QPoint rep = routesEndPoints[r];
				QRect rect(QPoint(rep.x(), rep.y()), QPoint(mixPoint.x(), mixPoint.y()));
				rect = rect.normalize();
				rect.setX(rect.x() - LINK_SELECT_WH);
				rect.setY(rect.y() - LINK_SELECT_WH);
				rect.setWidth(rect.width() + 2 * LINK_SELECT_WH);
				rect.setHeight(rect.height() + 2 * LINK_SELECT_WH);
				sv->updateContents((int)(rect.x() * zoom),
					(int)(rect.y() * zoom),
					(int)(rect.width() * zoom),
					(int)(rect.height() * zoom));
			}
			else
				sv->updateContents((int)((x() - LINK_SELECT_WH) * zoom),
					(int)((y() - LINK_SELECT_WH) * zoom),
					(int)((width() + 2 * LINK_SELECT_WH) * zoom),
					(int)((height() + 2 * LINK_SELECT_WH) * zoom));
		}
	}
}

// Determines whether a point is close enough to a another point, within LINK_SELECT_WH.
bool StrLink::containsPointPoint(QPoint &p, int xp, int yp)
{
	QRect r(p.x() - LINK_SELECT_WH / 2, p.y() - LINK_SELECT_WH / 2,
		LINK_SELECT_WH + 1, LINK_SELECT_WH + 1);

	return r.contains(xp, yp);
}

// Determines whether a point is on a line segment.
bool StrLink::containsPointSegment(QPoint &p1, QPoint &p2, int xp, int yp)
{
	QRect r(p1, p2);
	if (!r.normalize().contains(xp, yp))
		return false;
	
	float v1 = p2.x() - p1.x();
	float v2 = p2.y() - p1.y();
	float a = v2;
	float b = -v1;
	
	float c = v1 * p1.y() - v2 * p1.x();
	
	if (fabs(a * xp + b * yp + c) / sqrt(a * a + b * b) < 4.0)
		return true;
	else
		return false;
}

bool StrLink::containsPoint(int xp, int yp)
{
	int i;
	unsigned j;
	
	if (xp < x() || xp > x() + width() ||
		yp < y() || yp > y() + height())
		return false;
		
	if (useMixPoint)
	{
		if (containsPointPoint(mixPoint, xp, yp))
			return true;
	}
		
	for (i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			QPoint fp = routesEndPoints[i];
			if (containsPointPoint(fp, xp, yp))
				return true;
				
			for (j = 0; j < routesPoints[i].count(); j++)
			{
				QPoint tmpp = routesPoints[i][j];
				
				if (containsPointPoint(tmpp, xp, yp))
					return true;
			
				if (containsPointSegment(fp, tmpp, xp, yp))
					return true;
				fp = tmpp;
			}
			
			if (useMixPoint)
			{
				if (containsPointSegment(fp, mixPoint, xp, yp))
					return true;
			}
			else
			{
				if (containsPointSegment(fp, routesEndPoints[0], xp, yp))
					return true;
			}
		}
	}
	return false;
}

bool StrLink::containsHandlePoint(int xp, int yp)
{
	return getHandlePointNum(xp, yp) >= 0;
}

int StrLink::getHandlePointNum(int xp, int yp)
{
	int i;
	unsigned j;
	
	if (xp < x() || xp > x() + width() ||
		yp < y() || yp > y() + height())
		return -1;
		
	if (useMixPoint)
	{
		if (containsPointPoint(mixPoint, xp, yp))
			return 0;
	}
		
	for (i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			for (j = 0; j < routesPoints[i].count(); j++)
			{
				QPoint tmpp = routesPoints[i][j];
				
				if (containsPointPoint(tmpp, xp, yp))
					return (i + 1) * 256 + j + 1;
			}
		}
	}
	
	return -1;
}

void StrLink::moveHandleRel(int num, int dx, int dy)
{
	int i;
	int j;
	
	if (useMixPoint && num == 0)
	{
		moveRelPoint(&mixPoint, dx, dy);
		setMinMax();
		return;
	}
		
	for (i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			for (j = 0; j < (int)routesPoints[i].count(); j++)
			{
				if (num == (i + 1) * 256 + j + 1)			
				{
					QPoint tmpp = routesPoints[i][j];
					moveRelPoint(&tmpp, dx, dy);
					routesPoints[i][j] = tmpp;
					setMinMax();
					return;
				}
			}
		}
	}
}

void StrLink::disconnectAll()
{
	for (int i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			RSItemIO *tmp = routes[i];
			routes[i] = NULL;
			tmp->connectTo(NULL);
		}
	}
}

bool StrLink::isValid()
{
	int cnti = 0;
	int cnto = 0;
	for (int i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			if (routes[i]->isOutput())
				cnto++;
			else
				cnti++;
		}
	}
	
	if (getType() == LinkNormal)
		return cnti + cnto > 1 && (cnti > 0 && cnto > 0);
	else
		return cnti + cnto > 1;
	
}

bool StrLink::addSegmentPoint(int xp, int yp)
{
	int i;
	unsigned j;
	
	if (xp < x() || xp > x() + width() ||
		yp < y() || yp > y() + height())
		return false;
		
	for (i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			QPoint fp = routesEndPoints[i];
			for (j = 0; j < routesPoints[i].count(); j++)
			{
				QPoint tmpp = routesPoints[i][j];
				
				if (containsPointSegment(fp, tmpp, xp, yp))
				{
					// add new point
					routesPoints[i].insert(routesPoints[i].at(j), QPoint(xp,yp));
					return true;
				}
				fp = tmpp;
			}
			
			if (useMixPoint)
			{
				if (containsPointSegment(fp, mixPoint, xp, yp))
				{
					routesPoints[i].append(QPoint(xp,yp));
					return true;
				}
			}
			else
			{
				if (containsPointSegment(fp, routesEndPoints[0], xp, yp))
				{
					routesPoints[i].append(QPoint(xp,yp));
					return true;
				}
			}
		}
	}
	return false;
}

bool StrLink::delSegmentPoint(int num)
{
	if (num <= 0)
		return false;
		
	int r = ((num - 1) / 256) - 1;
	int i = ((num - 1) % 256);
		
	if (routes[r])
	{
		if (i < (int)routesPoints[r].count())
		{
			routesPoints[r].remove(routesPoints[r].at(i));
			return true;
		}
	}
	
	return false;
}

// Returns route number which segment containing point leads to, else -1
int StrLink::getRouteNumFromPoint(int xp, int yp)
{
	int i;
	unsigned j;
	
	if (xp < x() || xp > x() + width() ||
		yp < y() || yp > y() + height())
		return -1;
		
	if (useMixPoint)
	{
		if (containsPointPoint(mixPoint, xp, yp))
			return -1;
	}
		
	for (i = 0; i < POINTINFO_MAX_CONN_PER_POINT + 1; i++)
	{
		if (routes[i])
		{
			QPoint fp = routesEndPoints[i];
			if (containsPointPoint(fp, xp, yp))
				return i;
				
			for (j = 0; j < routesPoints[i].count(); j++)
			{
				QPoint tmpp = routesPoints[i][j];
				
				if (containsPointPoint(tmpp, xp, yp))
					return i;
			
				if (containsPointSegment(fp, tmpp, xp, yp))
					return i;
				fp = tmpp;
			}
			
			if (useMixPoint)
			{
				if (containsPointSegment(fp, mixPoint, xp, yp))
					return i;
			}
			else
			{
				if (containsPointSegment(fp, routesEndPoints[0], xp, yp))
					return i;
			}
		}
	}
	return -1;
}

