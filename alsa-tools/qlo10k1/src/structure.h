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
 
#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <qstring.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qwidget.h>
#include <qscrollview.h>

class RSItemBase;
class StrPatch;
class StrInput;
class StrOutput;
class StrFX;
class StrLink;

class RSItemIO
{
	bool output;
	int idx;
	QString desc;
	RSItemBase *owner;
	StrLink *connectedTo;
public:
	RSItemIO(RSItemBase *o, bool out, int i, QString d);
	~RSItemIO();
	bool isOutput();
	void setOutput(bool o);
	int getIdx();
	void setIdx(int i);
	QString getDesc();
	void setDesc(QString d);
	RSItemBase *getOwner();
	void connectTo(StrLink *to);
	StrLink *getConnectedTo();
};

class QWidget;
class QPainter;

class DrawingParams
{
public:
	float zoomKoef;
	DrawingParams(float zoom, QWidget *dt, QPaintEvent *pe, QPainter *p);
	
	QWidget *drawTo;
	QPaintEvent *paintEvent;
	QPainter *painter;
};

class RSItemBase
{
public:
	enum RSItemHandle {TopLeft, TopRight, BottomLeft, BottomRight, Other, None};
private:
	QPoint position;
	QSize size;
	QSize minSize;
	
	bool fNew;
	bool fUsed;
	bool fChanged;
	
	bool fSelected;
public:
	RSItemBase();
	
	// position
	int x();
	int y();
	void setPosition(int x, int y);
	// size
	int width();
	int height();
	// size
	int minWidth();
	int minHeight();
	void setSize(int w, int h);
	virtual void moveRel(int dx, int dy);
	virtual void moveHandleRel(int num, int dx, int dy);
	virtual void updateContents(QScrollView *sv, float zoom);
	QString getTooltipDecsription();
	virtual bool containsPoint(int xp, int yp);
	virtual RSItemIO *getIOAtPoint(int xp, int yp);	
	virtual bool containsHandlePoint(int xp, int yp);
	virtual RSItemHandle getHandlePoint(int xp, int yp);
	virtual int getHandlePointNum(int xp, int yp);
	bool flagUsed();
	void setFlagUsed(bool u);
	bool flagNew();
	void setFlagNew(bool n);
	bool flagChanged();
	void setFlagChanged(bool c);
	bool flagSelected();
	void setFlagSelected(bool s);
	virtual QString getDesc();
	virtual void draw(DrawingParams *dp);
	virtual void calcSize();
};

class RSItemBaseWithType : public RSItemBase
{
public:
	enum ItemType {In, Out, FX, Patch, Link};
private:
	ItemType iType;
public:
	RSItemBaseWithType(ItemType type);
	ItemType type();
	virtual int getMaxIOIdx(bool out);
	virtual RSItemIO *getIO(bool out, int idx);
	virtual void getIOPoint(RSItemIO *item, int *xp, int *yp);
	virtual ~RSItemBaseWithType();
};

#endif // STRUCTURE_H
