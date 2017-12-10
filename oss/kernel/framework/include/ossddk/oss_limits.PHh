/*
 * Misc limits
 */

#ifdef __USE_PHPMAKE__
<?php
	require getenv("PHPMAKE_LIBPATH") . "library.php";

	phpmake_init_c();

	// Set the initial defaults
	$MAX_MIXER_DEV = 16;
	$MAX_MIDI_DEV = 32;
	$MAX_TIMER_DEV = $MAX_MIDI_DEV + 4;
	$HARD_MAX_AUDIO_DEVFILES = 256;

	if (getenv("PHPMAKE_PROJECTPATH"))
	{
	    include getenv("PHPMAKE_PROJECTPATH") . "limits.php";
	}

	echo "#define MAX_MIXER_DEV\t" . $MAX_MIXER_DEV . "\n";
	echo "#define MAX_MIDI_DEV\t" . $MAX_MIDI_DEV . "\n";
	echo "#define MAX_TIMER_DEV\t" . $MAX_TIMER_DEV . "\n";
	echo "#define HARD_MAX_AUDIO_DEVFILES\t" . $HARD_MAX_AUDIO_DEVFILES . "\n";
?>
#else
/*
 * Altrnative version for systems where Phpmake is not used
 */
#define MAX_MIXER_DEV	16
#define MAX_MIDI_DEV	32
#define MAX_TIMER_DEV	(MAX_MIDI_DEV+4)
#define HARD_MAX_AUDIO_DEVFILES 256
#endif
