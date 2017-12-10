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

#include <stdio.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "Cus428Midi.h"

extern int verbose;

// Differential wheel tracking constants.
#define W_DELTA_MAX	0xff
#define W_DELTA_MIN	(W_DELTA_MAX >> 1)
// Shuttle speed wheel constants.
#define W_SPEED_MAX	0x3f

void us428_lights::init_us428_lights()
{
	int i = 0;
	memset(this, 0, sizeof(*this));
	for (i = 0; i < 7; ++i)
		Light[ i].Offset = i + 0x19;
}


void Cus428State::InitDevice(void)
{
	if (us428ctls_sharedmem->CtlSnapShotLast >= 0)
		SliderChangedTo(eFaderM, ((unsigned char*)(us428ctls_sharedmem->CtlSnapShot + us428ctls_sharedmem->CtlSnapShotLast))[eFaderM]);
}

int Cus428State::LightSend()
{
	int Next = us428ctls_sharedmem->p4outLast + 1;
	if(Next < 0  ||  Next >= N_us428_p4out_BUFS)
		Next = 0;
	memcpy(&us428ctls_sharedmem->p4out[Next].lights, Light, sizeof(us428_lights));
	us428ctls_sharedmem->p4out[Next].type = eLT_Light;
	return us428ctls_sharedmem->p4outLast = Next;
}

void Cus428State::SliderSend(int S)
{
	Midi.SendMidiControl(15, 0x40 + S, ((unsigned char*)us428_ctls)[S] / 2);
}

void Cus428State::SendVolume(usX2Y_volume &V)
{
	int Next = us428ctls_sharedmem->p4outLast + 1;
	if (Next < 0  ||  Next >= N_us428_p4out_BUFS)
		Next = 0;
	memcpy(&us428ctls_sharedmem->p4out[Next].vol, &V, sizeof(V));
	us428ctls_sharedmem->p4out[Next].type = eLT_Volume;
	us428ctls_sharedmem->p4outLast = Next;
}

void Cus428State::UserSliderChangedTo(int S, unsigned char New)
{
	SliderSend(S);
}

void Cus428State::SliderChangedTo(int S, unsigned char New)
{
	if (StateInputMonitor() && S <= eFader3 || S == eFaderM) {
		usX2Y_volume &V = Volume[S >= eFader4 ? eFader4 : S];
		V.SetTo(S, New);
		if (S == eFaderM || !LightIs(eL_Mute0 + S))
			SendVolume(V);
	} else {
		UserSliderChangedTo(S, New);
	}
}

