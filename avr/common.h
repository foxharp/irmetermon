
#define XTAL	16000000   // 1Mhz, default RC oscillator config

#define	BAUD	38400

#define bit(x) _BV(x)

typedef unsigned char  u8;
typedef   signed char  s8;
typedef unsigned short u16;
typedef   signed short s16;
typedef unsigned long  u32;
typedef   signed long  s32;

void monitor(void);
void puthex(unsigned char i);
void puthex16(unsigned int i);
void puthex32(long l);

uint8_t sgetchar(void);
void sputchar(char c);
void sputstring(const char *s);
void sputstring_p(const prog_char *s);

#ifdef USE_SUART
#include "suart.h"
#else // USE_LUFA
int kbhit(void);
#include "main-lufa.h"
#endif

