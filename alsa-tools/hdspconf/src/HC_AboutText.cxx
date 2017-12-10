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

#pragma implementation
#include "HC_AboutText.h"

HC_AboutText::HC_AboutText(int x, int y, int w, int h):Fl_Widget(x, y, w, h, "About Text")
{
	text = "HDSPConf " VERSION " (C) 2003 Thomas Charbonnel <thomas@@undata.org>\n\n"
	       "This Program is free software; you can redistribute it and/or modify\n"
	       "it under the terms of the GNU General Public License as published by\n"
	       "the Free Software Foundation; either version 2 of the License, or\n"
	       "(at your option) any later version.\n\n"
	       "This program is distributed in the hope that it will be useful,\n"
	       "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	       "See the GNU General Public License for more details.\n\n"
	       "You should have received a copy of the GNU General Public License\n"
	       "along with this program; if not, write to the Free Software\n"
	       "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n";
}

void HC_AboutText::draw()
{
	fl_color(FL_FOREGROUND_COLOR);
	fl_font(FL_HELVETICA, 10);
	fl_draw(text, x(), y(), w(), h(), FL_ALIGN_LEFT);
}

