#include "main-lufa.h"
#include "irmeter.h"

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();

	irmeter_hwinit();
}

void
sputchar(char c)
{
    CDC_Device_SendByte(&VirtualSerial2_CDC_Interface, (uint8_t)c);
}

#if 1
void
sputs_p(const prog_char *s)	// send string
{
    while (*s)
	sputchar(*s++);
}
#else
void
sputs_p(const prog_char *s)
{
    char c;
    while ( (c = pgm_read_byte(s++)) )
	sputchar(c);
}
#endif


int main(void)
{
	SetupHardware();

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;)
	{

		/* Discard all received data on the first CDC interface */
		CDC_Device_ReceiveByte(&VirtualSerial1_CDC_Interface);

		/* Echo all received data on the second CDC interface */
		int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface);
		if (!(ReceivedByte < 0))
		  CDC_Device_SendByte(&VirtualSerial2_CDC_Interface, (uint8_t)ReceivedByte);

		CDC_Device_USBTask(&VirtualSerial1_CDC_Interface);
		CDC_Device_USBTask(&VirtualSerial2_CDC_Interface);
		USB_USBTask();
	}
}
