/*
 * Testing task switching.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>

ARRAY (task, 400);

void hello (void *arg)
{
	debug_printf ("hello ok\n");	

    for (;;) {
		debug_printf ("Hello from (Press Enter)\n");    
//		debug_getchar ();
	}
}

void uos_init (void)
{
	/* Baud 9600. */
	UBRR = ((int) (KHZ * 1000L / 9600) + 8) / 16 - 1;

	debug_puts ("\nTesting task.\n");
	task_create (hello, "task", "hello", 1, task, sizeof (task));
}
