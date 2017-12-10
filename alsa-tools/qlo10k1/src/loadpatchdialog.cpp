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
 
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qinputdialog.h>
#include <qvaluelist.h>
#include <qmessagebox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qcombobox.h>
#include <qmap.h>

#include "app_global.h"
#include "loadpatchdialog.h"
#include "structure_patch.h"
#include "structure_io.h"

LoadPatchDlg::LoadPatchDlg(StrGlobal *glob, QString efileName, LD10k1File *efile)
	: LoadPatchDialogBase()
{
	global = glob;
	fileName = efileName;
	file = efile;

	leFileName->setText(fileName);
	lePatchName->setText(file->getPatchName());
	
	// load controls list
	int i;
	for (i = 0; i < file->getCtlCount(); i++)
	{
		QString text = getName(i);
		lbFileCtrl->insertItem(text);
		ctrlFileList.append(i);
	}

	for (i = 0; i < file->getIOCount(false); i++)
		inputNames.append(file->getIOName(false, i));
	for (i = 0; i < file->getIOCount(true); i++)
		outputNames.append(file->getIOName(true, i));
	
	for (i = 0; i < file->getIOCount(false); i++)
		lbInputs->insertItem(inputNames[i]);

	for (i = 0; i < file->getIOCount(true); i++)
		lbOutputs->insertItem(outputNames[i]);
	
	// create sorted patch list
	sortedPatches.clear();
	
	for (StrPatch *onePatch = global->Patches.first(); onePatch; onePatch = global->Patches.next())
	{
		i = 0;
		bool found = false;
		for (StrPatch *secondPatch = sortedPatches.first(); secondPatch; secondPatch = sortedPatches.next())
		{
			if (onePatch->order() < secondPatch->order())
			{
				sortedPatches.insert(i, onePatch);
				found = true;
				break;
			}
			i++;
		}
		
		if (!found)
			sortedPatches.append(onePatch);
	}
	
	// fill combobox
	cbWhere->clear();
	
	for (StrPatch *p = sortedPatches.first(); p; p = sortedPatches.next())
		cbWhere->insertItem(QString().sprintf("%03d  %03d  ", p->order(), p->num()) + p->name());
		
	cbWhere->setCurrentItem(sortedPatches.count() - 1);
	cbBefAfter->setCurrentItem(1);
	
	setBefore(1, sortedPatches.count() - 1);
	
	connect(pbOK, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

	connect(lbInputs, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(inputsDoubleClicked(QListBoxItem *)));
	connect(lbOutputs, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(outputsDoubleClicked(QListBoxItem *)));

	connect(lbFileCtrl, SIGNAL(selectionChanged()), this, SLOT(fileSelectionChanged()));
	
	connect(cbBefAfter, SIGNAL(activated(int)), this, SLOT(befAfterActivated(int)));
	connect(cbWhere, SIGNAL(activated(int)), this, SLOT(whereActivated(int)));
}

QString LoadPatchDlg::getName(int i)
{
	return file->getCtlName(i);
}

void LoadPatchDlg::okClicked()
{
	// load patch
	done(Accepted);
}

void LoadPatchDlg::cancelClicked()
{
	done(Rejected);
}

void LoadPatchDlg::inputsDoubleClicked(QListBoxItem *item)
{
	bool ok;
	int idx = lbInputs->index(item);
    	QString text = inputNames[idx];
	text = QInputDialog::getText(
            	APP_NAME, "Input name:", QLineEdit::Normal,
        	text, &ok, this );
    	if (ok && !text.isEmpty()) 
	{
		inputNames[idx] = text;
		lbInputs->changeItem(text, idx);
	}
}

void LoadPatchDlg::outputsDoubleClicked(QListBoxItem *item)
{
	bool ok;
	int idx = lbOutputs->index(item);
	QString text = outputNames[idx];
	text = QInputDialog::getText(
            	APP_NAME, "Output name:", QLineEdit::Normal,
            	text, &ok, this );
	if (ok && !text.isEmpty()) 
	{
		outputNames[idx] = text;
		lbOutputs->changeItem(text, idx);
	}
}

void LoadPatchDlg::fileSelectionChanged()
{
	QListBoxItem *item = lbFileCtrl->selectedItem();
	if (!item)
	{
		leTranslation->setText("");
		leMin->setText("");
		leMax->setText("");
		lbValues->clear();
		lbValues->setEnabled(false);
	}
	else
	{
		int iidx = lbFileCtrl->index(item);
		int i;
		QString transl;
		
		switch (file->getCtlTranslation(iidx))
		{
			case LD10k1File::None:
				transl = "None";
				break;
			case LD10k1File::Table100:
				transl = "Table100";
				break;
			case LD10k1File::Bass:
				transl = "Bass";
				break;
			case LD10k1File::Treble:
				transl = "Treble";
				break;
			case LD10k1File::OnOff:
				transl = "On/Off";
				break;
		}
		
		leTranslation->setText(transl);
		leMin->setText(QString().setNum(file->getCtlMin(iidx)));
		leMax->setText(QString().setNum(file->getCtlMax(iidx)));
		lbValues->clear();
		
		for (i = 0; i < file->getCtlValVCount(iidx); i++)
			lbValues->insertItem(QString("%1 : %2").arg(i).arg(file->getCtlVal(iidx, i)));
		
		lbValues->setEnabled(false);
	}
}

StrPatch *LoadPatchDlg::whereLoad()
{
	return before;
}

void LoadPatchDlg::setBefore(int i1, int i2)
{
	if (i1)
	{
		if (i2 < (int)sortedPatches.count() - 1)
			before = sortedPatches.at(i2 + 1);
		else
			before = NULL;
	}
	else
		before = sortedPatches.at(i2);
}

void LoadPatchDlg::whereActivated(int index)
{
	 setBefore(cbBefAfter->currentItem(), index);
}

void LoadPatchDlg::befAfterActivated(int index)
{
	setBefore(index, cbWhere->currentItem());
}
