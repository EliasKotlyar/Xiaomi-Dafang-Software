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
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include "filecontroldialog.h"

FileControlDlg::FileControlDlg(QValueList <int> *eselected, EMU10k1File *efile, TranslatedCtrl *etranslated)
{
	file = efile;
	translated = etranslated;

	int ctrlIdx;
	unsigned int i;
	
	if (eselected)
	{
		selected = *eselected;
		// insert file controls to listbox
		for (i = 0; i < selected.count(); i++)
		{
			ctrlIdx = selected[i];
			lbControls->insertItem(getName(ctrlIdx));
		}
		
		sbIndex->setValue(-1);

		sbVisible->setMinValue(1);
		sbVisible->setMaxValue(selected.count());
		sbVisible->setValue(selected.count());

		// set min, max
		EMU10k1Ctrl ctrl;
		file->getCtrl(selected[0], &ctrl);
		min = ctrl.min;
		max = ctrl.max;
		values.append(ctrl.value);
		for (i = 1; i < selected.count(); i++)
		{
			file->getCtrl(selected[i], &ctrl);
			values.append(ctrl.value);
			if (ctrl.max > max)
				max = ctrl.max;
			if (ctrl.min < min)
				min = ctrl.min;
		}
		
		if (min == 0 && max == 100)
			cbTranslation->setCurrentItem(1);
	}
	else
	{
		selected = translated->fileCtrlIdx;
		for (i = 0; i < translated->fileCtrlIdx.count(); i++)
		{
			lbControls->insertItem(getName(translated->fileCtrlIdx[i]));
		}

		sbVisible->setMinValue(1);
		sbVisible->setMaxValue(translated->fileCtrlIdx.count());
		sbVisible->setValue(translated->visible);

		leLoadedName->setText(translated->name);
		sbIndex->setValue(translated->index);

		switch(translated->translation)
		{
			case TranslatedCtrl::None:
				cbTranslation->setCurrentItem(0);
				break;
			case TranslatedCtrl::Table100:
				cbTranslation->setCurrentItem(1);
				break;
			case TranslatedCtrl::Bass:
				cbTranslation->setCurrentItem(2);
				break;
			case TranslatedCtrl::Treble:
				cbTranslation->setCurrentItem(3);
				break;
			case TranslatedCtrl::OnOff:
				cbTranslation->setCurrentItem(4);
				break;
		}

		for (unsigned int i = 0; i < translated->values.count(); i++)
			values.append(translated->values[i]);
		min = translated->min;
		max = translated->max;
	}

	connect(pbOK, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

	connect(pbUp, SIGNAL(clicked()), this, SLOT(upClicked()));
	connect(pbDown, SIGNAL(clicked()), this, SLOT(downClicked()));
}

QString FileControlDlg::getName(int i)
{
	EMU10k1Ctrl ctrl;

	file->getCtrl(i, &ctrl);
	QString name = QString(ctrl.name) +  " - " + QString().setNum(ctrl.value) +
		" (" + QString().setNum(ctrl.min) + "," + + QString().setNum(ctrl.max) + ")";
	return name;
}

void FileControlDlg::okClicked()
{
	translated->name = leLoadedName->text();
	translated->index = sbIndex->value();
	
	switch(cbTranslation->currentItem())
	{
		case 0:
			translated->translation = TranslatedCtrl::None;
			break;
		case 1:
			translated->translation = TranslatedCtrl::Table100;
			break;
		case 2:
			translated->translation = TranslatedCtrl::Bass;
			break;
		case 3:
			translated->translation = TranslatedCtrl::Treble;
			break;
		case 4:
			translated->translation = TranslatedCtrl::OnOff;
			break;
	}

	translated->fileCtrlIdx = selected;
	translated->visible = sbVisible->value();
	translated->values.clear();
	for (unsigned int i = 0; i < values.count(); i++)
		translated->values.append(values[i]);

	translated->min = min;
	translated->max = max;
	done(Accepted);
}

void FileControlDlg::cancelClicked()
{
	done(Rejected);
}
	
void FileControlDlg::upClicked()
{
	int idx = lbControls->currentItem();
	if (idx > 0)
	{
		int tmp = selected[idx - 1];
		selected[idx - 1] = selected[idx];
		selected[idx] = tmp;
		
		// update labels
		lbControls->changeItem(getName(selected[idx - 1]), idx - 1);
		lbControls->changeItem(getName(selected[idx]), idx);
		lbControls->setCurrentItem(idx - 1);
	}
}

void FileControlDlg::downClicked()
{
	int idx = lbControls->currentItem();
	if (idx >= 0 && idx < (int)lbControls->count() - 1)
	{
		int tmp = selected[idx + 1];
		selected[idx + 1] = selected[idx];
		selected[idx] = tmp;
		
		// update labels
		lbControls->changeItem(getName(selected[idx]), idx);
		lbControls->changeItem(getName(selected[idx + 1]), idx + 1);
		lbControls->setCurrentItem(idx + 1);
	}
}