void Cus428State::UserKnobChangedTo(eKnobs K, bool V)
{
	switch (K) {
	case eK_STOP:
		if (verbose > 1)
			printf("Knob STOP now %i\n", V);
		if (V) TransportToggle(T_STOP);
		Midi.SendMidiControl(15, K, V);
		break;
	case eK_PLAY:
		if (verbose > 1)
			printf("Knob PLAY now %i", V);
		if (V) TransportToggle(T_PLAY);
		if (verbose > 1)
			printf(" Light is %i\n", LightIs(eL_Play));
		Midi.SendMidiControl(15, K, V);
		break;
	case eK_REW:
		if (verbose > 1)
			printf("Knob REW now %i", V);
		if (V) TransportToggle(T_REW);
		if (verbose > 1)
			printf(" Light is %i\n", LightIs(eL_Rew));
		Midi.SendMidiControl(15, K, V);
		break;
	case eK_FFWD:
		if (verbose > 1)
			printf("Knob FFWD now %i", V);
		if (V) TransportToggle(T_F_FWD);
		if (verbose > 1)
			printf(" Light is %i\n", LightIs(eL_FFwd));
		Midi.SendMidiControl(15, K, V);
		break;
	case eK_RECORD:
		if (verbose > 1)
			printf("Knob RECORD now %i", V);
		if (V) TransportToggle(T_RECORD);
		if (verbose > 1)
			printf(" Light is %i\n", LightIs(eL_Record));
		Midi.SendMidiControl(15, K, V);
		break;
	case eK_SET:
		if (verbose > 1)
			printf("Knob SET now %i\n", V);
		bSetLocate = V;
		break;
	case eK_LOCATE_L:
		if (verbose > 1)
			printf("Knob LOCATE_L now %i\n", V);
		if (V) {
			if (bSetLocate)
				aWheel_L = aWheel;
			else {
				aWheel = aWheel_L;
				LocateSend();
			}
		}
		break;
	case eK_LOCATE_R:
		if (verbose > 1)
			printf("Knob LOCATE_R now %i\n", V);
		if (V) {
			if (bSetLocate)
				aWheel_R = aWheel;
			else {
				aWheel = aWheel_R;
				LocateSend();
			}
		}
		break;
	case eK_REC:
		if (verbose > 1)
			printf("Knob REC now %i\n", V);
		bSetRecord = V;
		break;
	case eK_SOLO:
		if (verbose > 1)
			printf("Knob SOLO now %i", V);
		if (V) {
			bool bSolo = ! LightIs(eL_Solo);
			if (StateInputMonitor()) {
				if (bSolo) {
					MuteInputMonitor = Light[2].Value;
					Light[2].Value = SoloInputMonitor;
				} else {
					SoloInputMonitor = Light[2].Value;
					Light[2].Value = MuteInputMonitor;
				}
			} else {
				if (bSolo) {
					Mute[aBank] = Light[2].Value;
					Light[2].Value = Solo[aBank];
				} else {
					Solo[aBank] = Light[2].Value;
					Light[2].Value = Mute[aBank];
				}
			}
			LightSet(eL_Solo, bSolo);
			LightSend();
		}
		if (verbose > 1)
			printf(" Light is %i\n", LightIs(eL_Solo));
		break;
	case eK_NULL:
		if (verbose > 1)
			printf("Knob NULL now %i", V);
		if (V) {
			bool bNull = ! LightIs(eL_Null);
			LightSet(eL_Null, bNull);
			LightSend();
		}
		if (verbose > 1)
			printf(" Light is %i\n", LightIs(eL_Null));
		break;
	case eK_BANK_L:
		if (verbose > 1)
			printf("Knob BANK_L now %i", V);
		if (V) BankSet(aBank - 1);
		if (verbose > 1)
			printf(" Light is %i\n", LightIs(eL_BankL));
		break;
	case eK_BANK_R:
		if (verbose > 1)
			printf("Knob BANK_R now %i", V);
		if (V) BankSet(aBank + 1);
		if (verbose > 1)
			printf(" Light is %i\n", LightIs(eL_BankR));
		break;
	default:
		if (verbose > 1)
			printf("Knob %i now %i\n", K, V);
		Midi.SendMidiControl(15, K, V);
	}
}


