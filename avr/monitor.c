/*-------------------------------------------------------------------------
   monitor.c - simple monitor

   Copyright (C) 2010 One Laptop per Child
   Copyright (C) 2011 Paul Fox

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!
-------------------------------------------------------------------------*/

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

typedef void (*voidfunc)(void);
voidfunc vp;


static char banner[] PROGMEM = IRMETERMON_VERSION "-irmetermon\r\n";
static char quickfox[] PROGMEM = "The Quick Brown Fox Jumps Over The Lazy Dog\r\n";

#if defined(MONITOR)

static unsigned char line[16];
static unsigned char l;
static unsigned int addr;
static unsigned char addr_is_data;


static char getline(void)
{
	char c;

	if (!kbhit()) return 0;

	c = sgetchar();

	// eat leading spaces
	if (l == 0 && c == ' ')
		return 0;

	// special for the +/-/= memory dump commands
	if (l == 0 && (c == '+' || c == '-' || c == '='))
	{
		line[l++] = c;
		line[l] = '\0';
		sputchar('\r'); // retreat over the prompt
		return 1;
	}

	if (c == '\r' || c == '\n')
	{
		// done
		sputchar('\n');
		sputchar('\r');
		return 1;
	}

	sputchar(c);

	// backspace
	if (c == '\b' || c == DEL)
	{
		sputchar(' ');
		sputchar('\b');
		if (l > 0) l--;
		line[l] = '\0';
		return 0;
	}

	// accumulate the character
	if (l < (sizeof(line) - 2))
	{
		line[l++] = c;
		line[l] = '\0';
		return 0;
	}

	// line too long
	if (isprint(c))
	{
		sputchar('\b');
		sputchar(' ');
		sputchar('\b');
	}
	return 0;
}

static int gethex(void)
{
	int n = 0;
	while (isspace(line[l]))
	{
		l++;
	}
	while (isxdigit(line[l]))
	{
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

	static char adc_show;
	static time_t adc_show_time;

	if (adc_show) {
	    if (check_timer(adc_show_time, 100)) {
		show_adc();
		adc_show_time = get_ms_timer();
	    }
	}

	if (!getline()) return;

	l = 0;
	cmd = line[l++];
	switch (cmd)
	{
		case '\0':
			break;

		case 'a':
			adc_show = gethex();
			adc_show_time = get_ms_timer();
			break;

		case 'D':
			// irmeter_set_debug(gethex());
			break;

		case 'v':
			sputstring_p(banner);
			break;

		case 'q':
		    for (i = 0; i < 20; i++)
			sputstring_p(quickfox);
		    break;
		case 'U':
		    for (i = 0; i < (80 * 20); i++)
			sputchar('U');
		    sputchar('\n');
		    break;
		case 'b':
		    cli();
		    MCUSR |= (1 << WDRF);
		    wdt_enable(WDTO_250MS);
		    while(1); //loop 
		    break;
		case 'j':
		    cli();
		    vp = *(voidfunc *)0xf000;
		    (*vp)();
		    break;


		case 'w':
			addr = gethex();
			m = gethex();
			*(unsigned char *)addr = m;
			break;

		case 'x':  //  read 1 byte from xdata
		case 'd':  //  read 1 byte from data
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
			printf("%04x: %02x\n", (uint)addr,
				   addr_is_data ?
				   (uint)*(unsigned char *)addr :
				   (uint)*(unsigned char xdata *)addr);
#else
			puthex16(addr);
			sputstring(": ");
		        if (addr_is_data)
			    puthex(*(unsigned char *)addr);
			else
			    puthex(pgm_read_byte(addr));
			sputchar('\n');
#endif
			break;

		default:
			sputstring("?");
	}

	prompt();
}


#elif defined(MINIMAL_MONITOR)
// smaller version, might be useful

void
monitor(void)
{
    int i;
    char c;

    if (!kbhit()) return;

    c = sgetchar();


    switch (c) {
    case 'v':
	sputstring(banner);
	break;
    case 'x':
	for (i = 0; i < 20; i++)
	    sputstring(quickfox);
	break;
    case 'U':
	for (i = 0; i < (80 * 20); i++)
	    sputchar('U');
	sputchar('\n');
	break;
    case 'b':
	cli();
	wdt_enable(WDTO_250MS);
	while(1); //loop 
	break;
    case 'j':
	cli();
	vp = *(voidfunc *)0x1f000;
	(*vp)();
	break;
    }
}

#endif
