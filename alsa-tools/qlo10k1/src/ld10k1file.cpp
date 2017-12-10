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
 
#include <stdlib.h>
#include "ld10k1file.h"

class EMU10k1FilePrivate
{
	liblo10k1_emu_patch_t *ep;
public:
	EMU10k1FilePrivate()
	{
		ep = liblo10k1_emu_new_patch();
	}
	
	~EMU10k1FilePrivate()
	{
		if (ep)
			liblo10k1_emu_free_patch(ep);
	}
	
	void setNativeStruct(liblo10k1_emu_patch_t *ns)
	{
		if (ep)
			liblo10k1_emu_free_patch(ep);
		ep = ns;
	}
	
	liblo10k1_emu_patch_t *getNativeStruct()
	{
		return ep;
	}
};

EMU10k1File::EMU10k1File()
{
	privData = new EMU10k1FilePrivate();
}

EMU10k1File::~EMU10k1File()
{
	if (privData)
		delete (privData);
}

int EMU10k1File::LoadFromFile(QString file, EMU10k1File **ef)
{
	liblo10k1_emu_patch_t *nf;
	char fn[1000]; // FIXME file name length
	int err;
	
	strncpy(fn, file.local8Bit(), 999);
	fn[999] = '\0';
	if ((err = liblo10k1_emu_load_patch(fn, &nf)) < 0)
		return err;
	
	EMU10k1File *efi = new EMU10k1File();
	efi->privData->setNativeStruct(nf);
	*ef = efi;
	return 0;
}

void EMU10k1File::getCtrl(unsigned int i, EMU10k1Ctrl *ctrl)
{
	liblo10k1_emu_patch_t *nf = privData->getNativeStruct();
	liblo10k1_emu_ctl_t *ctl;
	
	if (i >= nf->ctl_count)
		return;
		
	ctl = &(nf->ctls[i]);
		
	ctrl->value = ctl->ctl_val;
	ctrl->min = ctl->ctl_val_min;
	ctrl->max = ctl->ctl_val_max;
	ctrl->name = ctl->ctl_name;
}

void EMU10k1File::setCtrl(unsigned int i, EMU10k1Ctrl *ctrl)
{
	liblo10k1_emu_patch_t *nf = privData->getNativeStruct();
	liblo10k1_emu_ctl_t *ctl;
	
	if (i >= nf->ctl_count)
		return;
		
	ctl = &(nf->ctls[i]);
		
	ctl->ctl_val = ctrl->value;
	ctl->ctl_val_min = ctrl->min;
	ctl->ctl_val_max = ctrl->max;
	strcpy(ctl->ctl_name, ctrl->name.local8Bit());
}

QString EMU10k1File::getPatchName()
{
	liblo10k1_emu_patch_t *nf = privData->getNativeStruct();
	return QString(nf->patch_name);
}

int EMU10k1File::getCtrlCount()
{
	liblo10k1_emu_patch_t *nf = privData->getNativeStruct();
	return nf->ctl_count;
}

int EMU10k1File::getInCount()
{
	liblo10k1_emu_patch_t *nf = privData->getNativeStruct();
	return nf->in_count;
}

int EMU10k1File::getOutCount()
{
	liblo10k1_emu_patch_t *nf = privData->getNativeStruct();
	return nf->out_count;
}

class LD10k1FilePrivate
{
	liblo10k1_dsp_patch_t *lp;
public:
	LD10k1FilePrivate()
	{
		lp = liblo10k1_patch_alloc(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	
	~LD10k1FilePrivate()
	{
		if (lp)
			liblo10k1_patch_free(lp);
	}
	
	void setNativeStruct(liblo10k1_dsp_patch_t *ns)
	{
		if (lp)
			liblo10k1_patch_free(lp);
		lp = ns;
	}
	
	liblo10k1_dsp_patch_t *getNativeStruct()
	{
		return lp;
	}
};

LD10k1File::LD10k1File()
{
	privData = new LD10k1FilePrivate();
}

LD10k1File::~LD10k1File()
{
	if (privData)
		delete (privData);
}

liblo10k1_dsp_patch_t *LD10k1File::getLD10k1Format()
{
	return privData->getNativeStruct();
}

void LD10k1File::setLD10k1Format(liblo10k1_dsp_patch_t *patch)
{
	privData->setNativeStruct(patch);
}

int LD10k1File::transformFromEmuFile(EMU10k1File *ef, LD10k1FileTransfCtl *tc, int tc_count, LD10k1File **lf)
{
	liblo10k1_ctl_transform_t *ntc = (liblo10k1_ctl_transform_t *)malloc(sizeof(liblo10k1_ctl_transform_t) * tc_count);
	if (!ntc)
		return LD10K1_ERR_NO_MEM;
		
	// copy tc to ntc
	int i, j, err;
	liblo10k1_dsp_patch_t *np = NULL;
	
	for (i = 0; i < tc_count; i++)
	{
		for (j = 0; j < tc[i].emu_ctl_count; j++)
			ntc[i].emu_ctls[j] = tc[i].emu_ctls[j];
		
		ntc[i].emu_ctl_count = tc[i].emu_ctl_count;
		strncpy(ntc[i].ctl_name, tc[i].ctl_name.local8Bit(), 43);
		ntc[i].ctl_name[43] = '\0';
	}
	
	if ((err = liblo10k1_emu_transform_patch(ef->privData->getNativeStruct(),  ntc, tc_count, &np)) < 0)
	{
		free(ntc);
		return err;
	}
		
	free(ntc);
	
	LD10k1File *nlf = new LD10k1File();
	
	nlf->privData->setNativeStruct(np);
	*lf = nlf;
	return 0;
}

int LD10k1File::getIOCount(bool out)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (out)
		return nf->out_count;
	else
		return nf->in_count;
}

QString LD10k1File::getIOName(bool out, unsigned int i)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (out)
	{
		if (i >= nf->out_count)
			return "";
		return QString(nf->outs[i].name);
	}
	else
	{
		if (i >= nf->in_count)
			return "";
		return QString(nf->ins[i].name);
	}
}

