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
 
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qapplication.h>
#include <qtabwidget.h>
#include <qlistview.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qlayout.h>
#include <qsettings.h>

#include "app_global.h"
#include "mainwnd.h"
#include "preferences.h"
#include "loadpatchdialog.h"
#include "transformpatchdialog.h"
#include "aboutdialog.h"
#include "routingwidget.h"
#include "structure_patch.h"
#include "strparam.h"

QString gLastFileDir;

class PatchesListViewItem : public QListViewItem
{
public:
	StrPatch *patch;
	
	PatchesListViewItem(StrPatch *p, QListView *parent) : QListViewItem(parent)
	{
		patch = p;
		updateText();
	}
	
	PatchesListViewItem(StrPatch *p, QListView *parent, PatchesListViewItem *after) : QListViewItem(parent, after)
	{
		patch = p;
		updateText();
	}
	
	void updateText()
	{
		setText(0,QString().sprintf("%03d", patch->order()));
		setText(1,QString().sprintf("%03d", patch->num()));
		setText(2,patch->name());
	}
};

MainWnd::MainWnd() : MainWndBase()
{
	cardGlobal = new StrCardGlobal();
	CurrentCard = NULL;
	
	// try to load settings
	loadSettings();

	// main menu
	QPopupMenu *fileMenu = new QPopupMenu();
	fileMenu->insertItem(tr("&Load DSP config"), this, SLOT(menuLoadDSPConfigActivated(int)));
	fileMenu->insertItem(tr("&Save DSP config"), this, SLOT(menuSaveDSPConfigActivated(int)));
	fileMenu->insertSeparator();
	fileMenu->insertItem(tr("&Quit"), this, SLOT(menuQuitActivated(int)));

	menubar->insertItem(tr("&File"), fileMenu);

	cardMenu = new QPopupMenu();
	menubar->insertItem(tr("&Card"), cardMenu);
	cardMenu->setCheckable(true);

	QPopupMenu *settingMenu = new QPopupMenu();
	settingMenu->insertItem(tr("&Main settings"), this, SLOT(menuSettingsActivated(int)));
	menubar->insertItem(tr("&Settings"), settingMenu);

	QPopupMenu *helpMenu = new QPopupMenu();
	helpMenu->insertItem(tr("&About"), this, SLOT(menuHelpAboutActivated(int)));
	menubar->insertItem(tr("&Help"), helpMenu);

	// connect signals
	connect((QObject *)tabMain, SIGNAL(currentChanged(QWidget *)), this, SLOT(tabMainCurrentChanged(QWidget *)));

	createCardMenu();
	enableTabs(false);

	setCaption(tr(APP_NAME));

	connect(pbLoad, SIGNAL(clicked()), this, SLOT(loadClicked()));
	connect(pbUnload, SIGNAL(clicked()), this, SLOT(unloadClicked()));

	connect(lvPatches, SIGNAL(doubleClicked(QListViewItem *)), this, SLOT(patchDoubleClicked(QListViewItem *)));

	routing = new RoutingWidget(tabRouting);

	tabRoutingLayout->addWidget(routing, 0, 0);
	routing->show();
}

MainWnd::~MainWnd()
{
	CurrentCard = NULL;
	delete cardGlobal;
}

void MainWnd::closeEvent(QCloseEvent* ce)
{
	saveSettings();
	ce->accept();
}

void MainWnd::tabMainCurrentChanged(QWidget *tab)
{
	if (!CurrentCard)
		return;
	if (tab == tabLoad)
	{
		loadPatchesList(lvPatches);
	}
	else if (tab == tabRouting)
	{
		routing->refreshDrawing(CurrentCard);
	}
}

void MainWnd::loadPatchesList(QListView *lv)
{
	StrPatch *patch;
	
	lv->clear();
	PatchesListViewItem *after = NULL;
	for(patch = CurrentCard->Patches.first(); patch; patch = CurrentCard->Patches.next() )
	{
		if (after)
			after = new PatchesListViewItem(patch, lv, after);
		else
			after = new PatchesListViewItem(patch, lv);
	}
}

