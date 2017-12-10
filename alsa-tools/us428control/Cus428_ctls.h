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

#ifndef Cus428_ctls_h
#define Cus428_ctls_h
#include "usbus428ctldefs.h"


class Cus428_ctls: public us428_ctls{
public:
	Cus428_ctls() {
		memset(this, 0, sizeof(*this));
	}
	void dump(int n = 0);
	void analyse(Cus428_ctls *Previous, unsigned n = 0);
	bool Knob( int K) {
		return ((char*)this)[K / 8] & (1 << K % 8);
	}
protected:
	unsigned char ValAt(int i) {
		return ((unsigned char*)this)[i];
	}
	unsigned char DiffValAt(Cus428_ctls *Other, int i) {
		return ValAt(i) - Other->ValAt(i);
	}
	unsigned char DiffBitAt(Cus428_ctls *Other, int i) {
		return ValAt(i) ^ Other->ValAt(i);
	}
};

#endif
