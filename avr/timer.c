
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
// #include <avr/sleep.h>

#include "timer.h"
#include "irmeter.h"
#include "common.h"

time_t milliseconds;

void
init_timer(void)
{
    // set up for simple overflow operation
    TCCR0A = bit(WGM01);  // CTC
    TCCR0B = bit(CS01) | bit(CS00);   // prescaler is 64 
    OCR0A = 244;	// see below
    TIMSK0 = bit(OCIE0A);

}

#ifdef TIMER0_COMPA_vect   
ISR(TIMER0_COMPA_vect)
#else
ISR(TIM0_COMPA_vect)
#endif
{
    static unsigned int prescale;

    // xtal == 1024    *   244   *   N
    //       (prescale)  (overflow)
    // "244" is chosen so N is close to integral for 1Mhz, 8Mhz, 12Mhz, etc.
// #define N  (XTAL / (1024UL * 244UL))
#define N  1

    if ((prescale++ % N) == 0)
	milliseconds++;

#if 0
    if (milliseconds % 1000 == 0) {
	led_flash();
    }
#endif

    TIFR0 = bit(OCF0A);
}

time_t
get_ms_timer(void)
{
    time_t ms;
    char sreg;

    sreg = SREG;
    cli();

    ms = milliseconds;

    SREG = sreg;

    return ms;
}

unsigned char
check_timer(time_t time0, int duration)
{
    return get_ms_timer() > time0 + duration;
}
