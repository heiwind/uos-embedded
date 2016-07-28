/*
 * Testing task switching.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>

mutex_t lock;

ARRAY (task_space, 256);
ARRAY (helper_space, 256);

static void helper (void *arg)
{
	void *msg;
	mutex_lock(&lock);
	for (;;) {
		msg = mutex_wait(&lock);
		if (msg) {
			debug_puts (msg);
		} else {
			debug_puts ("\nEmpty\n");
		}
	}

}

static void task (void *arg)
{

	char q[] = {"q"};
	for (;;) {
		debug_printf ("Hello from `%s'!\n", arg);
		debug_printf ("Task space %d bytes, free %d bytes.\n",
			sizeof (task_space), task_stack_avail ((task_t*) task_space));
		debug_puts ("(Press Enter)\n");
		q[0] = debug_getchar();
		mutex_signal(&lock, q);
	}
}

void uos_init (void)
{
	/* Baud 9600. */
	UBRR = ((int) (KHZ * 1000L / 9600) + 8) / 16 - 1;

    debug_printf ("\nTesting task on AVR\n");
	task_create (task, "task", "task", 1, task_space, sizeof (task_space));
	task_create (helper, "helper", "helper", 2, helper_space, sizeof (helper_space));
}

