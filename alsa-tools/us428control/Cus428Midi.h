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

#include "Cus428State.h"

// MMC Command Codes.
#define MMC_CMD_STOP                    0x01
#define MMC_CMD_PLAY                    0x02
#define MMC_CMD_DEFERRED_PLAY           0x03
#define MMC_CMD_FAST_FORWARD            0x04
#define MMC_CMD_REWIND                  0x05
#define MMC_CMD_RECORD_STROBE           0x06
#define MMC_CMD_RECORD_EXIT             0x07
#define MMC_CMD_RECORD_PAUSE            0x08
#define MMC_CMD_PAUSE                   0x09
#define MMC_CMD_EJECT                   0x0a
#define MMC_CMD_CHASE                   0x0b
#define MMC_CMD_COMMAND_ERROR_RESET     0x0c
#define MMC_CMD_MMC_RESET               0x0d
#define MMC_CMD_JOG_START               0x20
#define MMC_CMD_JOG_STOP                0x21
#define MMC_CMD_WRITE                   0x40
#define MMC_CMD_MASKED_WRITE            0x41
#define MMC_CMD_READ                    0x42
#define MMC_CMD_UPDATE                  0x43
#define MMC_CMD_LOCATE                  0x44
#define MMC_CMD_VARIABLE_PLAY           0x45
#define MMC_CMD_SEARCH                  0x46
#define MMC_CMD_SHUTTLE                 0x47
#define MMC_CMD_STEP                    0x48
#define MMC_CMD_ASSIGN_SYSTEM_MASTER    0x49
#define MMC_CMD_GENERATOR_COMMAND       0x4a
#define MMC_CMD_MTC_COMMAND             0x4b
#define MMC_CMD_MOVE                    0x4c
#define MMC_CMD_ADD                     0x4d
#define MMC_CMD_SUBTRACT                0x4e
#define MMC_CMD_DROP_FRAME_ADJUST       0x4f
#define MMC_CMD_PROCEDURE               0x50
#define MMC_CMD_EVENT                   0x51
#define MMC_CMD_GROUP                   0x52
#define MMC_CMD_COMMAND_SEGMENT         0x53
#define MMC_CMD_DEFERRED_VARIABLE_PLAY  0x54
#define MMC_CMD_RECORD_STROBE_VARIABLE  0x55
#define MMC_CMD_WAIT                    0x7c
#define MMC_CMD_RESUME                  0x7f

// Available MMC Masked Write sub-commands (information fields).
#define MMC_CIF_TRACK_RECORD            0x4f
#define MMC_CIF_TRACK_MUTE              0x62
#define MMC_CIF_TRACK_SOLO              0x66 // Custom-implementation ;)


class Cus428Midi {
public:
	Cus428Midi():
		Seq(0){}

	int CreatePorts(){
		int Err;
		if (0 <= (Err = snd_seq_open(&Seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK))) {
			snd_seq_set_client_name(Seq, "US-428");
			Err = snd_seq_create_simple_port(Seq, "Controls",
							 SND_SEQ_PORT_CAP_READ
							 |SND_SEQ_PORT_CAP_WRITE
							 |SND_SEQ_PORT_CAP_SUBS_READ
							 |SND_SEQ_PORT_CAP_SUBS_WRITE,
							 SND_SEQ_PORT_TYPE_MIDI_GENERIC);
			if (Err >= 0) {
				Port = Err;
				snd_seq_ev_clear(&Ev);
				snd_seq_ev_set_direct(&Ev);
				snd_seq_ev_set_source(&Ev, Port);
				snd_seq_ev_set_subs(&Ev);
			}
		}
		return Err;
	}

	int SendMidiControl(char Channel, char Param, char Val){
		snd_seq_ev_set_controller(&Ev, Channel, Param, Val & 0x7F);
		SubMitEvent();
		return 0;
	}

	int SendMidiNote(char Channel, char Note, char Val){
		if (!Val)
			snd_seq_ev_set_noteoff(&Ev, Channel, Note, Val & 0x7F);
		else
			snd_seq_ev_set_noteon(&Ev, Channel, Note, Val & 0x7F);
		SubMitEvent();
		return 0;
	}

	int SendMidiControl(char Channel, Cus428State::eKnobs K, bool Down){
		return SendMidiControl(Channel, KnobParam[K - Cus428State::eK_RECORD], Down ? 0x7F : 0);
	}

	// To parse and dispatch input MIDI events.
	void ProcessMidiEvents();

	// Send MMC command.
	void SendMmcCommand(unsigned char MmcCmd, unsigned char *MmcData = 0, unsigned char MmcLen = 0);

	// Made public for friendliness.
	snd_seq_t *Seq;

private:
	int Port;
	snd_seq_event_t Ev;
	int SubMitEvent(){
		snd_seq_event_output(Seq, &Ev);
		snd_seq_drain_output(Seq);
		return 0;
	}
	static char KnobParam[];
};

extern Cus428Midi Midi;
