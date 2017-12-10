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
 
#ifndef STRUCTURE_IO_H
#define STRUCTURE_IO_H

#include "structure.h"
#include "custom_colors.h"

class StrIOBase : public RSItemBaseWithType
{
public:
	StrIOBase(RSItemBaseWithType::ItemType it);
	
	virtual void calcSize();	
	virtual void draw(DrawingParams *dp);
	virtual bool containsPoint(int xp, int yp);
	virtual bool containsHandlePoint(int xp, int yp);	
	virtual RSItemHandle getHandlePoint(int xp, int yp);
	
	virtual QString getIOName()
	{
		return "";
	}
	
	virtual void getIOPoint(RSItemIO *item, int *xp, int *yp);
	virtual void updateContents(QScrollView *sv, float zoom);
};



class StrInput : public StrIOBase
{
	QString inputName;
	int inputNum;
	
	RSItemIO *io;
public:
	QString name(void)
	{
		return inputName;
	}
	
	virtual QString getIOName()
	{
		return "I " + QString().setNum(inputNum) + "\n" + inputName;
	}
	
	void setName(QString nn)
	{
		inputName = nn;
		io->setDesc(nn);
	}

	StrInput(int in, QString name) :
		StrIOBase(RSItemBaseWithType::In)
	{
		inputName = name;
		inputNum = in;
		
		io = new RSItemIO(this, true, 0, name);
		calcSize();
	}
	
	int num(void)
	{
		return inputNum;
	}
	
	virtual int getMaxIOIdx(bool out)
	{
		if (out)
			return 1;
		else
			return 0;
	}
	
	virtual RSItemIO *getIO(bool out, int idx)
	{
		if (!out)
			return NULL;
		else if (idx == 0)
			return io;
		else
			return NULL;
	}
	
	virtual ~StrInput()
	{
		RSItemIO *tmp = io;
		delete io;
		io = NULL;
		tmp->connectTo(NULL);
	}
};

class StrOutput : public StrIOBase
{
	QString outputName;
	int outputNum;
	
	RSItemIO *io;
public:
	QString name(void)
	{
		return outputName;
	}
	
	virtual QString getIOName()
	{
		return "O " + QString().setNum(outputNum) + "\n" + outputName;
	}
	
	void setName(QString nn)
	{
		outputName = nn;
		io->setDesc(nn);
	}

	StrOutput(int on, QString name) :
		StrIOBase(RSItemBaseWithType::Out)
	{
		outputName = name;
		outputNum = on;
		
		io = new RSItemIO(this, false, 0, name);
		calcSize();
	}

	int num(void)
	{
		return outputNum;
	}
	
	virtual int getMaxIOIdx(bool out)
	{
		if (out)
			return 0;
		else
			return 1;
	}
	
	virtual RSItemIO *getIO(bool out, int idx)
	{
		if (out)
			return NULL;
		else if (idx == 0)
			return io;
		else
			return NULL;
	}
	
	virtual ~StrOutput()
	{
		RSItemIO *tmp = io;
		delete io;
		io = NULL;
		tmp->connectTo(NULL);
	}
};

class StrFX : public StrIOBase
{
	QString fxName;
	int fxNum;
	
	RSItemIO *io;
public:
	QString name(void)
	{
		return fxName;
	}
	
	virtual QString getIOName()
	{
		return "F " + QString().setNum(fxNum) + "\n" + fxName;
	}
	
	void setName(QString nn)
	{
		fxName = nn;
		io->setDesc(nn);
	}

	StrFX(int fn, QString name) :
		StrIOBase(RSItemBaseWithType::FX)
	{
		fxName = name;
		fxNum = fn;
		
		io = new RSItemIO(this, true, 0, name);
		calcSize();
	}

	int num(void)
	{
		return fxNum;
	}
	
	virtual int getMaxIOIdx(bool out)
	{
		if (out)
			return 1;
		else
			return 0;
	}
	
	virtual RSItemIO *getIO(bool out, int idx)
	{
		if (!out)
			return NULL;
		else if (idx == 0)
			return io;
		else
			return NULL;
	}
	
	virtual ~StrFX()
	{
		RSItemIO *tmp = io;
		delete io;
		io = NULL;
		tmp->connectTo(NULL);
	}
};

#endif // STRUCTURE_IO_H
