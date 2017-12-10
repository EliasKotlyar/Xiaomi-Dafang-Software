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
 
#include <qdockarea.h>
#include <qtoolbutton.h>
#include <qspinbox.h>
#include <qregion.h>
#include <qpushbutton.h>
#include <qpainter.h>
#include <qtooltip.h>

#include "routingwidget.h"
#include "structure.h"
#include "strglobal.h"
#include "structure_io.h"
#include "structure_patch.h"

//icons
#include "icons/routing_fx.xpm"
#include "icons/routing_in.xpm"
#include "icons/routing_out.xpm"
#include "icons/routing_route.xpm"
#include "icons/routing_effect.xpm"
#include "icons/routing_effect_stack.xpm"
#include "icons/routing_zoom_plus.xpm"
#include "icons/routing_zoom_minus.xpm"
#include "icons/routing_normal.xpm"

class RoutingToolTip : public QToolTip
{
protected:
	virtual void maybeTip(const QPoint &p);
public:
	RoutingToolTip(QWidget *widget, QToolTipGroup *group = 0);
};

RoutingDrawWidget::RoutingDrawWidget(RoutingWidget *parent, const char *name, WFlags f)
	: QScrollView(parent, name, f)
{
	setFocusPolicy(QWidget::StrongFocus);
	resizeContents(4000,3000);
	this->viewport()->setEraseColor(QColor(255,255,255));
	resizeItem = NULL;
	
	mode = None;
	
	//enable tooltips
	new RoutingToolTip(this);
}

RoutingDrawWidget::~RoutingDrawWidget()
{
}

void RoutingDrawWidget::drawContents(QPainter* p, int cx, int cy, int cw, int ch)
{
	RoutingWidget *parent = (RoutingWidget *)parentWidget();
	StrGlobal *structure = parent->getStructure();
	
	if (!structure)
		return;
	
	RSItemBaseWithType *item;
	DrawingParams dp(parent->getZoomLevel(), this->viewport(), NULL, p);
	
	p->setFont(createFont(parent->getZoomLevel()));
		
	for(item = structure->UsedItems.first(); item; item = structure->UsedItems.next() )
	{
		if (item->flagNew() || item->flagChanged())
		{
			item->calcSize();
			item->setFlagNew(false);
			item->setFlagChanged(false);
		}
		
 		item->draw(&dp);
	}
	
	// if in link mode
	if (mode == DragLink)
	{
		QPainter p(this->viewport());
		int piox, pioy;
		int ex = selectEndPoint.x();
		int ey = selectEndPoint.y();
		
		((StrPatch *)linkStart->getOwner())->getIOPoint(linkStart, &piox, &pioy);
		contentsToViewport(parent->zoomVal(piox), parent->zoomVal(pioy), piox, pioy);
		contentsToViewport(parent->zoomVal(ex), parent->zoomVal(ey), ex, ey);
		
		p.setPen(LINE_PEN);
		p.drawLine(QPoint(piox, pioy), QPoint(ex, ey));
		p.setPen(DEF_PAINTING_PEN);
	}
}