void Cus428State::KnobChangedTo(eKnobs K, bool V)
{
//	switch (K & ~(StateInputMonitor() ? 3 : -1)) {
	switch (K & ~3) {
	case eK_Select0:
		if (V) {
			int S = eL_Select0 + (K & 7);
			Light[eL_Select0 / 8].Value = 0;
			LightSet(S, !LightIs(S));
			if (bSetRecord) {
				int R = eL_Rec0 + (K & 7);
				LightSet(R, !LightIs(R));
				if (!StateInputMonitor()) {
					SendMaskedWrite(MMC_CIF_TRACK_RECORD,
						Y * aBank + (K & 7), LightIs(R));
				}
			}
			LightSend();
		}
		break;
	case eK_Mute0:
		if (V) {
			int M = eL_Mute0 + (K & 7);
			LightSet(M, !LightIs(M));
			LightSend();
			if (StateInputMonitor()) {
				if (LightIs(eL_Solo)) {
					for (int i = 0; i < 5; ++i) {
						usX2Y_volume V = Volume[i];
						if (!LightIs(eL_Mute0 + i) || (MuteInputMonitor & (1 << i)))
							V.LH = V.LL = V.RL = V.RH = 0;
						SendVolume(V);
					}
				} else {
					usX2Y_volume V = Volume[M - eL_Mute0];
					if (LightIs(M))
						V.LH = V.LL = V.RL = V.RH = 0;
					SendVolume(V);
				}
			} else {
				if (LightIs(eL_Solo)) {
					SendMaskedWrite(MMC_CIF_TRACK_SOLO,
						Y * aBank + (K & 7), LightIs(M));
				} else {
					SendMaskedWrite(MMC_CIF_TRACK_MUTE,
						Y * aBank + (K & 7), LightIs(M));
				}
			}
		}
		break;
	default:
		if (K == eK_InputMonitor) {
			if (verbose > 1)
				printf("Knob InputMonitor now %i", V);
			if (V) {
				bool bInputMonitor = ! StateInputMonitor();
				if (bInputMonitor) {
					Select[aBank] = Light[0].Value;
					Rec[aBank] = Light[1].Value;
					Light[0].Value = SelectInputMonitor;
					Light[1].Value = RecInputMonitor;
					if (LightIs(eL_Solo)) {
						Solo[aBank] = Light[2].Value;
						Light[2].Value = SoloInputMonitor;
					} else {
						Mute[aBank] = Light[2].Value;
						Light[2].Value = MuteInputMonitor;
					}
				} else {
					SelectInputMonitor = Light[0].Value;
					RecInputMonitor = Light[1].Value;
					Light[0].Value = Select[aBank];
					Light[1].Value = Rec[aBank];
					if (LightIs(eL_Solo)) {
						SoloInputMonitor = Light[2].Value;
						Light[2].Value = Solo[aBank];
					} else {
						MuteInputMonitor = Light[2].Value;
						Light[2].Value = Mute[aBank];
					}
				}
				LightSet(eL_InputMonitor, bInputMonitor);
				LightSend();
			}
			if (verbose > 1)
				printf(" Light is %i\n", LightIs(eL_InputMonitor));
		} else
			UserKnobChangedTo(K, V);
	}
}

void Cus428State::UserWheelChangedTo(E_In84 W, char Diff)
{
	char Param;
	switch (W) {
	case eWheelPan:
		Param = 0x4D;
		break;
	case eWheelGain:
		Param = 0x48;
		break;
	case eWheelFreq:
		Param = 0x49;
		break;
	case eWheelQ:
		Param = 0x4A;
		break;
	case eWheel:
		Param = 0x60;
		// Update the absolute wheel position.
		WheelDelta((int) ((unsigned char *) us428_ctls)[W]);
		break;
	}
	Midi.SendMidiControl(15, Param, ((unsigned char *) us428_ctls)[W]);
}

void Cus428State::WheelChangedTo(E_In84 W, char Diff)
{
	if (W == eWheelPan && StateInputMonitor() && Light[0].Value) {
		int index = 0;
		while( index < 4 && (1 << index) !=  Light[0].Value)
			index++;
		if (index >= 4)
			return;
		Volume[index].PanTo(Diff, us428_ctls->Knob(eK_SET));
		if (!LightIs(eL_Mute0 + index))
			SendVolume(Volume[index]);
		return;
	}

	UserWheelChangedTo(W, Diff);
}


// Convert time-code (hh:mm:ss:ff:fr) into absolute wheel position.
void Cus428State::LocateWheel ( unsigned char *tc )
{
	aWheel  = (60 * 60 * 30) * (int) tc[0]		// hh - hours    [0..23]
			+ (     60 * 30) * (int) tc[1]		// mm - minutes  [0..59]
			+ (          30) * (int) tc[2]		// ss - seconds  [0..59]
			+                  (int) tc[3];		// ff - frames   [0..29]
}


