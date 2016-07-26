/*
 * Testing task switching.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>

static mutex_t lock;

ARRAY (stack1, 512);
ARRAY (stack2, 512);

static void task1 (void *arg)
{
	for (;;) {
	    mutex_lock (&lock);
    	debug_puts ("-");
    	watchdog_alive();
	    mutex_unlock (&lock);
	}
}

static void task2 (void *arg)
{
	for (;;) {
	    mutex_lock (&lock);
    	debug_puts ("-");
    	watchdog_alive();
	    mutex_unlock (&lock);
	}
}

void uos_init (void)
{
	/* Baud 9600. */
	UBRR = ((int) (KHZ * 1000L / 9600) + 8) / 16 - 1;
	debug_puts ("Hello\n");

	task_create (task1, "argument1", "hello1", 10, stack1, sizeof (stack1));
	task_create (task2, "argument2", "hello2", 11, stack2, sizeof (stack2));

	//debug_dump ("\nMutex", (void*)&lock, sizeof(lock));// tmp
	//task_print (&debug, 0);
	//task_print (&debug, t);
}

