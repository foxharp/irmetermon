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
#include <string.h>

#include "common.h"
#include "timer.h"


#define STEP_SIZE	10			// adc steps
#define PULSE_LEN	10			// milliseconds

time_t led_time;
time_t now;

// these are all for debugging:
time_t fell;
unsigned char pre_rise, post_rise, pre_fall, post_fall;
unsigned char fellc;

#ifdef IRMETER_ADAFRUITU4
# define PORTLED PORTE
# define BITLED PE6
# define DDRLED DDRE
#elif defined(IRMETER_ATTINY44)
# define PORTLED PORTA
# define BITLED PA0
# define DDRLED DDRA
#else
# warning missing ADC configuration
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

#ifdef IRMETER_ADAFRUITU4
	// 16Mhz/128 --> 125khz, measured out at 6750 samples/sec
	ADCSRA = bit(ADPS2) | bit(ADPS1) | bit(ADPS0);
	// channel 11 (ADC11), 8-bit results, and use Avcc as the reference.
	ADMUX = bit(REFS0) | bit(ADLAR) | bit(MUX1) | bit(MUX0);
	ADCSRB = bit(MUX5);
	DIDR0 |= bit(ADC11D);		// disable channel 11 digital input
#elif defined(IRMETER_ATTINY44)
	// 1Mhz/8 --> 125khz
	ADCSRA = bit(ADPS1) | bit(ADPS0);
	ADMUX = bit(MUX1) | bit(MUX0);	// channel 3 (and Vcc as the reference)
	ADCSRB = bit(ADLAR);		//  8-bits (NB: ADLAR moved wrt ATMega32u4)
	DIDR0 |= bit(ADC3D);		// disable channel 3 digital input
#else
#warning missing ADC init code
#endif

	ADCSRA |= bit(ADEN);		// enable
	ADCSRA |= bit(ADIE);		// enable Interrupt
	ADCSRA |= bit(ADSC);		// start first conversion 
}

unsigned int adc_counter;
unsigned char filtered;

unsigned char new_adc, adc_avail;

ISR(ADC_vect)
{
	adc_counter++;
	new_adc = ADCH;
	adc_avail = 1;
	ADCSRA |= bit(ADSC);		// next conversion
}

void show_adc(void)
{
	sputstring("filt: ");
	puthex(filtered);
	sputstring(" raw: ");
	puthex(ADCH);
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

void show_pulse(char full)
{
	if (full) {
	    puthex32(fell);
	    sputchar(':');
	}
	puthex(pre_rise);
	sputchar('^');
	puthex(post_rise);
	sputchar(',');
	puthex(pre_fall);
	sputchar(fellc);
	puthex(post_fall);
}

void found_pulse(void)
{
	static unsigned long index;
	led_flash();
	puthex32(index++);
	sputchar(':');
	puthex32(now);
	sputchar(' ');
	show_pulse(0);
	sputchar('\n');
}

static unsigned char vals[8];
static unsigned char svals[8];

#if STREAMING_AVG
int avgfilter(int val)
{
	static int avg;

#define AVG_DEPTH	4

	avg = avg - (avg / AVG_DEPTH) + val;

	return avg / AVG_DEPTH;
}
#else
int avgfilter(int val)
{
	static unsigned char i;

#define NAVG	3

	vals[i % NAVG] = val;
//  if (i++ < NAVG)
//      return val;
	i++;
	return (vals[0] + vals[1] + vals[2]) / NAVG;

}
#endif

// nifty value swapper
#define swap(a,b) {(a)=(b)^(a); (b)=(a)^(b); (a)=(b)^(a);}


int median5(void)
{
	unsigned char i, j, t;
#define K 5
	memcpy(svals, vals, 5);
	for (i = 0; i < K - 1; i++) {
		for (j = i + 1; j < K; j++) {
			if (svals[i] > svals[j]) {
				t = svals[i];
				svals[i] = svals[j];
				svals[j] = t;
			}
		}
	}
	return svals[K / 2];
}

int medianfilter5(int val)
{
	static unsigned char i;

	vals[i] = val;
	if (++i == 5)
		i = 0;
	return median5();
}

int median3(void)
{
	unsigned char a = vals[0];
	unsigned char b = vals[1];
	unsigned char c = vals[2];

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

int medianfilter3(int val)
{
	static unsigned char i;

	vals[i] = val;
	if (++i == 3)
		i = 0;
	return median3();
}

#define abs(a) (((a) >= 0) ? (a) : -(a))

int about_time(int went_up)
{
	// within 1ms of expected pulse length?
	int diff = ((now - went_up) - PULSE_LEN);
	return abs(diff) <= 1;
}



unsigned int adc_fastdump;
char use_median;
char step_size;

void tracker(void)
{
	static unsigned char old;
	static time_t up;
	int delta;
	int new;

	if (!adc_avail)
		return;

	if (adc_fastdump)
		adc_fastdump--;

	new = new_adc;
	adc_avail = 0;

	now = get_ms_timer();

	if (adc_fastdump) {
		puthex(new);
		sputchar(' ');
		sputchar("0123456789abcdef"[now & 0xf]);	/* last digit of time */
	}

	if (use_median == 5)
		filtered = medianfilter5(new);
	else if (use_median == 3)
		filtered = medianfilter3(new);
	else
		filtered = avgfilter(new);

	if (adc_fastdump) {
		sputchar('(');
		puthex(filtered);
		sputchar(')');
	}
	// don't report until adc and averages have settled.
	if (now < 100) {
		old = filtered;
		return;
	}

	delta = filtered - old;

	if (!up && delta > step_size) {
		pre_rise = old;
		post_rise = filtered;
		up = now;
	} else if (up && about_time(up) && delta < -(step_size / 2)) {
		up = 0;
		pre_fall = old;
		post_fall = filtered;
		fell = now;
		fellc = 'v';
		found_pulse();
	} else if (up && now - up > 50) {
		pre_fall = old;
		post_fall = filtered;
		fell = now;
		fellc = 'X';
		found_pulse();
		up = 0;
	}
	old = filtered;
}

void irmeter_init(void)
{
	use_median = 5;
	step_size = STEP_SIZE;
	fellc = '?';

	init_adc();
	init_led();

}