// Convert absolute wheel position into time-code (hh:mm:ss:ff:fr)
void Cus428State::LocateTimecode ( unsigned char *tc )
{
	int W = aWheel;

	tc[0] = W / (60 * 60 * 30); W -= (60 * 60 * 30) * (int) tc[0];
	tc[1] = W / (     60 * 30); W -= (     60 * 30) * (int) tc[1];
	tc[2] = W / (          30); W -= (          30) * (int) tc[2];
	tc[3] = W;
	tc[4] = 0;
}


// Get the wheel differential.
void Cus428State::WheelDelta ( int W )
{
	// Compute the wheel differential.
	int dW = W - W0;
	if (dW > 0 && dW > +W_DELTA_MIN)
		dW -= W_DELTA_MAX;
	else
		if (dW < 0 && dW < -W_DELTA_MIN)
			dW += W_DELTA_MAX;

	W0 = W;
	aWheel += dW;

	// Can't be less than zero.
	if (aWheel < 0)
		aWheel = 0;

	// Now it's whether we're running transport already...
	if (aWheelSpeed)
		WheelShuttle(dW);
	else
		WheelStep(dW);
}

// Get the wheel step.
void Cus428State::WheelStep ( int dW )
{
	unsigned char step;

	if (dW < 0)
		step = (unsigned char) (((-dW & 0x3f) << 1) | 0x40);
	else
		step = (unsigned char) ((dW << 1) & 0x3f);

	Midi.SendMmcCommand(MMC_CMD_STEP, &step, sizeof(step));
}


// Set the wheel shuttle speed.
void Cus428State::WheelShuttle ( int dW )
{
	unsigned char shuttle[3];
	int V, forward;

	// Update the current absolute wheel shuttle speed.
	aWheelSpeed += dW;

	// Don't make it pass some limits...
	if (aWheelSpeed < -W_SPEED_MAX) aWheelSpeed = -W_SPEED_MAX;
	if (aWheelSpeed > +W_SPEED_MAX) aWheelSpeed = +W_SPEED_MAX;

	// Build the MMC-Shuttle command...
	V = aWheelSpeed;
	forward = (V >= 0);
	if (!forward)
		V = -(V);
	shuttle[0] = (unsigned char) ((V >> 3) & 0x07);		// sh
	shuttle[1] = (unsigned char) ((V & 0x07) << 4);		// sm
	shuttle[2] = (unsigned char) 0;						// sl
	if (!forward)
		shuttle[0] |= (unsigned char) 0x40;

	Midi.SendMmcCommand(MMC_CMD_SHUTTLE, &shuttle[0], sizeof(shuttle));
}


// Send the MMC wheel locate command...
void Cus428State::LocateSend ()
{
	unsigned char MmcData[6];
	// Timecode's embedded on MMC command.
	MmcData[0] = 0x01;
	LocateTimecode(&MmcData[1]);
	// Send the MMC locate command...
	Midi.SendMmcCommand(MMC_CMD_LOCATE, MmcData, sizeof(MmcData));
}


// Toggle application transport state.
void Cus428State::TransportToggle ( unsigned char T )
{
	switch (T) {
	case T_PLAY:
		if (uTransport & T_PLAY) {
			uTransport = T_STOP;
			Midi.SendMmcCommand(MMC_CMD_STOP);
		} else {
			uTransport &= T_RECORD;
			uTransport |= T_PLAY;
			Midi.SendMmcCommand(MMC_CMD_PLAY);
		}
		break;
	case T_RECORD:
		if (uTransport & T_RECORD) {
			uTransport &= ~T_RECORD;
			Midi.SendMmcCommand(MMC_CMD_RECORD_EXIT);
		} else {
			uTransport &= T_PLAY;
			uTransport |= T_RECORD;
			Midi.SendMmcCommand(uTransport & T_PLAY ? MMC_CMD_RECORD_STROBE : MMC_CMD_RECORD_PAUSE);
		}
		break;
	default:
		if (uTransport & T) {
			uTransport = T_STOP;
		} else {
			uTransport = T;
		}
		if (uTransport & T_STOP)
			Midi.SendMmcCommand(MMC_CMD_STOP);
		if (uTransport & T_REW)
			Midi.SendMmcCommand(MMC_CMD_REWIND);
		if (uTransport & T_F_FWD)
			Midi.SendMmcCommand(MMC_CMD_FAST_FORWARD);
		break;
	}

	TransportSend();
}


