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
 
#ifndef FILECONTROLDIALOG_H
#define FILECONTROLDIALOG_H

#include <qvaluelist.h>
#include "dlgs/filecontroldialogbase.h"
#include "ld10k1file.h"

class TranslatedCtrl
{
public:
	QString name;
	int index;
	
	enum TranslationType {None, Table100, Bass, Treble, OnOff };
	TranslationType translation;
	int visible;
	int min;
	int max;
	QValueList <int> fileCtrlIdx;
	QValueList <int> values;
};

class FileControlDlg : public FileControlDialogBase
{
	Q_OBJECT
	
	QValueList <int> selected;
	EMU10k1File *file;
	TranslatedCtrl *translated;
	unsigned int min;
	unsigned int max;
	QValueList <int> values;
public:
	FileControlDlg(QValueList <int> *eselected, EMU10k1File *efile, TranslatedCtrl *etranslated);
	
	QString getName(int i);
public slots:
	void okClicked();
	void cancelClicked();
	
	void upClicked();
	void downClicked();
};

#endif // FILECONTROLDIALOG_H
