/*
    (C) Copyright 2008, Stephen M. Cameron.

    This file is part of bethewumpus.

    Bethewumpus is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Bethewumpus is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Bethewumpus; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <portaudio.h>

#define GNU_SOURCE
#include <getopt.h>

#include "joystick.h"
#include "rumble.h"
#include "ogg_to_pcm.h"

#define PI (3.1415927)
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 600

#define MEALDIST 15

#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (1024)
#define NCLIPS 76 
#define MAX_CONCURRENT_SOUNDS NCLIPS 
#define ANY_SLOT -1
#define MUSIC_SLOT 1
#define WATER_SLOT 0
#define RUNNING_WATER 0
#define NOMINAL_WATER_VOL (0.01)

#define BREATH_SLOT 1
#define BREATH_SOUND 1
#define NOMINAL_BREATH_VOL (0.1)

#define HEARTBEAT_SLOT 2
#define HEARTBEAT_SOUND 2
#define NOMINAL_HEARTBEAT_VOL (0.05)

#define DRIP_SLOT 3
#define DRIP_SOUND 3
#define NOMINAL_DRIP_VOL (0.06)

#define SCARY_MUSIC_SLOT 4
#define SCARY_MUSIC_SOUND 4
#define NOMINAL_SCARY_MUSIC_VOL (0.009)

#define INTRO_MUSIC_SOUND 5
#define NOMINAL_INTRO_MUSIC_VOL (0.1)

#define BETHEWUMPUS_SOUND 6
#define NOMINAL_BETHEWUMPUS_VOL (0.1)

#define NOMINAL_LEVELONESOUND_VOL (0.1)

#define FALL_WITH_IMPACT_SOUND 8

#define MED_SPLASH1 9
#define MED_SPLASH2 10
#define MED_SPLASH3 11 
#define MED_SPLASH4 12
#define MED_SPLASH5 13
#define MED_SPLASH6 14
#define MED_SPLASH7 15
#define MED_SPLASH8 16
#define LOUD_SPLASH1 17
#define ROCKS1 18
#define ROCKS2 19
#define ROCKS3 20
#define ROCKS4 21
#define ROCKS5 22
#define ROCKS6 23
#define ROCKS7 24
#define ROCKS8 25
#define ROAR 26
#define DINE 27
#define LEVEL_ONE_SOUND 28 
#define LEVEL_TWO_SOUND 29 
#define LEVEL_THREE_SOUND 30 
#define LEVEL_FOUR_SOUND 31 
#define LEVEL_FIVE_SOUND 32 
#define LEVEL_SIX_SOUND 33 
#define LEVEL_SEVEN_SOUND 34 
#define LEVEL_EIGHT_SOUND 35 
#define LEVEL_NINE_SOUND 36 
#define LEVEL_TEN_SOUND 37 
#define FAST_BREATH1 38
#define FAST_BREATH2 39
#define MILD_SURPRISE 40
#define SURPRISE 41
#define HELLO 42
#define OHSHIT 43
#define ARROW_CLATTER 44
#define ARROW_WHOOSH 45
#define QUBODUP_FALL_WITH_IMPACT 46
#define QUBODUP_FUCK_FUCK 47
#define QUBODUP_HELLO 48
#define QUBODUP_JESUS_CHRIST1 49
#define QUBODUP_JESUS_CHRIST2 50
#define QUBODUP_MILD_SURPRISE1 51
#define QUBODUP_MILD_SURPRISE2 52
#define QUBODUP_NORMAL_BREATH1 53
#define QUBODUP_NORMAL_BREATH2 54
#define QUBODUP_NORMAL_BREATH3 55
#define QUBODUP_OH_GOD 56
#define QUBODUP_OH_GOD2 57
#define QUBODUP_SHIT_OH_SHIT 58
#define QUBODUP_WHAT_WAS_THAT 59
#define QUBODUP_WHO_IS_THERE 60
#define QUBODUP_WUMPUS_DINES 61
#define ALPHAHOG_BREATH1 62
#define ALPHAHOG_BREATH2 63
#define ALPHAHOG_BREATH3 64
#define ALPHAHOG_BREATH4 65
#define ALPHAHOG_BREATH5 66
#define ALPHAHOG_BREATH6 67
#define ALPHAHOG_FALL_WITH_IMPACT 68
#define ALPHAHOG_HELLO2 69
#define ALPHAHOG_HELLO_IS_SOMEONE_THERE 70
#define ALPHAHOG_HUUUH 71
#define ALPHAHOG_OH_SHEEIT_SHEEIT 72
#define ALPHAHOG_OHSHIT 73
#define ALPHAHOG_WUMPUS_DINE 74

int framerate_hz;
int sound_device = -1;
int debugmode = 0;
double water_x = SCREEN_WIDTH/2;
double water_y = 0;
int level = 0;
int intro_music_slot = -1;
int Be_The_Wumpus_slot = -1;

struct person_sound_clip {
	int index;
	double vol;
};

struct person_sound_clip_map_entry {
	int count; /* number of clips a person has of a certain type */
	int offset; /* offset into the person's list of the first sound of this type */
};

struct person_sound_clip_map_t {
	struct person_sound_clip *psc;
	struct person_sound_clip_map_entry fall;		/* falling type sounds */
	struct person_sound_clip_map_entry normal_breath;	/* normal breathing sounds */
	struct person_sound_clip_map_entry heartbeat;		/* ...etc. */
	struct person_sound_clip_map_entry dine;
	struct person_sound_clip_map_entry fast_breath;
	struct person_sound_clip_map_entry mild_surprise;
	struct person_sound_clip_map_entry surprise;
	struct person_sound_clip_map_entry fearful_mumbling;
};


struct person_sound_clip steve_clip[] = {
	{ FALL_WITH_IMPACT_SOUND, 1.0 },
	{ BREATH_SOUND, NOMINAL_BREATH_VOL },
	{ HEARTBEAT_SOUND, NOMINAL_HEARTBEAT_VOL },
	{ DINE, 1.0 },
	{ FAST_BREATH1, NOMINAL_BREATH_VOL },
	{ FAST_BREATH2, NOMINAL_BREATH_VOL },
	{ MILD_SURPRISE, NOMINAL_BREATH_VOL },
	{ SURPRISE, NOMINAL_BREATH_VOL },
	{ HELLO, NOMINAL_BREATH_VOL }, 
	{ OHSHIT, NOMINAL_BREATH_VOL },
};

struct person_sound_clip qubodup_clip[] = { /* qubodup is freesound username */
	{ QUBODUP_FALL_WITH_IMPACT, 1.0 },
	{ QUBODUP_NORMAL_BREATH1, NOMINAL_BREATH_VOL * 2.0 },
	{ HEARTBEAT_SOUND, NOMINAL_HEARTBEAT_VOL },
	{ QUBODUP_WUMPUS_DINES, 0.8 },
	{ QUBODUP_NORMAL_BREATH2, NOMINAL_BREATH_VOL * 3.0 },
	{ QUBODUP_NORMAL_BREATH3, NOMINAL_BREATH_VOL * 3.0 },
	{ QUBODUP_MILD_SURPRISE1, NOMINAL_BREATH_VOL * 3.0 },
	{ QUBODUP_MILD_SURPRISE2, NOMINAL_BREATH_VOL * 3.0 },
	{ QUBODUP_HELLO, NOMINAL_BREATH_VOL * 3.0 }, 
	{ QUBODUP_FUCK_FUCK, NOMINAL_BREATH_VOL * 3.0 }, 
	{ QUBODUP_OH_GOD,  NOMINAL_BREATH_VOL * 3.0 },
	{ QUBODUP_OH_GOD2, NOMINAL_BREATH_VOL * 3.0 },
	{ QUBODUP_SHIT_OH_SHIT, NOMINAL_BREATH_VOL * 3.0 },
	{ QUBODUP_WHAT_WAS_THAT, NOMINAL_BREATH_VOL * 3.0 },
	{ QUBODUP_WHO_IS_THERE, NOMINAL_BREATH_VOL * 3.0 },
};