void RoutingDrawWidget::mousePressEvent(QMouseEvent *e)
{
	QScrollView::mousePressEvent(e);
	setFocus();
	
	RoutingWidget *parent = (RoutingWidget *)parentWidget();
	StrGlobal *structure = parent->getStructure();
	
	if (!structure)
		return;
		
	//treba prepocet na zoom a ofset qscrollview - a niekde to pada
	int xp = e->x();
	int yp = e->y();
	
	viewportToContents(xp, yp, xp, yp);
	
	xp = parent->deZoomVal(xp);
	yp = parent->deZoomVal(yp);
	
	RSItemBaseWithType *item;
	
	QPtrList <RSItemBaseWithType> toClear;
	
	if (e->button() == MidButton)
		return;

	if (mode == DragLink)
	{
		QPoint tmpp = mapToGlobal(QPoint(e->x(), e->y()));
		connectLinkDrag(xp, yp, tmpp.x(), tmpp.y());
		return;
	}
		
	// get mode
	if (parent->getWidgetMode() != RoutingWidget::Normal &&
		e->button() == LeftButton)
	{
		parent->putNewObjectAt(xp, yp);
		return;
	}
		
		
	// find item under this point
	bool oneSelected = false;
	bool clearOthers = false;
	
	RSItemBase::RSItemHandle resHandle = RSItemBase::None;
	RSItemBaseWithType *itemToResize = NULL;
	RSItemBaseWithType *itemOn = NULL;
	linkHandle = -1;
	
	for(item = structure->UsedItems.first(); item; item = structure->UsedItems.next())
	{
		bool redraw = false;
		
		if (!oneSelected && item->containsPoint(xp, yp))
		{
			if (!item->flagSelected())
			{
				item->setFlagSelected(true);
				redraw = true;
				clearOthers = true;
			}
			else if (e->state() & Qt::ControlButton)
			{
				item->setFlagSelected(false);
				redraw = true;
			}
			oneSelected = true;
			
			itemOn = item;
			
			// check resize point
			resHandle = item->getHandlePoint(xp, yp);
			if (resHandle == RSItemBase::Other)
			{
				itemToResize = item;
				linkHandle = item->getHandlePointNum(xp, yp);
			}
			else
			{
				linkHandle = -1;			
				if (resHandle != RSItemBase::None)
					itemToResize = item;
			}
		}
		else
		{
			if (item->flagSelected())
				toClear.append(item);
		}
				
		if (redraw)
		{
			this->updateContents(parent->zoomVal(item->x()),
				parent->zoomVal(item->y()),
				parent->zoomVal(item->width()),
				parent->zoomVal(item->height()));
		}
	}
	
	EditMode newMode = None;
	RoutingWidget::MenuMode newMenuMode = RoutingWidget::MenuNone;
	
	parent->itemOn = NULL;
	
	bool doClearOthers = false;
	
	if (e->button() == LeftButton)
	{
		if (!oneSelected)
		{
			// set select mode
			newMode = Select;
		}
		else 
		{
			if (e->state() & Qt::ControlButton)
			{
				newMode = None;
			}
			else
			{
				if (clearOthers)
					doClearOthers = true;
				if (itemToResize)
				{
					resizeItem = itemToResize;
						
					if (linkHandle < 0)
					{
						switch (resHandle)
						{
							case RSItemBase::TopLeft:
								newMode = ResizeTopLeft;
								break;
							case RSItemBase::TopRight:
								newMode = ResizeTopRight;
								break;
							case RSItemBase::BottomLeft:
								newMode = ResizeBottomLeft;
								break;
							case RSItemBase::BottomRight:
								newMode = ResizeBottomRight;
								break;
							default:
								break;
						}
					}
					else
						newMode = HandleMove;
				}
				else if(itemOn->type() != RSItemBaseWithType::Link)
					newMode = Move;
				else
					newMode = None;
			}
		}
	}
	else
	{
		if (!oneSelected)
		{
			newMenuMode = RoutingWidget::MenuNone;
			doClearOthers = true;
		}
		else
		{
			if (clearOthers)
			{
				doClearOthers = true;
			}
				
			if (!doClearOthers && toClear.count() > 0)
				newMenuMode = RoutingWidget::MenuObjects;
			else
			{
				switch (itemOn->type())
				{
					case RSItemBaseWithType::In:
					case RSItemBaseWithType::Out:
					case RSItemBaseWithType::FX:
						newMenuMode = RoutingWidget::MenuIO;
						break;
					case RSItemBaseWithType::Link:
						newMenuMode = RoutingWidget::MenuLink;
						break;
					case RSItemBaseWithType::Patch:
						newMenuMode = RoutingWidget::MenuPatch;
						break;
					default:
						break;
				}
			}
		}
		
		parent->itemOn = itemOn;
		parent->posXOn = xp;
		parent->posYOn = yp;
	}
	
	if (doClearOthers)
	{
		for(item = toClear.first(); item; item = toClear.next())
		{
			item->setFlagSelected(false);
			
			this->updateContents(parent->zoomVal(item->x()),
				parent->zoomVal(item->y()),
				parent->zoomVal(item->width()),
				parent->zoomVal(item->height()));
		}		
	}
	
	if (e->button() == LeftButton)
		mode = newMode;
	else
	{
		QPoint p = mapToGlobal(QPoint(e->x(), e->y()));
		parent->openObjectMenuAt(itemOn, newMenuMode, xp, yp, p.x(), p.y());
	}
	
	selectStartPoint.setX(e->x());
	selectStartPoint.setY(e->y());
	selectEndPoint = selectStartPoint;

}

