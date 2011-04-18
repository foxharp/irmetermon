/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
 *
 * Based strongly on the DualVirtualSerial demo from the LUFA
 * distribution.  The original file header for that code follows:
 *
 *             LUFA Library
 *     Copyright (C) Dean Camera, 2010.
 *
 *  dean [at] fourwalledcubicle [dot] com
 *           www.lufa-lib.org
 *
 *  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)
 *
 *  Permission to use, copy, modify, distribute, and sell this
 *  software and its documentation for any purpose is hereby granted
 *  without fee, provided that the above copyright notice appear in
 *  all copies and that both that the copyright notice and this
 *  permission notice and warranty disclaimer appear in supporting
 *  documentation, and that the name of the author not be used in
 *  advertising or publicity pertaining to distribution of the
 *  software without specific, written prior permission.
 *
 *  The author disclaim all warranties with regard to this
 *  software, including all implied warranties of merchantability
 *  and fitness.  In no event shall the author be liable for any
 *  special, indirect or consequential damages or any damages
 *  whatsoever resulting from loss of use, data or profits, whether
 *  in an action of contract, negligence or other tortious action,
 *  arising out of or in connection with the use or performance of
 *  this software.
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "timer.h"
#include "common.h"
#include "irmeter.h"
#include "descriptors.h"

#include <LUFA/Version.h>
#include <LUFA/Drivers/USB/USB.h>



/** LUFA CDC Class driver interface configuration and state
 * information.  This structure is passed to all CDC Class driver
 * functions, so that multiple instances of the same class within
 * a device can be differentiated from one another.  This is for
 * the first CDC interface, which sends strings to the host for
 * each joystick movement.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial1_CDC_Interface = {
	.Config = {
			   .ControlInterfaceNumber = 0,

			   .DataINEndpointNumber = CDC1_TX_EPNUM,
			   .DataINEndpointSize = CDC_TXRX_EPSIZE,
			   .DataINEndpointDoubleBank = false,

			   .DataOUTEndpointNumber = CDC1_RX_EPNUM,
			   .DataOUTEndpointSize = CDC_TXRX_EPSIZE,
			   .DataOUTEndpointDoubleBank = false,

			   .NotificationEndpointNumber = CDC1_NOTIFICATION_EPNUM,
			   .NotificationEndpointSize = CDC_NOTIFICATION_EPSIZE,
			   .NotificationEndpointDoubleBank = false,
			   }
	,
};

/** LUFA CDC Class driver interface configuration and state
 * information.  This structure is passed to all CDC Class driver
 * functions, so that multiple instances of the same class within
 * a device can be differentiated from one another.  This is for
 * the second CDC interface, which echos back all received data
 * from the host.
 */
#if DUAL
USB_ClassInfo_CDC_Device_t VirtualSerial2_CDC_Interface = {
	.Config = {
			   .ControlInterfaceNumber = 2,

			   .DataINEndpointNumber = CDC2_TX_EPNUM,
			   .DataINEndpointSize = CDC_TXRX_EPSIZE,
			   .DataINEndpointDoubleBank = false,

			   .DataOUTEndpointNumber = CDC2_RX_EPNUM,
			   .DataOUTEndpointSize = CDC_TXRX_EPSIZE,
			   .DataOUTEndpointDoubleBank = false,

			   .NotificationEndpointNumber = CDC2_NOTIFICATION_EPNUM,
			   .NotificationEndpointSize = CDC_NOTIFICATION_EPSIZE,
			   .NotificationEndpointDoubleBank = false,
			   }
	,
};
#endif


/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &=
		CDC_Device_ConfigureEndpoints(&VirtualSerial1_CDC_Interface);
#if DUAL
	ConfigSuccess &=
		CDC_Device_ConfigureEndpoints(&VirtualSerial2_CDC_Interface);
#endif

	// LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial1_CDC_Interface);
#if DUAL
	CDC_Device_ProcessControlRequest(&VirtualSerial2_CDC_Interface);
#endif
}



// Simple character i/o based on LUFA calls 
static int hit_c;
uint8_t sgetchar(void)
{
	return hit_c & 0xff;
}

int kbhit(void)
{
	hit_c = CDC_Device_ReceiveByte(&VirtualSerial1_CDC_Interface);
	return hit_c >= 0;
}

void sputchar(char c)
{
	if (c == '\n')
		sputchar('\r');
	CDC_Device_SendByte(&VirtualSerial1_CDC_Interface, (uint8_t) c);
}

void sputstring(const char *s)
{
	while (*s)
		sputchar(*s++);
}

void sputstring_p(const prog_char * s)
{
	char c;
	while ((c = pgm_read_byte(s++)))
		sputchar(c);
}

void luart_init(void)
{
	USB_Init();
}

void luart_run(void)
{
	CDC_Device_USBTask(&VirtualSerial1_CDC_Interface);

#if DUAL						// simple echo on the second interface
	int16_t ReceivedByte =
		CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface);
	if (!(ReceivedByte < 0)) {
		CDC_Device_SendByte(&VirtualSerial2_CDC_Interface,
							(uint8_t) ReceivedByte);
	}
	CDC_Device_USBTask(&VirtualSerial2_CDC_Interface);
#endif

	USB_USBTask();
}
