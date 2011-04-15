/*
             LUFA Library
     Copyright (C) Dean Camera, 2010.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the DualVirtualSerial demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "main-lufa.h"

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the first CDC interface,
 *  which sends strings to the host for each joystick movement.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial1_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber           = 0,

				.DataINEndpointNumber             = CDC1_TX_EPNUM,
				.DataINEndpointSize               = CDC_TXRX_EPSIZE,
				.DataINEndpointDoubleBank         = false,

				.DataOUTEndpointNumber            = CDC1_RX_EPNUM,
				.DataOUTEndpointSize              = CDC_TXRX_EPSIZE,
				.DataOUTEndpointDoubleBank        = false,

				.NotificationEndpointNumber       = CDC1_NOTIFICATION_EPNUM,
				.NotificationEndpointSize         = CDC_NOTIFICATION_EPSIZE,
				.NotificationEndpointDoubleBank   = false,
			},
	};

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the second CDC interface,
 *  which echos back all received data from the host.
 */
#if DUAL
USB_ClassInfo_CDC_Device_t VirtualSerial2_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber           = 2,

				.DataINEndpointNumber             = CDC2_TX_EPNUM,
				.DataINEndpointSize               = CDC_TXRX_EPSIZE,
				.DataINEndpointDoubleBank         = false,

				.DataOUTEndpointNumber            = CDC2_RX_EPNUM,
				.DataOUTEndpointSize              = CDC_TXRX_EPSIZE,
				.DataOUTEndpointDoubleBank        = false,

				.NotificationEndpointNumber       = CDC2_NOTIFICATION_EPNUM,
				.NotificationEndpointSize         = CDC_NOTIFICATION_EPSIZE,
				.NotificationEndpointDoubleBank   = false,
			},
	};
#endif


/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial1_CDC_Interface);
#if DUAL
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial2_CDC_Interface);
#endif

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial1_CDC_Interface);
#if DUAL
	CDC_Device_ProcessControlRequest(&VirtualSerial2_CDC_Interface);
#endif
}
