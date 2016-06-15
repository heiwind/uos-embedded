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
// here declared NULL
#include <stddef.h>
#include <stdbool.h>

void mutex_init (mutex_t *lock)
{
	list_init (&lock->item);
	list_init (&lock->waiters);
	list_init (&lock->slaves);
	list_init (&lock->groups);
	lock->irq = 0;
	lock->prio = 0;
	lock->master = 0;
#if RECURSIVE_LOCKS
	lock->deep = 0;
#endif
}

/*
 * Get the lock. The calling task would block
 * waiting for the lock to be released by somebody.
 * In the case the lock has associated IRQ number,
 * after acquiring the lock the IRQ will be disabled.
 */
CODE_FAST 
void
mutex_lock (mutex_t *m)
{
#ifndef UOS_MUTEX_FASTER
    mutex_lock_until(m, NULL, NULL);
#else
    if (mutex_recurcived_lock(m))
        return;

	arch_state_t x;

	arch_intr_disable (&x);
	assert (STACK_GUARD (task_current));
#if !RECURSIVE_LOCKS
	assert (task_current != m->master);
#endif
	if (! m->item.next)
		mutex_init (m);

	mutex_lock_yiedling(m);
	arch_intr_restore (x);
#endif
}

/** this lock is blocks until <waitfor> return true, or mutex locked.
 * \return true - mutex succesfuly locked
 * \return false - if mutex not locked due to <waitfor> signalled
 * */
CODE_FAST 
bool_t mutex_lock_until (mutex_t *m, scheduless_condition waitfor, void* waitarg)
{
#if RECURSIVE_LOCKS
    if (mutex_recurcived_lock(m))
        return 1;
#endif

    arch_state_t x;
    arch_intr_disable (&x);
    assert_task_good_stack(task_current);
#if !RECURSIVE_LOCKS
    assert (task_current != m->master);
#endif
    if (! m->item.next)
        mutex_init (m);

    bool_t res = mutex_lock_yiedling_until(m, waitfor, waitarg);
    arch_intr_restore (x);
    return res;
}


CODE_FAST 
void mutex_slave_task(mutex_t *m, task_t* t)
{
    /* Monitor is locked, block the task. */
#if RECURSIVE_LOCKS
    assert (m->deep > 0);
#endif
    t->lock = m;

    /* Put this task into the list of lock slaves. */
    list_append (&m->slaves, &t->item);

    /* Update the value of lock priority.
     * It must be the maximum of all slave task priorities. */
    if (m->prio < t->prio) {
        m->prio = t->prio;

        /* Increase the priority of master task. */
        if (m->master->prio < m->prio) {
            m->master->prio = m->prio;
            /* No need to set task_need_schedule here. */
        }
    }
}

CODE_FAST 
void mutex_slaved_yield(mutex_t *m){
    mutex_slave_task(m, task_current);
    task_schedule ();
}

#if UOS_SIGNAL_SMART > 0
CODE_FAST 
bool_t mutex_wanted_task(task_t *t)
{
    if (t->MUTEX_WANT == 0)
        return 0;
    mutex_t *mm = t->MUTEX_WANT;
    t->MUTEX_WANT = 0;
    if ((mm->master == 0) || (mm->master == t))
        return 0;

    mutex_slave_task(mm, t);
    return 1;
}
#endif


/*
 * Try to get the lock. Return 1 on success, 0 on failure.
 * The calling task does not block.
 * In the case the lock has associated IRQ number,
 * after acquiring the lock the IRQ will be disabled.
 */
CODE_FAST 
bool_t 
mutex_trylock (mutex_t *m)
{
#if RECURSIVE_LOCKS
    if (mutex_recurcived_lock(m))
        return 1;
#endif

    if ((m->master != NULL) && (m->master != task_current))
        return 0;

    arch_state_t x;
    arch_intr_disable (&x);

    if (! m->item.next)
        mutex_init (m);

    assert_task_good_stack(task_current);
    bool_t res = mutex_trylock_in(m);
    arch_intr_restore (x);
    return res;
}

/*
 * Recalculate the value of task priority.
 * It must be the maximum of base priority,
 * and all slave lock priorities.
 */
CODE_FAST 
void
task_recalculate_prio (task_t *t)
{
	mutex_t *m;
	small_int_t old_prio;

	old_prio = t->prio;
	t->prio = t->base_prio;
	list_iterate (m, &t->slaves)
		if (t->prio < m->prio)
			t->prio = m->prio;

	if (t->prio != old_prio) {
		if (t->lock) {
			if (t->prio > old_prio) {
				/* Priority increased. */
				if (t->lock->prio < t->prio)
					mutex_recalculate_prio (t->lock);
			} else {
				/* Priority decreased. */
				if (t->lock->prio <= old_prio)
					mutex_recalculate_prio (t->lock);
			}
		} else {
			if (t->prio > old_prio) {
				if (task_current->prio < t->prio && ! task_is_waiting (t)) {
					/* Active task increased priority - reschedule. */
					task_need_schedule = 1;
				}
			} else if (t == task_current) {
				/* Current task decreased priority - reschedule. */
				task_need_schedule = 1;
			}
		}
	}
}

/*
 * Recalculate the value of lock priority.
 * It must be the maximum of all slave task priorities.
 */
CODE_FAST 
void
mutex_recalculate_prio (mutex_t *m)
{
	task_t *t;
	small_int_t old_prio;

	old_prio = m->prio;
	m->prio = 0;
	list_iterate (t, &m->slaves)
		if (m->prio < t->prio)
			m->prio = t->prio;

	if (m->prio != old_prio && m->master) {
		if (m->prio > old_prio) {
			/* Priority increased. */
			if (m->master->prio < m->prio)
				task_recalculate_prio (m->master);
		} else {
			/* Priority decreased. */
			if (m->master->prio <= old_prio &&
			    m->master->base_prio < old_prio)
				task_recalculate_prio (m->master);
		}
	}
}

/*
 * Release the lock. All tasks waiting for the lock
 * are unblocked, possibly causing task switch.
 * In the case the lock has associated IRQ number,
 * the IRQ will be enabled.
 */
CODE_FAST 
void
mutex_unlock (mutex_t *m)
{
    assert(m->master != NULL);
    assert(m->master == task_current);

    arch_state_t x;
    assert_task_good_stack(task_current);
	arch_intr_disable (&x);

#if RECURSIVE_LOCKS
	if (--m->deep > 0) {
		arch_intr_restore (x);
		return;
	}
#endif

	mutex_do_unlock(m);
#if MUTEX_LASY_SCHED <= 0
	if (task_need_schedule)
		task_schedule ();
#endif
	arch_intr_restore (x);
}

CODE_FAST 
void mutex_do_unlock(mutex_t *m){
    /* Remove this lock from the list of task slaves. */
    list_unlink (&m->item);

    /* Recalculate the value of task priority.
     * It must be the maximum of base priority,
     * and all slave lock priorities. */
    if (m->master->prio <= m->prio && m->master->base_prio < m->prio)
        task_recalculate_prio (m->master);
    m->master = 0;

    /* On pending irq, we must call fast handler. */
    mutex_check_pended_irq(m);

    while (! list_is_empty (&m->slaves)) {
        task_t *t = (task_t*) list_first (&m->slaves);
        assert (t->lock == m);
        t->lock = 0;
        task_activate (t);
    }
    m->prio = 0;
}
