/*
 * Testing task switching.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>

ARRAY (task, 400);

void hello (void *arg)
{
	for (;;) {
		debug_printf ("aa");
		//debug_getchar ();
	}
}

void uos_init (void)
{
    
	/* Baud 9600. */
	UBRR = ((int) (KHZ * 1000L / 9600) + 8) / 16 - 1;

	// Запускаем прерывание системного таймера для работы планировщика

    TCCR1A = 0;
    TCCR1B = 0;
    OCR1A = KHZ  * 10 / 8;
    TCNT1 = 0;
    TCCR1B = 0x0A;  /* clock source CK/8, clear on match A */

	debug_puts ("\nTesting task.\n");
	task_create (hello, "task", "hello", 1, task, sizeof (task));
}

