/* 
 * Event types 0 to 127 are available for private use
 * by applications
 */

#define	EV_PRIVATE_META		0

#define MAX_TRACK	256

struct midi_hdr
{
  int MThd_fmt;
  int MThd_ntrk;		/* Num of tracks */

  int time_mode;
#define TIME_MIDI	1
  int division;
#define TIME_SMPTE	2
  int SMPTE_format;
  int SMPTE_resolution;
};

struct mlib_track
{
  int len;
  unsigned char *events;

/*
 * The flags are set when loading the track. Let's hope they are
 * updated also when the track gets changed.
 */
  unsigned long flags;
#define TRK_MULTICHN		0x00000001	/* More than one channel */
#define TRK_MULTIPGM		0x00000002	/* More than one program */
#define TRK_VEL_NOTEON		0x00000004	/* Events with on vel. <> 64 */
#define TRK_AFTERTOUCH		0x00000008	/* Aftertouch events */
#define TRK_POLY_AFTERTOUCH	0x00000010	/* Polyph. aftertouch events */
#define TRK_VEL_NOTEOFF		0x00000020	/* Events with off vel. <> 64 */
#define TRK_CONTROLS		0x00000040	/* Controller events */
#define TRK_BENDER		0x00000080	/* Bender events */
#define TRK_NOTES		0x00000100	/* At least one note on */
  int init_chn;			/* First chn referenced by the track */
  int init_pgm;			/* First pgm referenced by the track */
  int chn;			/* chn assigned to the track */
  int chnmask;			/* channel bitmap */
  int port;			/* port assigned to the track */
  int pgm;			/* pgm assigned to the track */
  int current_time;
  int noteon_time;		/* Time of the first noteon */
  int end_time;
  int min_note, max_note;	/* Scale info */
  short pgm_map[128];		/* MIDI pgm mapping table */
  short drum_map[128];		/* MIDI drum pgm mapping table */
};
typedef struct mlib_track mlib_track;

struct mlib_desc
{
  int magic;			/* 0x121234 */
  int fd;
  char path[1024];
  struct midi_hdr hdr;

  int curr_trk;
  int trk_offs;
  int next_trk_offs;

  unsigned char buf[1024];
  int bufcnt, bufp;

  unsigned int timesig;

  unsigned char prev_status;	/* For running status */

  mlib_track *control_track;

  mlib_track *tracks[MAX_TRACK];
};

typedef struct mlib_desc mlib_desc;

int mlib_chkdesc (mlib_desc * desc);
mlib_track *mlib_loadtrack (mlib_desc * desc, int *end_detected);
void mlib_deltrack (mlib_track * track);
mlib_desc *mlib_open (char *path);
void mlib_close (mlib_desc * desc);
char *mlib_errmsg (void);