struct person_sound_clip alphahog_clip[] = { /* alphahog is freesound username */
	{ ALPHAHOG_FALL_WITH_IMPACT, 1.0 },
	{ ALPHAHOG_BREATH1, NOMINAL_BREATH_VOL * 3.0 },
	{ ALPHAHOG_BREATH2, NOMINAL_BREATH_VOL * 3.0 },
	{ ALPHAHOG_BREATH3, NOMINAL_BREATH_VOL * 3.0 },
	{ ALPHAHOG_BREATH4, NOMINAL_BREATH_VOL * 3.0 },
	{ ALPHAHOG_BREATH5, NOMINAL_BREATH_VOL * 3.0 },
	{ ALPHAHOG_BREATH6, NOMINAL_BREATH_VOL * 3.0 },
	{ HEARTBEAT_SOUND, NOMINAL_HEARTBEAT_VOL },
	{ ALPHAHOG_WUMPUS_DINE, 1.0 },
	{ ALPHAHOG_HUUUH, NOMINAL_BREATH_VOL * 3.0 },
	{ ALPHAHOG_HELLO2, NOMINAL_BREATH_VOL * 3.0 }, 
	{ ALPHAHOG_HELLO_IS_SOMEONE_THERE, NOMINAL_BREATH_VOL* 3.0  }, 
	{ ALPHAHOG_OHSHIT, NOMINAL_BREATH_VOL * 3.0 },
	{ ALPHAHOG_OH_SHEEIT_SHEEIT, NOMINAL_BREATH_VOL* 3.0  },
};


struct person_sound_clip_map_t person_sound_steve_map = {
	.psc			= steve_clip,
	.fall 			= { 1, 0 }, /* steve has one fall type sound, at offset 0. */
	.normal_breath 		= { 1, 1 }, /* and 1 normal breathing sound at offset 1, etc. */
	.heartbeat		= { 1, 2 },
	.dine			= { 1, 3 },
	.fast_breath		= { 2, 4 },
	.mild_surprise		= { 1, 6 },
	.surprise		= { 1, 7 },
	.fearful_mumbling	= { 2, 8 },
};

struct person_sound_clip_map_t person_sound_qubodup_map = {
	.psc			= qubodup_clip,
	.fall 			= { 1, 0 },
	.normal_breath 		= { 1, 1 },
	.heartbeat		= { 1, 2 },
	.dine			= { 1, 3 },
	.fast_breath		= { 2, 4 },
	.mild_surprise		= { 2, 6 },
	.surprise		= { 1, 7 },
	.fearful_mumbling	= { 7, 8 },
};

struct person_sound_clip_map_t person_sound_alphahog_map = {
	.psc			= alphahog_clip,
	.fall 			= { 1, 0 },
	.normal_breath 		= { 3, 1 },
	.fast_breath		= { 3, 4 },
	.heartbeat		= { 1, 7 },
	.dine			= { 1, 8 },
	.mild_surprise		= { 1, 6 },
	.surprise		= { 1, 9 },
	.fearful_mumbling	= { 4, 10 },
};

struct person_sound_clip_map_t *person_sound[] = {
	&person_sound_alphahog_map,
	&person_sound_steve_map,
	&person_sound_qubodup_map,
};

#define NPERSONS (sizeof(person_sound) / sizeof(person_sound[0]))

int current_person = -1;

struct level_struct {
	double maxv;
	double normalv;
} leveldata[] = {
	{ 0, 0 },
	{ 0.2, 0.2 },
	{ 0.2, 0.3 },
	{ 0.3, 0.4 },
	{ 0.3, 0.5 },
	{ 0.4, 0.5 },
	{ 0.5, 0.5 },
	{ 0.5, 0.6 },
	{ 0.5, 0.7 },
	{ 0.6, 0.8 },
};

struct sound_source {
	double x, y;
	int slot;
	int clip;
	int motion_sound_in_play;
};

struct sound_clip;
typedef void (*sound_end_callback)(int which_slot);
typedef void (*volume_adjust_function)(struct sound_clip *clip);

#define LINEARFALLOFF 1
#define SQRTFALLOFF 2
#define NO_FALLOFF 3

struct volume_adjusting_profile_t {
	double *sourcex, *sourcey;
	double *listenerx, *listenery, *listenerangle;
	int fallofftype;
	volume_adjust_function vaf;
};

struct volume_adjusting_profile_t heartbeat_vap;
struct volume_adjusting_profile_t drip_vap;
struct volume_adjusting_profile_t water_vap;
struct volume_adjusting_profile_t music_vap;

struct sound_clip {
	int active;
	int nsamples;
	int pos;
	int16_t *sample;
	double const_right_vol;
	double const_left_vol;
	double right_vol;
	double left_vol;
	sound_end_callback end_function;
	struct volume_adjusting_profile_t *vap;
} clip[NCLIPS];

struct sound_clip audio_queue[MAX_CONCURRENT_SOUNDS];

int nclips = 0;


struct meal_t {
	struct sound_source s;
	double maxv;
	int anxiety;
	double destx, desty;
	double v;
	int n_arrows;
#define INITIAL_N_ARROWS 0
#define ARROW_FIRE_CHANCE 30
	struct volume_adjusting_profile_t vap;
};

struct meal_t meal;
struct sound_source drip;

struct arrow_t {
	struct sound_source s;
	double vx, vy;
	double lastx, lasty;
	int state;
#define ARROW_STATE_QUIVERED 0
#define ARROW_STATE_IN_FLIGHT 1
#define ARROW_STATE_BOUNCING 2
#define ARROW_STATE_AT_REST 3
	int sound_playing;
	int sound_slot;
	struct volume_adjusting_profile_t vap;
#define ARROW_HIT_DIST 5
#define ARROW_MAX_VELOCITY 5
};

struct arrow_t arrow;

gint timer_tag;
GtkWidget *main_da;
GdkGC *gc = NULL;
GdkColor blackcolor;
int timer = 0;
int joystick;
int forcefeedback;

/* These are used if no joystick is present.  */
/* Very crude.  Go buy a game pad.  They're not */
/* very expensive. */
int up_arrow_pressed;
int down_arrow_pressed;
int left_arrow_pressed;
int right_arrow_pressed;
int space_bar_pressed;

struct player_t {
	double x, y;
	double angle;
	int motion_sound_in_play;
	int roar_sound_in_play;
	int won_round;
};

struct player_t player;

/* This is a callback function. The data arguments are ignored
 * in this example. More on callbacks below. */
static void Quit( GtkWidget *widget,
                   gpointer   data )
{
	stop_all_rumble_effects();
	g_print ("Bye!");
}

static inline int randomn(int n)
{
        return ((random() & 0x0000ffff) * n) >> 16;
}

void choose_random_person_sound(struct person_sound_clip *psc, struct person_sound_clip_map_entry *pscme,
	int *sound, double *vol) 
{
	int n;

	n = randomn(pscme->count);
	*sound = psc[pscme->offset + n].index;
	*vol = psc[pscme->offset + n].vol;
}

#define choose_random_sound(soundtype, sound, vol) \
	choose_random_person_sound(person_sound[current_person]->psc, \
		&person_sound[current_person]->soundtype, (&sound), (&vol))

void setup_volume_adjusting_profile(struct volume_adjusting_profile_t *vap, 
		double *sx, double *sy, double *lx, double *ly, double *la,
		int fallofftype, volume_adjust_function vaf)
{
	vap->sourcex = sx;
	vap->sourcey = sy;
	vap->listenerx = lx;
	vap->listenery = ly;
	vap->listenerangle = la;
	vap->fallofftype = fallofftype;
	vap->vaf = vaf;
}

double safe_denominator(double x) 
{
	if (x < 5.0)
		return 5.0;
	else
		return x;	
}
/***********************************************************************/
/* Beginning of AUDIO related code                                     */
/***********************************************************************/

