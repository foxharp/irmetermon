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
 * This program interfaces to a simple detector circuit in order
 * to report infrared pulses emitted by many modern household
 * electric utility meters.  It gets short timestamped and indexed
 * messages from a microcontroller, one per pulse of infrared at
 * the meter.  Each pulse represents one kwH.
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
#include <sched.h>
#include <sys/mman.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>


char *me;

void usage(char *me)
{
	fprintf(stderr,
			"usage: '%s tty-name'\n"
			" use -v for verbose.\n"
			" use -r for real-time scheduling.\n"
			" for tty-name, use '-' to read from stdin.\n"
			" (but use '/dev/tty' to test interactively)\n", me);
	exit(1);
}

int ir_fd;
FILE *ir_fp;
int is_tty;
int verbose;

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


void flushline(FILE *fp)
{
	int c;
	while ((c = fgetc(fp)) != EOF && c != '\n')
		;
}

void loop(void)
{
	int n;
	struct timeval tv;
	unsigned int index, tstamp;
	unsigned int last_index = 0;
	unsigned int pre_rise, post_rise, pre_fall, post_fall;
	unsigned char fellc;
	int retries;


	while (1) {
		retries = 15;

		/* AVR firmware sends:
		 *  - incrementing index
		 *  - timestamp
		 *  - adc before and after pulse rise
		 *  - adc before and after pulse fall -or- adc at pulse detect timeout
		 * 00000004:000066ab 46^51,53X63
		 * 00000005:000066b5 68^76,f7vec
		 */
		while (retries--) {
			n = fscanf(ir_fp, " %x:%8x %2x^%2x,%2x%c%2x", &index, &tstamp,
					   &pre_rise, &post_rise,
					   &pre_fall, &fellc, &post_fall);
			if (n == EOF) {
				fprintf(stderr, "fscanf returns EOF, quitting.\n");
				exit(1);
			}
			if (n == 2 || n == 7)
				break;
			flushline(ir_fp);
		}

		if (retries <= 0) {
			fprintf(stderr, "Bad scanf from tty (%d), quitting\n", n);
			exit(1);
		}
		// we don't currently use the reported timestamp
		gettimeofday(&tv, 0);

		if (n == 7 && verbose) {	// have rise/fall info
			fprintf(stderr, "%s: 0x%02x ^ 0x%02x,   0x%02x %c 0x%02x\n",
					log_string(tv.tv_sec),
					pre_rise, post_rise, pre_fall, fellc, post_fall);
		}

		printf("s0x%lx u0x%lx i%d l%d\n",
			   (unsigned long) tv.tv_sec, (unsigned long) tv.tv_usec,
			   index, last_index);

		last_index = index;
	}
}


int main(int argc, char *argv[])
{
	int opt;
	int realtime = 0;

	me = basename(argv[0]);

	while ((opt = getopt(argc, argv, "rv")) != -1) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;
		case 'r':
			realtime = 1;
			break;
		default:
			fprintf(stderr, "bad opt is 0x%02x\n", opt);
			usage(argv[0]);
		}
	}

	if (optind >= argc) {
		usage(argv[0]);
	} else if (!strcmp(argv[optind], "-")) {
		is_tty = 0;
		ir_fd = 0;
	} else {
		is_tty = 1;
		ir_fd = open(argv[optind], O_RDONLY);
		if (ir_fd < 0) {
			fprintf(stderr, "%s: opening given tty: %m\n", me);
			exit(1);
		}
	}

	if (is_tty) {
		struct termios term[1];

		if (tcgetattr(ir_fd, term)) {
			fprintf(stderr, "%s: getting attributes of tty: %m\n", me);
			exit(1);
		}

		cfmakeraw(term);

		if (cfsetspeed(term, B4800)
			|| tcsetattr(ir_fd, TCSAFLUSH, term)) {
			fprintf(stderr, "%s: setting attributes for tty: %m\n", me);
			exit(1);
		}

	}

	setlinebuf(stdout);

	if (realtime) {
		struct sched_param sparam;
		int min, max;

		/* first, lock down all our memory */
		long takespace[1024];
		memset(takespace, 0, sizeof(takespace)); /* force paging */
		if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0) {
			fprintf(stderr, "%s: unable to mlockall: %m\n", me);
			exit(1);
		}

		/* then, raise our scheduling priority */
		min = sched_get_priority_min(SCHED_FIFO);
		max = sched_get_priority_max(SCHED_FIFO);

		sparam.sched_priority = (min + max)/2; /* probably always 50 */
		if (sched_setscheduler(0, SCHED_FIFO, &sparam)) {
			fprintf(stderr, "%s: unable to set SCHED_FIFO: %m\n", me);
			exit(1);
		}

		if (verbose)
			fprintf(stderr, "memory locked, scheduler priority set\n");

	}

	ir_fp = fdopen(ir_fd, "r");
	if (!ir_fp) {
		fprintf(stderr, "%s: can't fdopen the ir file descriptor\n", me);
		exit(1);
	}
	setlinebuf(ir_fp);
	flushline(ir_fp);

	loop();

	return 0;

}
