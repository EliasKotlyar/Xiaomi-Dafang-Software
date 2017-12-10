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
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qgroupbox.h>
#include <qmessagebox.h>
#include <qinputdialog.h>
#include <qlineedit.h>
#include "preferences.h"
#include "app_global.h"
#include "strparam.h"

PreferencesDlg::PreferencesDlg(StrCardGlobal *glob)
{
	global = glob;
	connect(pbOK, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
	
	connect(lbCards, SIGNAL(highlighted(int)), this, SLOT(cardHighlighted(int)));
	
	connect(pbUpdate, SIGNAL(clicked()), this, SLOT(updateClicked()));
	connect(pbCardAdd, SIGNAL(clicked()), this, SLOT(cardAddClicked()));
	connect(pbCardDel, SIGNAL(clicked()), this, SLOT(cardDelClicked()));
	
	// copy card list
	for (unsigned int i = 0; i < global->Cards.count(); i++)
		cards.append(new CardParam(global->Cards.at(i)));
	
	refreshListBox();
	
	if (cards.count() > 1)
		pbCardDel->setEnabled(true);
	repDirSystem = global->RepDirSystem;
	repDirUser = global->RepDirUser;		
	
	leRepSystem->setText(repDirSystem);
	leRepUser->setText(repDirUser);	
}

void PreferencesDlg::okClicked()
{
	unsigned int i;
	CardParam *card;
	for (i = 0; i < cards.count(); i++)
	{
		// test connection
		card = cards.at(i);
		if (card->testConnection())
		{
			// error
			QMessageBox::critical( 0, APP_NAME,
        		QString("Couldn't connect to socket %1 for card %2").arg(card->CardSocket).arg(card->CardName));
			return;
		}
	}
	
	// copy values to global
	global->Cards.clear();
	for (i = 0; i < cards.count(); i++)
		global->Cards.append(new CardParam(cards.at(i)));
		
	repDirSystem = leRepSystem->text();
	repDirUser = leRepUser->text();
	global->RepDirSystem = repDirSystem;
	global->RepDirUser = repDirUser;
	
	done(Accepted);
}

void PreferencesDlg::cancelClicked()
{
	done(Rejected);
}

void PreferencesDlg::refreshListBox(void)
{
	lbCards->clear();
	
	CardParam *card;
	QString desc;
	
	for (unsigned int i = 0; i < cards.count() ;i++)
	{
		card = cards.at(i);
		desc = card->CardName + " - " + card->CardSocket;
		lbCards->insertItem(desc);	
	}
}

void PreferencesDlg::refreshListBox(int idx)
{
	CardParam *card;
	QString desc;
	
	card = cards.at(idx);
	desc = card->CardName + " - " + card->CardSocket;
	lbCards->changeItem(desc, idx);
}


void PreferencesDlg::cardHighlighted(int index)
{
	CardParam *card;
	
	if (index >= 0 && (unsigned int)index < cards.count())
	{
		card = cards.at(index);
		leCardName->setText(card->CardName);
		leCardSocket->setText(card->CardSocket);
		gbCard->setEnabled(true);
	}
	else
	{
		leCardName->setText("");
		leCardSocket->setText("");
		gbCard->setEnabled(false);
	}
}

void PreferencesDlg::updateClicked()
{
	int idx = lbCards->currentItem();
	
	if (idx >=0 && (unsigned int)idx < cards.count())
	{
		CardParam *card;
		
		card = cards.at(idx);
		
		card->CardName = leCardName->text();
		card->CardSocket = leCardSocket->text();
		refreshListBox(idx);
	}
}

void PreferencesDlg::cardAddClicked()
{
	CardParam *card = new CardParam();
	card->CardName = "Default card";
	card->CardSocket = "/tmp/.ld10k1_port";
	cards.append(card);
	refreshListBox();
}

void PreferencesDlg::cardDelClicked()
{
	int idx = lbCards->currentItem();
	
	if (idx >= 0)
	{
		cards.remove(idx);
		
		if (cards.count() <= 1)
			pbCardDel->setEnabled(false);
		refreshListBox();
	}
}
