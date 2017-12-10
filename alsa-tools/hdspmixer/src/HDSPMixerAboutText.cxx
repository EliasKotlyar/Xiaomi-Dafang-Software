/*
 *   HDSPMixer
 *    
 *   Copyright (C) 2003 Thomas Charbonnel (thomas@undata.org)
 *
 *   Copyright (C) 2011 Adrian Knoth (adi@drcomp.erfurt.thur.de)
 *                      Fredrik Lingvall (fredrik.lingvall@gmail.com)
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
#include "HDSPMixerAboutText.h"

HDSPMixerAboutText::HDSPMixerAboutText(int x, int y, int w, int h):Fl_Widget(x, y, w, h, "About Text")
{
	text = "HDSPMixer " VERSION " (C) 2003 Thomas Charbonnel <thomas@@undata.org>\n"
		"(C) 2009 Florian Faber <faber@@faberman.de>\n"
		"(C) 2011 Adrian Knoth <adi@@drcomp.erfurt.thur.de>\n"
		"(C) 2011 Fredrik lingvall <fredrik.lingvall@@gmail.com>\n\n"
	       "Bitmaps by Ralf Brunner\n"
	       "Many thanks to Martin Björnsen, Matthias Carstens and Paul Davis\n\n"
	       "This Program is free software; you can redistribute it and/or modify\n"
	       "it under the terms of the GNU General Public License as published by\n"
	       "the Free Software Foundation; either version 2 of the License, or\n"
	       "(at your option) any later version.\n\n"
	       "This program is distributed in the hope that it will be useful,\n"
	       "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	       "See the GNU General Public License for more details.\n\n"
	       "You should have received a copy of the GNU General Public License\n"
	       "along with this program; if not, write to the\n"
	       "Free Software Foundation, Inc., 51 Franklin Street, Suite 500,\n"
	       "Boston, MA 02110-1335, USA.\n";
}

void HDSPMixerAboutText::draw()
{
	fl_color(FL_FOREGROUND_COLOR);
	fl_font(FL_HELVETICA, 10);
	fl_draw(text, x(), y(), w(), h(), FL_ALIGN_LEFT);
}

