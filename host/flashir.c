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
#include <time.h>


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

void log_minute(time_t now, int watt_hours)
{
}

void log_tenminute(time_t now, int watt_hours)
{
}


#define NRECENT 8
static time_t recentavg;

void 
save_recent(time_t t)
{

    recentavg -= recentavg / NRECENT;
    recentavg += t;
}

time_t get_recent_avg(void)
{
    return recentavg / NRECENT;
}

void
loop(void)
{
    int i;
    long l;
    time_t t;

    time_t cur_min = 0;
    time_t cur_tenmin = 0;
    int min_count = 0;
    int tenmin_count = 0;

    while (1) {
	i = scanf("%ld", &l);
	if (i != 1) {
	    fprintf(stderr, "Short or failed read from pipe, quitting\n");
	    exit(1);
	}

	t = (time_t)l;

	if (cur_min == 0)
	    cur_min = minute(t);
	if (cur_tenmin == 0)
	    cur_tenmin = tenminute(t);

	if (cur_min == minute(t)) {
	    min_count++;
	} else {
	    if (min_count)
		log_minute(cur_min, min_count);
	    min_count = 1;
	    cur_min = minute(t);
	}

	if (cur_tenmin == tenminute(t)) {
	    tenmin_count++;
	} else {
	    if (tenmin_count)
		log_tenminute(cur_tenmin, tenmin_count);
	    tenmin_count = 1;
	    cur_tenmin = tenminute(t);
	}

	save_recent(t);
    }
}

int
main(int argc, char *argv[])
{

    me = basename(argv[0]);

    loop();

    exit(0);
}