void RoutingDrawWidget::mouseMoveEvent(QMouseEvent *e)
{
	RoutingWidget *parent = (RoutingWidget *)parentWidget();
	StrGlobal *structure = parent->getStructure();
	
	if (!structure)
		return;
		
	if (mode == Select && 
		(e->x() != selectStartPoint.x() || e->y() != selectStartPoint.y()))
	{
		QPainter p(this->viewport());
		
		p.drawWinFocusRect(selectStartPoint.x(),
			selectStartPoint.y(),
			selectEndPoint.x() - selectStartPoint.x(),
			selectEndPoint.y() - selectStartPoint.y());
			
		selectEndPoint.setX(e->x());
		selectEndPoint.setY(e->y());
			
		p.drawWinFocusRect(selectStartPoint.x(),
			selectStartPoint.y(),
			selectEndPoint.x() - selectStartPoint.x(),
			selectEndPoint.y() - selectStartPoint.y());
	}
	else
	{
		int dx = parent->deZoomVal((selectEndPoint.x() - selectStartPoint.x()));
		int dy = parent->deZoomVal((selectEndPoint.y() - selectStartPoint.y()));
		
		if (mode == Move)
		{
			RSItemBaseWithType *item;
			for(item = structure->UsedItems.first(); item; item = structure->UsedItems.next() )
			{
				if (item->flagSelected())
				{
					// update old
					item->updateContents(this, parent->getZoomLevel());
						
					item->moveRel(dx, dy);
					// update new
					item->updateContents(this, parent->getZoomLevel());
				}
			}
		}
		else if (mode == HandleMove)
		{
			// update old
			resizeItem->updateContents(this, parent->getZoomLevel());
				
			resizeItem->moveHandleRel(linkHandle, dx, dy);
			// update new
			resizeItem->updateContents(this, parent->getZoomLevel());
		}
		else if (mode == ResizeTopLeft || 
			mode == ResizeTopRight ||
			mode == ResizeBottomLeft ||
			mode == ResizeBottomRight)
		{
			int nx = resizeItem->x();
			int ny = resizeItem->y();
			int nw = resizeItem->width();
			int nh = resizeItem->height();
								
			switch (mode)
			{
				case ResizeTopLeft:
					nx = nx + dx;
					ny = ny + dy;
					nw = nw - dx;
					nh = nh - dy;
					break;
				case ResizeTopRight:
					ny = ny + dy;
					nw = nw + dx;
					nh = nh - dy;
					break;
				case ResizeBottomLeft:
					nx = nx + dx;
					nw = nw - dx;
					nh = nh + dy;
					break;
				case ResizeBottomRight:
					nw = nw + dx;
					nh = nh + dy;
					break;
				default:
					break;
			}
			
			if (nx < 0)
				nx = 0;
			if (ny < 0)
				ny = 0;

			if (nw < resizeItem->minWidth())
				nw = resizeItem->minWidth();
			if (nh < resizeItem->minHeight())
				nh = resizeItem->minHeight();
				
			resizeItem->updateContents(this, parent->getZoomLevel());

			resizeItem->setPosition(nx, ny);
			resizeItem->setSize(nw, nh);
			// update new
			resizeItem->updateContents(this, parent->getZoomLevel());
		}
		else if (mode == DragLink)
		{
			updateDragLink();
		}
		
		selectStartPoint = selectEndPoint;
	
		selectEndPoint.setX(e->x());
		selectEndPoint.setY(e->y());
		
		if (mode == DragLink)
		{
			int ex = selectEndPoint.x();
			int ey = selectEndPoint.y();
			
			viewportToContents(parent->deZoomVal(ex), parent->deZoomVal(ey), ex, ey);
			
			selectEndPoint.setX(ex);
			selectEndPoint.setY(ey);
		}
	}
}

