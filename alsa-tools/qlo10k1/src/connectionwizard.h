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
 
#ifndef CONNECTIONWIZARD_H
#define CONNECTIONWIZARD_H

#include <qvaluelist.h>

#include "strglobal.h"
#include "structure.h"
#include "dlgs/connectionwizardbase.h"

class RouteInfo
{
private:
	RSItemIO *toIO;

	bool Multi;
	bool Simple;

public:
	RouteInfo(bool multi, bool simple, RSItemIO *t)
	{
		toIO = t;
		Multi = multi;
		Simple = simple;
	}

	RouteInfo(const RouteInfo &from)
	{
		toIO = from.toIO;
		Multi = from.Multi;
		Simple = from.Simple;
	}

	RouteInfo()
	{
		toIO = NULL;
		Multi = false;
		Simple = false;
	}

	RSItemIO *getToIO()
	{
		return toIO;
	}
	
	void setToIO(RSItemIO *tio)
	{
		toIO = tio;
	}

	bool getMulti()
	{
		return Multi;
	}

	bool getSimple()
	{
		return Simple;
	}

	void setMulti(bool multi)
	{
		Multi = multi;
	}

	void setSimple(bool simple)
	{
		Simple = simple;
	}

	bool isValid()
	{
		return toIO != NULL;
	}
};

class ConnectionWizard : public ConnectionWizardBase
{
	Q_OBJECT
	
	enum IOType {None, In, Out, FX, PIn, POut} rtype;
	RouteInfo route;
	StrPatch *pto;
	RouteInfo *rptr;
	StrGlobal *global;
	bool out;
	QPtrList <RSItemIO> ios;
	QPtrList <StrPatch> patchList;
	StrPatch *selPatch;
public:
	ConnectionWizard(bool o, StrGlobal *g, RouteInfo *r);
	void loadEnd(IOType iotype);
	void setState(bool patch, bool end, bool back, bool next, bool finish);
public slots:
	void noneClicked();
	void fxClicked();
	void inClicked();
	void outClicked();
	void patchClicked();

	void finishClicked();

	void selected(const QString &page);
	void patchListClicked(QListBoxItem *item);
	void endListClicked(QListBoxItem *item);
};

#endif // CONNECTIONWIZARD_H
