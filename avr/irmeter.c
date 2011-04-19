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
#include "timer.h"


#define MS			1000		// or whatever
#define STEP_UP		10
#define STEP_DOWN	-10
#define PULSE_LEN	10 * MS
#define AVG_DEPTH	4

void tracker(int new);

time_t led_time;

#ifdef IRMETER_ADAFRUITU4
# define PORTLED PORTE
# define BITLED PE6
# define DDRLED DDRE
#elif defined(IRMETER_ATTINY44)
# define PORTLED PORTB
# define BITLED PB2
# define DDRLED DDRB
#endif

void init_led(void)
{
	DDRLED |= bit(BITLED);
}

void led_handle(void)
{
	if ((PORTLED & bit(BITLED)) && check_timer(led_time, 100))
		PORTLED &= ~bit(BITLED);
}

void led_flash(void)
{
	PORTLED |= bit(BITLED);
	led_time = get_ms_timer();
}

void init_adc(void)
{
	// prescalar is 128.  that's okay for 16Mhz, but not for 1Mhz
#ifdef IRMETER_ADAFRUITU4
	ADCSRA |= bit(ADPS2) | bit(ADPS1) | bit(ADPS0); // 16Mhz/128 --> 125khz
#elif defined(IRMETER_ATTINY44)
     // ADCSRA |= bit(ADPS1);			// 1Mhz/4 --> 250khz
	ADCSRA |= bit(ADPS1) | bit(ADPS0);	// 1Mhz/8 --> 125khz
#endif

	ADMUX |= bit(REFS0) | bit(ADLAR);	// use ADC0, use avcc, and get 8-bit results

	DIDR0 |= bit(ADC0D);		// disable ADC0 (PF0) digital input

	ADCSRA |= bit(ADEN);		// enable
	ADCSRA |= bit(ADIE);		// enable Interrupt
	ADCSRA |= bit(ADSC);		// start first conversion 
}

unsigned int adc_counter;
int filtered;

ISR(ADC_vect)
{
	adc_counter++;
	tracker(ADCH);
	ADCSRA |= bit(ADSC);		// next conversion
}

void show_adc(void)
{
#if 0
	sputstring("adc_counter: ");
	puthex16(adc_counter);
	sputchar('\n');
#endif

	sputstring("filtered: ");
	puthex16(filtered);
	sputchar('\n');
}

void puthex(unsigned char i)
{
	unsigned char j;

	j = i >> 4;
	i &= 0xf;

	if (j >= 10)
		sputchar(j + 'a' - 10);
	else
		sputchar(j + '0');
	if (i >= 10)
		sputchar(i + 'a' - 10);
	else
		sputchar(i + '0');
}

void puthex16(unsigned int i)
{
	puthex((i >> 8) & 0xff);
	puthex((i >> 0) & 0xff);
}

void puthex32(long l)
{
	puthex((l >> 24) & 0xff);
	puthex((l >> 16) & 0xff);
	puthex((l >> 8) & 0xff);
	puthex((l >> 0) & 0xff);
}


void found_pulse(time_t now, int delta)
{
	static unsigned long index;
	led_flash();
	puthex32(index++);
	sputchar(':');
	puthex32(now);
	sputchar(':');
	puthex16(delta);
	sputchar('\n');
}

#define AVERAGING 1
#ifdef AVERAGING
int filter(int val)
{
	static int avg;

	avg = avg - (avg / AVG_DEPTH) + val;

	return avg / AVG_DEPTH;
}
#else							// median filter

// nifty value swapper
#define swap(a,b) {(a)=(b)^(a); (b)=(a)^(b); (a)=(b)^(a);}

static unsigned char o_vals[3];

int median(void)
{
	unsigned char a = o_vals[0];
	unsigned char b = o_vals[1];
	unsigned char c = o_vals[2];

	// sort the values
	if (a > b)
		swap(a, b);
	if (b > c)
		swap(b, c);
	if (a > b)
		swap(a, b);

	// return the middle value
	return b;
}

int filter(int val)
{
	static unsigned char i;

	o_vals[i % 3] = val;
	if (i++ < 3)
		return val;
	return median();
}
#endif

#define abs(a) (((a) >= 0) ? (a) : -(a))

int about_time(int went_up, int now)
{
	// within 1ms of expected pulse length?
	int diff = ((now - went_up) - PULSE_LEN);
	return abs(diff) < (1 * MS);
}


#define UP_ONLY 1

void tracker(int new)
{
	static int old;
#if ! UP_ONLY
	static time_t up;
#endif
	time_t now;
	int delta;

	now = get_ms_timer();

	filtered = filter(new);

	delta = filtered - old;

#if UP_ONLY
	if (delta > STEP_UP) {
		found_pulse(now, delta);
	}
#else
	if (!up && delta > STEP_UP) {
		up = now;
	} else if (up && about_time(up, now) && delta < STEP_DOWN) {
		up = 0;
		if (now > 100)			// don't report until adc and averages have settled.
			found_pulse(now, delta);
	} else if (up && now - up > 50) {
		up = 0;
	}
#endif
	old = filtered;
}

void irmeter_hwinit(void)
{
	init_adc();
	init_timer();
	init_led();
}
