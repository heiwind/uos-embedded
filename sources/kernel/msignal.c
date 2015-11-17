/*
 * Copyright (C) 2000-2005 Serge Vakulenko, <vak@cronyx.ru>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 *
 * As a special exception to the GPL, permission is granted for additional
 * uses of the text contained in this file.  See the accompanying file
 * "COPY-UOS.txt" for details.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>

/*
 * Send the signal to the lock. All tasks waiting for the signal
 * on the lock are unblocked, possibly causing task switch.
 */
void
mutex_signal (mutex_t *m, void *message)
{
#if FASTER_LOCKS
    if (   list_is_empty (&m->waiters) 
        && list_is_empty (&m->groups)
        && (m->irq == NULL)
       )
        return;
#endif
	arch_state_t x;

	arch_intr_disable (&x);
	assert (STACK_GUARD (task_current));
	if (! m->item.next)
		mutex_init (m);

	if (! list_is_empty (&m->waiters) || ! list_is_empty (&m->groups)) {
		mutex_activate (m, message);
		if (task_need_schedule)
			task_schedule ();
	}
	arch_intr_restore (x);
}

/*
 * Wait for the signal on the lock. The calling task is blocked.
 * In the case the lock has associated IRQ number,
 * it is unmasked (permitted to happen).
 */
void *
mutex_wait (mutex_t *m)
{
	arch_state_t x;
#if RECURSIVE_LOCKS
	small_int_t deep;
#endif

	arch_intr_disable (&x);
	assert (STACK_GUARD (task_current));
	assert (task_current->wait == 0);
	if (! m->item.next)
		mutex_init (m);

	/* On pending irq, we must call fast handler. */
	if (m->irq) {
	    mutex_irq_t *   irq = m->irq;
		if (irq->pending) {
			irq->pending = 0;
			if ((irq->handler) (irq->arg) == 0) {
				/* Unblock all tasks, waiting for irq. */
				mutex_activate (m, 0);
				if (task_need_schedule)
					task_schedule ();
				arch_intr_restore (x);
				return 0;
			}
		}
		else if (irq->irq >= 0)
            arch_intr_allow (irq->irq);
	}

	task_current->wait = m;
	list_append (&m->waiters, &task_current->item);
 	if (m->master != task_current) {
		/* We do not keep this lock, so just wait for a signal. */
		task_schedule ();
 		arch_intr_restore (x);
 		return task_current->message;
 	}

 	/* The lock is hold by the current task - release it. */
#if RECURSIVE_LOCKS
	assert (m->deep > 0);
	deep = m->deep;
	m->deep = 0;
#endif
	list_unlink (&m->item);

	/* Recalculate the value of task priority.
	 * It must be the maximum of base priority,
	 * and all slave lock priorities. */
	if (task_current->prio <= m->prio && task_current->base_prio < m->prio)
		task_recalculate_prio (task_current);

	m->master = 0;
	while (! list_is_empty (&m->slaves)) {
		task_t *t = (task_t*) list_first (&m->slaves);
		assert (t->lock == m);
		t->lock = 0;
		task_activate (t);
	}
	m->prio = 0;
	task_schedule ();

	mutex_lock_yiedling(m);

	arch_intr_restore (x);
	return task_current->message;
}
