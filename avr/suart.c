/************************************************************************/
/*                                                                      */
/*                      Software UART using T1                          */
/*                                                                      */
/*              Author: P. Dannegger                                    */
/*                      danni@specs.de                                  */
/*                                                                      */
/************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "common.h"
#include "suart.h"

#define bit(x) _BV(x)

#define BIT_TIME	(u16)((XTAL + BAUD/2) / BAUD)


volatile u8 stx_count;
u8 stx_data;

#ifdef DO_RECEIVE
volatile u8 srx_done;
u8 srx_data;
u8 srx_mask;
u8 srx_tmp;
#endif


void
suart_init(void)
{
    OCR1A = TCNT1 + 1;		// force first compare
    TCCR1A = bit(COM1A1) | bit(COM1A0);	// set OC1A high, T1 mode 0
    STIMSK = bit(OCIE1A);	// enable tx
#ifdef DO_RECEIVE
    TCCR1B = bit(ICNC1) | bit(CS10);	// noise canceler, 1>0 transition,
    					// CLK/1, T1 mode 0
    STIFR = bit(ICF1);		// clear pending interrupt
    STIMSK |= bit(ICIE1);	// enable rx and wait for start

    srx_done = 0;		// nothing received
#endif
    stx_count = 0;		// nothing to sent
    STXDDR |= bit(STX);		// TX output
}


#ifdef DO_RECEIVE
u8
sgetchar(void)			// get byte
{
    while (!srx_done);		// wait until byte received
    srx_done = 0;
    return srx_data;
}


SIGNAL(SIG_INPUT_CAPTURE1)	// rx start
{
    OCR1B = ICR1 + (u16) (BIT_TIME * 1.5);	// scan 1.5 bits after start
    srx_tmp = 0;		// clear bit storage
    srx_mask = 1;		// bit mask
    STIFR = bit(OCF1B);		// clear pending interrupt
    if (!(SRXPIN & bit(SRX)))	// still low
	STIMSK = bit(OCIE1A) | bit(OCIE1B);	// wait for first bit
}


SIGNAL(SIG_OUTPUT_COMPARE1B)
{
    u8 in = SRXPIN;		// scan rx line

    if (srx_mask) {
	if (in & bit(SRX))
	    srx_tmp |= srx_mask;
	srx_mask <<= 1;
	OCR1B += BIT_TIME;	// next bit slice
    } else {
	srx_done = 1;		// mark rx data valid
	srx_data = srx_tmp;	// store rx data
	STIFR = bit(ICF1);	// clear pending interrupt
	STIMSK = bit(ICIE1) | bit(OCIE1A);	// enable tx and wait for start
    }
}
#endif


void
sputchar(u8 val)		// send byte
{
    if (val == '\n')
	sputchar('\r');
    while (stx_count);		// until last byte finished
    stx_data = ~val;		// invert data for Stop bit generation
    stx_count = 10;		// 10 bits: Start + data + Stop
}


void
sputs_p(const prog_char * txt)	// send string
{
    while (*txt)
	sputchar(*txt++);
}


SIGNAL(SIG_OUTPUT_COMPARE1A)	// tx bit
{
    u8 dout;
    u8 count;

    OCR1A += BIT_TIME;		// next bit slice
    count = stx_count;

    if (count) {
	stx_count = --count;	// count down
	dout = bit(COM1A1);	// set low on next compare
	if (count != 9) {	// no start bit
	    if (!(stx_data & 1))	// test inverted data
		dout = bit(COM1A1) | bit(COM1A0);	// set high on next compare
	    stx_data >>= 1;	// shift zero in from left
	}
	TCCR1A = dout;
    }
}