void MainWnd::patchesUpdateText()
{
	QListViewItemIterator it(lvPatches);
	while (it.current())
	{
		PatchesListViewItem *i = (PatchesListViewItem *)it.current();
		i->updateText();
		++it;
	}
}

void MainWnd::menuQuitActivated(int /*id*/)
{
	saveSettings();
	qApp->quit();
}

void MainWnd::menuLoadDSPConfigActivated(int id)
{
	if (!CurrentCard)
		return;
	QFileDialog *fd = new QFileDialog(this, "file dialog", TRUE);
	fd->setDir(gLastFileDir);
	fd->setMode(QFileDialog::ExistingFile);
	fd->setFilter("DSP config (*.ld10k1)");
	fd->setCaption("Save DSP config");
	int err = 0;

	QString fileName;
	if (fd->exec() == QDialog::Accepted)
	{
		fileName = fd->selectedFile();
		gLastFileDir = fd->dirPath();
		delete fd;
		
		LD10k1DspFile *dc = NULL;
		if ((err = dc->LoadFromFile(fileName, &dc)) < 0)
		{
			QMessageBox::critical(0, APP_NAME, QString("Couldn't load DSP config\n(ld10k1 error:%1)").arg(CurrentCard->errorStr(err)));
			return;
		}
		
		if ((err = CurrentCard->putDspConfig(dc)) < 0)
			QMessageBox::critical(0, APP_NAME, QString("Couldn't put DSP config\n(ld10k1 error:%1)").arg(CurrentCard->errorStr(err)));
		delete dc;
		CurrentCard->clear();
		CurrentCard->loadFromLD();
		refreshState();
	}
	else
		delete fd;
}

void MainWnd::menuSaveDSPConfigActivated(int id)
{
	if (!CurrentCard)
		return;
	QFileDialog *fd = new QFileDialog(this, "file dialog", TRUE);
	fd->setDir(gLastFileDir);
	fd->setMode(QFileDialog::AnyFile);
	fd->setFilter("DSP config (*.ld10k1)");
	fd->setCaption("Save DSP config");
	int err = 0;

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
			if (QMessageBox::question(0, APP_NAME, QString("File with name %1 exist. Overwite ?").arg(fileName), QMessageBox::Yes, QMessageBox::Cancel) != QMessageBox::Yes)
				return;
		}
		
		LD10k1DspFile *dc = NULL;
		if ((err = CurrentCard->getDspConfig(&dc)) < 0)
		{
			QMessageBox::critical(0, APP_NAME, QString("Couldn't get DSP config\n(ld10k1 error:%1)").arg(CurrentCard->errorStr(err)));
			return;
		}
		
		if ((err = dc->SaveToFile(fileName)) < 0)
			QMessageBox::critical(0, APP_NAME, QString("Couldn't save DSP config\n(ld10k1 error:%1)").arg(CurrentCard->errorStr(err)));
		delete dc;
	}
	else
		delete fd;
}

void MainWnd::refreshState()
{
	tabMainCurrentChanged(tabMain->currentPage());
}

void MainWnd::menuCardActivated(int id)
{
	CardParam *card;
	CardParam *old_card, *new_card;
	int item_id = 0, old_item_id = 0;

	new_card = NULL;
	if (CurrentCard && CurrentCard->Card->isConnected())
		old_card = CurrentCard->Card;
	else
		old_card = NULL;

	for (unsigned int i = 0; i < cardMenuItems.count(); i++)
	{
		item_id = cardMenuItems[i];
		card = cardGlobal->Cards.at(i);
		if (cardMenu->isItemChecked(item_id))
			old_item_id = item_id;

		if (id == item_id)
			new_card = card;
	}

	if (new_card && new_card != old_card)
	{
		if (!new_card->connect())
		{
			cardMenu->setItemChecked(id, true);
			enableTabs(true);
			CurrentCard = new_card->getStructure();
			// FIXME - refresh data
			CurrentCard->loadFromLD();
			refreshState();

			if (old_card)
			{
				old_card->disconnect();
				cardMenu->setItemChecked(old_item_id, false);
			}
		}
	}
}

