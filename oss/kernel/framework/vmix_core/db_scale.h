/*
 * Purpose: dB to linear conversion tables for vmix
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

/*
 * Attenuation table for dB->linear conversion. Indexed in steps of 0.5 dB.
 * Table size is 25 dB (first entry is handled as mute).
 */

#ifdef CONFIG_OSS_VMIX_FLOAT
const float vmix_db_table[DB_SIZE + 1] = {
  0.0 /* MUTE */ , 0.0035481, 0.0039811, 0.0044668, 0.0050119,
  0.0056234, 0.0063096, 0.0070795, 0.0079433, 0.0089125,
  0.01, 0.01122, 0.012589, 0.014125, 0.015849,
  0.017783, 0.019953, 0.022387, 0.025119, 0.028184,
  0.031623, 0.035481, 0.039811, 0.044668, 0.050119,
  0.056234, 0.063096, 0.070795, 0.079433, 0.089125,
  0.1, 0.1122, 0.12589, 0.14125, 0.15849,
  0.17783, 0.19953, 0.22387, 0.25119, 0.28184,
  0.31623, 0.35481, 0.39811, 0.44668, 0.50119,
  0.56234, 0.63096, 0.70795, 0.79433, 0.89125,
  1.0				/* Full level */
};
#else
/* #define VMIX_VOL_SCALE	moved to vmix.h */
const int vmix_db_table[DB_SIZE + 1] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 	
	1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 	
	4, 4, 5, 5, 6, 7, 8, 9, 10, 11, 	
	12, 14, 16, 18, 20, 22, 25, 28, 32, 36, 	
	40, 45, 50, 57, 64, 71, 80, 90, 101, 114, 	
	128
};
#endif
