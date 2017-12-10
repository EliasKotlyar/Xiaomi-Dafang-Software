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
 
#ifndef LOADPATCHDIALOG_H
#define LOADPATCHDIALOG_H

#include <qstringlist.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include "dlgs/loadpatchdialogbase.h"
#include "filecontroldialog.h"
#include "connectionwizard.h"
#include "structure.h"
#include "structure_patch.h"
#include "ld10k1file.h"

class LoadPatchDlg : public LoadPatchDialogBase
{
	Q_OBJECT
	StrGlobal *global;
	LD10k1File *file;
	QString fileName;
	QStringList inputNames;
	QStringList outputNames;

	QValueList <int> ctrlFileList;
	QValueList <TranslatedCtrl> translatedList;
	
	QPtrList <StrPatch> sortedPatches;
	
	StrPatch *before;
	
	void setBefore(int i1, int i2);
public:

	LoadPatchDlg(StrGlobal *glob, QString efileName, LD10k1File *efile);

	QString getName(int i);
	StrPatch *whereLoad();
public slots:
	void okClicked();
	void cancelClicked();

	void inputsDoubleClicked(QListBoxItem *item);
	void outputsDoubleClicked(QListBoxItem *item);

	void fileSelectionChanged();
	
	void whereActivated(int index);
	void befAfterActivated(int index);
};

#endif // LOADPATCHDIALOG_H
