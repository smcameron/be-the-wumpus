
/*
 *
 * This code is modified version of the fftest.c program
 * which Tests the force feedback driver by Johan Deneux.
 * Modifications to incorporate into Word War vi 
 * by Stephen M.Cameron
 * 
 * Copyright 2001-2002 Johann Deneux <deneux@ifrance.com>
 * Copyright 2008 Stephen M. Cameron <smcameron@yahoo.com> 
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * You can contact the author by email at this address:
 * Johann Deneux <deneux@ifrance.com>
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/input.h>

#define BITS_PER_LONG (sizeof(long) * 8)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)

#define N_EFFECTS 6

char* effect_names[] = {
	"Sine vibration",
	"Constant Force",
	"Spring Condition",
	"Damping Condition",
	"Strong Rumble",
	"Weak Rumble"
};

static int event_fd = -1;
static char *default_event_file = "/dev/input/event5";
static int n_effects;	/* Number of effects the device can play at the same time */
static unsigned long features[4];
static struct ff_effect effects[N_EFFECTS];

int stop_all_rumble_effects()
{
	int i;
	struct input_event stop;

	if (event_fd < 0)
		return;
	
	for (i=0; i<N_EFFECTS; ++i) {
		stop.type = EV_FF;
		stop.code =  effects[i].id;
		stop.value = 0;
        
		if (write(event_fd, (const void*) &stop, sizeof(stop)) == -1) {
			perror("Stop effect");
			exit(1);
		}
	}
}

int play_rumble_effect(int effect)
{
	if (effect < 0 || effect >= N_EFFECTS)
		return -1;

	if (event_fd < 0)
		return 0;

	struct input_event play;

	play.type = EV_FF;
	play.code = effects[effect].id;
	play.value = 1;

	if (write(event_fd, (const void*) &play, sizeof(play)) == -1)
		return -1;

	return 0;
}

void close_rumble_fd()
{
	close(event_fd);
	event_fd = -1;
}

int get_ready_to_rumble(char *filename)
{
	int i;

	if (filename == NULL)
		filename = default_event_file;

	event_fd = open(filename, O_RDWR);
	if (event_fd < 0) {
		fprintf(stderr, "Can't open %s: %s\n", 
			filename, strerror(errno));
		return -1;
	}

	printf("Device %s opened\n", filename);

	/* Query device */
	if (ioctl(event_fd, EVIOCGBIT(EV_FF, sizeof(unsigned long) * 4), features) == -1) {
		fprintf(stderr, "Query of rumble device failed: %s:%s\n", 
			filename, strerror(errno));
		return -1;	
	}

	printf("Axes query: ");

	if (test_bit(ABS_X, features)) printf("Axis X ");
	if (test_bit(ABS_Y, features)) printf("Axis Y ");
	if (test_bit(ABS_WHEEL, features)) printf("Wheel ");

	printf("\nEffects: ");

	if (test_bit(FF_CONSTANT, features)) printf("Constant ");
	if (test_bit(FF_PERIODIC, features)) printf("Periodic ");
	if (test_bit(FF_SPRING, features)) printf("Spring ");
	if (test_bit(FF_FRICTION, features)) printf("Friction ");
	if (test_bit(FF_RUMBLE, features)) printf("Rumble ");

	printf("\nNumber of simultaneous effects: ");

	if (ioctl(event_fd, EVIOCGEFFECTS, &n_effects) == -1) {
		fprintf(stderr, "Query of number of simultaneous "
			"effects failed, assuming 1. %s:%s\n", 
			filename, strerror(errno));
		n_effects = 1;	 /* assume 1. */
	}

	printf("%d\n", n_effects);

	/* download a periodic sinusoidal effect */
	effects[0].type = FF_PERIODIC;
	effects[0].id = -1;
	effects[0].u.periodic.waveform = FF_SINE;
	effects[0].u.periodic.period = 0.1*0x100;	/* 0.1 second */
	effects[0].u.periodic.magnitude = 0x1000;	/* 0.5 * Maximum magnitude */
	effects[0].u.periodic.offset = 0;
	effects[0].u.periodic.phase = 0;
	effects[0].direction = 0x4000;	/* Along X axis */
	effects[0].u.periodic.envelope.attack_length = 0x100;
	effects[0].u.periodic.envelope.attack_level = 0;
	effects[0].u.periodic.envelope.fade_length = 0x100;
	effects[0].u.periodic.envelope.fade_level = 0;
	effects[0].trigger.button = 0;
	effects[0].trigger.interval = 0;
	effects[0].replay.length = 250;  /* 0.25 seconds */
	effects[0].replay.delay = 0;

	memcpy(&effects[1], &effects[0], sizeof(effects[1]));
	effects[1].u.periodic.magnitude = 0x1500;
	memcpy(&effects[2], &effects[0], sizeof(effects[1]));
	effects[2].u.periodic.magnitude = 0x2000;
	memcpy(&effects[3], &effects[0], sizeof(effects[1]));
	effects[3].u.periodic.magnitude = 0x2500;
	memcpy(&effects[4], &effects[0], sizeof(effects[1]));
	effects[4].u.periodic.magnitude = 0x3000;
	memcpy(&effects[5], &effects[0], sizeof(effects[1]));
	effects[5].u.periodic.magnitude = 0x5000;

	for (i=0;i<6;i++) {
		if (ioctl(event_fd, EVIOCSFF, &effects[i]) == -1) {
			fprintf(stderr, "%s: failed to upload sine effect: %s\n", 
					filename, strerror(errno));
		}
	}
	return 0;
}