void find_sound_adjust_factors(double x, double y, double angle, 
	double sx, double sy, double *leftfactor, double *rightfactor, 
	int falloff)
{
	/* x and y are the position of the listener, angle is angle in rads */
	/* that listener is facing.  sx, sy are where the sound source is */
	/* leftfactor and rightfactor are how much to adjust the volume of */
	/* left and right channels */

	double leftdist, rightdist;
	double leftx, lefty, rightx, righty;
	double aheadx, aheady, behindx, behindy;
	double aheaddist, behinddist;

	/* compute positions of left and right ears, each 0.5 units from x,y */
	/* but in oppsite directions, 90 degrees off from the angle he's facing */
	rightx = x - 0.5 * cos(angle - PI/2.0);
	righty = y + 0.5 * sin(angle - PI/2.0);
	leftx = x - 0.5 * cos(angle + PI/2.0);
	lefty = y + 0.5 * sin(angle + PI/2.0);
	aheadx = x - 0.5 * cos(angle);
	aheady = y + 0.5 * sin(angle);
	behindx = x + 0.5 * cos(angle);
	behindy = y - 0.5 * sin(angle);

	/* compute distance of each ear from sound source. */ 

	leftdist = sqrt((sx - leftx)*(sx - leftx) + (sy - lefty)*(sy - lefty)); 	
	rightdist = sqrt((sx - rightx)*(sx - rightx) + (sy - righty)*(sy - righty)); 	

	if (falloff == SQRTFALLOFF) {
		*leftfactor = 10.0/safe_denominator(sqrt(leftdist));
		*rightfactor = 10.0/safe_denominator(sqrt(rightdist));
	} else if (falloff == LINEARFALLOFF) {
		*leftfactor = 4.0/safe_denominator(leftdist);
		*rightfactor = 4.0/safe_denominator(rightdist);
	}

	/* attenuate the volume in the ear facing away from the sound source: */
	if (rightdist > leftdist) {
		*rightfactor = *rightfactor * (1.0 - (0.85 * (rightdist-leftdist)));
	} else {
		*leftfactor = *leftfactor * (1.0 - (0.85 * (leftdist-rightdist)));
	}

	/* There's probably a better way to do this.  If we're facing generally */
	/* towards the sound source, leave things as they are.  If we're facing */
	/* generally away from the sound source, attenuate both ears a bit more */
	/* To figure this, project a point ahead and behind, and measure the dist */
	/* from each to the sound source.  If the point ahead is closer than the */
	/* point behind, then we're facing generally towards th sound source, */
	/* otherwise generally away from it. */

	aheaddist = sqrt((sx - aheadx)*(sx - aheadx) + ((sy - aheady)*(sy - aheady)));
	behinddist = sqrt((sx - behindx)*(sx - behindx) + ((sy - behindy)*(sy - behindy)));

	/* if (aheaddist < behinddist)
		return; */

	*leftfactor = *leftfactor * (1.0 - (0.5 * (aheaddist - behinddist)));
	*rightfactor = *rightfactor * (1.0 - (0.5 * (aheaddist - behinddist)));
}

void normal_volume_adjust_function(struct sound_clip *clip)
{
	struct volume_adjusting_profile_t *vap = clip->vap;
	double left, right;

	find_sound_adjust_factors(*vap->listenerx, *vap->listenery,
			*vap->listenerangle, *vap->sourcex, *vap->sourcey,
			&left, &right, vap->fallofftype);

	clip->left_vol = clip->const_left_vol * left;
	clip->right_vol = clip->const_right_vol * right;
}

void constant_volume_adjust_function(struct sound_clip *clip)
{
	return;
}


int read_ogg_clip(int clipnum, char *filename)
{
	unsigned long long nframes;
	char filebuf[PATH_MAX];
	struct stat statbuf;
	int samplesize, sample_rate;
	int nchannels;
	int rc;

	strncpy(filebuf, filename, PATH_MAX);
	rc = stat(filebuf, &statbuf);
	if (rc != 0) {
		snprintf(filebuf, PATH_MAX, "%s", filename);
		rc = stat(filebuf, &statbuf);
		if (rc != 0) {
			fprintf(stderr, "stat('%s') failed.\n", filebuf);
			return -1;
		}
	}
/*
	printf("Reading sound file: '%s'\n", filebuf);
	printf("frames = %lld\n", sfinfo.frames);
	printf("samplerate = %d\n", sfinfo.samplerate);
	printf("channels = %d\n", sfinfo.channels);
	printf("format = %d\n", sfinfo.format);
	printf("sections = %d\n", sfinfo.sections);
	printf("seekable = %d\n", sfinfo.seekable);
*/
	rc = ogg_to_pcm(filebuf, &clip[clipnum].sample, &samplesize,
		&sample_rate, &nchannels, &nframes);
	if (clip[clipnum].sample == NULL) {
		printf("Can't get memory for sound data for %llu frames in %s\n", 
			nframes, filebuf);
		goto error;
	}

	if (rc != 0) {
		fprintf(stderr, "Error: ogg_to_pcm('%s') failed.\n", 
			filebuf);
		goto error;
	}

	clip[clipnum].nsamples = (int) nframes * nchannels;
	if (clip[clipnum].nsamples < 0)
		clip[clipnum].nsamples = 0;

	return 0;
error:
	return -1;
}

