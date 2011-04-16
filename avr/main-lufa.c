#include "main-lufa.h"
#include "common.h"
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
    if (c == '\n')
	sputchar('\r');
    CDC_Device_SendByte(&VirtualSerial1_CDC_Interface, (uint8_t)c);
}

static int16_t hit_c;
uint8_t sgetchar( void )
{
    return hit_c & 0xff;
}

int kbhit(void)
{
    hit_c = CDC_Device_ReceiveByte(&VirtualSerial1_CDC_Interface);
    return hit_c >= 0;
}

#if 0
void
sputs_p(const prog_char *s)	// send string
{
    while (*s)
	sputchar(*s++);
}
#endif
void
sputstring_p(const prog_char *s)
{
    char c;
    while ( (c = pgm_read_byte(s++)) )
	sputchar(c);
}

void
sputstring(char *s)
{
    while (*s)
	sputchar(*s++);
}


int main(void)
{
	SetupHardware();

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;)
	{

		CDC_Device_USBTask(&VirtualSerial1_CDC_Interface);

#if DUAL
		/* Discard all received data on the second CDC interface */
		//CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface);
		int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface);
		if (!(ReceivedByte < 0)) {
		    if (ReceivedByte == 'f')
		      CDC_Device_SendByte(&VirtualSerial2_CDC_Interface, (uint8_t)'x');
		    else
		      CDC_Device_SendByte(&VirtualSerial2_CDC_Interface, (uint8_t)ReceivedByte);
		}

		CDC_Device_USBTask(&VirtualSerial2_CDC_Interface);
#endif
		USB_USBTask();
		monitor();
	}
}
