#ifndef WIN_MAIN_H
#define WIN_MAIN_H

#include "../../types.h"

// #defines

#define VNSCLIP  ((eoptions&EO_CLIPSIDES)?8:0)
#define VNSWID   ((eoptions&EO_CLIPSIDES)?240:256)

#define SO_FORCE8BIT  1
#define SO_SECONDARY  2
#define SO_GFOCUS     4
#define SO_D16VOL     8
#define SO_MUTEFA     16
#define SO_OLDUP      32

#define GOO_DISABLESS   1       /* Disable screen saver when game is loaded. */
#define GOO_CONFIRMEXIT 2       /* Confirmation before exiting. */
#define GOO_POWERRESET  4       /* Confirm on power/reset. */

/* Some timing-related variables (now ignored). */
static int maxconbskip = 32;             /* Maximum consecutive blit skips. */
static int ffbskip = 32;              /* Blit skips per blit when FF-ing */

static int moviereadonly = 1;

static int fullscreen = 0;
static int soundflush = 0;
// Flag that indicates whether Game Genie is enabled or not.
static int genie = 0;

// Flag that indicates whether PAL Emulation is enabled or not.
static int pal_emulation = 0;
static int status_icon = 1;

static int vmod = 0;
static char *gfsdir=0;

/** 
* Contains the names of the overridden standard directories
* in the order cheats, misc, nonvol, states, snaps, ..., base
**/
static char *directory_names[6] = {0, 0, 0, 0, 0, 0};

/**
* Contains the names of the default directories.
**/
static const char *default_directory_names[5] = {"cheats", "sav", "fcs", "snaps", "movie"};

#define NUMBER_OF_DIRECTORIES sizeof(directory_names) / sizeof(*directory_names)
#define NUMBER_OF_DEFAULT_DIRECTORIES sizeof(default_directory_names) / sizeof(*default_directory_names)

static double saspectw = 1, saspecth = 1;
static double winsizemulx = 1, winsizemuly = 1;
static int winwidth, winheight;
static int ismaximized = 0;
static uint32 goptions = GOO_DISABLESS;

static int soundrate = 44100;
static int soundbuftime = 50;
static int soundvolume = 100;
static int soundquality = 0;
static uint8 cpalette[192];
static int srendlinen = 8;
static int erendlinen = 231;
static int srendlinep = 0;
static int erendlinep = 239;

extern int soundo;
extern int eoptions;
extern int soundoptions;

#endif