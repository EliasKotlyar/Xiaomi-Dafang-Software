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
 
#include <qpushbutton.h>
#include <qlistview.h>

#include "newiodialog.h"
#include "routingwidget.h"
#include "structure_io.h"
#include "strparam.h"

class IOListViewItem : public QListViewItem
{
public:
	int num;
	QString name;
	
	IOListViewItem(int n, QString na, QListView *parent) : QListViewItem(parent)
	{
		num = n;
		name = na;
		updateText();
	}
	
	IOListViewItem(int n, QString na, QListView *parent, IOListViewItem *after) : QListViewItem(parent, after)
	{
		num = n;
		name = na;
		updateText();
	}
	
	void updateText()
	{
		setText(0, QString().sprintf("%03d", num));
		setText(1, name);
	}
};

NewIODlg::NewIODlg(StrGlobal *glob, DlgType t)
	: NewIODialogBase()
{
	global = glob;
	dt = t;
	
	connect(pbOK, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

int NewIODlg::init()
{
	int err;
	
	QString title;
	QString columnTitle;
	
	int cnt;
	if (dt == In)
		err = global->Card->getInputCount(&cnt);
	else if (dt == Out)
		err = global->Card->getOutputCount(&cnt);
	else
		err = global->Card->getFXCount(&cnt);
	
	if(err < 0)
	  return err;
		
	switch (dt)
	{
		case In:
			title = "New input";
			columnTitle = "Input";
			break;
		case Out:
			title = "New output";
			columnTitle = "Output";
			break;
		case FX:
			title = "New FX bus";
			columnTitle = "FX bus";
			break;
	}
	
	IOListViewItem *after = NULL;
	
	lvIOs->clear();
	
	for (int i = 0; i < cnt; i++)
	{
		QString ioname;
		bool used = false;
		
		if (dt == In)
			used = global->findInputByNum(i) != NULL;
		else if (dt == Out)
			used = global->findOutputByNum(i) != NULL;
		else
			used = global->findFXByNum(i) != NULL;
		
		if (!used)
		{
			if (dt == In)
				err = global->Card->getInput(i, ioname);
			else if (dt == Out)
				err = global->Card->getOutput(i, ioname);
			else
				err = global->Card->getFX(i, ioname);
			
			if(err < 0)
			  goto Error;	
			
			if (after)
				after = new IOListViewItem(i, ioname, lvIOs, after);
			else
				after = new IOListViewItem(i, ioname, lvIOs);
		}
	}
	
	setCaption(title);
	lvIOs->setColumnText(1, columnTitle);
	
	connect(lvIOs, SIGNAL(selectionChanged(QListViewItem *)), this, SLOT(ioSelectionChanged(QListViewItem *)));
	
	return 0;
	
	Error:
	
	return err;
}

void NewIODlg::okClicked()
{
	done(Accepted);
}

void NewIODlg::cancelClicked()
{
	done(Rejected);
}

void NewIODlg::ioSelectionChanged(QListViewItem *item)
{
	IOListViewItem *ioitem = (IOListViewItem *)item;
	
	num = ioitem->num;
	name = ioitem->name;
	pbOK->setEnabled(true);
}

StrIOBase *NewIODlg::getNewIO()
{
	if (dt == In)
		return new StrInput(num, name);
	else if (dt == Out)
		return new StrOutput(num, name);
	else
		return new StrFX(num, name);
}
