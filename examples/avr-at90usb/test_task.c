/*
 * Testing task switching.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>

mutex_t lock;

ARRAY (stack1, 512);
ARRAY (stack2, 512);

#if 1
static void task1 (void *arg)
{
	for (;;) {
	    mutex_lock (&lock);
    	debug_puts ("-");
	    mutex_unlock (&lock);
	}
}

static void task2 (void *arg)
{
	for (;;) {
	    mutex_lock (&lock);
    	debug_printf ("+");
	    mutex_unlock (&lock);
	}
}
#endif

void uos_init (void)
{
	/* Baud 9600. */
	UBRR = ((int) (KHZ * 1000L / 9600) + 8) / 16 - 1;

	debug_dump ("Memory", (void*)0x0, 4096 + 0x100);

	task_create (task1, "task1", "hello1", 10, stack1, sizeof (stack1));
	task_create (task2, "task2", "hello2", 10, stack2, sizeof (stack2));

}

