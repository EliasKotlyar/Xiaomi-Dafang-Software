/*
 *   HDSPConf
 *
 *   Copyright (C) 2003 Thomas Charbonnel (thomas@undata.org)
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "labels.h"

const char *card_names[5] = {
    "Digiface",
    "Multiface",
    "HDSP9652",
    "HDSP9632",
    "Undefined",
};

const char *freqs[10] = {
    "32.0 kHz",
    "44.1 kHz",
    "48.0 kHz",
    "64.0 kHz",
    "88.2 kHz",
    "96.0 kHz",
    "-----",
    "128.0 kHz",
    "176.4 kHz",
    "192.0 kHz",
};

const char *ref[7] = {
    "Word Clock",
    "ADAT Sync",
    "SPDIF",
    "-----",
    "ADAT1",
    "ADAT2",
    "ADAT3"
};

const char *lock_status[3] = {
    "No Lock",
    "Lock",
    "Sync"
};