void MainWnd::menuSettingsActivated(int /*id*/)
{
	PreferencesDlg d(cardGlobal);
	int item_id;
	
	if (CurrentCard && CurrentCard->Card->isConnected())
	{
		CurrentCard->Card->disconnect();
		CurrentCard = NULL;
		enableTabs(false);
		
		for (unsigned int i = 0; i < cardMenuItems.count(); i++)
		{
			item_id = cardMenuItems[i];
			cardMenu->setItemChecked(item_id, false);
		}
	}

	if (d.exec() == QDialog::Accepted)
	{
		saveSettings();
		createCardMenu();
	}
}

void MainWnd::menuHelpAboutActivated(int /* id */)
{
	AboutDlg d;
	d.exec();
}

void MainWnd::menuCardClearDSPActivated(int /* id */)
{
	if (CurrentCard)
	{
		CurrentCard->clearDSP();
		refreshState();
	}
}

void MainWnd::menuCardRefreshActivated(int /* id */)
{
	if (CurrentCard)
		CurrentCard->loadFromLD();
	
	refreshState();
}

void MainWnd::enableTabs(bool en)
{
	if (en)
	{
		tabLoad->setEnabled(true);
		tabRouting->setEnabled(true);
	}
	else
	{
		tabLoad->setEnabled(false);
		tabRouting->setEnabled(false);
	}
}

void MainWnd::createCardMenu(void)
{
	CardParam *card;
	int id;
	
	for (unsigned int i = 0; i < cardMenuItems.count() ;i++)
	{
		cardMenu->disconnectItem(cardMenuItems[i], this, SLOT(menuCardActivated(int)));
	}
	
	cardMenuItems.clear();
	cardMenu->clear();
	for (unsigned int i = 0; i < cardGlobal->Cards.count() ;i++)
	{
		card = cardGlobal->Cards.at(i);
		id = cardMenu->insertItem(card->CardName);
		cardMenuItems.append(id);
		cardMenu->connectItem(id, this, SLOT(menuCardActivated(int)));
	}
	
	// special items
	cardMenu->insertSeparator();
	cardMenu->insertItem(tr("&Refresh"), this, SLOT(menuCardRefreshActivated(int)));
	cardMenu->insertItem(tr("&Clear DSP"), this, SLOT(menuCardClearDSPActivated(int)));
}

void MainWnd::loadClicked()
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
	int err = 0;

	QString fileName;
	if ( fd->exec() == QDialog::Accepted )
	{
        	fileName = fd->selectedFile();
		gLastFileDir = fd->dirPath();
		delete fd;
		LD10k1File *ldfile = NULL;
		/* Try loading as an ld10k1 file first. */
		if ((err = LD10k1File::LoadFromFile(fileName, &ldfile)) < 0) 
		{
			EMU10k1File *emufile = NULL;
			if ((err = EMU10k1File::LoadFromFile(fileName, &emufile)) < 0) 
			{
				QMessageBox::critical(0, APP_NAME, QString("Couldn't load patch\n(ld10k1 error:%1)").arg(CurrentCard->errorStr(err)));
				return;
			}
			else
			{
				TransformPatchDlg d(fileName, emufile);
				if (d.exec() == QDialog::Accepted)
				{
					if ((err = d.transformFile(&ldfile)) < 0)
					{
						QMessageBox::critical(0, APP_NAME, QString("Couldn't transform patch\n(ld10k1 error:%1)").arg(CurrentCard->errorStr(err)));
						delete emufile;
						return;
					}
					delete emufile;
				}
				else
				{
					delete emufile;
					return;
				}
			}
		}
		
		LoadPatchDlg d(CurrentCard, fileName, ldfile);

		if (d.exec() == QDialog::Accepted)
		{
			// and now convert to loader
			StrPatch *loaded = NULL;
			if ((err = CurrentCard->load(ldfile, d.whereLoad(), &loaded)) < 0)
				QMessageBox::critical(0, APP_NAME, QString("Couldn't load patch\n(ld10k1 error:%1)").arg(CurrentCard->errorStr(err)));
			else
				refreshState();
		}
		delete ldfile;
	}
	else
		delete fd;
}

