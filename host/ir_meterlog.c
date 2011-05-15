/*
 * Copyright 2011 Paul Fox (pgf@foxharp.boston.ma.us)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * -----------------------
 *
 * Monitors stdin for timestamp values which indicate a watt-hour
 * has been used.  Aggregates these "wH ticks" into two logs:  one
 * per-minute, and one per-ten-minutes -- the former logs are expected
 * to be cleaned up more quickly than the latter.  Also maintains
 * a "current power usage" file giving an "instant" usage, based on
 * the interval of the most recent two pulses.
 *
 */

#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>


#define LOG_WATTS_NOW_FILE "watts_now"
#define WATTS_MIN_SAMPLES 3		// want at least this many samples to average...
#define WATTS_MIN_PERIOD 15		// ...over at least this many seconds of data

#define LOG_KWH_MINUTE_DIR	"wH-by-minute/"
#define LOG_KWH_TEN_MINUTE_DIR	"wH-by-ten-minute/"

#define LOG_FILENAME_PATTERN "watt-hours.%y-%m-%d-%A.log"

char *me;
int verbose, testing;
char *logdir = "/tmp";

void usage(void)
{
	fprintf(stderr, "usage: %s [-v] [-t] \n", me);
	fprintf(stderr, "  -v for verbose, -t for test (no logs)\n");
	exit(1);
}

/*
 * immediate consumption ("watts now") support
 */

double timeval_diff(struct timeval *a, struct timeval *b)
{
	return a->tv_sec - b->tv_sec + (a->tv_usec - b->tv_usec) / 1.e6;
}

void write_watts_now(struct timeval *this_tick)
{
	double delta_t;
	int watts;
	FILE *f;
	static struct timeval last_tick;

	if (!last_tick.tv_sec) {
	    last_tick = *this_tick;
	    return;
	}

	delta_t = timeval_diff(this_tick, &last_tick);
	last_tick = *this_tick;
	watts = (int) ((3600.0 / delta_t) + 0.5);
	if (verbose) {
		fprintf(stderr, "tick is %ld, delta_t %f\n", this_tick->tv_sec, delta_t);
		fprintf(stderr, "watts: %f / %f = %d watts\n", 3600.0, delta_t, watts);
	}

	if (testing)
		return;

	f = fopen(LOG_WATTS_NOW_FILE ".tmp", "w");
	if (!f) {
		fprintf(stderr, "%s: opening %s: %m\n", me,
				LOG_WATTS_NOW_FILE ".tmp");
		return;
	}

	fprintf(f, "%d\n", watts);

	fclose(f);
	if (rename(LOG_WATTS_NOW_FILE ".tmp", LOG_WATTS_NOW_FILE)) {
		fprintf(stderr, "%s: renaming to %s: %m\n", me,
				LOG_WATTS_NOW_FILE);
	}
}

/*
 *  logging support
 */

struct tm *localtime_wrapper(time_t t)
{
	static time_t last_t;
	static struct tm local_tm;

	if (t != last_t) {
		localtime_r(&t, &local_tm);
		last_t = t;
	}
	return &local_tm;

}

char *log_string(time_t t)
{
	static char date[50];
	static time_t last_t;

	if (t != last_t) {
		strftime(date, sizeof(date), "%D %R", localtime_wrapper(t));
		last_t = t;
	}
	return date;
}

void log_wH(char *file, time_t now, int watt_hours)
{
	FILE *f;

	if (verbose) {
		fprintf(stderr, "%slog to %s: ", testing ? "would " : "", file);
		fprintf(stderr, "%s %d\n", log_string(now), watt_hours);
	}

	if (testing)
		return;

	f = fopen(file, "a");
	if (!f) {
		fprintf(stderr, "%s: opening %s: %m\n", me, file);
		return;
	}

	fprintf(f, "%s %d\n", log_string(now), watt_hours);
	fclose(f);
}

typedef struct log_info {

	/* initialized: */
	/* determines what timeslot a given tick is allotted to */
	time_t(*cur_interval_calc) (time_t * seconds);

	/* choose a logfile name, based on timeslot */
	char *(*log_name) (time_t seconds);

	/* length of interval */
	int interval;

	/* runtime: */
	/* current accumulation interval */
	time_t cur_interval;

	/* what interval did we last log? */
	time_t last_logged_interval;

	/* accumlated ticks in current interval */
	int wH_count;

} log_info_t;