// Set application transport state.
void Cus428State::TransportSet ( unsigned char T, bool V )
{
	if (V) {
		if (T == T_RECORD) {
			uTransport |= T_RECORD;
		} else {
			uTransport  = T;
		}
	} else {
		if (T == T_RECORD) {
			uTransport &= ~T_RECORD;
		} else {
			uTransport  = T_STOP;
		}
	}

	TransportSend();
}


// Update transport status lights.
void Cus428State::TransportSend()
{
	// Common ground for shuttle speed set.
	if (uTransport & T_PLAY)
		aWheelSpeed = ((W_SPEED_MAX + 1) >> 3);
	else if (uTransport & T_REW)
		aWheelSpeed = -(W_SPEED_MAX + 1);
	else if (uTransport & T_F_FWD)
		aWheelSpeed = +(W_SPEED_MAX + 1);
	else
		aWheelSpeed = 0;

	// Lightning feedback :)
	LightSet(eL_Play,   (uTransport & T_PLAY));
	LightSet(eL_Record, (uTransport & T_RECORD));
	LightSet(eL_Rew,    (uTransport & T_REW));
	LightSet(eL_FFwd,   (uTransport & T_F_FWD));
	LightSend();
}


// Set new bank layer state.
void Cus428State::BankSet( int B )
{
	if (B >= 0 && B < cBanks) {
		if (!StateInputMonitor()) {
			bool bSolo = LightIs(eL_Solo);
			Select[aBank] = Light[0].Value;
			Rec[aBank] = Light[1].Value;
			if (bSolo) {
				Solo[aBank] = Light[2].Value;
			} else {
				Mute[aBank] = Light[2].Value;
			}
			Light[0].Value = Select[B];
			Light[1].Value = Rec[B];
			if (bSolo) {
				Light[2].Value = Solo[B];
			} else {
				Light[2].Value = Mute[B];
			}
		}
		aBank = B;
	}

	BankSend();
}


// Update bank status lights.
void Cus428State::BankSend()
{
	LightSet(eL_BankL, (aBank == 0));
	LightSet(eL_BankR, (aBank == cBanks - 1));
	LightSend();
}


// Reset MMC state.
void Cus428State::MmcReset()
{
	W0 = 0;
	aBank = 0;
	aWheel = aWheel_L = aWheel_R = 0;
	aWheelSpeed = 0;
	bSetLocate = false;
	bSetRecord = false;
	uTransport = 0;

	TransportSend();
	LocateSend();
	BankSend();
}


