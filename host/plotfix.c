/*
 *
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
 * ---------------------
 *
 * This program fixes up time-series logs that are missing some
 * data points -- i.e. logging is incomplete.  It adds lines for
 * the missing points with value '0'.  These can then either
 * simply be plotted, or (as we do), gnuplot can be told that
 * a zero watt-hours means "data missing", which in turn causes
 * a gap in the graph.
 */

#define _XOPEN_SOURCE			// for strptime(), per man page
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


char *me;

void usage(void)
{
	fprintf(stderr, "usage: %s [-i min/sample] \n", me);
	exit(1);
}

int main(int argc, char *argv[])
{
	struct tm tm[1];
	char line[80];
	char *wh_ptr;
	char newtime[80];
	int watthours;
	time_t t, last_t = 0;
	int interval;
	int opt;

	me = basename(argv[0]);

	interval = 1;

	while ((opt = getopt(argc, argv, "i:")) != -1) {
		switch (opt) {
		case 'i':
			interval = atoi(optarg);
			break;
		default:				/* '?' */
			usage();
		}
	}

	interval *= 60;				// seconds to minutes;

	// the logs have no timezone info, but if we don't do
	// this, DST screws up the strptime() conversion.
	putenv("TZ=UTC");

	while (1) {
		if (!fgets(line, sizeof(line), stdin))
			break;

		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = '\0';

		// pull out the logged time, and wH value.
		memset(tm, 0, sizeof(struct tm));
		wh_ptr = strptime(line, "%D %H:%M %t", tm);
		if (!wh_ptr) {
			fprintf(stderr, "%s: strptime failed on line '%s'\n", me,
					line);
			break;
		}
		if (sscanf(wh_ptr, "%d", &watthours) != 1) {
			fprintf(stderr,
					"%s: sscanf failed on line '%s', wh_ptr '%s'\n", me,
					line, wh_ptr);
			break;
		}
		// convert time to seconds since epoch
		t = mktime(tm);
		if (t == -1) {
			fprintf(stderr, "%s: mktime failed after line '%s'\n", me,
					line);
			break;
		}

		if (last_t && t - interval < last_t) {
			/* this will unfortunately happen every autumn
			 * at the end of daylight savings */
			fprintf(stderr, "%s: time goes backwards at line '%s'\n", me,
					line);
			fprintf(stderr, "t %ld, last_t %ld\n", (long) t,
					(long) last_t);
			continue;
		}
		// if we missed some log entries, fill them in.
		if (last_t && t - interval > last_t) {
			while (last_t < t - interval) {
				last_t += interval;
				gmtime_r(&last_t, tm);
				strftime(newtime, sizeof(newtime), "%D %R", tm);
				printf("%s 0\n", newtime);
			}
		}
		// in any case, write out the current log entry.
		printf("%s\n", line);
		last_t = t;
	}

	return 0;
}
