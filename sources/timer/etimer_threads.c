/*
 * etimer_threads.c
 *
 *  Created on: 21.10.2015
 *      Author: a_lityagin
 */

#include <uos-conf.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <runtime/arch.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef USER_TIMERS

#define INLINE_ETIMER
#include "etimer_threads.h"



bool_t etimer_mutex_timedlock(mutex_t* m, etimer* t, etimer_time_t timeout)
{
    if (mutex_trylock(m))
        return true;
    etimer_assign_task(t, task_current);
    etimer_set(t, timeout);
    bool_t res = mutex_lock_until(m, (etimer_is_timeout), (void*)t);
    etimer_stop(t);
    return res;
}

bool_t mutex_etimedlock(mutex_t* m, etimer_time_t timeout){
    etimer t;
    list_init(&t.item);
    return etimer_mutex_timedlock(m, &t, timeout);
}

bool_t etimer_mutex_timedwait(mutex_t* m, etimer* t, etimer_time_t timeout){
    etimer_assign_task(t, task_current);
    etimer_set(t, timeout);
    bool_t res = mutex_wait_until(m, (etimer_is_timeout), (void*)t);
    etimer_stop(t);
    return res;
}

bool_t mutex_etimedwait(mutex_t* m, etimer_time_t timeout){
    etimer t;
    list_init(&t.item);
    if (etimer_is_wait(&t))
        return etimer_mutex_timedwait(m, &t, timeout);
    return 0;
}

typedef struct {
    etimer  et;
    task_t* t;
} task_wait_timeout;

bool_t taskwait_is_timeout(void* arg){
    task_wait_timeout* self = (task_wait_timeout*)arg;
    return (self->t->base_prio <= 0) || etimer_is_timeout(&self->et);
}

void* task_etimedwait (task_t *t, etimer_time_t timeout){
    void *message = t;
    if (t->base_prio) {
        task_wait_timeout to;
        //etimer_init(&to.et);
        list_init(&to.et.item);
        etimer_assign_task(&to.et, task_current);
        etimer_set(&to.et, timeout);
        to.t    = t;

        mutex_lock (&t->finish);
        bool_t res = mutex_wait_until(&t->finish, (taskwait_is_timeout), (void*)&to);
        etimer_stop(&to.et);
        if (res)
            message = t->message;
        //else
        if (mutex_is_my(&t->finish))
            mutex_unlock (&t->finish);
    }
    return message;
}


/**
 *  \brief      sleep current thread for activation by etimer
 *              !!! it froces et to etimer_assign_task mode
 *  \param et   A pointer to the event timer
 * \param sanity == ewsUntilTimeout - ожидает до завершения таймаута
 *      sanity != 0 - ожидает до ближайшего просыпания нитки
 * \return = 0  - таймаут завершен
 * \return = -1 - таймаут незавершен
 */
int etimer_wait(etimer *t, bool_t sanity){
    if (!etimer_is_wait(t))
        return 0;
    etimer_assign_task(t, task_current);

    arch_state_t x = arch_intr_off();

    bool_t ok = 0;
    do {
    /* Suspend the task. */
    ok = etimer_not_active(t);
    if (ok)
        break;
    list_unlink (&task_current->item);
    task_need_schedule = 1;
    task_schedule ();
    } while (sanity == 0);
    if (!ok)
        etimer_stop(t);

    arch_intr_restore (x);
    return (ok)? 0: -1;
}

//* выполняет ожидание таймаута usec
//* \arg sanity == 0 - ожидает до завершения таймаута
//*      sanity != 0 - ожидает до ближайшего просыпания нитки
//* \return = 0 - таймаут завершен
int etimer_uswait(unsigned usec, bool_t sanity)
{
    if (usec == 0){
        task_yield();
        return 0;
    }

    etimer t;
    list_init(&t.item);
    etimer_assign_task(&t, task_current);
    etimer_set(&t, usec);
    bool_t ok = etimer_wait(&t, sanity);
    return ok;
}

#endif //USER_TIMERS

