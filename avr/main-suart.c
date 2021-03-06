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
#include <avr/power.h>

#include "suart.h"
#include "common.h"
#include "irmeter.h"
#include "timer.h"

void hardware_setup(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	suart_init();
	init_timer();
	irmeter_init();
}

int main()
{
	hardware_setup();

	sei();

	while (1) {

		monitor();
		led_handle();
		tracker();
	}

}
