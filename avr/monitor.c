/* vi: set sw=4 ts=4: */
/*
 * monitor.c - simple monitor
 *
 * Copyright (c) 2010 One Laptop per Child
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "timer.h"
#include "common.h"
#include "irmeter.h"

#define ctrl(c) (c ^ 0x40)
#define DEL 0x7f
#define tohex(c) (((c) >= 'a') ? ((c) - 'a' + 10) : ((c) - '0'))


#if 0
static char banner[] PROGMEM = IRMETERMON_VERSION "-irmetermon\n";
static char quickfox[] PROGMEM =
	"The Quick Brown Fox Jumps Over The Lazy Dog\n";
#else
#define BANNER IRMETERMON_VERSION "-irmetermon\n"
#define QUICKFOX "The Quick Brown Fox Jumps Over The Lazy Dog\n"
#endif

#if defined(MONITOR)

static unsigned char line[16];
static unsigned char l;
static unsigned int addr;
static unsigned char addr_is_data;


static char getline(void)
{
	char c;

	if (!kbhit())
		return 0;

	c = sgetchar();

	// eat leading spaces
	if (l == 0 && c == ' ')
		return 0;

	// special for the +/-/= memory dump commands
	if (l == 0 && (c == '+' || c == '-' || c == '=')) {
		line[l++] = c;
		line[l] = '\0';
		sputchar('\r');			// retreat over the prompt
		return 1;
	}

	if (c == '\r' || c == '\n') {
		// done
		sputchar('\n');
		sputchar('\r');
		return 1;
	}

	sputchar(c);

	// backspace
	if (c == '\b' || c == DEL) {
		sputchar(' ');
		sputchar('\b');
		if (l > 0)
			l--;
		line[l] = '\0';
		return 0;
	}
	// accumulate the character
	if (l < (sizeof(line) - 2)) {
		line[l++] = c;
		line[l] = '\0';
		return 0;
	}
	// line too long
	if (isprint(c)) {
		sputchar('\b');
		sputchar(' ');
		sputchar('\b');
	}
	return 0;
}

static int gethex(void)
{
	int n = 0;
	while (isspace(line[l])) {
		l++;
	}
	while (isxdigit(line[l])) {
		n = (n << 4) | tohex(line[l]);
		l++;
	}
	return n;
}

static void prompt(void)
{
	l = 0;
	line[0] = '\0';
	sputchar('>');
}


void monitor(void)
{
	unsigned int i;
	unsigned char cmd, m;

	if (!getline())
		return;

	l = 0;
	cmd = line[l++];
	switch (cmd) {
	case '\0':
		break;

	case 'a':
		adc_fastdump = gethex() & 0xfff;
		break;

	case 'A':
		sputstring("now: ");
		puthex32(get_ms_timer());
		sputchar('\n');
		show_adc();
		show_pulse(1);
		sputchar('\n');
		break;

	case 'm':
		use_median = gethex();
		break;

	case 's':
		step_size = gethex();
		break;

	case 'D':
		// irmeter_set_debug(gethex());
		break;

	case 'v':
		sputstring(BANNER);
		break;

	case 'q':
		for (i = 0; i < 20; i++)
			sputstring(QUICKFOX);
		break;

	case 'U':
		for (i = 0; i < (80 * 20); i++)
			sputchar('U');
		sputchar('\n');
		break;

	case 'e':
		// restarts the current image
		wdt_enable(WDTO_250MS);
		for (;;);
		break;

	case 'b':
		// yanks the h/w reset line (jumpered to PB1), which
		// takes us to the bootloader.
		DDRB = bit(PB1);
		PORTB &= ~bit(PB1);
		for (;;);
		break;

	case 'w':
		addr = gethex();
		m = gethex();
		*(unsigned char *) addr = m;
		break;

	case 'x':					//  read 1 byte from xdata
	case 'd':					//  read 1 byte from data
		addr = gethex();
		addr_is_data = (cmd == 'd');
		// fallthrough

	case '=':
	case '+':
	case '-':
		if (cmd == '+')
			addr++;
		else if (cmd == '-')
			addr--;

#if 0
		printf("%04x: %02x\n", (uint) addr,
			   addr_is_data ?
			   (uint) * (unsigned char *) addr :
			   (uint) * (unsigned char xdata *) addr);
#else
		puthex16(addr);
		sputstring(": ");
		if (addr_is_data)
			puthex(*(unsigned char *) addr);
		else
			puthex(pgm_read_byte(addr));
		sputchar('\n');
#endif
		break;

	default:
		sputchar('?');
	}

	prompt();
}


#elif defined(MINIMAL_MONITOR)
// smaller version, might be useful

void monitor(void)
{
	int i;
	char c;

	if (!kbhit())
		return;

	c = sgetchar();

#if TEST_RX
	puthex(c);
	sputchar(',');
	sputchar(' ');
	return;
#endif


	switch (c) {
	case 'v':
		sputstring(BANNER);
		break;

	case 'a':
		adc_fastdump = 1000;
		break;

	case 'q':
		adc_fastdump = 0;
		break;


	case 'x':
		for (i = 0; i < 20; i++)
			sputstring(QUICKFOX);
		break;

	case 'U':
		for (i = 0; i < (80 * 20); i++)
			sputchar('U');
		sputchar('\n');
		break;

	case 's':
		for (i = 0; i < (80 * 20); i++)
			sputchar("0123456789abcdef"[(get_ms_timer() / 1000) & 0xf]);	/* last digit of time */
		sputchar('\n');
		break;

	case 'e':
		wdt_enable(WDTO_250MS);
		for (;;);
		break;

	}
}

#endif
