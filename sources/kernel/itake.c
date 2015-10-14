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
 * Get the lock. The calling task would block
 * waiting for the lock to be released by somebody.
 * Associate IRQ with the lock, disable interrupts.
 * On interrupt, the signal message is sent to the lock.
 */

void 
mutex_lock_irq (mutex_t *m, int irq, handler_t func, void *arg)
{
	arch_state_t x;
    arch_intr_disable (&x);

	mutex_irq_t* swi = &mutex_irq [irq];

	if (! m->irq->lock)
		arch_intr_bind (irq);
	mutex_lock_swi(m, swi, func, arg);
	swi->irq = irq;

	arch_intr_restore (x);
}

void
mutex_lock_swi (mutex_t* m, mutex_irq_t* swi, handler_t func, void *arg)
{
    mutex_lock(m);

    m->irq = swi;
    m->irq->lock = m;
    m->irq->handler = func;
    m->irq->arg = arg;
    m->irq->pending = 0;
}
