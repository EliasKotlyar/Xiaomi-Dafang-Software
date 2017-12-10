/* -*- mode:C++; indent-tabs-mode:t; tab-width:8; c-basic-offset: 8 -*- */
/*
 * Controller for Tascam US-X2Y
 *
 * Copyright (c) 2003 by Karsten Wiese <annabellesgarden@yahoo.de>
 * Copyright (c) 2004-2007 by Rui Nuno Capela <rncbc@rncbc.org>
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

#ifndef Cus428State_h
#define Cus428State_h

#include "Cus428_ctls.h"

class Cus428State: public us428_lights
{
public:

	// Constructor.
	Cus428State(struct us428ctls_sharedmem* Pus428ctls_sharedmem, int y = 8)
		:us428ctls_sharedmem(Pus428ctls_sharedmem),Y(y)
		,us428_ctls(0)
		,MuteInputMonitor(0)
		,SoloInputMonitor(0)
		,RecInputMonitor(0)
		,SelectInputMonitor(0)
		,aBank(0)
		,cBanks(32 / y)
		,W0(0)
		,aWheel(0)
		,aWheel_L(0)
		,aWheel_R(0)
		,bSetLocate(false)
		,bSetRecord(false)
		,uTransport(0)
		,aWheelSpeed(0)
	{
		Mute = new unsigned char [cBanks];
		Solo = new unsigned char [cBanks];
		Rec = new unsigned char [cBanks];
		Select = new unsigned char [cBanks];
		for (int i = 0; i < cBanks; ++i)
			Mute[i] = Solo[i] = Rec[i] = Select[i] = 0;
		init_us428_lights();
		for (int v = 0; v < 5; ++v) {
			Volume[v].init(v);
		}
	}

	// Destructor.
	virtual ~Cus428State() {
		delete Select;
		delete Rec;
		delete Solo;
		delete Mute;
	}

	enum eKnobs {
		eK_RECORD =	72,
		eK_PLAY,
		eK_STOP,
		eK_FFWD,
		eK_REW,
		eK_SOLO,
		eK_REC,
		eK_NULL,
		eK_InputMonitor,	// = 80
		eK_BANK_L,
		eK_BANK_R,
		eK_LOCATE_L,
		eK_LOCATE_R,
		eK_SET =	85,
		eK_INPUTCD =	87,
		eK_HIGH =	90,
		eK_HIMID,
		eK_LOWMID,
		eK_LOW,
		eK_Select0 =	96,
		eK_Mute0 =	104,
		eK_Mute1,
		eK_Mute2,
		eK_Mute3,
		eK_Mute4,
		eK_Mute5,
		eK_Mute6,
		eK_Mute7,
		eK_AUX1 =	120,
		eK_AUX2,
		eK_AUX3,
		eK_AUX4,
		eK_ASGN,
		eK_F1,
		eK_F2,
		eK_F3,
	};

	void InitDevice(void);

	void KnobChangedTo(eKnobs K, bool V);
	void SliderChangedTo(int S, unsigned char New);
	void WheelChangedTo(E_In84 W, char Diff);
	virtual void UserSliderChangedTo(int S, unsigned char New);
	virtual void UserWheelChangedTo(E_In84 W, char Diff);
	virtual void UserKnobChangedTo(eKnobs K, bool V);

	void SliderSend(int S);
	Cus428_ctls *Set_us428_ctls(Cus428_ctls *New) {
		Cus428_ctls *Old = us428_ctls;
		us428_ctls = New;
		return Old;
	}
	// Update the LED lights state.
	int LightSend();
	// Time-code (hh:mm:ss:ff:fr) to/from absolute wheel position converters.
	void LocateWheel(unsigned char *tc);
	void LocateSend();
	// Set basic application transport state.
	void TransportToggle(unsigned char T);
	void TransportSet(unsigned char T, bool V);
	void TransportSend();
	// Set bank layer state.
	void BankSet(int B);
	void BankSend();
	// Process masked-write sub-command.
	void MaskedWrite(unsigned char *data);
	// Reset internal MMC state.
	void MmcReset();

protected:

	void SendVolume(usX2Y_volume &V);
	struct us428ctls_sharedmem* us428ctls_sharedmem;
	bool StateInputMonitor() {
		return  LightIs(eL_InputMonitor);
	}
	// Set the wheel differential.
	void WheelDelta(int W);
	// Set the wheel differential.
	void WheelStep(int dW);
	// Set the wheel shuttle speed.
	void WheelShuttle(int dW);
	// Get the curent wheel timecode.
	void LocateTimecode(unsigned char *tc);
	// Send own MMC masked-write subcommand.
	void SendMaskedWrite(unsigned char scmd, int track, bool V);

	usX2Y_volume_t	Volume[5];
	Cus428_ctls	*us428_ctls;
	// To hold channel light-mode states.
	unsigned char
		MuteInputMonitor,	*Mute,
		SoloInputMonitor,	*Solo,
		RecInputMonitor,	*Rec,
		SelectInputMonitor,	*Select;
	// The current selected bank, maximum number of bank/layers.
	int aBank, cBanks;
	// Differential wheel tracking.
	int W0;
	// Some way to convert wheel (absolute) position into hh:mm:ss:ff:fr
	int aWheel;
	// SET L/R points.
	int aWheel_L;
	int aWheel_R;
	// SET knob state.
	bool bSetLocate;
	// REC knob state.
	bool bSetRecord;
	// Last/current transport state.
	unsigned char uTransport;
	// Shuttle wheel absolute speed.
	int aWheelSpeed;
	// The official number of faders (channels per bank)
	int Y;
};


class Cus428StateMixxx: public Cus428State{
public:
	Cus428StateMixxx(struct us428ctls_sharedmem* Pus428ctls_sharedmem, int y);
	void UserKnobChangedTo(eKnobs K, bool V);
	void UserSliderChangedTo(int S, unsigned char New);
	void UserWheelChangedTo(E_In84 W, char Diff);
protected:
	int focus;
	int eq;
};

extern Cus428State* OneState;

#endif
