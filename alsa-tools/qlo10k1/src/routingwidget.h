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
 
#ifndef ROUTINGWIDGET_H
#define ROUTINGWIDGET_H

#include <qwidget.h>
#include <qlayout.h>
#include <qscrollview.h>

#include "custom_colors.h"

class RoutingWidget;
class QPushButton;
class QSpinBox;
class StrGlobal;
class RSItemBaseWithType;
class RSItemIO;

class RoutingDrawWidget : public QScrollView
{
	Q_OBJECT
	
	enum EditMode {None, Select, ResizeTopLeft, ResizeTopRight, ResizeBottomLeft, ResizeBottomRight, HandleMove, Move, DragLink};
	
	EditMode mode;
	
	// for select
	QPoint selectStartPoint;
	QPoint selectEndPoint;
	
	RSItemBaseWithType *resizeItem;
	int linkHandle;
	
	RSItemIO *linkStart;
	
	QFont titleFont;
public:
	RoutingDrawWidget(RoutingWidget *parent = 0, const char *name = 0, WFlags f = 0);
	~RoutingDrawWidget();
	
	void startLinkDrag(RSItemIO *si, int xp, int yp);
	void updateDragLink();
	void stopLinkDrag();
	void connectLinkDrag(int xp, int yp, int mxp, int myp);
	bool isDragLink()
	{
		return mode == DragLink;
	}
	
	void deleteAllSelected();
	void deleteOneSelected(RSItemBaseWithType *item);
	
	QFont &createFont(float zoom);
	
protected:
	void drawContents(QPainter* p, int cx, int cy, int cw, int ch);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void keyPressEvent(QKeyEvent *e);
};

class RoutingWidget : public QWidget
{
	Q_OBJECT

public:
	enum EditMode {Normal, FX, In, Out, Route, Effect, EffectStack};
	enum MenuMode {MenuNone, MenuObjects, MenuIO, MenuPatch, MenuLink};
private:
	RoutingDrawWidget *drawing;
	QVBoxLayout *RoutingWidgetLayout;

	EditMode widgetMode;

	void untoggleMode(EditMode m);
	void toggleMode(EditMode m);

	QPushButton *pbRoutingNormal;
	QPushButton *pbRoutingFX;
	QPushButton *pbRoutingIn;
	QPushButton *pbRoutingOut;
	QPushButton *pbRoutingRoute;
	QPushButton *pbRoutingEffect;
	QPushButton *pbRoutingEffectStack;

	QSpinBox *sbRoutingZoom;
	float zoomLevel;
	StrGlobal *structure;

	RSItemBaseWithType *itemOn;
	int posXOn;
	int posYOn;
public:
	RoutingWidget(QWidget *parent = 0, const char *name = 0, WFlags f = 0);
	~RoutingWidget();

	void createButtons();
	StrGlobal *getStructure()
	{
		return structure;
	}
	
	RoutingDrawWidget *getDrawing()
	{
		return drawing;
	}
	
	void refreshDrawing(StrGlobal *str);
	void setZoomLevel(float level);
	float getZoomLevel();
	int zoomVal(int val)
	{
		return (int)(getZoomLevel() * (float)val);
	};
	
	int deZoomVal(int val)
	{
		return (int)((float)val / getZoomLevel());
	};
	
	void updateZoomLevelCtrl(int level);
	
	EditMode getWidgetMode();
	
	void putNewObjectAt(int xp, int yp);
	RSItemBaseWithType *createNewIO(EditMode em);
	RSItemBaseWithType *createNewPatch();
	
	void startLinkDrag(int xp, int yp);
	
	void openObjectMenuAt(RSItemBaseWithType *item, MenuMode mm, int xp, int yp, int mxp, int myp);
	
	friend class RoutingDrawWidget;
public slots:
	void modeNormalClicked();
	void modeFxClicked();
	void modeInClicked();
	void modeOutClicked();
	void modeRouteClicked();
	void modeEffectClicked();
	void modeEffectStackClicked();


	void zoomValueChanged(int value);
	void zoomInClicked();
	void zoomOutClicked();
};

#endif // ROUTINGWIDGET_H

