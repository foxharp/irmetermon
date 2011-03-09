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
#define WATTS_NOW_UPDATE_PERIOD 7  // update file this often (seconds)
#define WATTS_NOW_AVG_PERIOD	15 // using this many seconds of data

// #define LOG_KWH_MINUTE_DIR   "/var/local/irmetermon/"
#define LOG_KWH_MINUTE_DIR	"/tmp/"
// #define LOG_KWH_TEN_MINUTE_DIR       "/var/local/irmetermon/"
#define LOG_KWH_TEN_MINUTE_DIR	"/tmp/"

#define minute(s) (((s) / 60) * 60)
#define tenminute(s) (((s) / 600) * 600)


char *me;

void
usage(char *me)
{
    fprintf(stderr, "%s: tty-name\n", me);
    exit(1);
}

void
signal_wrap(signo, handler)
int signo;
void (*handler)();
{
    struct sigaction act, oact;

    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;

    (void)sigaction(signo, &act, &oact);
}

struct tm *
localtime_wrap(time_t * t)
{
    static time_t last_t;
    static struct tm local_tm;

    if (*t != last_t) {
	localtime_r(t, &local_tm);
	last_t = *t;
    }
    return &local_tm;

}

char *
log_string(time_t * t)
{
    static char date[50];
    static time_t last_t;

    if (*t != last_t) {
	strftime(date, sizeof(date), "%D %R", localtime_wrap(t));
	last_t = *t;
    }
    return date;
}

char *
minute_log_name(time_t * t)
{
    static char name[256];
    static time_t last_t;

    if (*t != last_t) {
	strftime(name, sizeof(name),
		 LOG_KWH_MINUTE_DIR "wH-by-minute.%y-%m-%d.log",
		 localtime_wrap(t));
	last_t = *t;
    }
    return name;
}

char *
tenminute_log_name(time_t * t)
{
    static char tenname[256];
    static time_t tenlast_t;

    if (*t != tenlast_t) {
	strftime(tenname, sizeof(tenname),
		 LOG_KWH_TEN_MINUTE_DIR "wH-by-tenminute.%y-%m-%d.log",
		 localtime_wrap(t));
	tenlast_t = *t;
    }
    return tenname;
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

    fprintf(f, "%s %d\n", log_string(&now), watt_hours);
    fclose(f);
}

#define NRECENT 64  // power of two
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
    if (0 && !saved_recent) {
	int i;
	for (i = 0; i < NRECENT; i++)
	    recent[i] = *tv;
	saved_recent = 1;
	return;
    }
    recent[++r & (NRECENT-1)] = *tv;  // leave r pointing at most recent entry
    fprintf(stderr, "saved %d %ld\n", r, recent[r & (NRECENT-1)].tv_sec);
}

double
get_recent_avg_delta(void)
{
    int i;
    struct timeval tv;
#if BEFORE
    double avg_recorded;

    if (0 && !saved_recent)
	return 0;

    gettimeofday(&tv, 0);

    for (i = 0; i < NRECENT; i++) {
	if (recent[(r-i) & (NRECENT-1)].tv_sec < tv.tv_sec - 19)
	    break;
	fprintf(stderr, "found %d %ld\n", r-i, recent[(r-i) & (NRECENT-1)].tv_sec);
    }
    fprintf(stderr, "loop returns %d\n", i);

    if (i <= 1)
	return 0.0;

    avg_recorded = tvdiff(&recent[r & (NRECENT-1)],
    			&recent[(r-(i-1)) & (NRECENT-1)]) / (double)i;

    if (avg_recorded > tvdiff(&tv, &recent[r & (NRECENT-1)]))
	return avg_recorded;

    return tvdiff(&tv, &recent[(r-(i-1)) & (NRECENT-1)]) / (double)(i+1);
#else
    gettimeofday(&tv, 0);

    for (i = 0; i < NRECENT; i++) {
	if (recent[(r-i) & (NRECENT-1)].tv_sec < tv.tv_sec - WATTS_NOW_AVG_PERIOD)
	    break;
	fprintf(stderr, "found %d %ld\n", r-i, recent[(r-i) & (NRECENT-1)].tv_sec);
    }
    fprintf(stderr, "loop returns %d\n", i);

    return (double)WATTS_NOW_AVG_PERIOD/i;

#endif
}

double
get_recent_watts(void)
{
    double t;

    if (0 && !saved_recent)
	return 0;

    /* math is hard.  we get a tick once per watt-hour, and the ticks
     * are N seconds apart.  that's (1/N) w-h/second, or (3600/N)
     * w-h/hour, or 3600/N watts.
     */
    t = get_recent_avg_delta();

    if (t == 0)
	return 0;

    return 3600.0 / t;
}

void
sigusr1_handle(int n)
{
    printf("avg delta: %f, watts %f\n",
	   get_recent_avg_delta(), get_recent_watts());
}

void
sigalrm_handle(int n)
{
    FILE *f;

    // printf("avg delta: %f, watts %f\n",
    //	   get_recent_avg_delta(), get_recent_watts());

    f = fopen(LOG_WATTS_NOW_FILE ".tmp" , "w");
    if (!f) {
	fprintf(stderr, "%s: opening %s: %m\n", me, LOG_WATTS_NOW_FILE ".tmp");
	return;
    }

    fprintf(f, "%.2f\n", get_recent_watts());
    fclose(f);
    if (rename(LOG_WATTS_NOW_FILE ".tmp", LOG_WATTS_NOW_FILE))
	fprintf(stderr, "%s: renaming to %s: %m\n", me, LOG_WATTS_NOW_FILE);

    alarm(WATTS_NOW_UPDATE_PERIOD);
}


void
loop(void)
{
    int i;
    long l, u;
    struct timeval tv[1];

    time_t cur_min = 0,
	cur_tenmin = 0, min_lastlogged = 0, tenmin_lastlogged = 0;
    int min_wH = 0;
    int tenmin_wH = 0;

    while (1) {
	i = scanf(" s0x%lx u0x%lx", &l, &u);
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
	    min_wH++;
	} else if (cur_min != min_lastlogged) {
	    if (min_wH) {
		log_wH(minute_log_name(&cur_min), cur_min, min_wH);
		min_lastlogged = cur_min;
	    }
	    min_wH = 1;
	    cur_min = minute(tv->tv_sec);
	}

	if (tenminute(tv->tv_sec) == cur_tenmin) {
	    tenmin_wH++;
	} else if (cur_tenmin != tenmin_lastlogged) {
	    if (tenmin_wH) {
		log_wH(tenminute_log_name(&cur_tenmin), cur_tenmin, tenmin_wH);
		tenmin_lastlogged = cur_tenmin;
	    }
	    tenmin_wH = 1;
	    cur_tenmin = tenminute(tv->tv_sec);
	}

	save_recent(tv);
    }
}

int
main(int argc, char *argv[])
{

    me = basename(argv[0]);

    signal_wrap(SIGUSR1, sigusr1_handle);
    signal_wrap(SIGALRM, sigalrm_handle);

    alarm(WATTS_NOW_UPDATE_PERIOD);

    loop();

    exit(0);
}
