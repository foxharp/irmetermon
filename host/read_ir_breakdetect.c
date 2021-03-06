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
 * electric utility meters.  The detector circuit is wired such
 * that it zeros the rxdata line (pin 2) of the serial port. 
 * This program detects "break" (all-zero) conditions on the tty
 * line (which are reported as characters numerically equal to 0), and
 * reports their occurrence by writing a timestamp to
 * stdout.
 *
 * Can do testing interactively:
 *  read_ir /dev/tty
 * (try using ctrl-SPACE' to generate a null character), or with a script:
 *  while sleep 3; do echo -ne '\000'; done | read_ir -
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
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>


char *me;

void usage(char *me)
{
	fprintf(stderr,
			"usage: '%s tty-name', or\n"
			" '%s -' to get nulls from stdin, but use\n"
			" '%s /dev/tty' if testing interactively\n", me, me, me);
	exit(1);
}

struct termios oldterm[1], newterm[1];
int ir_fd;
int is_tty;

void restore_tty(void)
{
	if (is_tty)
		tcsetattr(ir_fd, TCSAFLUSH, oldterm);
}

void restore_tty_sighandler(int n)
{
	restore_tty();
	exit(1);
}

int main(int argc, char *argv[])
{
	unsigned char brk;
	int n, modemctl;

	me = basename(argv[0]);

	if (argc == 1 || (argv[1][0] == '-' && argv[1][1] != '\0'))
		usage(argv[0]);

	if (!strcmp(argv[1], "-")) {
		is_tty = 0;
		ir_fd = 0;
	} else {
		is_tty = 1;
		ir_fd = open(argv[1], O_RDONLY);
		if (ir_fd < 0) {
			fprintf(stderr, "%s: opening given tty: %m\n", me);
			exit(1);
		}
	}

	if (is_tty) {
		if (tcgetattr(ir_fd, oldterm)) {
			fprintf(stderr, "%s: getting attributes of tty: %m\n", me);
			exit(1);
		}

		*newterm = *oldterm;

		atexit(restore_tty);
		signal(SIGHUP, restore_tty_sighandler);
		signal(SIGINT, restore_tty_sighandler);
		signal(SIGQUIT, restore_tty_sighandler);
		signal(SIGTERM, restore_tty_sighandler);


		/* be sure IGNBRK/BRKING/IGNPAR/PAMRK are all the way we
		 * want them -- we just want a single '\0' character when
		 * an rs232 BREAK occurs. */
		newterm->c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK);
		newterm->c_lflag &= ~(ICANON);

		/*
		 * at 1200 baud, a BREAK is a line assertion that lasts longer
		 * than 4.17ms.  we expect the IR LED to assert for 10ms.
		 */
		if (cfsetspeed(newterm, B38400)
			|| tcsetattr(ir_fd, TCSANOW, newterm)) {
			fprintf(stderr, "%s: setting attributes for tty: %m\n", me);
			exit(1);
		}

		/* set up DTR and RTS with the right polarity:  disable RTS and
		 * enable DTR to leave RTS negative and DTR positive.
		 */
		if (ioctl(ir_fd, TIOCMGET, &modemctl) ||
			(modemctl &= ~TIOCM_RTS,
			 modemctl |= TIOCM_DTR, ioctl(ir_fd, TIOCMSET, &modemctl))) {
			fprintf(stderr, "%s: getting or setting RTS: %m\n", me);
			// exit(1);
		}
	}

	setlinebuf(stdout);



#warning buggy code -- see comments

	// there's something wrong the interaction of this loop, and
	// the pipe to ir_meterlog.  the timestamped events sent
	// to stdout seem to be delayed by a full pulse for some reason
	// that i don't understand right now.  -pgf

	while (1) {
		n = read(ir_fd, &brk, 1);
		if (n != 1) {
			fprintf(stderr,
					"Short or failed read from tty (%d), quitting\n", n);
			exit(1);
		}
		if (brk != 0) {
			fprintf(stderr, "%s: warning, non-zero read: 0x%x\n", me, brk);
		} else {
			struct timeval tv;
			gettimeofday(&tv, 0);
			printf("s0x%lx u0x%lx\n",
				   (unsigned long) tv.tv_sec, (unsigned long) tv.tv_usec);
		}
	}

}
