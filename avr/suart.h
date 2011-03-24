#ifdef _AVR_IOM8_H_
#define SRX     PB0     // ICP on Mega8
#define SRXPIN  PINB

#define STX     PB1     // OC1A on Mega8
#define STXDDR  DDRB

#define STIFR	TIFR
#define STIMSK	TIMSK

#define TICIE1	ICIE1   
#elif defined(_AVR_IOTN44_H_)
#define SRX     PB0     // ICP on Mega8
#define SRXPIN  PINB

#define STX     PB1     // OC1A on Mega8
#define STXDDR  DDRB

#define STIFR	TIFR1
#define STIMSK	TIMSK1

#elif defined(_AVR_IOTN2313_H_)
#define SRX     PD0     // ICP on Mega8
#define SRXPIN  PIND

#define STX     PD1     // OC1A on Mega8
#define STXDDR  DDRD

#define STIFR	TIFR
#define STIMSK	TIMSK

#else
#error
#error Please add the defines:
#error
#error SRX, SRXPIN, STX, STXDDR
#error
#error for the new target !
#error
#endif

#ifndef NO_RECEIVE
extern volatile u8 srx_done;
#define kbhit()	(srx_done)	// true if byte received
u8 sgetchar( void );
#endif

extern volatile u8 stx_count;
#define stx_active() (stx_count)

void suart_init( void );
void sputchar( u8 val );
void sputs_p( const prog_char *txt );


