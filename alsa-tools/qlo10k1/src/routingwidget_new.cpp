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
 
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qpopupmenu.h>

#include "app_global.h"
#include "routingwidget.h"
#include "structure_io.h"
#include "structure_link.h"
#include "newiodialog.h"
#include "loadpatchdialog.h"
#include "transformpatchdialog.h"

extern QString gLastFileDir;

RSItemBaseWithType *RoutingWidget::createNewIO(EditMode em)
{
	// show io dialog
	NewIODlg::DlgType t = NewIODlg::FX;
	
	switch (em)
	{
		case FX:
			t = NewIODlg::FX;
			break;
		case In:
			t = NewIODlg::In;
			break;
		case Out:
			t = NewIODlg::Out;
			break;
		default:
			break;
	}
	
	NewIODlg d(structure, t);

	int err;
	err = d.init();
	if(err < 0)
	{
		QMessageBox::critical(0, APP_NAME, QString("Error creating new IO dialog\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
		return NULL;
	}
	
	if (d.exec() ==  QDialog::Accepted)
		return d.getNewIO();
	else
		return NULL;
}

RSItemBaseWithType *RoutingWidget::createNewPatch()
{
	QFileDialog *fd = new QFileDialog(this, "file dialog", TRUE);
	fd->setMode(QFileDialog::ExistingFile);
	QStringList filterlist;
	filterlist << QString( "as10k1 Patch files (*.bin *.as10k1 *.emu10k1)" );
	filterlist << QString( "ld10k1 Native effect files (*.ld10k1)" );
	filterlist << QString( "All Files (*)" );
	QString filters = filterlist.join( ";;" );
	fd->setFilters( filters );
	fd->setDir(gLastFileDir);
	
	fd->setCaption("Load patch");

	StrPatch *loaded = NULL;
	int err;
					
	QString fileName;
	if ( fd->exec() == QDialog::Accepted )
	{
        	fileName = fd->selectedFile();
		gLastFileDir = fd->dirPath();
		delete fd;
		
		LD10k1File *ldfile = NULL;
		if ((err = LD10k1File::LoadFromFile(fileName, &ldfile)) < 0) 
		{
			EMU10k1File *emufile = NULL;
			if ((err = EMU10k1File::LoadFromFile(fileName, &emufile)) < 0) 
			{
				QMessageBox::critical(0, APP_NAME, QString("Couldn't load patch\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
				return NULL;
			}
			else
			{
				TransformPatchDlg d(fileName, emufile);
				if (d.exec() == QDialog::Accepted)
				{
					if ((err = d.transformFile(&ldfile)) < 0)
					{
						QMessageBox::critical(0, APP_NAME, QString("Couldn't transform patch\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
						delete emufile;
						return NULL;
					}
					delete emufile;
				}
				else
				{
					delete emufile;
					return NULL;
				}
			}
		}
		
		LoadPatchDlg d(structure, fileName, ldfile);

		if (d.exec() == QDialog::Accepted)
		{
			// and now convert to loader
			StrPatch *loaded = NULL;
			if ((err = structure->load(ldfile, d.whereLoad(), &loaded)) < 0) {
				QMessageBox::critical(0, APP_NAME, QString("Couldn't load patch\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
				delete ldfile;
				return NULL;
			}
			else
			{
				loaded->calcSize();
				loaded->updateContents(this->drawing, getZoomLevel());
			}
		}
		delete ldfile;		
	}
	return loaded;
}

void RoutingWidget::startLinkDrag(int xp, int yp)
{
	// find patch under - only on patch can start drag
	
	RSItemBaseWithType *item = NULL;
	
	for(item = structure->UsedItems.first(); item; item = structure->UsedItems.next())
	{
		if (item->containsPoint(xp, yp) && item->type() == RSItemBaseWithType::Patch)
		{
			// I have patch - find io
			RSItemIO *pio = ((StrPatch *)item)->getIOAtPoint(xp, yp);
			if (pio)
			{
				drawing->startLinkDrag(pio, xp, yp);
				return;
			}
		}
	}
}

void RoutingDrawWidget::connectLinkDrag(int xp, int yp, int mxp, int myp)
{
	RoutingWidget *parent = (RoutingWidget *)parentWidget();
	StrGlobal *structure = parent->getStructure();
	
	if (!structure)
		return;
		
	RSItemBaseWithType *item = NULL;
	
	for(item = structure->UsedItems.first(); item; item = structure->UsedItems.next())
	{
		if (item->containsPoint(xp, yp))
		{
			// I have patch - find io
			RSItemIO *pio = NULL;
			if (item->type() == RSItemBaseWithType::Patch)
				pio = ((StrPatch *)item)->getIOAtPoint(xp, yp);
			else if (item->type() == RSItemBaseWithType::In)
				pio = ((StrInput *)item)->getIO(true, 0);
			else if (item->type() == RSItemBaseWithType::Out)
				pio = ((StrInput *)item)->getIO(false, 0);
			else if (item->type() == RSItemBaseWithType::FX)
				pio = ((StrFX *)item)->getIO(true, 0);
			
			
			if (pio)
			{
				// check if connecting in to out or out to in
				if (pio->isOutput() != linkStart->isOutput())
				{
					if (pio->getConnectedTo() == linkStart->getConnectedTo() && pio->getConnectedTo() && linkStart->getConnectedTo())
						return;
						
					if (pio->getOwner() == linkStart->getOwner())
						return;
						
					QPopupMenu *contextMenu = new QPopupMenu();
					enum Action {Normal, SimpleMulti, Simple, Multi};
					
					contextMenu->insertItem(tr("&Normal"), Normal);
					contextMenu->insertItem(tr("&Multi"), Multi);
					if (((RSItemBaseWithType *)pio->getOwner())->type() == RSItemBaseWithType::Patch)
						contextMenu->insertItem(tr("S&imple"), Simple);
					contextMenu->insertItem(tr("&Simple + Multi"), SimpleMulti);
										
					
					int id  = contextMenu->exec(QPoint(mxp, myp));
					if (id < 0)
						return;
					
					int conn_id;
					int err;
					
					QValueList <int> actIds;
					
					actIds.clear();
					
					StrLink *linkTo = pio->getConnectedTo();
					if (linkTo && actIds.findIndex(linkTo->id()) < 0)
						actIds.append(linkTo->id());
					
					StrLink *linkFrom = linkStart->getConnectedTo();
					if (linkFrom && actIds.findIndex(linkFrom->id()) < 0)
						actIds.append(linkFrom->id());
						
					if (id == Normal && linkTo && ((RSItemBaseWithType *)pio->getOwner())->type() != RSItemBaseWithType::Patch)
					{
						if ((err = structure->deleteOneLink(linkTo)) < 0)
							QMessageBox::critical(0, APP_NAME, QString("Couldn't disconnect !\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
					}
					
					if ((err = structure->conAdd(id != Simple && id != Normal,
						 id != Multi && id != Normal,
						 linkStart, pio, &conn_id)) < 0)
						 QMessageBox::critical(0, APP_NAME, QString("Couldn't connect !\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
					else
					{
						if (actIds.findIndex(conn_id) < 0)
							actIds.append(conn_id);
							
						if (linkFrom)
							linkFrom->updateContents(this, parent->getZoomLevel());
						if (linkTo)
							linkTo->updateContents(this, parent->getZoomLevel());
						
						for (unsigned int i = 0; i < actIds.count(); i++)
						{
							StrLink *link = NULL;
							int aid = actIds[i];
							if ((err = structure->actualizeLink(aid, &link)) < 0 && aid == conn_id)
								QMessageBox::critical(0, APP_NAME, QString("Couldn't actualize link !\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
								
							if (link)
							{
								if (!link->isValid())
									structure->destroyLink(link);
								else
								{
									//wasn't error
									link->calcSize();
									link->updateContents(this, parent->getZoomLevel());
								}
							}							
						}
						
						stopLinkDrag();
						//parent->modeNormalClicked();
					}
					
					delete contextMenu;
					
					return;
				}
				return;
			}
		}
	}
}

void RoutingDrawWidget::deleteAllSelected()
{
	// through all selected
	RoutingWidget *parent = (RoutingWidget *)parentWidget();
	StrGlobal *structure = parent->getStructure();
	
	int err;
	
	if (!structure)
		return;
	
	if ((err = structure->deleteAllSelected()) < 0)
		QMessageBox::critical(0, APP_NAME, QString("Couldn't delete selected items !\n(ld10k1 error:%1)").arg(structure->errorStr(err)));
	
	this->updateContents();
}
