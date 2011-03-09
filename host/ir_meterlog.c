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


// #define LOG_IMMED_DIR "/var/run/ir_meterlog/watts_now"
#define LOG_WATTS_NOW_FILE "/tmp/watts_now"

// #define LOG_KWH_MINUTE_DIR	"/var/local/irmetermon/"
#define LOG_KWH_MINUTE_DIR	"/tmp/"
// #define LOG_KWH_TEN_MINUTE_DIR	"/var/local/irmetermon/"
#define LOG_KWH_TEN_MINUTE_DIR	"/tmp/"


char *me;

void
usage(char *me)
{
    fprintf(stderr, "%s: tty-name\n", me);
    exit(1);
}

#define minute(s) (((s) / 60) * 60)
#define tenminute(s) (((s) / 600) * 600)


char *
date_string(time_t *t)
{
    static char date[50];
    static time_t last_t;
    struct tm local_tm;

    if (*t != last_t) {
	strftime(date, sizeof(date), "%D %R", localtime_r(t, &local_tm));
	last_t = *t;
    }
    return date;
}

void
log_minute(time_t now, int watt_hours)
{
    fprintf(stdout, "mins: %lu %s %d\n", now, date_string(&now), watt_hours);
}

void
log_tenminute(time_t now, int watt_hours)
{
    fprintf(stdout, "tens: %lu %s %d\n", now, date_string(&now), watt_hours);
}


#define NRECENT 5
static struct timeval recent[NRECENT];
static unsigned char r;
static int saved_recent;

double
tvdiff(struct timeval *a, struct timeval *b)
{
    return a->tv_sec - b->tv_sec + (a->tv_usec - b->tv_usec) / 1.e6;
}

void
save_recent(struct timeval *tv)
{
    if (!saved_recent) {
	int i;
	for (i = 0; i < NRECENT; i++)
	    recent[i] = *tv;
	saved_recent = 1;
	return;
    }
    recent[r++ % NRECENT] = *tv;
}

double
get_recent_avg_delta(void)
{
    struct timeval tv;
    gettimeofday(&tv, 0);

    if (!saved_recent)
	return 0;

    return tvdiff(&tv, &recent[r % NRECENT]) / NRECENT;
}

double
get_recent_watts(void)
{
    if (!saved_recent)
	return 0;

    return 3600 / get_recent_avg_delta();
}

void
sigusr1_handle(int n)
{
    printf("avg delta: %f, watts %f\n",
	   get_recent_avg_delta(), get_recent_watts());
}


void
loop(void)
{
    int i;
    long l, u;
    struct timeval tv[1];

    time_t cur_min = 0,
	    cur_tenmin = 0,
	    min_lastlogged = 0,
	    tenmin_lastlogged = 0;
    int min_count = 0;
    int tenmin_count = 0;

    while (1) {
	i = scanf("s0x%lx u0x%lx ", &l, &u);
	if (i != 2) {
	    fprintf(stderr, "Short or failed read from pipe, quitting\n");
	    exit(1);
	}

	fprintf(stderr, "got %lx %lx\n", l, u);

	tv->tv_sec = (time_t) l;
	tv->tv_usec = (suseconds_t) u;

	if (cur_min == 0)
	    cur_min = minute(tv->tv_sec);
	if (cur_tenmin == 0)
	    cur_tenmin = tenminute(tv->tv_sec);

	if (minute(tv->tv_sec) == cur_min) {
	    min_count++;
	} else if (minute(tv->tv_sec) != min_lastlogged) {
	    if (min_count) {
		log_minute(cur_min, min_count);
		min_lastlogged = cur_min;
	    }
	    min_count = 1;
	    cur_min = minute(tv->tv_sec);
	}

	if (tenminute(tv->tv_sec) == cur_tenmin) {
	    tenmin_count++;
	} else if (tenminute(tv->tv_sec) != tenmin_lastlogged) {
	    if (tenmin_count) {
		log_tenminute(cur_tenmin, tenmin_count);
		tenmin_lastlogged = cur_min;
	    }
	    tenmin_count = 1;
	    cur_tenmin = tenminute(tv->tv_sec);
	}

	save_recent(tv);
    }
}

int
main(int argc, char *argv[])
{

    me = basename(argv[0]);

    signal(SIGUSR1, sigusr1_handle);

    loop();

    exit(0);
}
