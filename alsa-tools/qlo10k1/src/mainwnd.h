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
 
#ifndef MAINWND_H
#define MAINWND_H

#include "dlgs/mainwndbase.h"
#include "structure.h"
#include "strglobal.h"

class QPopupMenu;
class RoutingWidget;

class MainWnd : public MainWndBase
{
	Q_OBJECT
	StrCardGlobal *cardGlobal;
	StrGlobal *CurrentCard;

	QPopupMenu *cardMenu;
	QValueList <int> cardMenuItems;
	RoutingWidget *routing;
public:
	MainWnd(void);
	~MainWnd(void);
	
	void enableTabs(bool en);
	
	void createCardMenu(void);
	void loadPatchesList(QListView *lv);
	void refreshState();

	void saveSettings();
	void loadSettings();
	
	void patchesUpdateText();

protected:
	void closeEvent(QCloseEvent*);

public slots:
	// menu
	void menuLoadDSPConfigActivated(int id);
	void menuSaveDSPConfigActivated(int id);
	void menuQuitActivated(int id);
	
	
	void menuCardActivated(int id);
	void menuSettingsActivated(int id);
	void menuHelpAboutActivated(int id);

	void menuCardClearDSPActivated(int id);
	void menuCardRefreshActivated(int id);

	void tabMainCurrentChanged(QWidget *tab);

	void loadClicked();
	void unloadClicked();

	void patchDoubleClicked(QListViewItem *item);
};

#endif // MAINWND_H
