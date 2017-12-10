/* -*- mode:C++; indent-tabs-mode:t; tab-width:8; c-basic-offset: 8 -*- */
/*
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

#include <alsa/asoundlib.h>
#include "Cus428Midi.h"


char Cus428Midi::KnobParam[] = {
	0x17,
	0x16,
	0x15,
	0x14,
	0x13,
	0x2A,
	0x29,
	0x28,
	(char)-1,
	0x10,
	0x11,
	0x18,
	0x19,
	0x1A,
	(char)-1,
	(char)-1,
	(char)-1,
	(char)-1,
	0x2C,
	0x2D,
	0x2E,
	0x2F,
	(char)-1,
	(char)-1,
	0x20,
	0x21,
	0x22,
	0x23,
	0x24,
	0x25,
	0x26,
	0x27,
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	(char)-1,
	(char)-1,
	(char)-1,
	(char)-1,
	(char)-1,
	(char)-1,
	(char)-1,
	(char)-1,
	0x30,
	0x31,
	0x32,
	0x33,
	0x34,
	0x35,
	0x36,
	0x37,
};

extern int verbose;


// Parse and dispatch input MIDI events.
void Cus428Midi::ProcessMidiEvents()
{
	snd_seq_event_t *ev;
	unsigned char *data;

	do {
		ev = NULL;
		snd_seq_event_input(Seq, &ev);
		if (ev == NULL)
			break;
		switch (ev->type) {
		case SND_SEQ_EVENT_SYSEX:
			data = (unsigned char *) ev->data.ext.ptr;
			if (data[1] == 0x7f && data[3] == 0x06) {
				// MMC Command code is in data[4]...
				if (verbose > 1)
					fprintf(stderr, "MMC Command 0x%02x: ", data[4]);
				switch (data[4]) {
				case MMC_CMD_STOP:
					if (verbose > 1)
						fprintf(stderr, "STOP.\n");
					OneState->TransportSet(T_STOP, true);
					break;
				case MMC_CMD_PLAY:
				case MMC_CMD_DEFERRED_PLAY:
					if (verbose > 1)
						fprintf(stderr, "PLAY.\n");
					OneState->TransportSet(T_PLAY, true);
					break;
				case MMC_CMD_FAST_FORWARD:
					if (verbose > 1)
						fprintf(stderr, "FFWD.\n");
					OneState->TransportSet(T_F_FWD, true);
					break;
				case MMC_CMD_REWIND:
					if (verbose > 1)
						fprintf(stderr, "REW.\n");
					OneState->TransportSet(T_REW, true);
					break;
				case MMC_CMD_RECORD_STROBE:
				case MMC_CMD_RECORD_PAUSE:
					if (verbose > 1)
						fprintf(stderr, "RECORD ON.\n");
					OneState->TransportSet(T_RECORD, true);
					break;
				case MMC_CMD_RECORD_EXIT:
					if (verbose > 1)
						fprintf(stderr, "RECORD OFF.\n");
					OneState->TransportSet(T_RECORD, false);
					break;
				case MMC_CMD_LOCATE:
					if (verbose > 1)
						fprintf(stderr, "LOCATE.\n");
					OneState->LocateWheel(&data[7]);
					break;
				case MMC_CMD_MASKED_WRITE:
					if (verbose > 1)
						fprintf(stderr, "MASKED WRITE.\n");
					OneState->MaskedWrite(&data[6]);
					break;
				case MMC_CMD_MMC_RESET:
					if (verbose > 1)
						fprintf(stderr, "MMC RESET.\n");
					OneState->MmcReset();
					break;
				default:
					if (verbose > 1)
						fprintf(stderr, "Not implemented.\n");
					break;
				}
			}	// Look for Tascam US-224/US-428(?) specific LED codes.
			else if (data[1] == 0x4e && data[3] == 0x12) {
				if (verbose > 1)
					fprintf(stderr, "TASCAM Command 0x%02x.\n", data[4]);
				// Transport LEDs.
				switch (data[4]) {
				case 0x01:		// Transport LEDs...
					switch(data[5]) {
					case 0x13:	// REWIND.
						OneState->LightSet(Cus428State::eL_Rew,  data[6]);
						OneState->LightSend();
						break;
					case 0x14:	// FFWD.
						OneState->LightSet(Cus428State::eL_FFwd, data[6]);
						OneState->LightSend();
						break;
					case 0x16:	// PLAY.
						OneState->LightSet(Cus428State::eL_Play, data[6]);
						OneState->LightSend();
						break;
					case 0x17:	// REC.
						OneState->LightSet(Cus428State::eL_Record, data[6]);
						OneState->LightSend();
						break;
					default:
						break;
					}
					break;
				case 0x02:		// Mute LEDs
					OneState->LightSet(Cus428State::eL_Mute0 + data[5], data[6]);
					OneState->LightSend();
					break;
				case 0x03:		// Select LEDs
					OneState->LightSet(Cus428State::eL_Select0 + data[5], data[6]);
					OneState->LightSend();
					break;
				case 0x04:		// Record LEDs
					OneState->LightSet(Cus428State::eL_Rec0 + data[5], data[6]);
					OneState->LightSend();
					break;
				case 0x05:		// Null LED
					OneState->LightSet(Cus428State::eL_Null, data[5]);
					OneState->LightSend();
					break;
				case 0x06:		// Solo LED
					OneState->LightSet(Cus428State::eL_Solo, data[5]);
					OneState->LightSend();
					break;
				case 0x07:		// Bank L LED
					OneState->LightSet(Cus428State::eL_BankL, data[5]);
					OneState->LightSend();
					break;
				case 0x08:		// Bank R LED
					OneState->LightSet(Cus428State::eL_BankR, data[5]);
					OneState->LightSend();
					break;
				case 0x10:		// Dump fader position.
					OneState->SliderSend(data[5]);
					break;
				}
			}
			break;
		default:
			break;
		}
		snd_seq_free_event(ev);
	}
	while (snd_seq_event_input_pending(Seq, 0) > 0);
}


// Send MMC command.
void Cus428Midi::SendMmcCommand(unsigned char MmcCmd, unsigned char *MmcData, unsigned char MmcLen)
{
	unsigned char  SysexSize;
	unsigned char  SysexLen;
	unsigned char *SysexData;

	SysexSize = 6;
	if (MmcLen > 0)
		SysexSize += 1 + MmcLen;
	SysexData = (unsigned char *) alloca(SysexSize);
	SysexLen  = 0;

	SysexData[SysexLen++] = 0xf0;	// Sysex header.
	SysexData[SysexLen++] = 0x7f;	// Realtime sysex.
	SysexData[SysexLen++] = 0x7f;	// All-caller-id.
	SysexData[SysexLen++] = 0x06;	// MMC command mode.
	SysexData[SysexLen++] = MmcCmd;	// MMC command code.
	if (MmcData && MmcLen > 0) {
		SysexData[SysexLen++] = MmcLen;
		memcpy(&SysexData[SysexLen], MmcData, MmcLen);
		SysexLen += MmcLen;
	}
	SysexData[SysexLen++] = 0xf7;	// Sysex trailer.

	snd_seq_ev_set_sysex(&Ev, SysexLen, SysexData);

	SubMitEvent();
}