/* precompute 16 2-second clips of various sine waves */
int init_clips()
{
	memset(&audio_queue, 0, sizeof(audio_queue));

	printf("Decoding audio data."); fflush(stdout);
	read_ogg_clip(RUNNING_WATER, "sounds/running_water.ogg");
	read_ogg_clip(BREATH_SOUND, "sounds/breath2.ogg");
	read_ogg_clip(HEARTBEAT_SOUND, "sounds/32857_zimm_heartbeat_regular_trimmed.ogg");
	read_ogg_clip(DRIP_SOUND, "sounds/drips2.ogg");
	read_ogg_clip(SCARY_MUSIC_SOUND, "sounds/scary_wumpus_music.ogg");
	read_ogg_clip(INTRO_MUSIC_SOUND, "sounds/wumpus_intro_music.ogg");
	read_ogg_clip(BETHEWUMPUS_SOUND, "sounds/be_the_wumpus.ogg");
	read_ogg_clip(FALL_WITH_IMPACT_SOUND, "sounds/fall_with_impact.ogg");
	read_ogg_clip(LOUD_SPLASH1, "sounds/loud_splash1.ogg");
	read_ogg_clip(MED_SPLASH1, "sounds/medium_splash1.ogg");
	read_ogg_clip(MED_SPLASH2, "sounds/medium_splash2.ogg");
	read_ogg_clip(MED_SPLASH3, "sounds/medium_splash3.ogg");
	read_ogg_clip(MED_SPLASH4, "sounds/medium_splash4.ogg");
	read_ogg_clip(MED_SPLASH5, "sounds/medium_splash5.ogg");
	read_ogg_clip(MED_SPLASH6, "sounds/medium_splash6.ogg");
	read_ogg_clip(MED_SPLASH7, "sounds/medium_splash7.ogg");
	read_ogg_clip(MED_SPLASH8, "sounds/medium_splash8.ogg");
	read_ogg_clip(ROCKS1, "sounds/rocks1.ogg");
	read_ogg_clip(ROCKS2, "sounds/rocks2.ogg");
	read_ogg_clip(ROCKS3, "sounds/rocks3.ogg");
	read_ogg_clip(ROCKS4, "sounds/rocks4.ogg");
	read_ogg_clip(ROCKS5, "sounds/rocks5.ogg");
	read_ogg_clip(ROCKS6, "sounds/rocks6.ogg");
	read_ogg_clip(ROCKS7, "sounds/rocks7.ogg");
	read_ogg_clip(ROCKS8, "sounds/rocks8.ogg");
	read_ogg_clip(ROAR, "sounds/wumpus_roar.ogg");
	read_ogg_clip(DINE, "sounds/wumpus_dines.ogg");
	read_ogg_clip(LEVEL_ONE_SOUND, "sounds/level_one.ogg");
	read_ogg_clip(LEVEL_TWO_SOUND, "sounds/level_two.ogg");
	read_ogg_clip(LEVEL_THREE_SOUND, "sounds/level_three.ogg");
	read_ogg_clip(LEVEL_FOUR_SOUND, "sounds/level_four.ogg");
	read_ogg_clip(LEVEL_FIVE_SOUND, "sounds/level_five.ogg");
	read_ogg_clip(LEVEL_SIX_SOUND, "sounds/level_six.ogg");
	read_ogg_clip(LEVEL_SEVEN_SOUND, "sounds/level_seven.ogg");
	read_ogg_clip(LEVEL_EIGHT_SOUND, "sounds/level_eight.ogg");
	read_ogg_clip(LEVEL_NINE_SOUND, "sounds/level_nine.ogg");
	read_ogg_clip(LEVEL_TEN_SOUND, "sounds/level_ten.ogg");
	read_ogg_clip(FAST_BREATH1, "sounds/fast_breath1.ogg");
	read_ogg_clip(FAST_BREATH2, "sounds/fast_breath2.ogg");
	read_ogg_clip(MILD_SURPRISE, "sounds/mild_surprise_breath.ogg");
	read_ogg_clip(SURPRISE, "sounds/huuuuuh.ogg");
	read_ogg_clip(HELLO, "sounds/hello.ogg");
	read_ogg_clip(OHSHIT, "sounds/oh_shit_oh_shit.ogg");
	read_ogg_clip(ARROW_CLATTER, "sounds/arrow_clatter.ogg");
	read_ogg_clip(ARROW_WHOOSH, "sounds/arrow_whoosh.ogg");

	read_ogg_clip(QUBODUP_FALL_WITH_IMPACT, "sounds/qubodup_fall_with_impact.ogg");
	read_ogg_clip(QUBODUP_FUCK_FUCK, "sounds/qubodup_fuck_fuck.ogg");
	read_ogg_clip(QUBODUP_HELLO, "sounds/qubodup_hello.ogg");
	read_ogg_clip(QUBODUP_JESUS_CHRIST1, "sounds/qubodup_jesus_christ1.ogg");
	read_ogg_clip(QUBODUP_JESUS_CHRIST2, "sounds/qubodup_jesus_christ2.ogg");
	read_ogg_clip(QUBODUP_MILD_SURPRISE1, "sounds/qubodup_mild_surprise1.ogg");
	read_ogg_clip(QUBODUP_MILD_SURPRISE2, "sounds/qubodup_mild_surprise2.ogg");
	read_ogg_clip(QUBODUP_NORMAL_BREATH1, "sounds/qubodup_normal_breath1.ogg");
	read_ogg_clip(QUBODUP_NORMAL_BREATH2, "sounds/qubodup_normal_breath2.ogg");
	read_ogg_clip(QUBODUP_NORMAL_BREATH3, "sounds/qubodup_normal_breath3.ogg");
	read_ogg_clip(QUBODUP_OH_GOD, "sounds/qubodup_oh_god.ogg");
	read_ogg_clip(QUBODUP_OH_GOD2, "sounds/qubodup_oh_god2.ogg");
	read_ogg_clip(QUBODUP_SHIT_OH_SHIT, "sounds/qubodup_shit_oh_shit.ogg");
	read_ogg_clip(QUBODUP_WHAT_WAS_THAT, "sounds/qubodup_what_was_that.ogg");
	read_ogg_clip(QUBODUP_WHO_IS_THERE, "sounds/qubodup_who_is_there.ogg");
	read_ogg_clip(QUBODUP_WUMPUS_DINES, "sounds/qubodup_wumpus_dines.ogg");

	read_ogg_clip(ALPHAHOG_BREATH1, "sounds/alphahog_breath1.ogg");
	read_ogg_clip(ALPHAHOG_BREATH2, "sounds/alphahog_breath2.ogg");
	read_ogg_clip(ALPHAHOG_BREATH3, "sounds/alphahog_breath3.ogg");
	read_ogg_clip(ALPHAHOG_BREATH4, "sounds/alphahog_breath4.ogg");
	read_ogg_clip(ALPHAHOG_BREATH5, "sounds/alphahog_breath5.ogg");
	read_ogg_clip(ALPHAHOG_BREATH6, "sounds/alphahog_breath6.ogg");
	read_ogg_clip(ALPHAHOG_FALL_WITH_IMPACT, "sounds/alphahog_fall_with_impact.ogg");
	read_ogg_clip(ALPHAHOG_HELLO2, "sounds/alphahog_hello2.ogg");
	read_ogg_clip(ALPHAHOG_HELLO_IS_SOMEONE_THERE, "sounds/alphahog_hello_is_someone_there.ogg");
	read_ogg_clip(ALPHAHOG_HUUUH, "sounds/alphahog_huuuh.ogg");
	read_ogg_clip(ALPHAHOG_OH_SHEEIT_SHEEIT, "sounds/alphahog_oh_sheeit_sheeit.ogg");
	read_ogg_clip(ALPHAHOG_OHSHIT, "sounds/alphahog_ohshit.ogg");
	read_ogg_clip(ALPHAHOG_WUMPUS_DINE, "sounds/alphahog_wumpus_dine.ogg");

	printf("\n");
	return 0;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int SoundCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags, __attribute__ ((unused)) void *userData )
{
	// void *data = userData; /* Prevent unused variable warning. */
	float *out = (float*)outputBuffer;
	int i, j, sample, count = 0;
	(void) inputBuffer; /* Prevent unused variable warning. */
	float outputleft;
	float outputright;

	/* Adjust sound volumes */
	for (i=0;i<NCLIPS;i++) {
		if (!audio_queue[i].active)
			continue;
		if (audio_queue[i].vap != NULL)
			audio_queue[i].vap->vaf(&audio_queue[i]);
	}
	for (i=0; i<framesPerBuffer; i++) {
		outputleft = 0.0;
		outputright = 0.0;
		count = 0;
		for (j=0; j<NCLIPS; j++) {
			if (!audio_queue[j].active || 
				audio_queue[j].sample == NULL)
				continue;
			sample = i*2 + audio_queue[j].pos*2;
			count++;
			if (sample >= audio_queue[j].nsamples) {
				audio_queue[j].active = 0;
				if (audio_queue[j].end_function != NULL)
					audio_queue[j].end_function(i);
				continue;
			}
			outputleft += (float)audio_queue[j].sample[sample]    / (float)INT16_MAX * audio_queue[j].left_vol;
			outputright += (float)audio_queue[j].sample[sample+1] / (float)INT16_MAX * audio_queue[j].right_vol;
		}
		// *out++ = (float) (outputleft * audio_queue[j].left_vol / 2.0);
		// *out++ = (float) (outputright * audio_queue[j].right_vol / 2.0);
		*out++ = (float) outputleft / 2;
		*out++ = (float) outputright / 2;
		// *out++ = (float) output / 2; /* (output / count); */
        }
	for (i=0;i<NCLIPS;i++) {
		if (!audio_queue[i].active)
			continue;
		audio_queue[i].pos += framesPerBuffer;
		if (audio_queue[i].pos*2 >= audio_queue[i].nsamples) {
			printf("Calling end funtion %d\n", i);
			audio_queue[i].end_function(i);
			audio_queue[i].active = 0;
		}
	}
	return 0; /* we're never finished */
}

static PaStream *stream = NULL;

void decode_paerror(PaError rc)
{
	if (rc == paNoError)
		return;
	fprintf(stderr, "An error occured while using the portaudio stream\n");
	fprintf(stderr, "Error number: %d\n", rc);
	fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(rc));
}

void terminate_portaudio(PaError rc)
{
	Pa_Terminate();
	decode_paerror(rc);
}

int initialize_portaudio()
{
	PaStreamParameters outparams;
	PaError rc;

	init_clips();

	rc = Pa_Initialize();
	if (rc != paNoError)
		goto error;
    
	outparams.device = Pa_GetDefaultOutputDevice();  /* default output device */

	if (sound_device != -1)
		outparams.device = sound_device;

	if (outparams.device < 0) {
		rc = -1;
		goto error;
	}

	outparams.channelCount = 2;                      /* stereo output */
	outparams.sampleFormat = paFloat32;              /* 32 bit floating point output */
	outparams.suggestedLatency = 
		Pa_GetDeviceInfo(outparams.device)->defaultLowOutputLatency;
	outparams.hostApiSpecificStreamInfo = NULL;

	rc = Pa_OpenStream(&stream,
		NULL,         /* no input */
		&outparams, SAMPLE_RATE, FRAMES_PER_BUFFER,
		paNoFlag, /* paClipOff, */   /* we won't output out of range samples so don't bother clipping them */
		SoundCallback, NULL /* cookie */);    
	/* printf("Pa_OpenStream returns: %d\n", rc); */
	if (rc != paNoError)
		goto error;
	if ((rc = Pa_StartStream(stream)) != paNoError) {
		printf("Pa_StartSTream returns: %d, paNoError = %d\n", rc, paNoError);
		goto error;
	}
#if 0
	for (i=0;i<20;i++) {
		for (j=0;j<NCLIPS;j++) {
			// printf("clip[%d].pos = %d, active = %d\n", j, clip[j].pos, clip[j].active);
			Pa_Sleep( 250 );
			if (clip[j].active == 0) {
				clip[j].nsamples = CLIPLEN;
				clip[j].pos = 0;
				clip[j].active = 1;
			}
		}
		Pa_Sleep( 1500 );
	}
#endif
	/* printf("REturning %d\n", rc); */
	return rc;
error:
	printf("It no worky!\n");
	terminate_portaudio(rc);
	return rc;
}


void stop_portaudio()
{
	int rc;

	if ((rc = Pa_StopStream(stream)) != paNoError)
		goto error;
	rc = Pa_CloseStream(stream);
error:
	terminate_portaudio(rc);
	return;
}

int add_sound(int which_sound, int which_slot, double left_vol, double right_vol, 
	struct volume_adjusting_profile_t *vap, sound_end_callback end_function);

void advance_level();
void player_roar_sound_end_callback(int which_slot)
{
	player.roar_sound_in_play = 0;
	if (player.won_round) 
		advance_level();
	player.won_round = 0;
}
   