// Process MMC maked-write sub-command.
void Cus428State::MaskedWrite ( unsigned char *data )
{
	// data[0] - sub-command / information field.
	// data[1] - target track bitmap byte address.
	// data[2] - bitmap changed mask.
	// data[3] - bitmap changed value.

	int track = (data[1] > 0 ? (data[1] * 7) : 0) - 5;
	for (int i = 0; i < 7; ++i) {
		int mask = (1 << i);
		if (data[2] & mask) {
			// Only touch tracks that have the "mask" bit set.
			int enable = (data[3] & mask);
			int bank = (track / Y);
			int N = (track % Y);
			switch (data[0]) {
			case MMC_CIF_TRACK_RECORD:
				if (verbose > 1)
					fprintf(stderr, "TRACK RECORD(%d, %d).\n", track, enable);
				if (!StateInputMonitor() && bank >= 0 && bank < cBanks) {
					if (bank == aBank) {
						LightSet(eL_Rec0 + N, enable);
						LightSend();
					} else if (enable) {
						Rec[bank] |=  (1 << N);
					} else {
						Rec[bank] &= ~(1 << N);
					}
				}
				break;
			case MMC_CIF_TRACK_MUTE:
				if (verbose > 1)
					fprintf(stderr, "TRACK MUTE(%d, %d).\n", track, enable);
				if (!StateInputMonitor() && bank >= 0 && bank < cBanks) {
					if (bank == aBank && !LightIs(eL_Solo)) {
						LightSet(eL_Mute0 + N, enable);
						LightSend();
					} else if (enable) {
						Mute[bank] |=  (1 << N);
					} else {
						Mute[bank] &= ~(1 << N);
					}
				}
				break;
			case MMC_CIF_TRACK_SOLO:
				if (verbose > 1)
					fprintf(stderr, "TRACK SOLO(%d, %d).\n", track, enable);
				if (!StateInputMonitor() && bank >= 0 && bank < cBanks) {
					if (bank == aBank && LightIs(eL_Solo)) {
						LightSet(eL_Mute0 + N, enable);
						LightSend();
					} else if (enable) {
						Solo[bank] |=  (1 << N);
					} else {
						Solo[bank] &= ~(1 << N);
					}
				}
				break;
			default:
				break;
			}
		}
		track++;
	}
}


// Send own MMC masked-write subcommand.
void Cus428State::SendMaskedWrite ( unsigned char scmd, int track, bool V )
{
	unsigned char data[4];
	int mask = (1 << (track < 2 ? track + 5 : (track - 2) % 7));

	data[0] = scmd;
	data[1] = (unsigned char) (track < 2 ? 0 : 1 + (track - 2) / 7);
	data[2] = (unsigned char) mask;
	data[3] = (unsigned char) (V ? mask : 0);

	Midi.SendMmcCommand(MMC_CMD_MASKED_WRITE, &data[0], sizeof(data));
}



Cus428StateMixxx::Cus428StateMixxx(
	struct us428ctls_sharedmem* Pus428ctls_sharedmem, int y)
	: Cus428State(Pus428ctls_sharedmem, y)
{
	focus = 0;
	eq = 0;
	LightSet(eL_Low, 1);
	LightSet(eL_LowMid, 0);
	LightSet(eL_HiMid, 0);
	LightSet(eL_High, 0);
	LightSend();
}

