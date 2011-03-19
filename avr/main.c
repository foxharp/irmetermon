
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "common.h"
#include "suart.h"

static prog_char banner_s[] = IRMETERMON_VERSION "irmetermon\n";

#define MS 1000			// or whatever
#define STEP_UP	     10
#define STEP_DOWN   -10
#define PULSE_LEN	10 * MS
#define AVG_DEPTH 8


void
init_comm(void)
{
    suart_init();
}


void
init_timer(void)
{
}

void
init_adc(void)
{
}

void
puthex(unsigned char c)
{
    sputchar("0123456789abcdef"[(c >> 4) & 0xf]);
    sputchar("0123456789abcdef"[(c >> 0) & 0xf]);
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
found_pulse(int now)
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
    static int avg, averaging;

    if (!averaging) {
	avg = val * AVG_DEPTH;
	averaging = 1;
    } else {
	avg = avg - (avg / AVG_DEPTH) + val;
    }

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
tracker(int new, int now)
{
    static int old, tracking;
    static int up;

    new = moving_avg(new);

    if (!tracking) {
	old = new;
	tracking = 1;
    } else {
	if (new - old > STEP_UP) {
	    up = now;
	} else if (up && about_time(up, now) && (new - old) < STEP_DOWN) {
	    up = 0;
	    found_pulse(now);
	}
    }
}

int
main()
{
    init_comm();
    init_adc();
    init_timer();

    sei();
    sputs_p(banner_s);

    for (;;) {
	wdt_reset();
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
    }

}