void RoutingDrawWidget::updateDragLink()
{
	RoutingWidget *parent = (RoutingWidget *)parentWidget();
	
	int piox, pioy;
			
	int ex = selectEndPoint.x();
	int ey = selectEndPoint.y();

	((StrPatch *)linkStart->getOwner())->getIOPoint(linkStart, &piox, &pioy);
	
	QRect rect(QPoint(piox, pioy), QPoint(ex, ey));
	rect = rect.normalize();
	rect.setX(rect.x() - 1);
	rect.setY(rect.y() - 1);
	rect.setWidth(rect.width() + 2 * 1);
	rect.setHeight(rect.height() + 2 * 1);
	this->updateContents(parent->zoomVal(rect.x()),
		parent->zoomVal(rect.y()),
		parent->zoomVal(rect.width()),
		parent->zoomVal(rect.height()));
}

void RoutingDrawWidget::mouseReleaseEvent(QMouseEvent *e)
{
	RoutingWidget *parent = (RoutingWidget *)parentWidget();
	StrGlobal *structure = parent->getStructure();
	
	if (!structure)
		return;
		
	if (mode == Select)
	{
		selectEndPoint.setX(e->x());
		selectEndPoint.setY(e->y());
		
		QRect selArea(selectStartPoint, selectEndPoint);
		selArea = selArea.normalize();
		
		// transform
		int xt1;
		int yt1;
		
		int xt2;
		int yt2;
		
		viewportToContents(selArea.x(), selArea.y(), xt1, yt1);
		xt1 = parent->deZoomVal(xt1);
		yt1 = parent->deZoomVal(yt1);
		
		viewportToContents(selArea.right(), selArea.bottom(), xt2, yt2);
		xt2 = parent->deZoomVal(xt2);
		yt2 = parent->deZoomVal(yt2);
		
		// set flag for selected
		QRect selAreaTrans(QPoint(xt1, yt1), QPoint(xt2, yt2));
		
		RSItemBaseWithType *item;		
		for(item = structure->UsedItems.first(); item; item = structure->UsedItems.next() )
		{
			if (selAreaTrans.contains(item->x(), item->y()) &&
				selAreaTrans.contains(item->x() + item->width(), item->y() + item->height()))
			{
				if (!item->flagSelected())
					item->setFlagSelected(true);
			}
			else
			{
				if (!(e->state() & Qt::ControlButton) && item->flagSelected()) 
				{
					item->setFlagSelected(false);
					
					this->updateContents(parent->zoomVal(item->x()),
						parent->zoomVal(item->y()),
						parent->zoomVal(item->width()),
						parent->zoomVal(item->height()));
				}
			}
			
			
		}
		
		//this->updateContents(selArea);
		this->viewport()->update(selArea);
	}
	
	if (mode != DragLink)
		mode = None;
}

void RoutingDrawWidget::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Key_Escape && isDragLink())
		stopLinkDrag();
	
	if (e->key() == Key_Delete && !isDragLink())
	{
		deleteAllSelected();
	}
}

void RoutingDrawWidget::startLinkDrag(RSItemIO *si, int xp, int yp)
{
	mode = DragLink;
	linkStart = si;
	
	selectStartPoint.setX(xp);
	selectStartPoint.setY(yp);
	selectEndPoint = selectStartPoint;
	
	setMouseTracking(true);
	viewport()->setMouseTracking(true);
}

void RoutingDrawWidget::stopLinkDrag()
{
	mode = None;
	updateDragLink();
}

QFont &RoutingDrawWidget::createFont(float zoom)
{
	//QFont f("Helvetica", 10 * zoom);
	titleFont.setStyleHint(QFont::Helvetica);
	titleFont.setPointSize((int)(10 * zoom));
	return titleFont;
}


RoutingWidget::RoutingWidget(QWidget *parent, const char *name, WFlags f)
	: QWidget(parent, name, f)
{
	structure = NULL;
	setFocusPolicy(QWidget::StrongFocus);

	RoutingWidgetLayout = new QVBoxLayout(this);

	createButtons();

	drawing = new RoutingDrawWidget(this);

	RoutingWidgetLayout->addWidget(drawing/*, 1, 1*/);
	drawing->setUpdatesEnabled(TRUE);

	widgetMode = Normal;
	toggleMode(widgetMode);

	setZoomLevel(1.00);
	updateZoomLevelCtrl(100);
}

