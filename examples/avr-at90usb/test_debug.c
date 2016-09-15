/*
 * Testing debug output.
 */
#include <runtime/lib.h>

int main (void)
{
    UBRR = ((int) (KHZ * 1000L / 9600) + 8) / 16 - 1; // Set baud rate
    for (;;)
	{
    	debug_printf ("Hello, World!\n");
    	debug_getchar ();
	}
}