void LD10k1File::setIOName(bool out, unsigned int i, QString name)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (out)
	{
		if (i >= nf->out_count)
			return;
		strncpy(nf->outs[i].name, name.local8Bit(), MAX_NAME_LEN - 1);
		nf->outs[i].name[MAX_NAME_LEN - 1] = '\0';
	}
	else
	{
		if (i >= nf->in_count)
			return;
		strncpy(nf->ins[i].name, name.local8Bit(), MAX_NAME_LEN - 1);
		nf->ins[i].name[MAX_NAME_LEN - 1] = '\0';
	}
}

int LD10k1File::getCtlCount()
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	return nf->ctl_count;
}

QString LD10k1File::getCtlName(unsigned int i)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return "";
	return QString(nf->ctl[i].name);
}

void LD10k1File::setCtlName(unsigned int i, QString name)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return;
	strncpy(nf->ctl[i].name, name.local8Bit(), 43);
	nf->ctl[i].name[43] = '\0';
}

LD10k1File::TranslationType LD10k1File::getCtlTranslation(unsigned int i)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return None;
	switch (nf->ctl[i].translation)
	{
		case EMU10K1_GPR_TRANSLATION_NONE:
			return LD10k1File::None;
		case EMU10K1_GPR_TRANSLATION_TABLE100:
			return LD10k1File::Table100;
		case EMU10K1_GPR_TRANSLATION_BASS:
			return LD10k1File::Bass;
		case EMU10K1_GPR_TRANSLATION_TREBLE:
			return LD10k1File::Treble;
		case EMU10K1_GPR_TRANSLATION_ONOFF:
			return LD10k1File::OnOff;
		default:
			return LD10k1File::None;
	}
}

void LD10k1File::setCtlTranslation(unsigned int i, LD10k1File::TranslationType t)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return;
	switch (t)
	{
		case LD10k1File::None:
			nf->ctl[i].translation = EMU10K1_GPR_TRANSLATION_NONE;
			break;
		case LD10k1File::Table100:
			nf->ctl[i].translation = EMU10K1_GPR_TRANSLATION_TABLE100;
			break;
		case LD10k1File::Bass:
			nf->ctl[i].translation = EMU10K1_GPR_TRANSLATION_BASS;
			break;
		case LD10k1File::Treble:
			nf->ctl[i].translation = EMU10K1_GPR_TRANSLATION_TREBLE;
			break;
		case LD10k1File::OnOff:
			nf->ctl[i].translation = EMU10K1_GPR_TRANSLATION_ONOFF;
			break;
		default:
			break;
	}
}

int LD10k1File::getCtlValCount(unsigned int i)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return 0;
	return nf->ctl[i].count;
}

int LD10k1File::getCtlValVCount(unsigned int i)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return 0;
	return nf->ctl[i].vcount;
}

void LD10k1File::setCtlValVCount(unsigned int i, unsigned int cnt)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return;
	nf->ctl[i].vcount = cnt;
}

unsigned int LD10k1File::getCtlVal(unsigned int i, unsigned int vi)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return 0;
		
	if (vi >= nf->ctl[i].vcount)
		return 0;
	return nf->ctl[i].value[vi];
}

void LD10k1File::setCtlVal(unsigned int i, unsigned int vi, unsigned int val)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return;
		
	if (vi >= nf->ctl[i].vcount)
		return;
	nf->ctl[i].value[vi] = val;
}

QString LD10k1File::getPatchName()
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	return nf->patch_name;
}

void LD10k1File::setPatchName(QString name)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	strncpy(nf->patch_name, name.local8Bit(), MAX_NAME_LEN - 1);
	nf->patch_name[MAX_NAME_LEN - 1] = '\0';
}