void arrow_whoosh_end_callback(int which_slot)
{
	arrow.state = ARROW_STATE_BOUNCING;
	arrow.sound_playing = 0;
} 

void arrow_clatter_end_callback(int which_slot)
{
	arrow.state = ARROW_STATE_QUIVERED;
	arrow.sound_playing = 0;
} 

void player_motion_sound_end_callback(int which_slot)
{
	player.motion_sound_in_play = 0;
}
    
void meal_motion_sound_end_callback(int which_slot)
{
	meal.s.motion_sound_in_play = 0;
}
    
void heartbeat_end_callback(int which_slot)
{
	add_sound(HEARTBEAT_SOUND, HEARTBEAT_SLOT, 
		NOMINAL_HEARTBEAT_VOL, NOMINAL_HEARTBEAT_VOL,
		&heartbeat_vap, heartbeat_end_callback);
}

void scary_music_end_callback(int which_slot)
{
	add_sound(SCARY_MUSIC_SOUND, SCARY_MUSIC_SLOT, 
		NOMINAL_SCARY_MUSIC_VOL, NOMINAL_SCARY_MUSIC_VOL,
		NULL, scary_music_end_callback);
}

void drip_end_callback(int which_slot)
{
	add_sound(DRIP_SOUND, DRIP_SLOT, 
		NOMINAL_DRIP_VOL, NOMINAL_DRIP_VOL,
		&drip_vap, drip_end_callback);
}

void try_firing_arrow() /* called by breath end callback */
{
	double dx, dy;

	if (meal.n_arrows <= 0) /* no arrows left, bail */
		return;

	if (arrow.state != ARROW_STATE_QUIVERED) /* arrow is not in quiver, bail. */
		return;

	if (randomn(100) > ARROW_FIRE_CHANCE+level) /* not this time, bail. */
		return;

	/* fire the arrow... */
	meal.n_arrows--;
	arrow.s.x = meal.s.x;
	arrow.s.y = meal.s.y;

	dx = player.x - meal.s.x;
	dy = player.y - meal.s.y;

	if (abs(dx) >= abs(dy)) {
		if (dx < 0)
			arrow.vx = - ARROW_MAX_VELOCITY;
		else
			arrow.vx = ARROW_MAX_VELOCITY;
		arrow.vy = dy * ARROW_MAX_VELOCITY/abs(dx);
	} else { /* dy > dx */ 
		if (dy < 0)
			arrow.vy = - ARROW_MAX_VELOCITY;
		else
			arrow.vy = ARROW_MAX_VELOCITY;
		arrow.vx = dx * ARROW_MAX_VELOCITY/abs(dy);
	}

	arrow.vx += (randomn(2*meal.anxiety+1)-meal.anxiety);
	arrow.vy += (randomn(2*meal.anxiety+1)-meal.anxiety);
	
	arrow.state = ARROW_STATE_IN_FLIGHT;
	arrow.sound_playing = 0; /* should already be so */
}

void breath_end_callback(int which_slot)
{

	if (player.won_round)
		return; /* don't breath if meal's been eaten. */	
	if (meal.anxiety > 8) { /* VERY anxious */
		int surprise_sound;
		double surprise_volume;

		choose_random_sound(surprise, surprise_sound, surprise_volume);
		meal.anxiety = 4;
		add_sound(surprise_sound, BREATH_SLOT, 
			surprise_volume, surprise_volume,
			&meal.vap, breath_end_callback);
	} else if (meal.anxiety > 5) {/* pretty damned anxious */
		int mild_surprise_sound;
		double mild_surprise_volume;

		choose_random_sound(mild_surprise, mild_surprise_sound, mild_surprise_volume);
		meal.anxiety = 4;
		add_sound(mild_surprise_sound, BREATH_SLOT, 
			mild_surprise_volume, mild_surprise_volume,
			&meal.vap, breath_end_callback);
		try_firing_arrow();
	} else if (meal.anxiety > 3) {/* anxious */
		int n;
		if (randomn(100) < 50)
			meal.anxiety = 3;
		if (randomn(100) < 10)
			meal.anxiety = 2;
		n = randomn(100);
		if (n < 40) {
			int mumble;
			double mumble_vol;

			choose_random_sound(fearful_mumbling, mumble, mumble_vol);
			add_sound(mumble, BREATH_SLOT, 
				mumble_vol, mumble_vol,
				&meal.vap, breath_end_callback);
		} else {
			int fast_breath_sound;
			double fast_breath_vol;
		
			choose_random_sound(fast_breath, fast_breath_sound, fast_breath_vol);
			add_sound(fast_breath_sound, BREATH_SLOT, 
				fast_breath_vol, fast_breath_vol,
				&meal.vap, breath_end_callback);
		}
		try_firing_arrow();
	} else if (meal.anxiety > 2) {/* starting to get anxious */
		int fast_breath_sound;
		double fast_breath_vol;
		choose_random_sound(fast_breath, fast_breath_sound, fast_breath_vol);
		if (randomn(100) < 10)
			meal.anxiety = 2;
		add_sound(fast_breath_sound, BREATH_SLOT, 
			fast_breath_vol, fast_breath_vol,
			&meal.vap, breath_end_callback);
		try_firing_arrow();
	} else {  	/* as calm as can be when in a dark cave with a hungry wumpus */
		int breath_sound;
		double breath_vol;

		choose_random_sound(normal_breath, breath_sound, breath_vol);
		add_sound(breath_sound, BREATH_SLOT, 
			breath_vol, breath_vol,
			&meal.vap, breath_end_callback);
	}
}

void water_end_callback(int which_slot)
{
	add_sound(RUNNING_WATER, WATER_SLOT, 
		NOMINAL_WATER_VOL, NOMINAL_WATER_VOL,
		&water_vap, water_end_callback);
}

void start_game_callback(int which_slot) 
{


	drip.x = SCREEN_WIDTH;
	drip.y = SCREEN_HEIGHT/2;

	/* add_sound(RUNNING_WATER, WATER_SLOT, 0.07, 0.01, water_end_callback); */
	breath_end_callback(BREATH_SLOT);
	heartbeat_end_callback(HEARTBEAT_SLOT);
}

void add_falling_meal(int which_slot)
{
	
	double dist1r, dist1p, dist2r;
	int fall_sound;
	double fall_sound_vol;

	meal.s.x = 10000;
	meal.s.y = 10000;

	do {
		meal.s.x = randomn(SCREEN_WIDTH);
		meal.s.y = randomn(SCREEN_HEIGHT);
		player.x = randomn(SCREEN_WIDTH);
		player.y = randomn(SCREEN_HEIGHT);

		dist1r = sqrt((meal.s.x - (SCREEN_WIDTH/2)) * (meal.s.x - (SCREEN_WIDTH/2)) +
			(meal.s.y - (SCREEN_HEIGHT/2)) * (meal.s.y - (SCREEN_HEIGHT/2)));
		dist2r = sqrt((player.x - (SCREEN_WIDTH/2)) * (player.x - (SCREEN_WIDTH/2)) +
			(player.y - (SCREEN_HEIGHT/2)) * (player.y - (SCREEN_HEIGHT/2)));

		dist1p = sqrt((meal.s.x - player.x) * (meal.s.x - player.x) + (meal.s.y - player.y) * (meal.s.y - player.y));
	} while (dist1r > (SCREEN_HEIGHT/2) - 10 ||
		dist2r > (SCREEN_HEIGHT/2) - 10 ||
		dist1p < (SCREEN_HEIGHT/3));

	/* find_sound_adjust_factors(player.x, player.y, player.angle, 
		meal.s.x, meal.s.y, &leftadjust, &rightadjust, LINEARFALLOFF); */

	choose_random_sound(fall, fall_sound, fall_sound_vol);
	add_sound(fall_sound, ANY_SLOT, fall_sound_vol, fall_sound_vol, 
			&meal.vap, start_game_callback);
	water_end_callback(WATER_SLOT);
	drip_end_callback(DRIP_SLOT);
	scary_music_end_callback(SCARY_MUSIC_SLOT);
}

void level_callback(int which_slot)
{
	add_sound(LEVEL_ONE_SOUND + level, ANY_SLOT, 0.1, 0.1, NULL, add_falling_meal);
}

void intro_music_callback(int which_slot)
{
	intro_music_slot = -1;
	level_callback(which_slot);
}

void Be_The_Wumpus_callback(int which_slot)
{
	Be_The_Wumpus_slot = -1;
}

