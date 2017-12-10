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
 
#ifndef PATCHDIALOG_H
#define PATCHDIALOG_H

#include "dlgs/patchdialogbase.h"
#include "strglobal.h"

class PatchDlg : public PatchDialogBase
{
	Q_OBJECT
	StrGlobal *global;
	/*EMU10k1File *file;
	QString fileName;*/
	QStringList inputNames;
	QStringList outputNames;

	/*QValueList <int> ctrlFileList;
	QValueList <TranslatedCtrl> translatedList;
	QValueList <RouteInfo> inRoutes;
	QValueList <RouteInfo> outRoutes;*/

	//LD10k1Route getRoute(bool out, int idx);
	StrPatch *patch;
public:

	PatchDlg(StrGlobal *glob, StrPatch *p);

	QString getName(int i);
	QString getRouteName(bool output, int i);

	/*void enableSBLive(bool en);
	void enableAudigy(bool en);

	void transformFile(LD10k1File **outFile, QValueList <LD10k1Route> *routes);*/
public slots:
	void okClicked();
	void cancelClicked();

	void inputsDoubleClicked(QListBoxItem *item);
	void outputsDoubleClicked(QListBoxItem *item);

	/*void ctrlAddClicked();
	void ctrlDelClicked();

	void loadedDoubleClicked(QListBoxItem *item);
	void fileDoubleClicked(QListBoxItem *item);*/

	void connInputsDoubleClicked(QListBoxItem *item);
	void connOutputsDoubleClicked(QListBoxItem *item);

	/*void insertInRepStateChanged(int state);

	void archBothClicked();
	void archEmu10k1Clicked();
	void archEmu10k2Clicked();*/
};

#endif // PATCHDIALOG_H