int LD10k1File::LoadFromFile(QString file, LD10k1File **lf)
{
	liblo10k1_dsp_patch_t *nf = NULL;
	liblo10k1_file_info_t *fi = NULL;
	char fn[1000]; // FIXME file name length
	int err;
	
	strncpy(fn, file.local8Bit(), 999);
	fn[999] = '\0';
	if ((err = liblo10k1lf_load_dsp_patch(&nf, fn, &fi)) < 0)
		return err;
	
	LD10k1File *lfi = new LD10k1File();
	lfi->privData->setNativeStruct(nf);
	*lf = lfi;
	
	lfi->fileName = fi->name;
	lfi->fileDesc = fi->desc;
	lfi->fileCreater = fi->creater;
	lfi->fileAuthor = fi->author;
	lfi->fileCopyright = fi->copyright;
	lfi->fileLicense = fi->license;
	
	liblo10k1lf_file_info_free(fi);
	return 0;
}

int LD10k1File::SaveToFile(QString file)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();

	liblo10k1_file_info_t *fi = liblo10k1lf_file_info_alloc();
	char fn[1000]; // FIXME file name length
	int err;
	
	strncpy(fn, file.local8Bit(), 999);
	fn[999] = '\0';
	
	fi->name = strdup(fileName.local8Bit());
	fi->desc = strdup(fileDesc.local8Bit());
	fi->creater = strdup(fileCreater.local8Bit());
	fi->author = strdup(fileAuthor.local8Bit());
	fi->copyright = strdup(fileCopyright.local8Bit());
	fi->license = strdup(fileLicense.local8Bit());
	
	if ((err = liblo10k1lf_save_dsp_patch(nf, fn, fi)) < 0) 
	{
		liblo10k1lf_file_info_free(fi);
		return err;
	}
		
	liblo10k1lf_file_info_free(fi);
	return 0;
}

unsigned int LD10k1File::getCtlMin(unsigned int i)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return 0;
		
	return nf->ctl[i].min;
}

unsigned int LD10k1File::getCtlMax(unsigned int i)
{
	liblo10k1_dsp_patch_t *nf = privData->getNativeStruct();
	if (i >= nf->ctl_count)
		return 0;
		
	return nf->ctl[i].max;
}


class LD10k1DspFilePrivate
{
	liblo10k1_file_dsp_setup_t *ls;
public:
	LD10k1DspFilePrivate()
	{
		ls = liblo10k1lf_dsp_config_alloc();
	}
	
	~LD10k1DspFilePrivate()
	{
		if (ls)
			liblo10k1lf_dsp_config_free(ls);
	}
	
	void setNativeStruct(liblo10k1_file_dsp_setup_t *ns)
	{
		if (ls)
			liblo10k1lf_dsp_config_free(ls);
		ls = ns;
	}
	
	liblo10k1_file_dsp_setup_t *getNativeStruct()
	{
		return ls;
	}
};

LD10k1DspFile::LD10k1DspFile()
{
	privData = new LD10k1DspFilePrivate();
}

LD10k1DspFile::~LD10k1DspFile()
{
	if (privData)
		delete (privData);
}

liblo10k1_file_dsp_setup_t *LD10k1DspFile::getLD10k1DspFormat()
{
	return privData->getNativeStruct();
}

void LD10k1DspFile::setLD10k1DspFormat(liblo10k1_file_dsp_setup_t *setup)
{
	privData->setNativeStruct(setup);
}

int LD10k1DspFile::LoadFromFile(QString file, LD10k1DspFile **lf)
{
	liblo10k1_file_dsp_setup_t *nf = NULL;
	liblo10k1_file_info_t *fi = NULL;
	char fn[1000]; // FIXME file name length
	int err;
	
	strncpy(fn, file.local8Bit(), 999);
	fn[999] = '\0';
	if ((err = liblo10k1lf_load_dsp_config(&nf, fn, &fi)) < 0)
		return err;
	
	LD10k1DspFile *lfi = new LD10k1DspFile();
	lfi->privData->setNativeStruct(nf);
	*lf = lfi;
	
	lfi->fileName = fi->name;
	lfi->fileDesc = fi->desc;
	lfi->fileCreater = fi->creater;
	lfi->fileAuthor = fi->author;
	lfi->fileCopyright = fi->copyright;
	lfi->fileLicense = fi->license;
	
	liblo10k1lf_file_info_free(fi);
	return 0;
}

int LD10k1DspFile::SaveToFile(QString file)
{
	liblo10k1_file_dsp_setup_t *nf = privData->getNativeStruct();

	liblo10k1_file_info_t *fi = liblo10k1lf_file_info_alloc();
	char fn[1000]; // FIXME file name length
	int err;
	
	strncpy(fn, file.local8Bit(), 999);
	fn[999] = '\0';
	
	fi->name = strdup(fileName.local8Bit());
	fi->desc = strdup(fileDesc.local8Bit());
	fi->creater = strdup(fileCreater.local8Bit());
	fi->author = strdup(fileAuthor.local8Bit());
	fi->copyright = strdup(fileCopyright.local8Bit());
	fi->license = strdup(fileLicense.local8Bit());
	
	if ((err = liblo10k1lf_save_dsp_config(nf, fn, fi)) < 0) 
	{
		liblo10k1lf_file_info_free(fi);
		return err;
	}
		
	liblo10k1lf_file_info_free(fi);
	return 0;
}

