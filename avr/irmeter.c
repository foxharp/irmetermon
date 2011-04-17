
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "common.h"
#include "timer.h"


#define MS 1000			// or whatever
#define STEP_UP	     10
#define STEP_DOWN   -10
#define PULSE_LEN	10 * MS
#define AVG_DEPTH 4

void tracker(int new);
time_t led_flash;

void
led_handle(void)
{
    if ((PORTE & bit(PE6)) && check_timer(led_flash, 100))
	PORTE &= ~bit(PE6);
}


void
init_adc(void)
{
    // prescalar is 128 (for now -- FIXME)
    ADCSRA |= bit(ADPS2) | bit(ADPS1) | bit(ADPS0);

    ADMUX |= bit(REFS0) | bit(ADLAR); // use avcc, and get 8-bit results

    ADCSRA |= bit(ADEN);  // enable
    ADCSRA |= bit(ADIE);  // enable Interrupt
    ADCSRA |= bit(ADSC);  // start first conversion 
}

unsigned int adc_counter;
int filtered;

ISR(ADC_vect)
{
    adc_counter++;
    tracker(ADCH);
    ADCSRA |= bit(ADSC);  // next conversion
}

void
show_adc(void)
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

void
puthex(unsigned char i)
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

void
puthex16(unsigned int i)
{
    puthex((i >> 8) & 0xff);
    puthex((i >> 0) & 0xff);
}

void
puthex32(long l)
{
    puthex((l >> 24) & 0xff);
    puthex((l >> 16) & 0xff);
    puthex((l >> 8) & 0xff);
    puthex((l >> 0) & 0xff);
}


void
found_pulse(time_t now)
{
    static unsigned long index;
    puthex32(index);
    sputchar(':');
    puthex32(now);
    sputchar('\n');
}

#define AVERAGING 1
#ifdef AVERAGING
int
filter(int val)
{
    static int avg;

    avg = avg - (avg / AVG_DEPTH) + val;

    return avg / AVG_DEPTH;
}
#else // median filter

// nifty value swapper
#define swap(a,b) {(a)=(b)^(a); (b)=(a)^(b); (a)=(b)^(a);}

static unsigned char o_vals[3];

int
median(void)
{
    unsigned char a = o_vals[0];
    unsigned char b = o_vals[1];
    unsigned char c = o_vals[2];

    // sort the values
    if (a > b) swap(a, b);
    if (b > c) swap(b, c);
    if (a > b) swap(a, b);

    // return the middle value
    return b;
}

int
filter(int val)
{
    static unsigned char i;

    o_vals[i % 3] = val;
    if (i++ < 3) return val;
    return median();
}
#endif

#define abs(a) (((a) >= 0) ? (a) : -(a))

int
about_time(int went_up, int now)
{
    // within 1ms of expected pulse length?
    int diff = ((now - went_up) - PULSE_LEN);
    return abs(diff) < (1 * MS);
}


void
tracker(int new)
{
    static int old;
    static time_t up;
    time_t now;

    now = ms_timer();

    filtered = filter(new);

    if (filtered - old > STEP_UP) {
	up = now;
    } else if (up && about_time(up, now) && (filtered - old) < STEP_DOWN) {
	up = 0;
	if (now > 100) // don't report until adc and averages have settled.
	    found_pulse(now);
    }
    old = filtered;
}

void
irmeter_hwinit(void)
{
    init_adc();
    init_timer();
    DDRE |= bit(PE6);
}

