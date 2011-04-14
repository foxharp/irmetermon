
#define XTAL	1000000   // 1Mhz, default RC oscillator config

#define	BAUD	38400


typedef unsigned char  u8;
typedef   signed char  s8;
typedef unsigned short u16;
typedef   signed short s16;
typedef unsigned long  u32;
typedef   signed long  s32;

#ifdef USE_SUART
#include "suart.h"
#else // USE_LUFA
#include "main-lufa.h"
#endif

