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

list_t task_active;			/* list of tasks ready to run */
task_t *task_current;			/* current running task */
task_t *task_idle;			/* background system task */
task_t *task_yelds;         //points to 1st yelded task in active tasks list
mutex_irq_t mutex_irq [ARCH_INTERRUPTS]; /* interrupt handlers */

#ifndef IDLE_TASK_STACKSZ
#define IDLE_TASK_STACKSZ   (256+MIPS_FSPACE)

#endif

#define ALIGNED_IDLE_TASK_STACKSZ ((IDLE_TASK_STACKSZ + UOS_STACK_ALIGN - 1) & ~(UOS_STACK_ALIGN - 1))

static ARRAY (task_idle_data, sizeof(task_t) + ALIGNED_IDLE_TASK_STACKSZ - UOS_STACK_ALIGN);

//если надо пропустить (yeld) текущую задачу - то этот параметр = task_current
unsigned task_need_schedule;

/*
 * Switch to most priority task if needed.
 */
CODE_ISR 
void task_schedule ()
{
	task_t *new_task;

	if (task_need_schedule != (unsigned)task_current){
	    task_need_schedule  = 0;
	    task_yelds          = 0;
	}
	else {
	    //текущая задача в конец списка
	    /* Enqueue always puts element at the tail of the list. */
        list_append(&task_active, &task_current->item);
	    if (task_yelds == 0)
	        task_yelds = task_current;
	    if (task_yelds != (task_t*)list_first(&task_active)){
	        //отключу временно пропускаемые задачи чтобы task_policy их обошел мимо
            __list_connect_together(task_yelds->item.prev, &task_active);
	    }
	    else{
            task_yelds = 0;
	    }
	}
	new_task = task_policy ();
	if (task_yelds != 0){
        //восстановлю пропускаемые задачи обратно в активные
        __list_connect_together(task_yelds->item.prev, &task_yelds->item);
        __list_connect_together(&task_current->item, &task_active);
	}

	if (new_task != task_current) {
		new_task->ticks++;
#ifndef NDEBUG
        unsigned sp = (unsigned)mips_get_stack_pointer ();
        sp -= (MIPS_FSPACE+CONTEXT_WORDS*4);
        if (task_current->fpu_state != ~0)
            sp -= 32*4;
        assert2(sp > (unsigned)(task_current->stack), uos_assert_task_name_msg, task_current->name);
#endif
		arch_task_switch (new_task);
	}
}

/*
 * Activate all waiters of the lock.
 */
CODE_ISR 
void 
mutex_activate (mutex_t *m, void *message)
{
	task_t *t;
	mutex_slot_t *s;

	assert (m != 0);
	if (! m->item.next)
		mutex_init (m);

	while (! list_is_empty (&m->waiters)) {
		t = (task_t*) list_first (&m->waiters);
		assert2 (t->wait == m, uos_assert_task_name_msg, t->name);
		t->wait = 0;
        t->message = message;
#if UOS_SIGNAL_SMART > 0
        //получили сигнал, и жду захвата мутеха, потому пермещу нитку из ожидания сигнала в 
        //  захватчик мутеха
	    if (t->MUTEX_WANT != 0)
		if (mutex_wanted_task(t))
		    continue;
#endif
		task_activate (t);
	}

	/* Activate groups. */
	list_iterate (s, &m->groups) {
		assert (s->lock == m);
		s->message = message;
		s->active = 1;
		t = s->group->waiter;
		if (t) {
		    //group_lockwait - use groun in paralel with 
		    //  lock operation
			//assert (list_is_empty (&t->item));
			s->group->waiter = 0;
#if UOS_SIGNAL_SMART > 0
            //получили сигнал, и жду захвата мутеха, потому пермещу нитку из ожидания сигнала в 
            //  захватчик мутеха
            if (t->MUTEX_WANT != 0)
            if (mutex_wanted_task(t))
                continue;
#endif
			task_activate (t);
		}
	}
}

/*
 * Additional machine-dependent initialization after
 * user initialization. Needed for some processor 
 * architectures.
 */
void __attribute__ ((weak, noinline))
uos_post_init ()
{
}

/*
 * Call user initialization routine uos_init(),
 * then create the idle task, and run the OS.
 * The idle task uses the default system stack.
 */
int __attribute__ ((weak))
__attribute ((noreturn))
main (void)
{
	/* Create the idle task. */
	task_idle = (task_t*) task_idle_data;
	memset (task_idle->stack, STACK_MAGIC, ALIGNED_IDLE_TASK_STACKSZ);
	assert_task_good_stack (task_idle);
	
	/* Move stack pointer to task_idle stack area */
    unsigned sp = (unsigned)(&task_idle->stack[ALIGNED_IDLE_TASK_STACKSZ]);
    /* stack pointer should align to doubles */
    set_stack_pointer ((void *)( sp & ~(sizeof(double)-1) ));

	task_idle->name = "idle";
	list_init (&task_idle->item);
	list_init (&task_idle->slaves);
#ifdef ARCH_HAVE_FPU
	/* Default state of float point unit. */
	task_idle->fpu_state = ARCH_FPU_STATE;
#endif

	/* Make list of active tasks. */
	list_init (&task_active);
	task_current = task_idle;
	task_activate (task_idle);

	/* Create user tasks. */
	uos_init ();
	
	/* Additional machine-dependent initialization */
	uos_post_init ();

	/* Switch to the most priority task. */
	assert (task_current == task_idle);

    arch_state_t x;
    arch_intr_disable (&x);
	task_schedule ();
	arch_intr_restore(x);

	/* Idle task activity. */
	for (;;) {
		arch_idle ();
	}
}
