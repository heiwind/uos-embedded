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
#include <stddef.h> 

/*
 * Send the signal to the lock. All tasks waiting for the signal
 * on the lock are unblocked, possibly causing task switch.
 */
CODE_FAST 
void 
mutex_signal (mutex_t *m, void *message)
{
#if FASTER_LOCKS
    if (!mutex_is_wait(m))
        return;
#endif
	arch_state_t x;

	arch_intr_disable (&x);
	assert_task_good_stack(task_current);
	if (! m->item.next)
		mutex_init (m);

	if (! list_is_empty (&m->waiters) || ! list_is_empty (&m->groups)) {
	    mutex_awake(m, message);
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
#ifdef UOS_MUTEX_FASTER

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
    if (mutex_check_pended_irq(m)){
        if (task_need_schedule)
            task_schedule ();
        arch_intr_restore (x);
        return 0;
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
    mutex_do_unlock(m);
	task_schedule ();
	mutex_lock_yiedling(m);
#if RECURSIVE_LOCKS
    m->deep = deep;
#endif
	
	arch_intr_restore (x);
    return task_current->message;
#else //UOS_MUTEX_FASTER
    mutex_wait_until(m, (scheduless_condition)NULL, NULL);
    return task_current->message;
#endif //UOS_MUTEX_FASTER
}

bool_t mutex_wait_until (mutex_t *m
        , scheduless_condition waitfor, void* waitarg
        )
{
    arch_state_t x;
#if RECURSIVE_LOCKS
    small_int_t deep;
#endif

    arch_intr_disable (&x);

    if (! m->item.next)
        mutex_init (m);

    assert_task_good_stack(task_current);
    assert (task_current->wait == 0);

    /* On pending irq, we must call fast handler. */
    if (mutex_check_pended_irq(m)){
        if (task_need_schedule)
            task_schedule ();
    }

    if (m->irq){
        //для ожидания на прерывании, форсирую разрешение прерывания
        mutex_irq_t *   irq = m->irq;
        if (irq->irq >= 0)
            arch_intr_allow (irq->irq);
    }

    task_current->wait = m;
    list_append (&m->waiters, &task_current->item);
    if (m->master != task_current) {
        /* We do not keep this lock, so just wait for a signal. */
        bool_t res = 1;
        do {
            task_schedule ();
            if (task_current->wait == 0)
                break;
            if (waitfor != NULL)
                res = !(*waitfor)(waitarg);
        } while (res);
        arch_intr_restore (x);
        return res;
    }

    /* The lock is hold by the current task - release it. */
#if RECURSIVE_LOCKS
    assert (m->deep > 0);
    deep = m->deep;
    m->deep = 0;
#endif
    mutex_do_unlock(m);

#if UOS_SIGNAL_SMART > 0
    if (waitfor == 0)
        task_current->MUTEX_WANT = m;
#endif

    task_schedule ();

    bool_t res = mutex_lock_yiedling_until(m, waitfor, waitarg);
    //if (task_current->wait =! 0)
        task_current->wait = 0;
#if RECURSIVE_LOCKS
    m->deep = deep;
#endif
    arch_intr_restore (x);
    return res;
}

// it is try to handle IRQ handler and then mutex_activate, if handler return true
// \return 0 - no activation was pended
// \return else - value of handler:
CODE_ISR 
bool_t mutex_awake (mutex_t *m, void *message)
{
    if (m->irq) {
        mutex_irq_t *   irq = m->irq;
        if (irq->handler) {
            /* If the lock is free -- call fast handler. */
            if (m->master) { //(irq->lock->master)
                /* Lock is busy -- remember pending irq.
                 * Call fast handler later, in mutex_unlock(). */
                irq->pending = 1;
            }
            else {
                bool_t res = (irq->handler) (irq->arg);
                if (res != 0) {
                    /* The fast handler returns 1 when it fully
                     * serviced an interrupt. In this case
                     * there is no need to wake up the interrupt
                     * servicing task, stopped on mutex_wait.
                     * Task switching is not performed. */
                    return res;
                }
            }
        }
    }
    mutex_activate (m, message);
    return 0;
}
