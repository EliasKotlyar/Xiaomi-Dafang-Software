/* -*- mode:C++; indent-tabs-mode:t; tab-width:8; c-basic-offset: 8 -*- */
/*
 * Controller for Tascam US-X2Y
 *
 * Copyright (c) 2003 by Karsten Wiese <annabellesgarden@yahoo.de>
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
 */

#include <stdio.h>

#include "Cus428_ctls.h"
#include "Cus428State.h"

Cus428State* OneState;

void
Cus428_ctls::dump(int n)
{
	for (int m = 0; m < n; m++)
		printf("   ");
	for (; n < sizeof(*this); n++)
		printf("%02hhX ", ((char*)this)[n]);
	printf("\n");
}

void
Cus428_ctls::analyse(Cus428_ctls *Previous, unsigned n)
{
	Cus428_ctls *PreviousL = Previous ? Previous : new Cus428_ctls();
	OneState->Set_us428_ctls(this);
	for (; n < 9; n++) {			//Sliders
		char Diff = DiffValAt(PreviousL, n);
		if (Diff)
			OneState->SliderChangedTo(n, ValAt(n));
	}
	for (; n < 16; n++) {			//Knobs
		unsigned char Diff = DiffBitAt(PreviousL, n);
		unsigned o = 0;
		while (o < 8) {
			if (Diff & (1 << o))
				OneState->KnobChangedTo((Cus428State::eKnobs)(8*n + o), ValAt(n) & (1 << o));
			++o;
		}
	}
	for (; n < sizeof(*this); n++) {	//wheels
		char Diff = DiffValAt(PreviousL, n);
		if (Diff)
			OneState->WheelChangedTo((E_In84)n, Diff);
	}
	if (0 == Previous)
		delete PreviousL;
}
