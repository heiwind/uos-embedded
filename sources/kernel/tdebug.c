#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <stream/stream.h>

static const char *ptr_valid_or_bad (void *ptr)
{
	return uos_valid_memory_address (ptr) ? "" : " (bad)";
}

/*
 * Print the task info.
 * When t==0, print idle task with header.
 */
void task_print (stream_t *stream, task_t *t)
{
	small_uint_t n;
	const bool_t verbose = 1;

	if (! t) {
		puts (stream, "Task\t  Address\t Prio\t    Stack\tSpace    Msg\tTicks\n");
		return;
	}
	if (! uos_valid_memory_address (t)) {
		printf (stream, "%p (junk)\n", t);
		return;
	}
	printf (stream, "%S\t%9p\t%c%d\t%9p\t%n\t%p\t%lu",
		task_name (t), t, t == task_current ? '*' : ' ', t->prio,
		t->stack_context, task_stack_avail (t), t->message, t->ticks);
	if (t->wait) {
		printf (stream, "\n\tWaiting for %p%S", t->wait, ptr_valid_or_bad (t->wait));
		if (verbose)
		if (uos_valid_memory_address (t->wait))
		    mutex_print(stream, t->wait);
	}
	if (t->lock) {
		printf (stream, "\n\tLocked by %p%S", t->lock, ptr_valid_or_bad (t->lock));
        if (verbose)
        if (uos_valid_memory_address (t->lock))
            mutex_print(stream, t->lock);
	}

	if (! list_is_empty (&t->slaves)) {
		mutex_t *m;
		puts (stream, "\n\tOwning");
		n = 0;
		list_iterate (m, &t->slaves) {
		    puts (stream, "\n\t\t");
            if (verbose && uos_valid_memory_address (m))
                mutex_print(stream, m);
            else
			printf (stream, "%p%S ", m, ptr_valid_or_bad (m));
			if (! uos_valid_memory_address (m))
				break;
			if (++n > 8 || list_is_empty (&m->item)) {
				puts (stream, "...");
				break;
			}
		}
	}
	putchar (stream, '\n');
}

void task_print_list(stream_t *stream, list_t* ts){
    task_t *t;
    list_iterate (t, ts) {
        printf (stream, "\n\t\t%p%S ", t, ptr_valid_or_bad (t));
        if (! uos_valid_memory_address (t)){
            break;
        }
        printf (stream, " %S", t->name);
    }
    const bool_t verbose = 0;
    if (verbose){
        task_print(stream, 0);
        list_iterate (t, ts)
           task_print(stream, t);
    }
    putchar (stream, '\n');
}

void mutex_print (stream_t *stream, mutex_t *m)
{
    const bool_t verbose = 0;
    //small_uint_t n;

    if (! m)
        return;
    if (! uos_valid_memory_address (m)) {
        printf (stream, "%p (junk)\n", m);
        return;
    }

    printf (stream, "mutex %p ", m);
    if (m->irq != 0){
        printf (stream, "( irq%d pended(%d) on m:%p) ", m->irq->irq, m->irq->pending, m->irq->lock);
    }
    if (m->master != 0) {
        task_t *t = m->master;
        printf (stream, "locked by %s", t->name);
    }
    if (verbose) {
        if (m->irq != 0)
            mutex_print(stream, m->irq->lock);
        if (m->master != 0)
            task_print(stream, m->master);
    }
    if (! list_is_empty (&m->slaves)){
            puts (stream, "\n\tBlocks:");
            task_print_list(stream, &m->slaves);
    }
    if (! list_is_empty (&m->waiters)){
            puts (stream, "\n\tListens:");
            task_print_list(stream, &m->waiters);
    }
    puts (stream, "\n");
}
