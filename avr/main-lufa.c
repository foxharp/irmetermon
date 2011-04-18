#include "main-lufa.h"
#include "timer.h"
#include "common.h"
#include "irmeter.h"


// simple character i/o based on LUFA calls 
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

void
sputchar(char c)
{
    if (c == '\n')
	sputchar('\r');
    CDC_Device_SendByte(&VirtualSerial1_CDC_Interface, (uint8_t)c);
}

void
sputstring(const char *s)
{
    while (*s)
	sputchar(*s++);
}

void
sputstring_p(const prog_char *s)
{
    char c;
    while ( (c = pgm_read_byte(s++)) )
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
		led_handle();
	}
}

