/*
 * Проверка новой функции timer timeout.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <timer/timer.h>
#include <timer/timeout.h>

ARRAY (task, 1000);
timer_t timer;
mutex_t mutex;
timeout_t timeout;

void hello (void *arg)
{
    int i = 0;
    void *signal;
    
	for (;;) {
	    mutex_lock (&mutex);
	    timeout_set_signal (&timeout, (void *) i++);
	    timeout_start (&timeout);
		signal = mutex_wait (&mutex);
		debug_printf ("%s: msec = %d, signal = %d\n",
		    arg, timer_milliseconds (&timer), (int) signal);
		mutex_unlock (&mutex);
	}
}

void uos_init (void)
{
	debug_printf ("\nTesting timer timeout.\n");
	
	timer_init (&timer, KHZ, 10);
	timeout_init (&timeout, &timer, &mutex);
	timeout_set_value (&timeout, 1000);
	
	task_create (hello, "Timer", "hello", 1, task, sizeof (task));
}

