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
 * a "current power usage" file giving an "instant" usage -- actually
 * an average over the previous few (15?) seconds.
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
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>


// #define LOG_IMMED_DIR "/var/run/irmetermon/watts_now"
#define LOG_WATTS_NOW_FILE "/tmp/watts_now"
#define WATTS_NOW_AVG_PERIOD	15	// using this many seconds of data
#define USE_ALARM 0
#if USE_ALARM
#define WATTS_NOW_UPDATE_PERIOD 7	// update file this often (seconds)
#endif

#define USE_SIGUSR1 0

// #define LOG_KWH_MINUTE_DIR   "/var/local/irmetermon/minute/"
#define LOG_KWH_MINUTE_DIR	"/tmp/wH-by-minute/"
// #define LOG_KWH_TEN_MINUTE_DIR "/var/local/irmetermon/tenminute/"
#define LOG_KWH_TEN_MINUTE_DIR	"/tmp/wH-by-ten-minute/"

#define LOG_FILENAME_PATTERN "watt-hours.%y-%m-%d-%A.log"

void write_watts_now(void);

char *me;

void
usage(void)
{
    fprintf(stderr, "usage: %s (no arguments)\n", me);
    exit(1);
}

#if USE_ALARM || USE_SIGUSR1
void
signal_wrapper(signo, handler)
int signo;
void (*handler) ();
{
    struct sigaction act, oact;

    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;

    (void) sigaction(signo, &act, &oact);
}
#endif


/*
 * immediate consumption ("watts now") support
 */

#define NSAMP 128  // power of two, big enough to hold as many
		     // ticks as we'll ever get in one averaging period.
static struct timeval recent[NSAMP];
static unsigned char rcnt;

void
save_recent(struct timeval *tv)
{
    // leave rcnt pointing at most recent entry
    // printf("adding sample rcnt %d\n", rcnt);
    recent[rcnt++ & (NSAMP-1)] = *tv;
#if ! USE_ALARM
    write_watts_now();
#endif
}

double
timeval_diff(struct timeval *a, struct timeval *b)
{
    return a->tv_sec - b->tv_sec + (a->tv_usec - b->tv_usec) / 1.e6;
}

double
get_recent_avg_delta(void)
{
    int i;
    struct timeval tv;
    gettimeofday(&tv, 0);
    double interval = 0;

    /* count the ticks in our averaging period */
    for (i = 0; i < rcnt && i < NSAMP; i++) {
	// printf("interval %d of %d is %f\n", i, rcnt - 1, interval);
	interval = timeval_diff(&tv, &recent[(rcnt - 1 - i) & (NSAMP-1)]);
	if (interval > WATTS_NOW_AVG_PERIOD) {
	    i++;
	    break;
	}
    }

    if (i <= 1) return 0;

    // printf("dividing  %f  by (%d - 1)\n", interval, i);
    return interval / (i - 1);
}

double
get_recent_watts(void)
{
    double t;

    /* math is hard.  we get a tick once per watt-hour, and the ticks
     * are N seconds apart.  that's:
     *	(1/N) w-h per second
     * or:
     *  (3600/N) w-h per hour
     * or:
     *	(3600/N) watts.
     */
    t = get_recent_avg_delta();

    if (t == 0)
	return 0;

    return 3600.0 / t;
}

void
write_watts_now(void)
{
    FILE *f;

    f = fopen(LOG_WATTS_NOW_FILE ".tmp", "w");
    if (!f) {
	fprintf(stderr, "%s: opening %s: %m\n", me, LOG_WATTS_NOW_FILE ".tmp");
	return;
    }

    fprintf(f, "%.2f\n", get_recent_watts());
    fclose(f);
    if (rename(LOG_WATTS_NOW_FILE ".tmp", LOG_WATTS_NOW_FILE)) {
	fprintf(stderr, "%s: renaming to %s: %m\n", me, LOG_WATTS_NOW_FILE);
    }

#if USE_ALARM
    alarm(WATTS_NOW_UPDATE_PERIOD);
#endif
}

