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
#include <runtime/list.h>
#include <kernel/uos.h>
#include <kernel/internal.h>

/*
 * Release the lock. All tasks waiting for the lock
 * are unblocked, possibly causing task switch.
 * Disassociate IRQ with the lock, enable interrupts.
 */
void
mutex_unlock_irq (mutex_t *m)
{
	arch_state_t x;

	assert (STACK_GUARD (task_current));
	arch_intr_disable (&x);
	assert (m->master != 0);

#if RECURSIVE_LOCKS
	assert (m->deep > 0);
	if (--m->deep > 0) {
		arch_intr_restore (x);
		return;
	}
#endif
    mutex_do_unlock(m);
    if (m->irq) {
        mutex_irq_t* irq = m->irq;
        if (irq->irq >= 0)
            arch_intr_unbind (irq->irq);
        irq->lock = 0;
        m->irq = 0;
    }
	if (task_need_schedule)
		task_schedule ();
	arch_intr_restore (x);
}
