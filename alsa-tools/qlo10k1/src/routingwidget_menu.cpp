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
 
#include <qpopupmenu.h>
#include <qmessagebox.h>
#include <qfiledialog.h>

#include "app_global.h"
#include "routingwidget.h"
#include "structure.h"
#include "structure_io.h"
#include "structure_link.h"
#include "structure_patch.h"
#include "strglobal.h"
#include "ld10k1file.h"

extern QString gLastFileDir;

void RoutingWidget::openObjectMenuAt(RSItemBaseWithType *item, MenuMode mm, int xp, int yp, int mxp, int myp)
{
	QPopupMenu *contextMenu = new QPopupMenu();
	
	enum Action {Refresh, ClearDSP, Delete, Rename, Disconnect, DelPoint, AddPoint, Connect, Save};
	
	int rn = -1;
	
	if (mm == MenuNone)
	{
		contextMenu->insertItem(tr("&Refresh"), Refresh);
		//contextMenu->insertItem(tr("&AutoArange")/*, this, SLOT(menuQuitActivated(int))*/);
		contextMenu->insertItem(tr("&Clear DSP"), ClearDSP);
	}
	else if (mm == MenuObjects)
	{
		contextMenu->insertItem(tr("&Delete (Unload)"), Delete);
	}
	else if (mm == MenuIO)
	{
		//contextMenu->insertItem(tr("&Rename"), Rename);
		
		bool isConnected = false;
		
		if (item->type() == RSItemBaseWithType::Out)
			isConnected = item->getIO(false, 0)->getConnectedTo() != NULL;
		else
			isConnected = item->getIO(true, 0)->getConnectedTo() != NULL;
		
		if (isConnected)
			contextMenu->insertItem(tr("D&isconnect"), Disconnect);
			
		contextMenu->insertSeparator();
		contextMenu->insertItem(tr("&Delete"), Delete);
	}
	else if (mm == MenuLink)
	{
		int pn = item->getHandlePointNum(xp, yp);
		if (pn > 0)
			contextMenu->insertItem(tr("D&elete point"), DelPoint);
		else if (pn < 0)
			contextMenu->insertItem(tr("A&dd point"), AddPoint);
		
		if (item->type() == RSItemBaseWithType::Link)
		{
			StrLink *lnk = (StrLink *)item;
			
			rn = lnk->getRouteNumFromPoint(xp, yp);
			if(rn >= 0)
			{
				RSItemIO *io = NULL;
				RSItemBaseWithType *own = NULL;
				io = lnk->getRoutePoint(rn); 
				if(io)
				{
					own = (RSItemBaseWithType*)io->getOwner();
					if(own && (own->type() == RSItemBaseWithType::Patch))
					  contextMenu->insertItem(tr("D&isconnect"), Disconnect);
				}
			}
		}
			
		contextMenu->insertSeparator();
		contextMenu->insertItem(tr("&Delete"), Delete);
	}
	else if (mm == MenuPatch)
	{
		//contextMenu->insertItem(tr("&Rename"), Rename);
		
		RSItemIO *io = item->getIOAtPoint(xp, yp);
		
		if (io)
		{
			//contextMenu->insertItem(tr("&IO Rename"), Rename);
			
			if (io->getConnectedTo())
				contextMenu->insertItem(tr("D&isconnect"), Disconnect);
			else
				contextMenu->insertItem(tr("&Connect"), Connect);
		}
		else
			contextMenu->insertItem(tr("&Save"), Save);
		
		contextMenu->insertSeparator();
		contextMenu->insertItem(tr("&Unload"), Delete);
	}
	
	
	
	int id = contextMenu->exec(QPoint(mxp, myp));
	
	delete contextMenu;
	
	if (id < 0)
		return;
		
	if (id == AddPoint)
	{
		StrLink *l = (StrLink *)item;
		if (l->addSegmentPoint(xp, yp))
		{
			l->updateContents(drawing, getZoomLevel());
		}
	}
	else if (id == DelPoint)
	{
		StrLink *l = (StrLink *)item;
		// FIXME - update before del
		l->updateContents(drawing, getZoomLevel());
		l->delSegmentPoint(l->getHandlePointNum(xp, yp));
		l->calcSize();
	}
	else if (id == Delete)
	{
		drawing->deleteAllSelected();
		structure->loadFromLD();
	}
	else if (id == Connect)
	{
		StrPatch *p = (StrPatch *)item;
		RSItemIO *pio = p->getIOAtPoint(xp, yp);
		if (pio)
			drawing->startLinkDrag(pio, xp, yp);
	}
	else if (id == Disconnect)
	{
		RSItemIO *io = NULL;
		if (item->type() == RSItemBaseWithType::Patch)
		{
			int err;
			StrPatch *p = (StrPatch *)item;
			io = p->getIOAtPoint(xp, yp);
			
			StrLink *l = io->getConnectedTo();
			// FIXME - update before disconnect
			l->updateContents(drawing, getZoomLevel());
			
			if ((err = structure->disconnectFromLink(io)) < 0)
			  QMessageBox::critical(0, APP_NAME, QString("Couldn't disconnect !\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
		}
		else
		{
			if (item->type() == RSItemBaseWithType::In)
			{
				StrInput *in = (StrInput *)item;
				io = in->getIO(true, 0);
			}
			else if (item->type() == RSItemBaseWithType::Out)
			{
				StrOutput *out = (StrOutput *)item;
				io = out->getIO(false, 0);
			}
			else if (item->type() == RSItemBaseWithType::FX)
			{
				StrFX *fx = (StrFX *)item;
				io = fx->getIO(true, 0);
			}
			
			if(io)
			{  
				StrLink *l = io->getConnectedTo();
				structure->deleteOneLink(l);
				structure->loadFromLD();
				drawing->updateContents();
			}  
			else if (item->type() == RSItemBaseWithType::Link)
			{
				StrLink *lnk = (StrLink *)item;
				
				if(rn >= 0)
				{  
					io = lnk->getRoutePoint(rn);
					if(io)
					{
						RSItemBaseWithType *own = NULL;
						own = (RSItemBaseWithType*)io->getOwner();
						if(own && (own->type() == RSItemBaseWithType::Patch))
						{
							int err;
							if ((err = structure->disconnectFromLink(io)) < 0)
								QMessageBox::critical(0, APP_NAME, QString("Couldn't disconnect !\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
			
							drawing->updateContents();
						}
					}
				}
			}
		}
	}
	else if (id == ClearDSP)
	{
		structure->clearDSP();
		drawing->updateContents();
	}
	else if (id == Refresh)
	{
		structure->loadFromLD();
		drawing->updateContents();
	}
	else if (id == Save)
	{
		StrPatch *p = (StrPatch *)item;
		int err;
		
		QFileDialog *fd = new QFileDialog(this, "file dialog", TRUE);
		fd->setMode(QFileDialog::AnyFile);
		fd->setFilter("ld10k1 Native effect files (*.ld10k1)");
		fd->setCaption("Save patch");
		fd->setDir(gLastFileDir);
		
		QString fileName;
		if (fd->exec() == QDialog::Accepted)
		{
			fileName = fd->selectedFile();
			gLastFileDir = fd->dirPath();
			delete fd;
			
			if (!fileName.endsWith(".ld10k1"))
				fileName += ".ld10k1";
			if (QFile::exists(fileName))
			{
				if (QMessageBox::question(0, APP_NAME, QString("File with name %1 exists. Overwrite ?").arg(fileName), QMessageBox::Yes, QMessageBox::Cancel) != QMessageBox::Yes)
					return;
			}
		
			LD10k1File *pf = NULL;
			if ((err = structure->get(p->num(), &pf)) < 0)
			{
				QMessageBox::critical(0, APP_NAME, QString("Couldn't get patch !\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
				return;
			}
			
			if ((err = pf->SaveToFile(fileName)) < 0)
				QMessageBox::critical(0, APP_NAME, QString("Couldn't save patch\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
			delete pf;
		}
		else
			delete fd;
	}
}
