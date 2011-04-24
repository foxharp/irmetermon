/************************************************************************/
/*                                                                      */
/*                      Software UART using T1                          */
/*                                                                      */
/*              Author: P. Dannegger                                    */
/*                      danni@specs.de                                  */
/*                                                                      */
/************************************************************************/
/*
 * This file included in irmetermon by written permission of the
 * author.  irmetermon is licensed under GPL version 2, see accompanying
 * LICENSE file for details.
 */
#ifdef _AVR_IOM8_H_
# define SRX     PB0			// ICP on Mega8
# define SRXPIN  PINB

# define STX     PB1			// OC1A on Mega8
# define STXDDR  DDRB

# define STIFR	TIFR
# define STIMSK	TIMSK

# define TICIE1	ICIE1
#elif defined(_AVR_IOTN44_H_)
# define SRX     PB0			// ICP on Mega8
# define SRXPIN  PINB

# define STX     PB1			// OC1A on Mega8
# define STXDDR  DDRB

# define STIFR	TIFR1
# define STIMSK	TIMSK1

#elif defined(_AVR_IOTN13_H_)
#warning code wont work on attiny13 -- suart.c and timer.c want the same timer right now
# define SRX     PB0			// ICP on Mega8
# define SRXPIN  PINB

# define STX     PB1			// OC1A on Mega8
# define STXDDR  DDRB

# define STIFR	TIFR1
# define STIMSK	TIMSK1

#elif defined(_AVR_IOTN2313_H_)
#warning:  suart.c works on a 2313, but not irmetermon -- no ADC
# define SRX     PD0			// ICP on Mega8
# define SRXPIN  PIND

# define STX     PD1			// OC1A on Mega8
# define STXDDR  DDRD

# define STIFR	TIFR
# define STIMSK	TIMSK

#else
# error
# error Please add the defines:
# error SRX, SRXPIN, STX, STXDDR
# error for the new target !
# error
#endif

void sputchar(char c);
void sputstring_p(const prog_char * s);
#if ! ALL_STRINGS_PROGMEM
void sputstring(const char *s);
#else
#define sputstring(s) sputstring_p(PSTR(s))
#endif

#ifndef NO_RECEIVE
extern volatile unsigned char srx_done;
#define kbhit()	(srx_done)		// true if byte received
unsigned char sgetchar(void);
#endif

extern volatile unsigned char stx_count;
#define stx_active() (stx_count)

void suart_init(void);
// void sputchar( unsigned char val );
// void sputs_p( const prog_char *txt );
