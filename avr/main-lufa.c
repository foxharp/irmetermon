/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
 */
#include <setjmp.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

#include "luart.h"
#include "common.h"
#include "irmeter.h"
#include "timer.h"


void hardware_setup(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	luart_init();
	init_timer();
	irmeter_init();
}

int main(void)
{

	hardware_setup();

	sei();

	while (1) {

		luart_run();
		monitor();
		led_handle();
		tracker();
	}

}
