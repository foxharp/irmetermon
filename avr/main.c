
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "common.h"
#include "suart.h"

#define bit(x) _BV(x)

static prog_char banner[] = IRMETERMON_VERSION "-irmetermon\n";
static prog_char quickfox[] = "The Quick Brown Fox Jumps Over The Lazy Dog\n";

#define MS 1000			// or whatever
#define STEP_UP	     10
#define STEP_DOWN   -10
#define PULSE_LEN	10 * MS
#define AVG_DEPTH 4

typedef unsigned long time_t;
void tracker(int new);

unsigned long milliseconds;
#define ms_timer() (milliseconds)

void
init_timer(void)
{
    // suart.c uses the 16 bit timer.  that could change, if XTAL is
    // slow enough to allow bit rates to be timed in 8 bits.  but the
    // overhead of the clock interrupt is very low, so we'll just use
    // the set 8-bit timer0
    // set up for simple overflow operation
    TCCR0A = bit(WGM01);  // CTC
    TCCR0B = bit(CS02) | bit(CS00);   // prescaler is 1024 
    OCR0A = 244;	// see below
    TIMSK0 = bit(OCIE0A);

}

ISR(TIM0_OVF_vect)
{
    static unsigned char prescale;

    // xtal == 1024    *   244   *   N
    //       (prescale)  (overflow)
    // "244" is chosen so N is close to integral for 1Mhz, 8Mhz, 12Mhz, etc.
#define N  (XTAL / (1024UL * 244UL))

    if ((prescale++ % N) == 0)
	milliseconds++;

    TIFR0 = bit(OCF0A);
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

ISR(ADC_vect)
{
    tracker(ADCH);
    ADCSRA |= bit(ADSC);  // next conversion
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
puthex_l(long l)
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
    puthex_l(index);
    sputchar(':');
    puthex_l(now);
    sputchar('\n');
}

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

static unsigned char ovals[3];

int
median(void)
{
    unsigned char a = ovals[0];
    unsigned char b = ovals[1];
    unsigned char c = ovals[2];

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

    ovals[i % 3] = val;
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

    new = filter(new);

    if (new - old > STEP_UP) {
	up = now;
    } else if (up && about_time(up, now) && (new - old) < STEP_DOWN) {
	up = 0;
	if (now > 100) // don't report until adc and averages have settled.
	    found_pulse(now);
    }
}

int
main()
{
    int i;

    suart_init();

    init_adc();
    init_timer();

    sei();
#if LATER
    sputs_p(banner);
#endif

    for (;;) {
	wdt_reset();
	if (kbhit()) {
	    switch (sgetchar()) {
	    case 'v':
		sputs_p(banner);
		break;
	    case 'x':
		for (i = 0; i < 20; i++)
		    sputs_p(quickfox);
		break;
	    case 'U':
		for (i = 0; i < (80 * 20); i++)
		    sputchar('U');
		sputchar('\n');
		break;
	    }
	}

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

}
