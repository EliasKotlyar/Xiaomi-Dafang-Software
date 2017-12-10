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
 
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlistbox.h>

#include "structure_patch.h"
#include "structure_io.h"
#include "connectionwizard.h"

ConnectionWizard::ConnectionWizard(bool o, StrGlobal *g, RouteInfo *r)
	: ConnectionWizardBase()
{
	out = o;
	global = g;
	route = *r;
	rptr = r;
	
	rtype = None;
	pto = NULL;
	
	RSItemIO *io = r->getToIO();
	
	if (io)
	{
		RSItemBaseWithType *owner = (RSItemBaseWithType *)io->getOwner();
		switch (owner->type())
		{
			case RSItemBaseWithType::In:
				rtype = In;
				break;
			case RSItemBaseWithType::Out:
				rtype = Out;
				break;
			case RSItemBaseWithType::FX:
				rtype = FX;
				break;
			case RSItemBaseWithType::Patch:
				if (io->isOutput())
					rtype = POut;
				else
					rtype = PIn;
				pto = (StrPatch *)owner;
				break;
			default:
				break;
		}
	}

	if (!out)
	{
		rbOutput->hide();
	}
	else
	{
		rbFX->hide();
		rbInput->hide();
	}
	
	StrPatch *patch;
	
	lbPatch->clear();
	for(patch = global->Patches.first(); patch; patch = global->Patches.next() )
	{
		lbPatch->insertItem(patch->name());
		patchList.append(patch);
	}
	
	connect(rbNone, SIGNAL(clicked()), this, SLOT(noneClicked()));
	connect(rbFX, SIGNAL(clicked()), this, SLOT(fxClicked()));
	connect(rbInput, SIGNAL(clicked()), this, SLOT(inClicked()));
	connect(rbOutput, SIGNAL(clicked()), this, SLOT(outClicked()));
	connect(rbPatch, SIGNAL(clicked()), this, SLOT(patchClicked()));
	
	connect(lbPatch, SIGNAL(clicked(QListBoxItem *)), this, SLOT(patchListClicked(QListBoxItem *)));
	connect(lbEnd, SIGNAL(clicked(QListBoxItem *)), this, SLOT(endListClicked(QListBoxItem *)));
	
	connect(finishButton(), SIGNAL(clicked()), this, SLOT(finishClicked()));
	
	connect(this, SIGNAL(selected(const QString &)), this, SLOT(selected(const QString &)));
	helpButton()->hide();
	
	setFinishEnabled(pageType, true);
	setNextEnabled(pageType, true);
	setFinishEnabled(pagePatch, true);
	setNextEnabled(pagePatch, true);
	setFinishEnabled(pageEnd, true);
	setNextEnabled(pageEnd, true);
}

void ConnectionWizard::setState(bool patch, bool end, bool back, bool next, bool finish)
{
	setAppropriate(pagePatch, patch ? true : false);
	setAppropriate(pageEnd, end ? true : false);
	
	backButton()->setEnabled(back ? true : false);
	nextButton()->setEnabled(next ? true : false);
	finishButton()->setEnabled(finish ? true : false);
}

void ConnectionWizard::loadEnd(IOType iotype)
{
	lbEnd->clear();
	ios.clear();
	
	if (iotype == In)
	{
		for (int i = 0; i < (int)global->Inputs.count(); i++)
		{
			StrInput *io = global->Inputs.at(i);
			ios.append(io->getIO(false, 0));
			lbEnd->insertItem(io->name());
		}
	}
	else if (iotype == Out)
	{
		for (int i = 0; i < (int)global->Outputs.count(); i++)
		{
			StrOutput *io = global->Outputs.at(i);
			ios.append(io->getIO(true, 0));
			lbEnd->insertItem(io->name());
		}
	}
	else if (iotype == FX)
	{
		for (int i = 0; i < (int)global->FXs.count(); i++)
		{
			StrFX *io = global->FXs.at(i);
			ios.append(io->getIO(false, 0));
			lbEnd->insertItem(io->name());
		}
	}
}

