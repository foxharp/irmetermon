
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "common.h"
#include "suart.h"

#define bit(x) _BV(x)

static prog_char banner_s[] = IRMETERMON_VERSION "irmetermon\n";

#define MS 1000			// or whatever
#define STEP_UP	     10
#define STEP_DOWN   -10
#define PULSE_LEN	10 * MS
#define AVG_DEPTH 4

typedef unsigned long time_t;
void tracker(int new);


void
init_timer(void)
{
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

int
moving_avg(int val)
{
    static int avg;

    avg = avg - (avg / AVG_DEPTH) + val;

    return avg / AVG_DEPTH;
}

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

    // now = ms_timer();
    now = 0;  // FIXME

    new = moving_avg(new);

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
    suart_init();
    // init_adc();
    init_timer();

    sei();
    sputs_p(banner_s);

    for (;;) {
	wdt_reset();
	if (kbhit()) {
	    switch (sgetchar()) {
	    case 'x':
		while (!kbhit())
		    sputs_p("The Quick Brown Fox Jumps Over The Lazy Dog\n");
		sgetchar();
		break;
	    case 'U':
		while (!kbhit())
		    sputchar('U');
		sgetchar();
		break;
	    }
	}

#if LATER
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
