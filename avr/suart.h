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
#elif defined(_AVR_IOTN44_H_) || defined(IRMETER_ATTINY44)
# define SRX     PA7			// ICP on tiny44
# define SRXPIN  PINA

# define STX     PA6			// OC1A on tiny44
# define STXDDR  DDRA

# define STIFR	TIFR1
# define STIMSK	TIMSK1

#else
# error
# error Please add the defines:
# error SRX, SRXPIN, STX, STXDDR
# error for the new target !
# error
#endif

void putch(char c);
void putstr_p(const prog_char * s);
#if ! ALL_STRINGS_PROGMEM
void putstr(const char *s);
#else
#define putstr(s) putstr_p(PSTR(s))
#endif

#ifndef NO_RECEIVE
extern volatile unsigned char srx_done;
#define kbhit()	(srx_done)		// true if byte received
unsigned char getch(void);
#endif

extern volatile unsigned char stx_count;
#define stx_active() (stx_count)

void suart_init(void);
// void putch( unsigned char val );
// void sputs_p( const prog_char *txt );