void ConnectionWizard::noneClicked()
{
	setState(false, false, false, false, true);
	if (rtype != ConnectionWizard::None)
	{
		rtype = ConnectionWizard::None;
		pto = NULL;
	}
}


void ConnectionWizard::fxClicked()
{
	setState(false, true, false, true, false);
	if (rtype != ConnectionWizard::FX)
	{
		rtype = ConnectionWizard::FX;
		pto = NULL;
	}
}

void ConnectionWizard::inClicked()
{
	setState(false, true, false, true, false);
	if (rtype != ConnectionWizard::In)
	{
		rtype = ConnectionWizard::In;
		pto = NULL;
	}
}

void ConnectionWizard::outClicked()
{
	setState(false, true, false, true, false);
	if (rtype != ConnectionWizard::Out) 
	{
		rtype = ConnectionWizard::Out;
		pto = NULL;
	}
}

void ConnectionWizard::patchClicked()
{
	setState(true, true, false, true, false);
	if (rtype != ConnectionWizard::PIn && rtype != ConnectionWizard::POut)
	{
		if (out)
			rtype = ConnectionWizard::PIn;
		else
			rtype = ConnectionWizard::POut;
		pto = NULL;
	}
}

void ConnectionWizard::selected(const QString &page)
{
	if (page == "Connection target")
	{
		switch (rtype)
		{
			case None:
				rbNone->setChecked(true);
				setState(false, false, false, false, true);
				break;
			case FX:
				rbFX->setChecked(true);
				setState(false, true, false, true, false);
				break;
			case In:
				rbInput->setChecked(true);
				setState(false, true, false, true, false);
				break;
			case Out:
				rbOutput->setChecked(true);
				setState(false, true, false, true, false);
				break;
			case PIn:
			case POut:
				rbPatch->setChecked(true);
				setState(true, true, false, true, false);
				break;
		}
	}
	else if (page == "Patch")
	{
		finishButton()->setEnabled(false);
		if (pto)
		{
			// find patch
			lbPatch->setCurrentItem(patchList.find(pto));
			nextButton()->setEnabled(true);
		}
		else
		{
			QListBoxItem *si = lbPatch->selectedItem();
			if (si)
				lbPatch->setSelected(si, false);
			lbPatch->setCurrentItem(-1);
			nextButton()->setEnabled(false);
		}

		cbMulti->setChecked(route.getMulti());
		cbSimple->setChecked(route.getSimple());
	}
	else if (page == "End")
	{
		nextButton()->setEnabled(false);
		switch (rtype)
		{
			case None:
				break;
			case FX:
			case In:
			case Out:
				loadEnd(rtype);
				break;
			case PIn:
			case POut:
				// load patch in or out
				ios.clear();
				lbEnd->clear();

				for (int i = 0; i < (int)pto->getMaxIOIdx(out); i++)
				{
					RSItemIO *io = pto->getIO(out, i);
					if (io)
					{
						ios.append(io);
						lbEnd->insertItem(io->getDesc());
					}
				}
				
				route.setMulti(cbMulti->isChecked());
				route.setSimple(cbSimple->isChecked());
				break;
		}

		if (route.getToIO())
		{
			lbEnd->setCurrentItem(ios.find(route.getToIO()));
			finishButton()->setEnabled(true);
		}
		else
		{
			QListBoxItem *si = lbEnd->selectedItem();
			if (si)
				lbEnd->setSelected(si, false);
			lbEnd->setCurrentItem(-1);
			finishButton()->setEnabled(false);
		}
	}
}

void ConnectionWizard::patchListClicked(QListBoxItem *item)
{
	pto = patchList.at(lbPatch->index(item));
	route.setToIO(NULL);
	nextButton()->setEnabled(true);
}

void ConnectionWizard::endListClicked(QListBoxItem *item)
{
	route.setToIO(ios.at(lbEnd->index(item)));
	finishButton()->setEnabled(true);
}

void ConnectionWizard::finishClicked()
{
	*rptr = route;
}
