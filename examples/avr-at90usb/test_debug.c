/*
 * Testing debug output.
 */
#include <runtime/lib.h>

static char ok[] = "ok!\n";
static char hello_world[] = "Hello, World!\n";

int main(void) __attribute__((noreturn));

int main (void)
{
    UBRR = ((int) (KHZ * 1000L / 9600) + 8) / 16 - 1; // Set baud rate

    debug_printf (ok);

    for (;;)
	{
    	debug_printf (hello_world);
    	debug_getchar ();
	}
}

