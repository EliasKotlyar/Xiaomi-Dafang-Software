#define FALSE			0
#define TRUE			1

/* Default device for output */
#define AUDIONAME		"/dev/dsp"

#define ZCAT			"/usr/bin/zcat"

/* Playback frequency in Hertz:
 *
 * 386SX  ->  8000 (or 4000)
 * 386/25 -> 12000
 * 486/50 -> 23000
 */
#define DEFAULT_DSP_SPEED	44100

#define DSP_DEFAULT_STEREO	TRUE

/* The default sample size can be 8 or 16 bits/sample */
#define DSP_DEFAULT_SAMPLESIZE	8

typedef struct
{						/*************************************/
  char *info;			/* Sample data as a series of bytes  */
  unsigned int length;		/* Length of sample in bytes         */
  int volume;			/* Default volume 0-64 (min-max)     */
  unsigned int rep_start;	/* Byte offset of repeat start       */
  unsigned int rep_end;		/* Byte offset of repeat end         */
}
Voice;				/*************************************/

typedef struct
{						/*************************************/
  int period[64][4];		/* Period (pitch) of note        */
  char sample[64][4];		/* Sample number to use          */
  char effect[64][4];		/* Effect number (command)       */
  unsigned char params[64][4];	/* Effect parameters             */
}
Pattern;			/*************************************/

typedef struct
{						/*************************************/
  unsigned int pointer;		/* Current position in sample        */
  unsigned int step;		/* Sample offset increment       */
  int samp;			/* Number of sample currently used   */
  int pitch;			/* Current pitch             */
  int volume;			/* Volume of current note (0-64)     */
  int doarp;			/* TRUE if doing arpeggio        */
  int arp[3];			/*   The three notes in the arpeggio */
  int doslide;			/* TRUE if doing slide           */
  int slide;			/*   Slide speed and direction       */
  int doporta;			/* TRUE if doing portamento      */
  int pitchgoal;		/*   Pitch to reach          */
  int portarate;		/*   Rate at which to approach pitch */
  int dovib;			/* TRUE if doing vibrato         */
  int vibspeed;			/*   Speed of vibrato 0-15 << 2      */
  int vibamp;			/*   Amplitude 0-15          */
  int viboffs;			/*   Current offset in sine wave     */
  int doslidevol;		/* TRUE if doing volume slide        */
  int volslide;			/*   Slide speed 0-64            */
}
Channel;			/*************************************/

#define MIN(a, b)		((a) < (b) ? (a) : (b))
#define MAX(a, b)		((a) > (b) ? (a) : (b))

#define ABUF_SIZE		abuf_size

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS		0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE		1
#endif