time_t which_minute(time_t * t)
{
	return ((*t) / 60) * 60;
}

time_t which_ten_minutes(time_t * t)
{
	return ((*t) / 600) * 600;
}

char *minute_log_name(time_t t)
{
	static char name[256];
	static time_t last_t;

	if (t != last_t) {
		strftime(name, sizeof(name),
				 LOG_KWH_MINUTE_DIR LOG_FILENAME_PATTERN,
				 localtime_wrapper(t));
		last_t = t;
	}
	return name;
}

char *ten_minute_log_name(time_t t)
{
	static char tenname[256];
	static time_t tenlast_t;

	if (t != tenlast_t) {
		strftime(tenname, sizeof(tenname),
				 LOG_KWH_TEN_MINUTE_DIR LOG_FILENAME_PATTERN,
				 localtime_wrapper(t));
		tenlast_t = t;
	}
	return tenname;
}

log_info_t minute_log_info = {
  cur_interval_calc:which_minute,
  log_name:minute_log_name,
  interval:60,
};

log_info_t ten_minute_log_info = {
  cur_interval_calc:which_ten_minutes,
  log_name:ten_minute_log_name,
  interval:600,
};

void interval_log(log_info_t * li, time_t wh_tick_time)
{
	time_t intvl, newintvl;
	if (li->cur_interval == 0)
		li->cur_interval = li->cur_interval_calc(&wh_tick_time);

	if (li->cur_interval_calc(&wh_tick_time) == li->cur_interval) {
		li->wH_count++;
	} else if (li->cur_interval != li->last_logged_interval) {
		if (li->wH_count) {
			log_wH(li->log_name(li->cur_interval),
				   li->cur_interval, li->wH_count);
			li->last_logged_interval = li->cur_interval;
		}
		newintvl = li->cur_interval_calc(&wh_tick_time);
		for (intvl = li->cur_interval + li->interval;
			 intvl < newintvl; intvl += li->interval) {
			log_wH(li->log_name(intvl), intvl, 0);
		}
		li->wH_count = 1;
		li->cur_interval = newintvl;
	}
}

void loop(void)
{
	int r;
	long s, u;
	int i, l;
	struct timeval wh_tick[1];
	int badindex = 0;

	while (1) {
		r = scanf(" s0x%lx u0x%lx i%d l%d", &s, &u, &i, &l);
		if (r == EOF) {
			fprintf(stderr, "scanf returns EOF, quitting.\n");
			exit(1);
		}
		if (r != 4 && r != 2) {
			fprintf(stderr, "Short or failed read from pipe, quitting.\n");
			exit(1);
		}

		if (r == 4) {
			if (i != l + 1) {
				fprintf(stderr, "Reported pulse index "
								"mismatch %d and %d\n", i, l);
				if (badindex++ > 5) {
					fprintf(stderr, "Too many index errors, quitting.\n");
					exit(1);
				}
				continue;
			} else {
				badindex = 0;
			}
		}

		wh_tick->tv_sec = (time_t) s;
		wh_tick->tv_usec = (suseconds_t) u;

		/* one-minute logs */
		interval_log(&minute_log_info, wh_tick->tv_sec);

		/* ten-minute logs */
		interval_log(&ten_minute_log_info, wh_tick->tv_sec);

		/* "instant" consumption */
		write_watts_now(wh_tick);

	}
}

int main(int argc, char *argv[])
{
	int opt;

	me = basename(argv[0]);

	while ((opt = getopt(argc, argv, "vtd:")) != -1) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		case 't':
			testing = 1;
			break;
		case 'd':
			logdir = optarg;
			break;
		default:				/* '?' */
			usage();
		}
	}

	if (chdir(logdir) < 0) {
		fprintf(stderr, "%s: can't chdir to %s (%m)\n", me, logdir);
		exit(1);
	}

	mkdir(LOG_KWH_MINUTE_DIR, 0755);
	mkdir(LOG_KWH_TEN_MINUTE_DIR, 0755);

	loop();

	exit(0);
}
