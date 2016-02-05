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
#include <stdbool.h>
#include <stddef.h>

/*
 * Initialize the group data structure.
 * Buffer must have at least sizeof(mutex_group_t) bytes.
 * It must be zeroed before call to mutex_group_init().
 */
mutex_group_t *
mutex_group_init (array_t *buf, unsigned buf_size)
{
	mutex_group_t *g;

	assert (buf_size >= sizeof(mutex_group_t));
	g = (mutex_group_t*) buf;
	g->size = 1 + (buf_size - sizeof(mutex_group_t)) / sizeof(mutex_slot_t);
	return g;
}

/*
 * Add a lock to the group.
 * Return 1 on success, 0 on failure.
 */
bool_t
mutex_group_add (mutex_group_t *g, mutex_t *m)
{
	mutex_slot_t *s;

	if (! m->item.next)
		mutex_init (m);
	if (g->num >= g->size)
		return 0;
	s = g->slot + g->num;
	list_init (&s->item);
	s->group = g;
	s->lock = m;
	s->message = 0;
    //s->active = 0;
	++g->num;
	return 1;
}

/*
 * Start listening on all locks in the group.
 * Attach slots to the lock->groups linked list of every lock.
 * Use mutex_group_unlisten() to stop listening.
 * Beware, multiple calls to mutex_group_listen() will crash the system!
 */
void
mutex_group_listen (mutex_group_t *g)
{
	arch_state_t x;
	mutex_slot_t *s;

	arch_intr_disable (&x);
	assert_task_good_stack(task_current);
	for (s = g->slot + g->num; --s >= g->slot; ) {
		assert (list_is_empty (&s->item));
		s->message = 0;
	    s->active = 0;
		list_prepend (&s->lock->groups, &s->item);
	}
	arch_intr_restore (x);
}

/*
 * Stop listening on the group.
 * Detach slots from the lock->groups linked lists.
 * Use mutex_group_listen() to start listening.
 * Beware, multiple calls to mutex_group_unlisten() will crash the system!
 */
void
mutex_group_unlisten (mutex_group_t *g)
{
	arch_state_t x;
	mutex_slot_t *s;

	arch_intr_disable (&x);
	assert_task_good_stack(task_current);
	for (s = g->slot + g->num; --s >= g->slot; ) {
		assert (! list_is_empty (&s->item));
		//s->message = 0;
		list_unlink (&s->item);
	}
	arch_intr_restore (x);
}

struct mg_signal_ctx {
    mutex_group_t *g;
    mutex_t **lock_ptr;
    void **msg_ptr;
};

bool_t mutex_group_signaled(struct mg_signal_ctx* ctx)
{
    mutex_slot_t *s;
    mutex_group_t *g = ctx->g;
    /* Find an active slot. */
    for (s = g->slot + g->num; --s >= g->slot; ) {
        if (s->active) {
            if (ctx->lock_ptr)
                *(ctx->lock_ptr) = s->lock;
            if (ctx->msg_ptr)
                *(ctx->msg_ptr) = s->message;
            s->active = 0;
            return true;
        }
    }
    return false;
}

/*
 * Wait for the signal on any lock in the group.
 * The calling task is blocked until the mutex_signal().
 * Returns the lock and the signalled message.
 */
void
mutex_group_wait (mutex_group_t *g, mutex_t **lock_ptr, void **msg_ptr)
{
	arch_state_t x;
	struct mg_signal_ctx signaler = {g, lock_ptr, msg_ptr};

	arch_intr_disable (&x);
	assert_task_good_stack(task_current);
	assert (task_current->wait == 0);
	assert (g->num > 0);

	for (;;) {
	    if (mutex_group_signaled(&signaler)){
				arch_intr_restore (x);
				return;
		}

		/* Suspend the task. */
		list_unlink (&task_current->item);
		g->waiter = task_current;
		task_schedule ();
	}
}

bool_t 
mutex_group_lockwaiting (mutex_t *m, mutex_group_t *g, mutex_t **lock_ptr, void **msg_ptr)
{
    if (mutex_recurcived_lock(m))
        return 1;

    arch_state_t x;
    struct mg_signal_ctx signaler = {g, lock_ptr, msg_ptr};

    arch_intr_disable (&x);
    assert_task_good_stack(task_current);
    assert (task_current->wait == 0);
    assert (g->num > 0);
    if (m != NULL)
    if (! m->item.next)
        mutex_init (m);

    
    for (;;) {
        if (m != NULL)
        if (mutex_trylock_in(m)) {
            arch_intr_restore (x);
            return true;
        }
        if (mutex_group_signaled(&signaler)){
                arch_intr_restore (x);
                return false;
        }

        if (m != NULL)
            mutex_slaved_yield(m);
        else {
            /* Suspend the task. */
            list_unlink (&task_current->item);
            task_schedule ();
        }
    }
}
