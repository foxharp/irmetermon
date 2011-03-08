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
#include <errno.h>
#include <sys/time.h>


char *me;

void
usage(char *me)
{
    fprintf(stderr, "%s: tty-name\n", me);
    exit(1);
}

#define minute(s) ((s) / 60)
#define tenminute(s) ((s) / 600)

#define match_minute(a, b) (minute(a) == minute(b))
#define match_ten_minute(a, b) (tenminute(a) == tenminute(b))

void
log_minute(time_t now, int watt_hours)
{
}

void
log_tenminute(time_t now, int watt_hours)
{
}


#define NRECENT 5
static struct timeval recent[NRECENT];
static unsigned char r;

double
tvdiff(struct timeval *a, struct timeval *b)
{
    return a->tv_sec - b->tv_sec + (a->tv_usec - b->tv_usec) / 1.e6;
}

void
save_recent(struct timeval *tv)
{
    static int saved;
    if (!saved) {
	int i;
	for (i = 0; i < NRECENT; i++)
	    recent[i] = *tv;
	saved = 1;
	return;
    }
    recent[(++r) % NRECENT] = *tv;
}

double
get_recent_avg(void)
{
    return tvdiff(&recent[r % NRECENT],
		  &recent[(r + 1) % NRECENT]) / (NRECENT - 1);
}

void
loop(void)
{
    int i;
    long l, u;
    struct timeval tv[1];

    time_t cur_min = 0;
    time_t cur_tenmin = 0;
    int min_count = 0;
    int tenmin_count = 0;

    while (1) {
	i = scanf("s0x%lx u0x%lx ", &l, &u);
	if (i != 2) {
	    fprintf(stderr, "Short or failed read from pipe, quitting\n");
	    exit(1);
	}

	tv->tv_sec = (time_t) l;
	tv->tv_usec = (suseconds_t) u;

	if (cur_min == 0)
	    cur_min = minute(tv->tv_sec);
	if (cur_tenmin == 0)
	    cur_tenmin = tenminute(tv->tv_sec);

	if (cur_min == minute(tv->tv_sec)) {
	    min_count++;
	} else {
	    if (min_count)
		log_minute(cur_min, min_count);
	    min_count = 1;
	    cur_min = minute(tv->tv_sec);
	}

	if (cur_tenmin == tenminute(tv->tv_sec)) {
	    tenmin_count++;
	} else {
	    if (tenmin_count)
		log_tenminute(cur_tenmin, tenmin_count);
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

    loop();

    exit(0);
}
