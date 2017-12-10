/*
 * Purpose: devfs interface for BeOS/Haiku
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2007.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution. Please contact sales@opensound.com for further info.
 *
 */

#include "oss_config.h"
#include "midi_core.h"
#include <oss_pci.h>
#include <KernelExport.h>
#include <Drivers.h>

#define DRIVER_NAME "ossdrv"

int32 api_version = B_CUR_DRIVER_API_VERSION;

oss_core_module_info *gOSSCore = NULL;

//	#pragma mark -

// XXX: malloc it + update it from device list

const char **
publish_devices(void)
{
	return gOSSCore->oss_publish_devices();
}


device_hooks *
find_device(const char *name)
{
	FENTRYA("%s", name);

	FEXIT();
	return gOSSCore->oss_get_driver_hooks();
}

//	#pragma mark -

status_t
init_hardware(void)
{
	status_t err;
	FENTRY();

	err = get_module(OSS_CORE_MODULE_NAME, (module_info **)&gOSSCore);
	if (err < B_OK) {
		FEXIT();
		return err;
	}
	
	put_module(OSS_CORE_MODULE_NAME);

	FEXIT();
	return B_OK;
}

status_t
init_driver(void)
{
	status_t err = ENOMEM;
	FENTRY();

	err = get_module(OSS_CORE_MODULE_NAME, (module_info **)&gOSSCore);
	if (err < B_OK)
		goto err1;
	err = gOSSCore->init_osscore();
	dprintf("oss:init_osscore: 0x%08lx\n", err);
	if (err < B_OK)
		goto err2;
	err = gOSSCore->oss_load_drivers();
	err = B_OK;
	FEXITR(err);
	return err;
	
err2:
	put_module(OSS_CORE_MODULE_NAME);
err1:
	FEXITR(err);
	return err;
}

void
uninit_driver(void)
{
	status_t err;
	FENTRY();
	
	err = gOSSCore->oss_unload_all_drivers();
	err = gOSSCore->uninit_osscore();
	dprintf("oss:uninit_osscore: 0x%08lx\n", err);
	put_module(OSS_CORE_MODULE_NAME);
	FEXIT();
}
