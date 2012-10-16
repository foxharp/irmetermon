/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
 */
#ifndef F_CPU
#error F_CPU must be defined
#endif

// #define  BAUD    38400
// #define  BAUD    9600
// #define  BAUD    4800
#define	BAUD	4800

#define bit(x) _BV(x)

void monitor(void);
void puthex(unsigned char i);
void puthex16(unsigned int i);
void puthex32(long l);

unsigned char getch(void);
void putch(char c);
void putstr_p(const prog_char * s);
#if ! ALL_STRINGS_PROGMEM
void putstr(const char *s);
#else
#define putstr(s) putstr_p(PSTR(s))
#endif

void force_reboot(void);

#ifdef USE_SUART
#include "suart.h"
#else							// USE_LUFA
int kbhit(void);
#endif
