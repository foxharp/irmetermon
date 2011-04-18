/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "common.h"
#include "irmeter.h"
#include "suart.h"


void kill_time(void)
{
#if LATER
	// try and get some sleep
	cli();
	if (!stx_active()) {
		// only sleep if there's no pulse data to emit
		// (see <sleep.h> for explanation of this snippet)
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
	}
	sei();
	// fixme emit_pulse_data();
#endif
}

int main()
{

	suart_init();

	irmeter_hwinit();

	sei();
#if LATER
	sputs_p(banner);
#endif

	for (;;) {
		wdt_reset();

		monitor();

		kill_time();
	}

}