void start_intro_music()
{
	intro_music_slot = add_sound(INTRO_MUSIC_SOUND, ANY_SLOT, 0.1, 0.1, NULL, intro_music_callback);
	Be_The_Wumpus_slot = add_sound(BETHEWUMPUS_SOUND, ANY_SLOT, 0.1, 0.1, NULL, Be_The_Wumpus_callback);
}

void adjust_volume(int which_slot, double left_vol, double right_vol)
{
	audio_queue[which_slot].left_vol = left_vol;
	audio_queue[which_slot].right_vol = right_vol;
}

int add_sound(int which_sound, int which_slot, double left_vol, double right_vol, 
	struct volume_adjusting_profile_t *vap, sound_end_callback end_function)
{
	int i;

	if (which_slot != ANY_SLOT) {
		if (audio_queue[which_slot].active)
			audio_queue[which_slot].active = 0;
		audio_queue[which_slot].pos = 0;
		audio_queue[which_slot].nsamples = 0;
		/* would like to put a memory barrier here. */
		audio_queue[which_slot].sample = clip[which_sound].sample;
		audio_queue[which_slot].nsamples = clip[which_sound].nsamples;
		/* would like to put a memory barrier here. */
		audio_queue[which_slot].active = 1;
		audio_queue[which_slot].left_vol = left_vol;
		audio_queue[which_slot].right_vol = right_vol;
		audio_queue[which_slot].const_left_vol = left_vol;
		audio_queue[which_slot].const_right_vol = right_vol;
		audio_queue[which_slot].end_function = end_function;
		audio_queue[which_slot].vap = vap;
		if (vap != NULL && vap->vaf != NULL) 
			vap->vaf(&audio_queue[which_slot]);
		return which_slot;
	}
	for (i=MAX_CONCURRENT_SOUNDS-1;i>=0;i--) {
		if (audio_queue[i].active == 0) {
			audio_queue[i].nsamples = clip[which_sound].nsamples;
			audio_queue[i].pos = 0;
			audio_queue[i].sample = clip[which_sound].sample;
			audio_queue[i].active = 1;
			audio_queue[i].left_vol = left_vol;
			audio_queue[i].right_vol = right_vol;
			audio_queue[i].const_left_vol = left_vol;
			audio_queue[i].const_right_vol = right_vol;
			audio_queue[i].end_function = end_function;
			audio_queue[i].vap = vap;
			if (vap != NULL && vap->vaf != NULL) 
				vap->vaf(&audio_queue[i]);
			break;
		}
	}
	return (i >= MAX_CONCURRENT_SOUNDS) ? -1 : i;
}

void cancel_sound(int queue_entry)
{
	audio_queue[queue_entry].active = 0;
}

void cancel_all_sounds()
{
	int i;
	for (i=0;i<MAX_CONCURRENT_SOUNDS;i++)
		audio_queue[i].active = 0;
}

/***********************************************************************/
/* End of AUDIO related code                                     */
/***********************************************************************/


static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    /* If you return FALSE in the "delete_event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */

    g_print ("delete event occurred\n");

    /* Change TRUE to FALSE and the main window will be destroyed with
     * a "delete_event". */

    return TRUE;
}

void player_draw(GtkWidget *w)
{
	int x2, y2;
	gdk_draw_line(w->window, gc, player.x-3, player.y-3, player.x+3, player.y+3);
	gdk_draw_line(w->window, gc, player.x+3, player.y-3, player.x-3, player.y+3);
	x2 = player.x -cos(player.angle)*25;
	y2 = player.y +sin(player.angle)*25;
	gdk_draw_line(w->window, gc, player.x, player.y, x2, y2);
}

void arrow_draw(GtkWidget *w)
{
	if (arrow.state != ARROW_STATE_IN_FLIGHT && arrow.state != ARROW_STATE_BOUNCING)
		return;

	gdk_draw_line(w->window, gc, arrow.s.x, arrow.s.y, arrow.lastx, arrow.lasty);
}


void meal_draw(GtkWidget *w)
{
	gdk_draw_line(w->window, gc, meal.s.x-3, meal.s.y-3, meal.s.x+3, meal.s.y+3);
	gdk_draw_line(w->window, gc, meal.s.x+3, meal.s.y-3, meal.s.x-3, meal.s.y+3);
}

static gint key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
	switch (event->keyval) {
	case GDK_Up:
		up_arrow_pressed = 1;
		return TRUE;
	case GDK_Down:
		down_arrow_pressed = 1;
		return TRUE;
	case GDK_Left:
		left_arrow_pressed = 1;
		return TRUE;
	case GDK_Right:
		right_arrow_pressed = 1;
		return TRUE;
	case GDK_space:
		space_bar_pressed = 1;
		return TRUE;
	}
	return FALSE;
}

static int main_da_expose(GtkWidget *w, GdkEvent *event, gpointer p)
{
	int i;
	int x, y;
	double angle;

	gdk_gc_set_foreground(gc, &blackcolor);
	// gdk_draw_line(w->window, gc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	// gdk_draw_line(w->window, gc, 0, SCREEN_HEIGHT, SCREEN_WIDTH, 0);

	for (i=0;i<90;i++) {
		angle = (2.0 * PI * (((i*4)+timer) % 360)) / 360.0;
		x = (SCREEN_WIDTH/2) + (SCREEN_WIDTH/2) * cos(angle);
		y = (SCREEN_HEIGHT/2) + (SCREEN_HEIGHT/2) * sin(angle);
		gdk_draw_line(w->window, gc, x-1, y-1, x+1, y+1);
		gdk_draw_line(w->window, gc, x+1, y-1, x-1, y+1);
	}
	player_draw(w);
	meal_draw(w);
	arrow_draw(w);
	return 0;
}

void advance_level()
{
	level++;
	current_person = ((current_person + 1) % NPERSONS);
	if (level > 9)
		level = 9;

	meal.s.x = 1000;
	meal.s.y = 1000;
	meal.maxv = leveldata[level].maxv; 
	meal.v = leveldata[level].normalv;
	meal.anxiety = 0;
	meal.n_arrows = INITIAL_N_ARROWS + level * 2;

	if (level == 0) {
		start_intro_music();	   
	} else {
		level_callback(ANY_SLOT);
	}
}

