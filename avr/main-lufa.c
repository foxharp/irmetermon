/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 2011 Paul Fox, pgf@foxharp.boston.ma.us
 *
 * Licensed under GPL version 2, see accompanying LICENSE file
 * for details.
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

USB_ClassInfo_CDC_Device_t VirtualSerial1_CDC_Interface;
USB_ClassInfo_CDC_Device_t VirtualSerial2_CDC_Interface;

char reboot;

void force_reboot(void)
{
	reboot = 1;
}

// simple character i/o based on LUFA calls 
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

// hardware setup
void hardware_setup(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	USB_Init();

	irmeter_hwinit();
}


int main(void)
{
	hardware_setup();

	sei();

	while (!reboot) {

		CDC_Device_USBTask(&VirtualSerial1_CDC_Interface);

#if DUAL
		/* Discard all received data on the second CDC interface */
		//CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface);
		int16_t ReceivedByte =
			CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface);
		if (!(ReceivedByte < 0)) {
			if (ReceivedByte == 'f')
				CDC_Device_SendByte(&VirtualSerial2_CDC_Interface,
									(uint8_t) 'x');
			else
				CDC_Device_SendByte(&VirtualSerial2_CDC_Interface,
									(uint8_t) ReceivedByte);
		}

		CDC_Device_USBTask(&VirtualSerial2_CDC_Interface);
#endif
		USB_USBTask();

		monitor();
		led_handle();
	}

	// this doesn't seem to work like i think it should.
	// linux doesn't see that the current device is gone
	// before the next instantiation appears.
	wdt_enable(WDTO_4S);
	USB_Detach();
	USB_USBTask();
	USB_ShutDown();
	for (;;);

}


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

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the second CDC interface,
 *  which echos back all received data from the host.
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