void RoutingWidget::createButtons()
{
	QHBoxLayout *hbox = new QHBoxLayout(RoutingWidgetLayout);


	pbRoutingNormal = new QPushButton(this);
	pbRoutingNormal->setIconSet(QIconSet(QPixmap((const char**)routing_normal)));
	hbox->addWidget(pbRoutingNormal);
	pbRoutingNormal->setToggleButton(true);

	pbRoutingFX = new QPushButton(this);
	pbRoutingFX->setIconSet(QIconSet(QPixmap((const char**)routing_fx)));
	hbox->addWidget(pbRoutingFX);
	pbRoutingFX->setToggleButton(true);

	pbRoutingIn = new QPushButton(this);
	pbRoutingIn->setIconSet(QIconSet(QPixmap((const char**)routing_in)));
	hbox->addWidget(pbRoutingIn);
	pbRoutingIn->setToggleButton(true);

	pbRoutingOut = new QPushButton(this);
	pbRoutingOut->setIconSet(QIconSet(QPixmap((const char**)routing_out)));
	hbox->addWidget(pbRoutingOut);
	pbRoutingOut->setToggleButton(true);

	pbRoutingRoute = new QPushButton(this);
	pbRoutingRoute->setIconSet(QIconSet(QPixmap((const char**)routing_route)));
	hbox->addWidget(pbRoutingRoute);
	pbRoutingRoute->setToggleButton(true);

	pbRoutingEffect = new QPushButton(this);
	pbRoutingEffect->setIconSet(QIconSet(QPixmap((const char**)routing_effect)));
	hbox->addWidget(pbRoutingEffect);
	pbRoutingEffect->setToggleButton(true);

	pbRoutingEffectStack = new QPushButton(this);
	pbRoutingEffectStack->setIconSet(QIconSet(QPixmap((const char**)routing_effect_stack)));
	hbox->addWidget(pbRoutingEffectStack);
	pbRoutingEffectStack->setToggleButton(true);
	
	pbRoutingEffectStack->hide();

	QSpacerItem *spacer1 = new QSpacerItem(10, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
	hbox->addItem(spacer1);

	QPushButton *pbRoutingZoomPlus = new QPushButton(this);
	pbRoutingZoomPlus->setIconSet(QIconSet(QPixmap((const char**)routing_zoom_plus)));
	hbox->addWidget(pbRoutingZoomPlus);

	sbRoutingZoom = new QSpinBox(this);
	hbox->addWidget(sbRoutingZoom);
	sbRoutingZoom->setMinValue(10);
	sbRoutingZoom->setMaxValue(500);

	QPushButton *pbRoutingZoomMinus = new QPushButton(this);
	pbRoutingZoomMinus->setIconSet(QIconSet(QPixmap((const char**)routing_zoom_minus)));
	hbox->addWidget(pbRoutingZoomMinus);


	QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
	hbox->addItem(spacer);

	connect(pbRoutingNormal, SIGNAL(clicked()), this, SLOT(modeNormalClicked()));
	connect(pbRoutingFX, SIGNAL(clicked()), this, SLOT(modeFxClicked()));
	connect(pbRoutingIn, SIGNAL(clicked()), this, SLOT(modeInClicked()));
	connect(pbRoutingOut, SIGNAL(clicked()), this, SLOT(modeOutClicked()));
	connect(pbRoutingRoute, SIGNAL(clicked()), this, SLOT(modeRouteClicked()));
	connect(pbRoutingEffect, SIGNAL(clicked()), this, SLOT(modeEffectClicked()));
	connect(pbRoutingEffectStack, SIGNAL(clicked()), this, SLOT(modeEffectStackClicked()));

	connect(sbRoutingZoom, SIGNAL(valueChanged(int)), this, SLOT(zoomValueChanged(int)));

	connect(pbRoutingZoomMinus, SIGNAL(clicked()), this, SLOT(zoomOutClicked()));
	connect(pbRoutingZoomPlus, SIGNAL(clicked()), this, SLOT(zoomInClicked()));
}

RoutingWidget::~RoutingWidget()
{
}

void RoutingWidget::untoggleMode(EditMode m)
{
	if (m != Normal)
		pbRoutingNormal->setOn(false);
	if (m != FX)
		pbRoutingFX->setOn(false);
	if (m != In)
		pbRoutingIn->setOn(false);
	if (m != Out)
		pbRoutingOut->setOn(false);
	if (m != Route)
		pbRoutingRoute->setOn(false);
	if (m != Effect)
		pbRoutingEffect->setOn(false);
	if (m != EffectStack)
		pbRoutingEffectStack->setOn(false);
}

void RoutingWidget::toggleMode(EditMode m)
{
	switch (m)
	{
		case Normal:
			pbRoutingNormal->setOn(true);
			break;
		case FX:
			pbRoutingFX->setOn(true);
			break;
		case In:
			pbRoutingIn->setOn(true);
			break;
		case Out:
			pbRoutingOut->setOn(true);
			break;
		case Route:
			pbRoutingRoute->setOn(true);
			break;
		case Effect:
			pbRoutingEffect->setOn(true);
			break;
		case EffectStack:
			pbRoutingEffectStack->setOn(true);
			break;
	}
}

void RoutingWidget::modeNormalClicked()
{
	widgetMode = Normal;
	toggleMode(widgetMode);
	untoggleMode(widgetMode);
}

void RoutingWidget::modeFxClicked()
{
	widgetMode = FX;
	toggleMode(widgetMode);
	untoggleMode(widgetMode);
}

void RoutingWidget::modeInClicked()
{
	widgetMode = In;
	toggleMode(widgetMode);
	untoggleMode(widgetMode);
}

void RoutingWidget::modeOutClicked()
{
	widgetMode = Out;
	toggleMode(widgetMode);
	untoggleMode(widgetMode);
}

void RoutingWidget::modeRouteClicked()
{
	widgetMode = Route;
	toggleMode(widgetMode);
	untoggleMode(widgetMode);
}

void RoutingWidget::modeEffectClicked()
{
	widgetMode = Effect;
	toggleMode(widgetMode);
	untoggleMode(widgetMode);
}

void RoutingWidget::modeEffectStackClicked()
{
	widgetMode = EffectStack;
	toggleMode(widgetMode);
	untoggleMode(widgetMode);
}

void RoutingWidget::setZoomLevel(float level)
{
	if (level < 0.10)
		level = 0.10;
	if (level > 5.00)
		level = 5.00;

	zoomLevel = level;
	drawing->viewport()->update();
	drawing->resizeContents((int)(4000 * zoomLevel), (int)(3000 * zoomLevel));
}

void RoutingWidget::updateZoomLevelCtrl(int level)
{
	sbRoutingZoom->setValue(level);
}

float RoutingWidget::getZoomLevel()
{
	return zoomLevel;
}

void RoutingWidget::zoomValueChanged(int value)
{
	setZoomLevel(value / 100.0);
}

void RoutingWidget::zoomInClicked()
{
	setZoomLevel(zoomLevel + 0.10);
	updateZoomLevelCtrl((int)(zoomLevel * 100));
}

void RoutingWidget::zoomOutClicked()
{
	setZoomLevel(zoomLevel - 0.10);
	updateZoomLevelCtrl((int)(zoomLevel * 100));
}

void RoutingWidget::refreshDrawing(StrGlobal *str)
{
	structure = str;
	
	drawing->updateContents();
}

RoutingWidget::EditMode RoutingWidget::getWidgetMode()
{
	return widgetMode;
}

void RoutingWidget::putNewObjectAt(int xp, int yp)
{
	RSItemBaseWithType *newItem = NULL;
	
	switch (widgetMode)
	{
		case FX:
		case In:
		case Out:
			newItem = createNewIO(widgetMode);
			if (widgetMode == FX)
				structure->FXs.append((StrFX *)newItem);
			else if (widgetMode == In)
				structure->Inputs.append((StrInput *)newItem);
			else
				structure->Outputs.append((StrOutput *)newItem);
			break;
		case Effect:
			newItem = createNewPatch();
			structure->updatePatchesOrder();
			break;
		case Route:
			if (!drawing->isDragLink())
				startLinkDrag(xp, yp);
			return;
		default:
			break;
	}
	
	if (newItem)
	{
		if (newItem->type() != RSItemBaseWithType::Patch)
			structure->UsedItems.append(newItem);
		newItem->setPosition(xp, yp);
		
		drawing->updateContents();
	
		//modeNormalClicked();
	}
}




RoutingToolTip::RoutingToolTip(QWidget *widget, QToolTipGroup *group) : QToolTip(widget, group)
{
}

void RoutingToolTip::maybeTip(const QPoint &p)
{
	RoutingDrawWidget *drawWidget = (RoutingDrawWidget *)parentWidget();
	RoutingWidget *parent = (RoutingWidget *)drawWidget->parentWidget();
	StrGlobal *structure = parent->getStructure();
	
	if (!structure)
		return;
		
	int i, j;
	// find object under cursor
	int xp = p.x();
	int yp = p.y();
	
	drawWidget->viewportToContents(xp, yp, xp, yp);
	
	xp = parent->deZoomVal(xp);
	yp = parent->deZoomVal(yp);
	
	RSItemBaseWithType *item;

	for(item = structure->UsedItems.first(); item; item = structure->UsedItems.next())
	{
		if (item->containsPoint(xp, yp))
		{
			int ix;
			int iy;
			int iw;
			int ih;
			
			QString tipText = "";
			
			StrInput *in = NULL;
			StrOutput *out = NULL;
			StrFX *fx = NULL;
			StrPatch *patch = NULL;
			
			ix = item->x();
			iy = item->y();
			iw = item->width();
			ih = item->height();
			
			RSItemIO *pio = NULL;
			
			switch(item->type())
			{
				case RSItemBaseWithType::In:
					in = (StrInput *)item;
					tipText = QString("Input: %1\nName: %2").arg(in->num()).arg(in->name());
					break;
				case RSItemBaseWithType::Out:
					out = (StrOutput *)item;
					tipText = QString("Output: %1\nName: %2").arg(out->num()).arg(out->name());
					break;
				case RSItemBaseWithType::FX:
					fx = (StrFX *)item;
					tipText = QString("FX: %1\nName: %2").arg(fx->num()).arg(fx->name());
					break;
				case RSItemBaseWithType::Patch:
					patch = (StrPatch *)item;
					
					pio = patch->getIOAtPoint(xp, yp);
					if (pio)
					{
						if (pio->isOutput())
							tipText = QString("Patch output: %1\nOutput name: %2\n\n").arg(pio->getIdx()).arg(pio->getDesc());
						else
							tipText = QString("Patch input: %1\nInput name: %2\n\n").arg(pio->getIdx()).arg(pio->getDesc());
						
						QRect r;
						patch->getIORect(pio->isOutput(), pio->getIdx(), r);
						
						ix = r.x();
						iy = r.y();
						iw = r.width();
						ih = r.height();
					}
					else
					{
						// create region without ios
						QRegion reg(ix, iy, iw, ih);
						QRect ior;
						
						for (j = 0; j < 2; j++)
						{
							for (i = 0; i < patch->getMaxIOIdx(j); i++)
							{
								patch->getIORect(j, i, ior);
								reg = reg.subtract(QRegion(ior));
							}
						}
						
						// find rect with point
						QMemArray<QRect> rects = reg.rects();
						
						for (i = 0; i < (int)rects.count(); i++)
						{
							if (rects[i].contains(xp, yp))
							{
								ix = rects[i].x();
								iy = rects[i].y();
								iw = rects[i].width();
								ih = rects[i].height();
								break;
							}
						}
					}
					
					tipText = tipText + QString("Patch: %1\nOrder: %2\nName: %3").arg(patch->num()).arg(patch->order()).arg(patch->name());
					break;
				default:
					break;
			}
			
			if (tipText != "")
			{
				ix = parent->zoomVal(ix);
				iy = parent->zoomVal(iy);
				iw = parent->zoomVal(iw);
				ih = parent->zoomVal(ih);
			
				drawWidget->contentsToViewport(ix, iy, ix, iy);
				
				tip(QRect(ix, iy, iw, ih), tipText);
			}
		}
	}	
}
