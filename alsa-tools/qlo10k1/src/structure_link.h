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
 
#ifndef STRUCTURE_LINK_H
#define STRUCTURE_LINK_H

#include "structure.h"
#include "strparam.h"

class StrLink : public RSItemBaseWithType
{
public:
	enum LinkType {LinkFX, LinkIn, LinkOut, LinkNormal};
	enum IOType {IOIn, IOOut};
private:
	int linkId;
	LinkType type;
	bool simple;
	bool multi;
	
	RSItemIO *routes[POINTINFO_MAX_CONN_PER_POINT + 1];
	
	QValueList <QPoint> routesPoints[POINTINFO_MAX_CONN_PER_POINT + 1];
	QPoint routesEndPoints[POINTINFO_MAX_CONN_PER_POINT + 1];
	
	bool useMixPoint;
	QPoint mixPoint;
	// Determines whether a point is close enough to a another point, within LINK_SELECT_WH.
	bool containsPointPoint(QPoint &p, int xp, int yp);
	// Determines whether a point is on a line segment.
	bool containsPointSegment(QPoint &p1, QPoint &p2, int xp, int yp);
public:
	StrLink(int id, LinkType t);
	virtual ~StrLink();
	
	void setId(int id);
	int id();
	
	void setType(LinkType t);
	LinkType getType();
	
	bool isSimple();
	void setSimple(bool s);
	
	bool isMulti();
	void setMulti(bool m);
	
	int getMaxRoute()
	{
		return POINTINFO_MAX_CONN_PER_POINT;
	}
	
	void disconnectAll();

	int findRoute(RSItemIO *r);
	RSItemIO *getRoutePoint(int r);
	void setRoutePoint(int r, RSItemIO *ri);
	void clearRoutesPoints(int r);
	
	// Returns route number which segment containing point leads to, else -1
	int getRouteNumFromPoint(int xp, int yp);
	
	virtual void calcSize();
	virtual void draw(DrawingParams *dp);
	
	virtual void moveRel(int dx, int dy);
	
	void setMinMaxPoint(QPoint p, int *minx, int *maxx, int *miny, int *maxy);
	void setMinMax();
	void moveRelPoint(QPoint *p, int dx, int dy);
	void updateOneSegment(QScrollView *sv, int r, float zoom);
	
	virtual bool containsPoint(int xp, int yp);
	
	virtual bool containsHandlePoint(int xp, int yp);
	virtual int getHandlePointNum(int xp, int yp);
	
	virtual RSItemHandle getHandlePoint(int xp, int yp)
	{
		if (getHandlePointNum(xp, yp) >= 0)
			return RSItemBase::Other;
		else
			return RSItemBase::None;
	}
	
	virtual void moveHandleRel(int num, int dx, int dy);
	
	bool isValid();
	bool addSegmentPoint(int xp, int yp);
	bool delSegmentPoint(int num);
};

#endif // STRUCTURE_LINK_H
