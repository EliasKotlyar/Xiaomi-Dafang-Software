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

#include "patchdialog.h"
#include "structure_patch.h"

PatchDlg::PatchDlg(StrGlobal *glob, StrPatch *p)
	: PatchDialogBase()
{
	global = glob;
	patch = p;

	lePatchName->setText(p->name());

	// load controls list
	/*
	FIXME
	int i;
	for (i = 0; i < file->ctrl_count; i++)
	{
		QString text = getName(i);
		lbFileCtrl->insertItem(text);
		ctrlFileList.append(i);
	}*/

	// set default in names
	/* FIXME
	switch (file->in_count)
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
			for (i = 0; i < file->in_count; i++)
				inputNames.append("");
			break;
	}

	// set default out names
	switch (file->out_count)
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
			for (i = 0; i < file->out_count; i++)
				outputNames.append("");
			break;
	}

	for (i = 0; i < file->in_count; i++)
	{
		lbInputs->insertItem(inputNames[i]);

		RouteInfo r;
		inRoutes.append(r);

		lbConnInputs->insertItem(getRouteName(false, i));
	}

	for (i = 0; i < file->out_count; i++)
	{
		lbOutputs->insertItem(outputNames[i]);

		RouteInfo r;
		outRoutes.append(r);

		lbConnOutputs->insertItem(getRouteName(true, i));
	}
	*/

	connect(pbOK, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

	connect(lbInputs, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(inputsDoubleClicked(QListBoxItem *)));
	connect(lbOutputs, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(outputsDoubleClicked(QListBoxItem *)));

	/*connect(pbCtrlAdd, SIGNAL(clicked()), this, SLOT(ctrlAddClicked()));
	connect(pbCtrlDel, SIGNAL(clicked()), this, SLOT(ctrlDelClicked()));

	connect(lbLoadedCtrl, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(loadedDoubleClicked(QListBoxItem *)));
	connect(lbFileCtrl, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(fileDoubleClicked(QListBoxItem *)));
*/
	connect(lbConnInputs, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(connInputsDoubleClicked(QListBoxItem *)));
	connect(lbConnOutputs, SIGNAL(doubleClicked(QListBoxItem *)), this, SLOT(connOutputsDoubleClicked(QListBoxItem *)));
/*
	connect(cbInsertInRep, SIGNAL(stateChanged(int)), this, SLOT(insertInRepStateChanged(int)));

	connect(rbArchBoth, SIGNAL(clicked()), this, SLOT(archBothClicked()));
	connect(rbArchEmu10k1, SIGNAL(clicked()), this, SLOT(archEmu10k1Clicked()));
	connect(rbArchEmu10k2, SIGNAL(clicked()), this, SLOT(archEmu10k2Clicked()));

	tabRepository2->setEnabled(false);*/
}

/*QString LoadPatchDlg::getName(int i)
{
	EMU10k1CtrlReg *ctrl;

	ctrl = &(file->ctrls[i]);
	QString name = QString(ctrl->name) +  " - " + QString().setNum(ctrl->value) +
		" (" + QString().setNum(ctrl->min) + "," + + QString().setNum(ctrl->max) + ")";
	return name;
}*/

/*QString LoadPatchDlg::getRouteName(bool output, int i)
{
	QString name;
	RouteInfo ri;

	if (output)
	{
		name = outputNames[i];
		ri = outRoutes[i];
	}
	else
	{
		name = inputNames[i];
		ri = inRoutes[i];
	}

	switch (ri.io)
	{
		case RouteInfo::None:
			break;
		case RouteInfo::FX:
			name += QString(" -> FX:") + global->Registers.at(ri.reg)->name();
			break;
		case RouteInfo::In:
			name += QString(" -> In:") + global->Registers.at(ri.reg)->name();
			break;
		case RouteInfo::Out:
			name += QString(" -> Out:") + global->Registers.at(ri.reg)->name();
			break;
		case RouteInfo::PatchIn:
		case RouteInfo::PatchOut:
			StrPatch *patch = global->Patches.at(ri.patch);
			if (ri.io == RouteInfo::PatchIn)
				name += QString(" -> In:") + patch->name() + "-" + patch->Registers.at(ri.reg)->name();
			else
				name += QString(" -> Out:") + patch->name() + "-" + patch->Registers.at(ri.reg)->name();
			break;
	}

	return name;
}*/

void PatchDlg::okClicked()
{
	// update routing and rename patch
	done(Accepted);
}

void PatchDlg::cancelClicked()
{
	done(Rejected);
}

void PatchDlg::inputsDoubleClicked(QListBoxItem *item)
{
	/*bool ok;
	int idx = lbInputs->index(item);
    QString text = inputNames[idx];
	text = QInputDialog::getText(
            APP_NAME, "Input name:", QLineEdit::Normal,
            text, &ok, this );
    if (ok && !text.isEmpty())
	{
		inputNames[idx] = text;
		lbInputs->changeItem(text, idx);
		lbConnInputs->changeItem(getRouteName(false, idx), idx);
	}*/
}

void PatchDlg::outputsDoubleClicked(QListBoxItem *item)
{
	/*bool ok;
	int idx = lbOutputs->index(item);
    QString text = outputNames[idx];
	text = QInputDialog::getText(
            APP_NAME, "Output name:", QLineEdit::Normal,
            text, &ok, this );
    if (ok && !text.isEmpty())
	{
		outputNames[idx] = text;
		lbOutputs->changeItem(text, idx);
		lbConnOutputs->changeItem(getRouteName(false, idx), idx);
	}*/
}


void PatchDlg::connInputsDoubleClicked(QListBoxItem *item)
{
	/*int idx = lbConnInputs->index(item);
	RouteInfo ri = inRoutes[idx];

	ConnectionWizard d(false, global, &ri);

	if (d.exec() == QDialog::Accepted)
	{
		inRoutes[idx] = ri;
		lbConnInputs->changeItem(getRouteName(false, idx), idx);
	}
	*/
}

void PatchDlg::connOutputsDoubleClicked(QListBoxItem *item)
{
	/*int idx = lbConnOutputs->index(item);
	RouteInfo ri = outRoutes[idx];

	ConnectionWizard d(true, global, &ri);

	if (d.exec() == QDialog::Accepted)
	{
		outRoutes[idx] = ri;
		lbConnOutputs->changeItem(getRouteName(true, idx), idx);
	}*/
}


/*LD10k1Route LoadPatchDlg::getRoute(bool out, int idx)
{
	RouteInfo *route;
	if (out)
		route = &(outRoutes[idx]);
	else
		route = &(inRoutes[idx]);

	LD10k1Route nroute;

	nroute.PatchTo = route->patch;
	nroute.Multi = route->multi;

	if (route->patch >= 0 && route->reg >= 0)
	{
		StrPatch *patch = global->Patches.at(route->patch);
		StrRegister *reg = patch->Registers.at(route->reg);
		nroute.RegTo = reg->idx();
	}
	else if (route->reg >= 0)
	{
		StrRegister *reg = global->Registers.at(route->reg);
		nroute.RegTo = reg->phIdx();
	}
	else
	{
		nroute.RegTo = -1;
	}

	return nroute;
}
*/
