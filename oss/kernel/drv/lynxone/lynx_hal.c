/*
 * sound/lynx_hal.c
 *
 * Driver for LynxONE Studio Interface.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

#include "lynxone_cfg.h"
#define OSS
#include "Environ.h"
#include "HalLynx.h"
#include "HalAudio.h"
#include "HalDwnld.h"
#include "DosCmn.h"
#include "DrvDebug.h"
#include "Pathfind.h"
#include "lynx_hal.h"

#include "HalDwnld.inc"
#include "HalLynx.inc"
