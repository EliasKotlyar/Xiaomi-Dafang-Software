/*
 *   hdsploader - firmware loader for RME Hammerfall DSP cards
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

#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <alsa/sound/hdsp.h>

static u_int32_t code[24413];

#ifndef RPM
# define RPM	4
#endif

int read_bin_file(u_int32_t *array, const char *filename)
{
	FILE *out;

	if ((out = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Unable to open file '%s' for reading\n", filename);
		return -1;
	}
	if (fread(array, 4, 24413, out) != 24413) {
		fclose(out);
		return -1;
	}
	fclose(out);

	return 0;
}

void upload_firmware(int card)
{
    int err;
    snd_hwdep_t *hw;
    snd_hwdep_info_t *info;
    char card_name[6];
    hdsp_version_t version;
    
    hdsp_firmware_t firmware;
    
    snd_hwdep_info_alloca(&info);
    snprintf(card_name, 6, "hw:%i", card);

    printf("Upload firmware for card %s\n", card_name);			    

    if ((err = snd_hwdep_open(&hw, card_name, SND_HWDEP_OPEN_DUPLEX)) != 0) {
	fprintf(stderr, "Error opening hwdep device on card %s.\n", card_name);
	return; 
    }

    if ((err = snd_hwdep_ioctl(hw, SNDRV_HDSP_IOCTL_GET_VERSION, &version)) < 0) {
	fprintf(stderr, "Hwdep ioctl error on card %s : %s.\n", card_name, snd_strerror(err));
	snd_hwdep_close(hw);
	return;
    }

    firmware.firmware_data = code;
    switch (version.io_type) {
    case Multiface:
	if (version.firmware_rev == 0xa) {
	    err = read_bin_file(code, DATAPATH "/multiface_firmware.bin");
	} else {
	    err = read_bin_file(code, DATAPATH "/multiface_firmware_rev11.bin");
	}
	break;
    case Digiface:
	if (version.firmware_rev == 0xa) {
	    err = read_bin_file(code, DATAPATH "/digiface_firmware.bin");
	} else {
	    err = read_bin_file(code, DATAPATH "/digiface_firmware_rev11.bin");
	}
	break;
    case RPM:
	err = read_bin_file(code, DATAPATH "/rpm_firmware.bin");
	break;
    default:
	fprintf(stderr, "Unknown iobox or firmware revision\n");
	snd_hwdep_close(hw);
	return;
    }	
    if (err < 0)
    	return;
    
    if ((err = snd_hwdep_ioctl(hw, SNDRV_HDSP_IOCTL_UPLOAD_FIRMWARE, &firmware)) < 0) {
	fprintf(stderr, "Hwdep ioctl error on card %s : %s.\n", card_name, snd_strerror(err));
	snd_hwdep_close(hw);
	return;
    }

    printf("Firmware uploaded for card %s\n", card_name);
    
    snd_hwdep_close(hw);

    return;
}

int main(int argc, char **argv)
{
    char *name;
    int card;

    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
    
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);    
    card = -1;
    printf("hdsploader - firmware loader for RME Hammerfall DSP cards\n");
    printf("Looking for HDSP + Multiface or Digiface cards :\n");
    while (snd_card_next(&card) >= 0) {
	if (card < 0) {
	    break;
	} else {
	    snd_card_get_longname(card, &name);
	    printf("Card %d : %s\n", card, name);
	    if (!strncmp(name, "RME Hammerfall DSP", 18)) {
		upload_firmware(card);
	    } 
	}
    }
    return 0;    
}