void MainWnd::unloadClicked()
{
	PatchesListViewItem *current = (PatchesListViewItem *)lvPatches->currentItem();
	int err;

	if (current)
	{
		// find patch index
		StrPatch *p = current->patch;
		if ((err = CurrentCard->unload(p)))
			QMessageBox::critical(0, APP_NAME, QString("Couldn't unload patch\n(ld10k1 error:%1)").arg(CurrentCard->errorStr(err)));
		else
			refreshState();
	}
}

void MainWnd::patchDoubleClicked(QListViewItem *item)
{
	/*int patch_num = item->text(0).toInt();
	int err;

	liblo10k1_dsp_patch_t *patch = NULL;
	if ((err = global->CurrentCard->get(patch_num, &patch)))
	{
		QMessageBox::critical(0, APP_NAME, QString("Couldn't get patch\n(ld10k1 error:%1)").arg(global->CurrentCard->error_str(err)));
		return;
	}*/
}

void MainWnd::saveSettings()
{
	QSettings settings(QSettings::Ini);
	settings.setPath("pzad", "qlo10k1", QSettings::User);
	
	settings.beginGroup("/qlo10k1");
	settings.writeEntry("/Version", "0.0.1");
	settings.writeEntry("/RepDirSystem", cardGlobal->RepDirSystem);
	settings.writeEntry("/RepDirUser", cardGlobal->RepDirUser);
	settings.writeEntry("/LastDir", gLastFileDir);
	
	// first save cards
	settings.beginGroup("/Cards");
	settings.writeEntry("/CardCount", (int)cardGlobal->Cards.count());
	for (unsigned int i = 0; i < cardGlobal->Cards.count(); i++)
	{
		settings.beginGroup(QString("/Card_%1").arg(i));
		CardParam *card = cardGlobal->Cards.at(i);
		
		settings.writeEntry("/Name", card->CardName);
		settings.writeEntry("/Socket", card->CardSocket);
			
		settings.endGroup();
	}
	// write current card	
	settings.endGroup();
}

void MainWnd::loadSettings()
{
	QSettings settings(QSettings::Ini);
	settings.setPath("pzad", "qlo10k1", QSettings::User);
	settings.beginGroup("/qlo10k1");
	
	QString version = settings.readEntry("/Version", "");
	if (version == "")
	{
		// settings doesn't exists
		cardGlobal->RepDirSystem = "";
		cardGlobal->RepDirUser = "";
		gLastFileDir = "./";
		
		CardParam *card = new CardParam();
		card->CardName = "Default card";
		card->CardSocket = "/tmp/.ld10k1_port";
		
		if (!card->testConnection())
		{
			cardGlobal->Cards.append(card);
		}
	}
	else
	{
		cardGlobal->RepDirSystem = settings.readEntry("/RepDirSystem", "");
		cardGlobal->RepDirUser = settings.readEntry("/RepDirUser", "");
		gLastFileDir = settings.readEntry("/LastDir", "./");
	
		settings.beginGroup("/Cards");
		
		unsigned int cardCount = settings.readNumEntry("/CardCount", 0);
		for (unsigned int i = 0; i < cardCount; i++)
		{
			settings.beginGroup(QString("/Card_%1").arg(i));
		
			CardParam *card = new CardParam();
			card->CardName = settings.readEntry("/Name", "");
			card->CardSocket = settings.readEntry("/Socket", "");
			
			if (!card->testConnection())
			{
				cardGlobal->Cards.append(card);
			}
				
			settings.endGroup();
		}
		
		
		settings.endGroup();
	}
}
