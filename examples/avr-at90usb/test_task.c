/*
 * Testing task switching.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>

ARRAY (task, 200);
static char oks[] = "ok!\n";
static char testing_tasks[] = "\nTesting task.\n";
static char tasks[] = "task";
static char hellos[] = "hello";
static char msgs[] = "Hello from `%S'! (Press Enter)\n";

void hello (void *arg)
{
	debug_puts (oks);
	for (;;) {
		debug_printf (msgs, arg);
		debug_getchar ();
	}
}

void uos_init (void)
{
	/* Baud 9600. */
	UBRR = ((int) (KHZ * 1000L / 9600) + 8) / 16 - 1;

	debug_puts (testing_tasks);
	task_create (hello, tasks, hellos, 1, task, sizeof (task));
}
