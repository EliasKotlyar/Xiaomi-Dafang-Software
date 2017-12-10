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

#pragma interface
#ifndef HC_SYNCCHECK_H
#define HC_SYNCCHECK_H

#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include "HC_CardPane.h"

#define NO_LOCK 0
#define LOCK    1
#define SYNC    2

class HC_CardPane;

class HC_SyncCheck:public Fl_Widget
{
public:
    HC_SyncCheck(int x, int y, int w, int h);
    void draw();
    int adat1_lock_status;
    int adat2_lock_status;
    int adat3_lock_status;
    int wordclock_lock_status;
    int adatsync_lock_status;
    int spdif_lock_status;    
    void setSpdifStatus(unsigned char s);
    void setAdat1Status(unsigned char s);
    void setAdat2Status(unsigned char s);
    void setAdat3Status(unsigned char s);
    void setAdatSyncStatus(unsigned char s);
    void setWCStatus(unsigned char s);
private:
    char *adat_name;
    int h_step;
    Fl_Box_Draw_F *draw_box;
};

#endif