int in_the_water(double x, double y);
struct wwvi_js_event jse;
void player_move()
{
	int rc, i;
	double radius, oldx, oldy, dx, dy;
	double diffx, diffy, v;
	double keyboard_multiplier;

	rc = get_joystick_status(&jse);
	if (rc != 0) {
		/* Probably because they don't have a joystick. */
		/* Map keys to joystick.  Very crude. */
		jse.stick_x = 0;
		jse.stick_y = 0;
		jse.button[0] = 0;
		keyboard_multiplier = 5;

		if (up_arrow_pressed) {
			jse.stick_y = -16000; 
			up_arrow_pressed = 0;
		}
		if (down_arrow_pressed) {
			jse.stick_y = 16000; 
			down_arrow_pressed = 0;
		}
		if (left_arrow_pressed) {
			jse.stick_x = -16000; 
			left_arrow_pressed = 0;
		}
		if (right_arrow_pressed) {
			jse.stick_x = 16000; 
			right_arrow_pressed = 0;
		}
		if (space_bar_pressed) {
			jse.button[0] = 1;
			space_bar_pressed = 0;
		}
	} else
		keyboard_multiplier = 1.0;

	// printf("x=%d, y=%d\n", jse.stick_x, jse.stick_y);
	/* rotate */
	if (jse.stick_x < -8000) {
		player.angle = player.angle + keyboard_multiplier * 0.1 * abs((jse.stick_x + 7000))/32767.0;
	} else if (jse.stick_x > 8000) {
		player.angle = player.angle - keyboard_multiplier * 0.1 * abs((jse.stick_x - 7000))/32767.0;
	}

	/* move */
	if (jse.stick_y < -8000 || jse.stick_y > 8000) {
		/* if (jse.stick_y < -8000)
			jse.stick_y += 7000;
		if (jse.stick_y > 8000)
			jse.stick_y -= 7000; */
		
		oldx = player.x;
		oldy = player.y;
		player.x = player.x + cos(player.angle) * 3.0 * keyboard_multiplier * (double) jse.stick_y / 32767.0;
		player.y = player.y - sin(player.angle) * 3.0 * keyboard_multiplier * (double) jse.stick_y / 32767.0;
		dx = player.x - (SCREEN_WIDTH/2);
		dy = player.y - (SCREEN_HEIGHT/2);
		radius = sqrt(dx * dx + dy * dy);

		/* See if the player has made movement noise */
		if (!player.motion_sound_in_play) {
			double dist;
			dist = sqrt((meal.s.x - player.x) * (meal.s.x - player.x) + 
				(meal.s.y - player.y) * (meal.s.y - player.y));
			diffx = player.x - oldx;
			diffy = player.y - oldy; 
			v = sqrt(diffx*diffx + diffy*diffy);
			if (in_the_water(player.x, player.y)) {
				if (v > 0.8) {
					player.motion_sound_in_play = 1;
					add_sound(MED_SPLASH1 + (randomn(8)), ANY_SLOT, 0.3 * (v/3.0), 0.3 * (v/3.0), 
						NULL, player_motion_sound_end_callback);
					if (dist < 15)
						meal.anxiety = 10;
					else if (dist < 30)
						meal.anxiety = 8;
					else if (dist < 60)
						meal.anxiety = 5;
					else if (dist < 120)
						meal.anxiety = 3;
					else if (dist < 200)
						meal.anxiety = 2;
				}
			} else {
				if (v > 0.8) {
					player.motion_sound_in_play = 1;
					add_sound(ROCKS1 + (randomn(8)), ANY_SLOT, 0.3 * (v/3.0), 0.3 * (v/3.0), 
						NULL, player_motion_sound_end_callback);
					if (dist < 15)
						meal.anxiety = 10;
					else if (dist < 30)
						meal.anxiety = 8;
					else if (dist < 60)
						meal.anxiety = 5;
					else if (dist < 120)
						meal.anxiety = 3;
					else if (dist < 200)
						meal.anxiety = 2;
				}
			}
		}

		/* If player hit the wall, bring him back */
		if (radius > SCREEN_HEIGHT/2) {
			player.x = oldx;
			player.y = oldy;
			/* Here's where to trigger force feedback, if only it worked. */
		}
	} /* else if (jse.stick_y > 2000) {
		player.x = player.x + cos(player.angle) * 3.0;
		player.y = player.y - sin(player.angle) * 3.0;
	} */

	if (jse.button[0]) {
		if (!player.roar_sound_in_play) {
			double dist, tx, ty;
			player.roar_sound_in_play = 1;
			tx = player.x + cos(player.angle) * 15.0 * (double) jse.stick_y / 32767.0;
			ty = player.y - sin(player.angle) * 15.0 * (double) jse.stick_y / 32767.0;
			dist = sqrt((meal.s.x - tx) * (meal.s.x - tx) + (meal.s.y - ty) * (meal.s.y - ty));
			if (dist < MEALDIST) {
				int dine_sound;
				double dine_vol;

				choose_random_sound(dine, dine_sound, dine_vol);
				add_sound(dine_sound, ANY_SLOT, dine_vol, dine_vol, 
					NULL, player_roar_sound_end_callback);
				stop_all_rumble_effects();

				player.won_round = 1;
			} else {
				add_sound(ROAR, ANY_SLOT, 1.0, 1.0, 
					NULL, player_roar_sound_end_callback);
				player.won_round = 0;
				meal.anxiety = 10;
			}
		}
	}

	if (intro_music_slot == -1)  /* if intro music is not playing... we're done. */
		return;

	/* intro music is playing... allow player to skip it by pressing a button. */
	for (i=0;i<10;i++) {
		if (jse.button[i]) {
			cancel_sound(intro_music_slot);
			intro_music_slot = -1;
			if (Be_The_Wumpus_slot != -1) 
				cancel_sound(Be_The_Wumpus_slot);
			Be_The_Wumpus_slot = -1;
			level_callback(-1);
		}
	}
}

int on_board(double x, double y)
{
	double dist;

	dist = sqrt((x - (SCREEN_WIDTH/2)) * (x - (SCREEN_WIDTH/2)) +
		(y - (SCREEN_HEIGHT/2)) * (y - (SCREEN_HEIGHT/2)));
	return (dist < (SCREEN_HEIGHT/2) - 10);
}

int in_the_water(double x, double y)
{
	return (x > (3.0 * SCREEN_WIDTH/4.0) || y < (SCREEN_HEIGHT/4.0));
}

void arrow_move(struct arrow_t *arrow)
{
	double dist2;

	switch (arrow->state) {
		case ARROW_STATE_QUIVERED:
		case ARROW_STATE_AT_REST:
			return;
		case ARROW_STATE_IN_FLIGHT:
			if (arrow->sound_playing == 0) {
				arrow->sound_slot = add_sound(ARROW_WHOOSH, ANY_SLOT, 1.0, 1.0, 
					&arrow->vap, arrow_whoosh_end_callback);
				arrow->sound_playing = 1;
			}
			arrow->lastx = arrow->s.x;
			arrow->lasty = arrow->s.y;
			arrow->s.x += arrow->vx;
			arrow->s.y += arrow->vy;

			/* hit the wall? */
			if (!on_board(arrow->s.x, arrow->s.y)) {
				arrow->s.x = arrow->lastx;
				arrow->s.y = arrow->lasty;
				arrow->vx = 0;
				arrow->vy = 0;
				if (arrow->sound_playing) {
					cancel_sound(arrow->sound_slot);
				}
				arrow->sound_playing = 1;
				arrow->sound_slot = add_sound(ARROW_CLATTER, ANY_SLOT,
					0.3, 0.3, &arrow->vap, arrow_clatter_end_callback);
				arrow->state = ARROW_STATE_BOUNCING;
			}

			/* See if the wumpus is hit... */
			dist2 =	(arrow->s.x - player.x) * (arrow->s.x - player.x) + 
				(arrow->s.y - player.y) * (arrow->s.y - player.y); 

			if (dist2 < (ARROW_HIT_DIST * ARROW_HIT_DIST)) {
				printf("Arrow hit the wumpus!\n");
				if (arrow->sound_playing) {
					cancel_sound(arrow->sound_slot);
					arrow->sound_playing = 0;
				}
			}

			break;
		case ARROW_STATE_BOUNCING:
			if (arrow->sound_playing == 0) {
				arrow->sound_slot = add_sound(ARROW_CLATTER, ANY_SLOT, 1.0, 1.0, 
					&arrow->vap, arrow_clatter_end_callback);
				arrow->vx = 0;
				arrow->vy = 0;
				arrow->sound_playing = 1;
			}
			break;
		default:
			printf("Unknown arrow state %d\n", arrow->state);
	}
}

void meal_move()
{
	double dx, dy, vx, vy;
	double maxvx, maxvy;

	if (meal.maxv < 0.0005)
		return;

	if (!on_board(meal.destx, meal.desty)) {
		do {
			meal.destx = randomn(SCREEN_WIDTH);
			meal.desty = randomn(SCREEN_HEIGHT);
		} while (!on_board(meal.destx, meal.desty));

		/* printf("New dest %3.1f/%3.1f\n", meal.destx, meal.desty); */
	}
	dx = meal.destx - meal.s.x;
	dy = meal.desty - meal.s.y;

	maxvx = meal.v;
	maxvy = meal.v;

	if (abs(dy) < maxvy)
		maxvy = abs(dy);
	if (abs(dx) < maxvx)
		maxvx = abs(dx);

	if ((abs(dy) < 0.3 && abs(dx) < 0.3) || meal.anxiety == 10) {
		/* reached destination, set new dest off board (will trigger above code next time) */
		meal.destx = 1000;
		meal.desty = 1000;
		/* printf("destination reached.\n"); */
		return;
	}

	if (abs(dy) <= 0.1) {
		vx = (dx < 0) ?  -maxvx: maxvx;
		vy = 0.0;
		meal.s.x += vx;
		meal.s.y += vy;
		/* printf("a d=%3.1f/%3.1f, p=%3.1f/%3.1f, v=%3.1f,%3.1f\n", 
			meal.destx, meal.desty,
			meal.s.x, meal.s.y, vx, vy); */
		return;
	}

	if (abs(dx) <= 0.1) {
		vy = (dy < 0) ? -maxvy : maxvy;
		vx = 0.0;
		meal.s.x += vx;
		meal.s.y += vy;
		/* printf("b d=%3.1f/%3.1f, p=%3.1f/%3.1f, v=%3.1f,%3.1f\n", 
			meal.destx, meal.desty,
			meal.s.x, meal.s.y, vx, vy); */
		return;
	}

	if (abs(dx) < abs(dy)) {
		vy = (dy < 0) ? -maxvy : maxvy;
		vx = vy * dx/dy;
	} else {
		vx = (dx < 0) ? -maxvx : maxvx;
		vy = vx * dy/dx;
	}
	/* printf("maxvx = %3.1lf, maxvy=%3.1lf\n", maxvx, maxvy);
	printf("c d=%3.1f/%3.1f, p=%3.1f/%3.1f, dx/dy=%3.1lf/%3.1lf, v=%3.1f,%3.1f\n", 
			meal.destx, meal.desty,
			meal.s.x, meal.s.y, dx, dy, vx, vy); */

	/* see if the meal is making movement noise... */
	if (!meal.s.motion_sound_in_play) {
		double v;
		v = sqrt((vx*vx) + (vy*vy));

		if (v > 0.3) {
			if (in_the_water(meal.s.x, meal.s.y)) {
				meal.s.motion_sound_in_play = 1;	
				add_sound(MED_SPLASH1 + (randomn(8)), ANY_SLOT, 
					v * 0.2, v * 0.2, 
					&meal.vap, meal_motion_sound_end_callback);
			} else {
				meal.s.motion_sound_in_play = 1;	
				add_sound(ROCKS1 + (randomn(8)), ANY_SLOT, 
					v * 0.2, v * 0.2, 
					&meal.vap, meal_motion_sound_end_callback);
			}
		}
	}

	meal.s.x += vx;
	meal.s.y += vy;
}