void Cus428StateMixxx::UserKnobChangedTo(eKnobs K, bool V)
{
	switch (K) {
	case eK_BANK_L:
		if (verbose > 1)
			printf("Knob BANK_L now %i\n", V);
		if (V) LightSet(eL_BankL, !LightIs(eL_BankL));
		LightSend();
		Midi.SendMidiNote(0, 51, V ? 127 : 0);
		break;
	case eK_BANK_R:
		if (verbose > 1)
			printf("Knob BANK_R now %i\n", V);
		if (V) LightSet(eL_BankR, !LightIs(eL_BankR));
		LightSend();
		Midi.SendMidiNote(1, 51, V ? 127 : 0);
		break;
	case eK_REW:
		if (verbose > 1)
			printf("Knob REW now %i\n", V);
		Midi.SendMidiNote(focus, 60, V ? 127 : 0);
		break;
	case eK_FFWD:
		if (verbose > 1)
			printf("Knob FFWD now %i\n", V);
		Midi.SendMidiNote(focus, 61, V ? 127 : 0);
		break;
	case eK_STOP:
		if (verbose > 1)
			printf("Knob STOP now %i\n", V);
		Midi.SendMidiNote(focus, 62, V ? 127 : 0);
		break;
	case eK_PLAY:
		if (verbose > 1)
			printf("Knob PLAY now %i\n", V);
		Midi.SendMidiNote(focus, 63, V ? 127 : 0);
		break;
	case eK_RECORD:
		if (verbose > 1)
			printf("Knob RECORD now %i\n", V);
		Midi.SendMidiNote(focus, 64, V ? 127 : 0);
		break;
	case eK_LOW:
		if (verbose > 1)
			printf("Knob LOW now %i\n", V);
		if (V)
			{
				eq = 0;
				LightSet(eL_Low, 1);
				LightSet(eL_LowMid, 0);
				LightSet(eL_HiMid, 0);
				LightSet(eL_High, 0);
				LightSend();
			}
		break;
	case eK_LOWMID:
		if (verbose > 1)
			printf("Knob LOWMID now %i\n", V);
		if (V)
			{
				eq = 1;
				LightSet(eL_Low, 0);
				LightSet(eL_LowMid, 1);
				LightSet(eL_HiMid, 0);
				LightSet(eL_High, 0);
				LightSend();
			}
		break;
	case eK_HIMID:
		if (verbose > 1)
			printf("Knob HIMID now %i\n", V);
		if (V)
			{
				eq = 2;
				LightSet(eL_Low, 0);
				LightSet(eL_LowMid, 0);
				LightSet(eL_HiMid, 1);
				LightSet(eL_High, 0);
				LightSend();
			}
		break;
	case eK_HIGH:
		if (verbose > 1)
			printf("Knob HIGH now %i\n", V);
		if (V)
			{
				eq = 3;
				LightSet(eL_Low, 0);
				LightSet(eL_LowMid, 0);
				LightSet(eL_HiMid, 0);
				LightSet(eL_High, 1);
				LightSend();
			}
		break;
	case eK_SET:
		if (verbose > 1)
			printf("Knob SET now %i\n", V);
		Midi.SendMidiNote(focus, 65, V ? 127 : 0);
		break;
	case eK_LOCATE_L:
		if (verbose > 1)
			printf("Knob LOCATE_L now %i\n", V);
		if (V) {
			focus = 0;
		}
		break;
	case eK_LOCATE_R:
		if (verbose > 1)
			printf("Knob LOCATE_R now %i\n", V);
		if (V) {
			focus = 1;
		}
		break;
	default:
		if (verbose > 1)
			printf("Knob %i now %i\n", K, V);
		if (K >= eK_Select0 && K <= eK_Select0 + 7) {
			if (V) LightSet(eL_Select0 + (K - eK_Select0), !LightIs(eL_Select0 + (K - eK_Select0)));
			LightSend();
		} else if (K >= eK_Mute0 && K <= eK_Mute0 + 7) {
			if (V) LightSet(eL_Mute0 + (K - eK_Mute0), !LightIs(eL_Mute0 + (K - eK_Mute0)));
			LightSend();
		}
		Midi.SendMidiNote(0, K, V);
	}
}

void Cus428StateMixxx::UserSliderChangedTo(int S, unsigned char New)
{
	//  if (verbose > 1)
	//  printf("Slider : %d - %d - %d\n", S, New, ((unsigned char*)us428_ctls)[S]);
	Midi.SendMidiControl(0, 0x40 + S, ((unsigned char*)us428_ctls)[S] / 2);
}

void Cus428StateMixxx::UserWheelChangedTo(E_In84 W, char Diff)
{
	char Param;
	char Value;
	char Channel;
	//if (verbose > 1)
	//  printf("Slider : %d - %d - %d\n", W, Diff, ((unsigned char *) us428_ctls)[W]);

	Channel = 0;
	switch (W) {
	case eWheelGain:
		Param = 0x48 + eq * 4;
		break;
	case eWheelFreq:
		Param = 0x49 + eq * 4;
		break;
	case eWheelQ:
		Param = 0x4A + eq * 4;
		break;
	case eWheelPan:
		Param = 0x4B + eq * 4;
		break;
	case eWheel:
		Param = 0x60;
		Channel = focus;
		// Update the absolute wheel position.
		//WheelDelta((int) ((unsigned char *) us428_ctls)[W]);
		break;
	}
	Value = 64 + Diff;
	Midi.SendMidiControl(Channel, Param, Value);
	//Midi.SendMidiControl(0, Param, ((unsigned char *) us428_ctls)[W]);
}
