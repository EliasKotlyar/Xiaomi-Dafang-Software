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
 
#ifndef LD10K1FILE_H
#define LD10K1FILE_H

#include <qstring.h>
#include <qvaluelist.h>
#include <lo10k1/lo10k1.h>
#include "structure.h"

class EMU10k1FilePrivate;
class LD10k1File;

struct EMU10k1Ctrl
{
	unsigned int value;
	unsigned int min;
	unsigned int max;
	QString name;
};

class EMU10k1File
{
	EMU10k1FilePrivate *privData;
public:
	EMU10k1File();
	~EMU10k1File();
	
	static int LoadFromFile(QString file, EMU10k1File **ef);
	
	int getInCount();
	int getOutCount();
	
	int getCtrlCount();
	void getCtrl(unsigned int i, EMU10k1Ctrl *ctrl);
	void setCtrl(unsigned int i, EMU10k1Ctrl *ctrl);
	QString getPatchName();
	
	friend class LD10k1File;
};


class LD10k1FilePrivate;

class LD10k1FileTransfCtl
{
public:
	int emu_ctls[32];
	int emu_ctl_count;
	QString ctl_name;
};

class LD10k1File
{
	LD10k1FilePrivate *privData;
	QString fileName;
	QString fileDesc;
	QString fileCreater;
	QString fileAuthor;
	QString fileCopyright;
	QString fileLicense;
public:
	enum TranslationType {None, Table100, Bass, Treble, OnOff};
	
	LD10k1File();
	~LD10k1File();
	
	QString getPatchName();
	void setPatchName(QString name);
	
	int getIOCount(bool out);
	QString getIOName(bool out, unsigned int i);
	void setIOName(bool out, unsigned int i, QString name);
	
	int getCtlCount();
	QString getCtlName(unsigned int i);
	void setCtlName(unsigned int i, QString name);
	
	TranslationType getCtlTranslation(unsigned int i);
	void setCtlTranslation(unsigned int i, TranslationType t);
	
	int getCtlValCount(unsigned int i);
	
	unsigned int getCtlMin(unsigned int i);
	unsigned int getCtlMax(unsigned int i);
	
	int getCtlValVCount(unsigned int i);
	void setCtlValVCount(unsigned int i, unsigned int cnt);
	
	unsigned int getCtlVal(unsigned int i, unsigned int vi);
	void setCtlVal(unsigned int i, unsigned int vi, unsigned int val);
	
	static int transformFromEmuFile(EMU10k1File *ef, LD10k1FileTransfCtl *tc, int tc_count, LD10k1File **lf);
	liblo10k1_dsp_patch_t *getLD10k1Format();
	void setLD10k1Format(liblo10k1_dsp_patch_t *patch);
		
	static int LoadFromFile(QString file, LD10k1File **lf);
	int SaveToFile(QString file);
};

class LD10k1DspFilePrivate;
class CardParam;

class LD10k1DspFile
{
	LD10k1DspFilePrivate *privData;
	QString fileName;
	QString fileDesc;
	QString fileCreater;
	QString fileAuthor;
	QString fileCopyright;
	QString fileLicense;
public:
	LD10k1DspFile();
	~LD10k1DspFile();
	
	liblo10k1_file_dsp_setup_t *getLD10k1DspFormat();
	void setLD10k1DspFormat(liblo10k1_file_dsp_setup_t *setup);
		
	static int LoadFromFile(QString file, LD10k1DspFile **lf);
	int SaveToFile(QString file);
	
	friend class CardParam;
};

#endif // LD10K1FILE_H
