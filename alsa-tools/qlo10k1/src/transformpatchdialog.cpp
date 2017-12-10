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
#include "transformpatchdialog.h"
#include "structure_patch.h"
#include "structure_io.h"

TransformPatchDlg::TransformPatchDlg(QString efileName, EMU10k1File *efile)
	: TransformPatchBase()
{
	fileName = efileName;
	file = efile;

	leFileName->setText(fileName);
	lePatchName->setText(file->getPatchName());
	
	// load controls list
	int i;
	for (i = 0; i < file->getCtrlCount(); i++)
	{
		QString text = getName(i);
		lbFileCtrl->insertItem(text);
		ctrlFileList.append(i);
	}
	
	// set default in names
	switch (file->getInCount())
	{
		case 1:
			inputNames.append("I");
			break;
		case 2:
			inputNames.append("IL");
			inputNames.append("IR");
			break;
		case 4:
			inputNames.append("IL");
			inputNames.append("IR");
			inputNames.append("IRL");
			inputNames.append("IRR");
			break;
		case 6:
			inputNames.append("IL");
			inputNames.append("IR");
			inputNames.append("IRL");
			inputNames.append("IRR");
			inputNames.append("IC");
			inputNames.append("ILFE");
			break;
		default:
			for (i = 0; i < file->getInCount(); i++)
				inputNames.append("");
			break;
	}
	
	// set default out names
	switch (file->getOutCount())
	{
		case 1:
			outputNames.append("O");
			break;
		case 2:
			outputNames.append("OL");
			outputNames.append("OR");
			break;
		case 4:
			outputNames.append("OL");
			outputNames.append("OR");
			outputNames.append("ORL");
			outputNames.append("ORR");
			break;
		case 6:
			outputNames.append("OL");
			outputNames.append("OR");
			outputNames.append("ORL");
			outputNames.append("ORR");
			outputNames.append("OC");
			outputNames.append("OLFE");
			break;
		default:
			for (i = 0; i < file->getOutCount(); i++)
				outputNames.append("");
			break;
	}

	for (i = 0; i < file->getInCount(); i++)
		lbInputs->insertItem(inputNames[i]);

	for (i = 0; i < file->getOutCount(); i++)
		lbOutputs->insertItem(outputNames[i]);
	
	connect(pbOK, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

	connect(lbInputs, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(inputsDoubleClicked(QListBoxItem *)));
	connect(lbOutputs, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(outputsDoubleClicked(QListBoxItem *)));

	connect(pbCtrlAdd, SIGNAL(clicked()), this, SLOT(ctrlAddClicked()));
	connect(pbCtrlDel, SIGNAL(clicked()), this, SLOT(ctrlDelClicked()));

	connect(lbLoadedCtrl, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(loadedDoubleClicked(QListBoxItem *)));
	connect(lbFileCtrl, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(fileDoubleClicked(QListBoxItem *)));
}

QString TransformPatchDlg::getName(int i)
{
	EMU10k1Ctrl ctrl;

	file->getCtrl(i, &ctrl);
	QString name = QString(ctrl.name) +  " - " + QString().setNum(ctrl.value) +
		" (" + QString().setNum(ctrl.min) + "," + + QString().setNum(ctrl.max) + ")";
	return name;
}

void TransformPatchDlg::okClicked()
{
	// load patch
	done(Accepted);
}

void TransformPatchDlg::cancelClicked()
{
	done(Rejected);
}

void TransformPatchDlg::inputsDoubleClicked(QListBoxItem *item)
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

void TransformPatchDlg::outputsDoubleClicked(QListBoxItem *item)
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

void TransformPatchDlg::ctrlAddClicked()
{
	QValueList <int> selectedCtrl, *tmpList;

	int i;
	unsigned int min = 0;
	unsigned int max = 0;
	bool first = true;
	for (i = 0; i < (int)lbFileCtrl->count(); i++)
	{
		if (lbFileCtrl->isSelected(i))
		{
			selectedCtrl.append(ctrlFileList[i]);
			
			EMU10k1Ctrl ctrl;
			file->getCtrl(ctrlFileList[i], &ctrl);
			
			if (first)
			{
				min = ctrl.min;
				max = ctrl.max;
			}
			else
			{
				if (min != ctrl.min ||
					max != ctrl.max)
				{
					QMessageBox::critical(0, APP_NAME,
        					QString("Wrong range for control %1").arg(ctrl.name));
					return;
				}
			}
			first = false;
		}
	}
	
	if (selectedCtrl.count() > 0)
	{
		TranslatedCtrl ctrl;
		tmpList = &selectedCtrl;
		
		for (;;)
		{
			FileControlDlg d(tmpList, file, &ctrl);
	
			if (d.exec() == QDialog::Accepted)
			{
				bool found = false;
				// check name - in translated
				for (i = 0; i < (int)translatedList.count(); i++)
				{
					if (translatedList[i].name == ctrl.name)
					{
						found = true;
						break;	
					}
				}
				
				// check in other
				for (i = 0; i < (int)ctrlFileList.count(); i++)
				{
					EMU10k1Ctrl ctrl1;
					file->getCtrl(ctrlFileList[i], &ctrl1);
					if (selectedCtrl.findIndex(ctrlFileList[i]) < 0 && ctrl1.name == ctrl.name)
					{
						found = true;
						break;
					}
				}				
				
				if (found)
				{
					QMessageBox::critical( 0, APP_NAME,
        					QString("There already is control with this name %1").arg(ctrl.name));
					tmpList = NULL;
					continue;
				}
				else
				{
					// add to translated
					translatedList.append(ctrl);
					// remove selected controls from ctrlList
					for (i = 0; i < (int)selectedCtrl.count(); i++)
					{
						int fidx = ctrlFileList.findIndex(selectedCtrl[i]);
						if (fidx >= 0)
						{
							ctrlFileList.remove(selectedCtrl[i]);
							lbFileCtrl->removeItem(fidx);
						}
					}
					lbLoadedCtrl->insertItem(ctrl.name);
					
					break;
				}
			}
			else
				break;
		}
	}
}

void TransformPatchDlg::ctrlDelClicked()
{
	QValueList <int> selectedCtrl;

	int i, j;
	
	for (i = 0; i < (int)lbLoadedCtrl->count(); i++)
	{
		if (lbLoadedCtrl->isSelected(i))
		{
			selectedCtrl.append(i);
		}
	}
	
	if (selectedCtrl.count() > 0)
	{
		// add controls to FileList
		for (i = 0; i < (int)selectedCtrl.count(); i++)
		{
			TranslatedCtrl ctrl = translatedList[selectedCtrl[i]];
			
			for (j = 0; j < (int)ctrl.fileCtrlIdx.count(); j++)
			{
				lbFileCtrl->insertItem(getName(ctrl.fileCtrlIdx[j]));
				ctrlFileList.append(ctrl.fileCtrlIdx[j]);
			}
		}
		
		// delete selected
		for (i = 0; i < (int)selectedCtrl.count(); i++)
		{
			translatedList.remove(translatedList.at(selectedCtrl[i]));
			lbLoadedCtrl->removeItem(selectedCtrl[i]);
		}
	}
}

void TransformPatchDlg::loadedDoubleClicked(QListBoxItem *item)
{
	int idx = lbLoadedCtrl->index(item);
	int i;
	
	TranslatedCtrl ctrl = translatedList[idx];
		
	for (;;)
	{
		FileControlDlg d(NULL, file, &ctrl);
	
		if (d.exec() == QDialog::Accepted)
		{
			bool found = false;
			// check name - in translated
			for (i = 0; i < (int)translatedList.count(); i++)
			{
				if (i != idx && translatedList[i].name == ctrl.name)
				{
					found = true;
					break;	
				}
			}
				
			// check in other
			for (i = 0; i < (int)ctrlFileList.count(); i++)
			{
				EMU10k1Ctrl ctrl1;
				file->getCtrl(ctrlFileList[i], &ctrl1);
				
				if (ctrl.name == ctrl.name)
				{
					found = true;
					break;
				}
			}				
				
			if (found)
			{
				QMessageBox::critical( 0, APP_NAME,
        			QString("There already is control with this name %1").arg(ctrl.name));
				continue;
			}
			else
			{
				// update name
				lbLoadedCtrl->changeItem(ctrl.name, idx);
				translatedList[idx] = ctrl;					
				break;
			} 
		}
		else
			break;
	}
}

void TransformPatchDlg::fileDoubleClicked(QListBoxItem *item)
{
	bool ok;
	int idx = lbFileCtrl->index(item);
	EMU10k1Ctrl ctrl;
	file->getCtrl(ctrlFileList[idx], &ctrl);
    	QString text = ctrl.name;
	text = QInputDialog::getText(
            APP_NAME, "Control name:", QLineEdit::Normal,
            text, &ok, this );
    	if (ok && !text.isEmpty())
	{
		ctrl.name= text;
		file->setCtrl(ctrlFileList[idx], &ctrl);
		lbFileCtrl->changeItem(getName(idx), idx);
	}
}

int TransformPatchDlg::transformFile(LD10k1File **outFile)
{
	// controls - translated
	int i, j, err;
	
	LD10k1File *of = NULL;
	
	LD10k1FileTransfCtl translCtls[32];
	for (i = 0; i < (int)translatedList.count(); i++)
	{
		TranslatedCtrl tctrl = translatedList[i];
		
		translCtls[i].emu_ctl_count = (int)tctrl.fileCtrlIdx.count();
		for (j = 0; j < translCtls[i].emu_ctl_count; j++)
			translCtls[i].emu_ctls[j] = tctrl.fileCtrlIdx[j];
			
		translCtls[i].ctl_name = tctrl.name;
	}
	
	if ((err = LD10k1File::transformFromEmuFile(file, translCtls, (int)translatedList.count(), outFile)) < 0)
		return err;
		
	of = *outFile;
		
	// rename inputs
	for (i = 0; i < of->getIOCount(false); i++)
		of->setIOName(false, i, inputNames[i]);
	
	// rename output
	for (i = 0; i < of->getIOCount(true); i++)
		of->setIOName(true, i, outputNames[i]);
	
	// set others parameters for patch controls	
	for (i = 0; i < (int)translatedList.count(); i++)
	{
		TranslatedCtrl tctrl = translatedList[i];
		
		switch (tctrl.translation)
		{
			case TranslatedCtrl::None:
				of->setCtlTranslation(i, LD10k1File::None);
				break;
			case TranslatedCtrl::Table100:
				of->setCtlTranslation(i, LD10k1File::Table100);
				break;
			case TranslatedCtrl::Bass:
				of->setCtlTranslation(i, LD10k1File::Bass);
				break;
			case TranslatedCtrl::Treble:
				of->setCtlTranslation(i, LD10k1File::Treble);
				break;
			case TranslatedCtrl::OnOff:
				of->setCtlTranslation(i, LD10k1File::OnOff);
				break;
		}
		of->setCtlValVCount(i, tctrl.visible);
		for (j = 0; j < (int)tctrl.values.count(); j++)
			of->setCtlVal(i, j, tctrl.values[j]);
	}
	
	return 0;
}
