/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file for details.
 * for details.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
// #include <avr/sleep.h>

#include "timer.h"
#include "irmeter.h"
#include "common.h"

time_t milliseconds;

void init_timer(void)
{
	// set up for simple overflow operation
	TCCR0A = bit(WGM01);		// CTC
#ifdef IRMETER_ADAFRUITU4
	TCCR0B = bit(CS01) | bit(CS00);	// prescaler is 64 
#elif defined(IRMETER_ATTINY44)
	TCCR0B = bit(CS01);			// prescaler is 8
#endif
	OCR0A = 125;				// see below
	TIMSK0 = bit(OCIE0A);

}

#ifdef TIMER0_COMPA_vect
ISR(TIMER0_COMPA_vect)
#else
ISR(TIM0_COMPA_vect)
#endif
{
	// xtal == 1024    *   244   *   N
	//       (prescale)  (overflow)
	// "244" is chosen so N is close to integral for 1Mhz, 8Mhz, 12Mhz, etc.
#ifdef IRMETER_ADAFRUITU4
	static unsigned char prescale;
	if ((prescale++ & 1) == 0)
		milliseconds++;
#elif defined(IRMETER_ATTINY44)
	milliseconds++;
#endif

#if 0
	if (milliseconds % 1000 == 0) {
		led_flash();
#if 0
		extern unsigned int adc_counter;
		sputstring("adccnt: ");
		puthex16(adc_counter);
		sputchar('\n');
#endif
	}
#endif
}

time_t get_ms_timer(void)
{
	time_t ms;
	char sreg;

	sreg = SREG;
	cli();

	ms = milliseconds;

	SREG = sreg;

	return ms;
}

unsigned char check_timer(time_t base, int duration)
{
	return get_ms_timer() > (base + duration);
}
