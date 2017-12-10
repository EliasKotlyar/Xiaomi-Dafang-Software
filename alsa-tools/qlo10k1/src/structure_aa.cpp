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

#include <math.h> 
#include "structure.h"
#include "strglobal.h"

// simple, but very far from perfect

void StrGlobal::AutoArange(QPtrList <RSItemBaseWithType> *items)
{
	int maxx = 0;
	int maxy = 0;
	int inc = 0;
	int pc = 0;
	int outc = 0;
	
	// iterate through all
	RSItemBaseWithType *item;
	
	for(item = items->first(); item; item = items->next() )
	{
		if (item->flagNew())
		{
			switch (item->type())
			{
				case RSItemBaseWithType::In:
				case RSItemBaseWithType::FX:
					inc++;
					break;
				case RSItemBaseWithType::Out:
					outc++;
					break;
				case RSItemBaseWithType::Patch:
					pc++;
					break;
				default:
					break;				
			}
		}
		else
		{
			int tmpx = item->x() + item->width();
			int tmpy = item->y() + item->height();
			
			if (tmpx > maxx)
				maxx = tmpx;
			if (tmpy > maxy)
				maxy = tmpy;
		}
	}
	
	int ini = 0;
	int pi = 0;
	int outi = 0;
	
	int sizex = (int)(sqrt(pc) + 1);
	//int sizey = (int)((float)pc / sizex + 1);
	
	int offiny = maxy + 20;
	int offinx = 0 + 20;
	int offpy = maxy + 20;
	int offpx = 0 + 20 + 90;
	int offouty = maxy + 20;
	int offoutx = sizex * 90 + 20 + 70 + 70;
	int offheight = 0;
	int maxoffheight = 0;
	int heightaccum = 0;
	
	for(item = items->first(); item; item = items->next() )
	{
		if (item->flagNew())
		{
			switch (item->type())
			{
				case RSItemBaseWithType::In:
				case RSItemBaseWithType::FX:
					item->setPosition(offinx, offiny + ini * 40);
					ini++;
					break;
				case RSItemBaseWithType::Out:
					item->setPosition(offoutx, offouty + outi * 40);
					outi++;
					break;
				case RSItemBaseWithType::Patch:
					item->setPosition(offpx + (pi % sizex) * 90, offpy + (pi / sizex) * 90 + heightaccum);
					item->calcSize();
					offheight = item->height() - sizex * 10;
					if ( offheight > maxoffheight )
						maxoffheight = offheight;
					pi++;
					break;
				default:
					break;				
			}
			if(offpx + (pi % sizex) * 90 == offpx) {
				heightaccum += maxoffheight;
				maxoffheight = 0;
			}
		}
	}
}