#if USE_SIGUSR1
void
sigusr1_handle(int n)
{
    write_watts_now();
    printf("avg delta: %f, watts %f\n",
	   get_recent_avg_delta(), get_recent_watts());
}
#endif

#if USE_ALARM
void
sigalrm_handle(int n)
{
    write_watts_now();
}
#endif


/*
 *  logging support
 */

struct tm *
localtime_wrapper(time_t t)
{
    static time_t last_t;
    static struct tm local_tm;

    if (t != last_t) {
	localtime_r(&t, &local_tm);
	last_t = t;
    }
    return &local_tm;

}

char *
log_string(time_t t)
{
    static char date[50];
    static time_t last_t;

    if (t != last_t) {
	strftime(date, sizeof(date), "%D %R", localtime_wrapper(t));
	last_t = t;
    }
    return date;
}

void
log_wH(char *file, time_t now, int watt_hours)
{
    FILE *f;

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
    time_t (*cur_interval_calc)(time_t *seconds);

    /* choose a logfile name, based on timeslot */
    char * (*log_name)(time_t seconds);

  /* runtime: */
    /* current accumulation interval */
    time_t cur_interval;

    /* what interval did we last log? */
    time_t last_logged_interval;

    /* accumlated ticks in current interval */
    int wH_count;

} log_info_t;

time_t
which_minute(time_t *t) {
    return ((*t) / 60) * 60;
}

time_t
which_ten_minutes(time_t *t) {
    return ((*t) / 600) * 600;
}

char *
minute_log_name(time_t t)
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

char *
ten_minute_log_name(time_t t)
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
    cur_interval_calc: which_minute,
    log_name: minute_log_name,
};

log_info_t ten_minute_log_info = {
    cur_interval_calc: which_ten_minutes,
    log_name: ten_minute_log_name,
};

void
interval_log(log_info_t *li, time_t wh_tick_time)
{
    if (li->cur_interval == 0)
	li->cur_interval = li->cur_interval_calc(&wh_tick_time);

    if (li->cur_interval_calc(&wh_tick_time) == li->cur_interval) {
	li->wH_count++;
    } else if (li->cur_interval != li->last_logged_interval) {
	if (li->wH_count) {
	    log_wH(li->log_name(li->cur_interval), li->cur_interval, li->wH_count);
	    li->last_logged_interval = li->cur_interval;
	}
	li->wH_count = 1;
	li->cur_interval = li->cur_interval_calc(&wh_tick_time);
    }
}

void
loop(void)
{
    int i;
    long l, u;
    struct timeval wh_tick[1];

    while (1) {
	i = scanf(" s0x%lx u0x%lx", &l, &u);
	if (i != 2) {
	    fprintf(stderr, "Short or failed read from pipe, quitting\n");
	    exit(1);
	}

	wh_tick->tv_sec = (time_t) l;
	wh_tick->tv_usec = (suseconds_t) u;

	/* one-minute logs */
	interval_log(&minute_log_info, wh_tick->tv_sec);

	/* ten-minute logs */
	interval_log(&ten_minute_log_info, wh_tick->tv_sec);

	/* save "instant consumption" data */
	save_recent(wh_tick);
    }
}

int
main(int argc, char *argv[])
{

    me = basename(argv[0]);

    if (argc > 1)
	usage();

#if USE_SIGUSR1
    signal_wrapper(SIGUSR1, sigusr1_handle);
#endif
#if USE_ALARM
    signal_wrapper(SIGALRM, sigalrm_handle);
    alarm(WATTS_NOW_UPDATE_PERIOD);
#endif

    mkdir(LOG_KWH_MINUTE_DIR, 0755);
    mkdir(LOG_KWH_TEN_MINUTE_DIR, 0755);

    loop();

    exit(0);
}