void check_for_rumble()
{
	double dist;

	dist = sqrt((meal.s.x - player.x) * (meal.s.x - player.x) +
		(meal.s.y - player.y) * (meal.s.y - player.y));

	if (dist < (SCREEN_WIDTH / 50)) {
		play_rumble_effect(5);	
		return;
	}
	if (dist < (SCREEN_WIDTH / 40)) {
		play_rumble_effect(4);	
		return;
	}
	if (dist < (SCREEN_WIDTH / 30)) {
		play_rumble_effect(3);	
		return;
	}
	if (dist < (SCREEN_WIDTH / 15)) {
		play_rumble_effect(2);	
		return;
	}
	if (dist < (SCREEN_WIDTH / 10)) {
		play_rumble_effect(1);	
		return;
	}
	if (dist < (SCREEN_WIDTH / 5)) {
		play_rumble_effect(0);	
		return;
	}
	stop_all_rumble_effects();
}

gint advance_game(gpointer data)
{
	player_move();
	meal_move();
	arrow_move(&arrow);
	gdk_threads_enter();
	gtk_widget_queue_draw(main_da);
	gdk_threads_leave();
	timer++;

	if (timer % (framerate_hz / 4) == 0)
		check_for_rumble();
	
	// printf("Advance game called, timer = %d.\n", timer);
	return TRUE;
}

/* Another callback */
static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}

static struct option btw_opts[] = {
	{ "sounddevice", 1, 0, 0 },
	{ "version", 0, 0, 1 },
	{ "rumbledevice", 1, 0, 2 },
};

int main( int   argc,
          char *argv[] )
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *vbox;
	GdkColor whitecolor;
	char *rumbledevicestring = NULL;
	char rumbledevice[PATH_MAX];

	struct timeval time;

	while (1) {
		int rc, n;
		rc = getopt_long_only(argc, argv, "", btw_opts, NULL);
		if (rc == -1)
			break;
		switch (rc) {
			case 0: /* --sounddevice option */
				n = sscanf(optarg, "%d", &sound_device);
				if (n != 1) {
					fprintf(stderr, "bethewumpus: Bad sound device specified:"
						" '%s', using default.\n", optarg);
					sound_device = -1;
				}
				break;
			case 1:
				printf("Be The Wumpus, v. 0.04\n");
				printf("(c) Copyright Stephen M. Cameron, 2008\n");
				printf("See the file COPYING for terms of redistribution.\n");
				exit(0);
			case 2: /* rumbledevice */ 
				strcpy(rumbledevice, optarg);
				rumbledevicestring = rumbledevice;
				break;
			default:
				break;
		}
	}

	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	player.x = SCREEN_WIDTH / 2;
	player.y = SCREEN_WIDTH / 2;
	player.angle = 0.0;
	player.roar_sound_in_play = 0;

	joystick = open_joystick("/dev/input/js0");

	if (initialize_portaudio() != paNoError) {
		printf("Sound's not working... bye.\n");
		exit(1);
	}
 
	gtk_init (&argc, &argv);
   
	gdk_color_parse("white", &whitecolor);
	gdk_color_parse("black", &blackcolor);
 
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
	g_signal_connect (G_OBJECT (window), "delete_event",
		G_CALLBACK (delete_event), NULL);
    
	g_signal_connect (G_OBJECT (window), "destroy",
		G_CALLBACK (destroy), NULL);
    
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);
	vbox = gtk_vbox_new(FALSE, 0); 
	main_da = gtk_drawing_area_new();
	if (debugmode)
		gtk_widget_modify_bg(main_da, GTK_STATE_NORMAL, &whitecolor);
	else
		gtk_widget_modify_bg(main_da, GTK_STATE_NORMAL, &blackcolor);
        gtk_widget_set_size_request(main_da, SCREEN_WIDTH, SCREEN_HEIGHT);
	g_signal_connect(G_OBJECT (main_da), "expose_event", G_CALLBACK (main_da_expose), NULL);

	button = gtk_button_new_with_label ("Quit");
	g_signal_connect (G_OBJECT (button), "clicked",
		G_CALLBACK (Quit), NULL);
    
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
		G_CALLBACK (gtk_widget_destroy), G_OBJECT (window));
	g_signal_connect(G_OBJECT (window), "key_press_event",
		G_CALLBACK (key_press_cb), "window");
    
	gtk_container_add (GTK_CONTAINER (window), vbox);

	gtk_widget_set_size_request(main_da, SCREEN_WIDTH, SCREEN_HEIGHT);

	gtk_box_pack_start(GTK_BOX (vbox), main_da, TRUE /* expand */, FALSE /* fill */, 2);
	gtk_box_pack_start(GTK_BOX (vbox), button, FALSE /* expand */, FALSE /* fill */, 2);
    
	gtk_widget_show (vbox);
	gtk_widget_show (main_da);
	gtk_widget_show (button);

	/* Apparently (some versions of?) portaudio calls g_thread_init(). */
	/* It may only be called once, and subsequent calls abort, so */
	/* only call it if the thread system is not already initialized. */
	if (!g_thread_supported ())
		g_thread_init(NULL);
	gdk_threads_init();

	framerate_hz = 30; 
	if (joystick < 0)
		framerate_hz = 20; /* not sure this is necessary. */
	else {
		if (get_ready_to_rumble(rumbledevicestring) == 0) {
			/* assume xbox 360... remap axes */
			set_joystick_x_axis(0);
			set_joystick_y_axis(4);
		} else {
			/* assume logitech dual action rumble... remap axes */
			set_joystick_x_axis(0);
			set_joystick_y_axis(3);
		}
	}

	timer_tag = g_timeout_add(1000 / framerate_hz, advance_game, NULL);
    
	gtk_widget_show (window);
	gc = gdk_gc_new(GTK_WIDGET(main_da)->window);
	gdk_gc_set_foreground(gc, &blackcolor);


	meal.s.x = 1000;
	meal.s.y = 1000;
	meal.maxv = 1.0;
	meal.v = 0.5;
	meal.anxiety = 0;
	meal.n_arrows = INITIAL_N_ARROWS;

	arrow.s.x = meal.s.x;
	arrow.s.y = meal.s.y;
	arrow.vx = 0.0;
	arrow.vy = 0.0;
	arrow.state = ARROW_STATE_QUIVERED;
	arrow.sound_playing = 0;

	setup_volume_adjusting_profile(&meal.vap, &meal.s.x, &meal.s.y,
		&player.x, &player.y, &player.angle, SQRTFALLOFF, 
		normal_volume_adjust_function);
	setup_volume_adjusting_profile(&arrow.vap, &arrow.s.x, &arrow.s.y,
		&player.x, &player.y, &player.angle, SQRTFALLOFF, 
		normal_volume_adjust_function);
	setup_volume_adjusting_profile(&heartbeat_vap, &meal.s.x, &meal.s.y,
		&player.x, &player.y, &player.angle, LINEARFALLOFF, 
		normal_volume_adjust_function);
	setup_volume_adjusting_profile(&drip_vap, &drip.x, &drip.y,
		&player.x, &player.y, &player.angle, SQRTFALLOFF, 
		normal_volume_adjust_function);
	setup_volume_adjusting_profile(&water_vap, &water_x, &water_y,
		&player.x, &player.y, &player.angle, SQRTFALLOFF, 
		normal_volume_adjust_function);
		
	level = -1;
	advance_level();

	gtk_main ();
	stop_portaudio();
	stop_all_rumble_effects();
	close_joystick(joystick);
	exit(0);	
}

