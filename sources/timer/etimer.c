/*
 *  Created on: 06.10.2015
 *      Author: a_lityagin <alexraynepe196@gmail.com>
 *                         <alexraynepe196@hotbox.ru>
 *                         <alexraynepe196@mail.ru>
 *                         <alexrainpe196@hotbox.ru>

 * This file is proted from the Contiki operating system.
 *      Author: Adam Dunkels <adam@sics.se>
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

/**
 * \addtogroup etimer
 * @{
 */

/**
 * \file
 * Event timer library implementation.
 * \author
 * Adam Dunkels <adam@sics.se>
 */

#include <uos-conf.h>
#include <runtime/arch.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <stddef.h>

#ifdef USER_TIMERS

#include <timer/etimer.h>

etimer_device system_etimer = {NULL};

#define DEBUG_ETIMER 1

#if DEBUG_ETIMER
#include <runtime/lib.h>
#define ETIMER_printf(...) debug_printf(__VA_ARGS__)
#define ETIMER_putchar(x)  debug_putchar(0, x)
#define ETIMER_puts(x)     debug_puts(x)
#define ETIMER_print_tasks(x)
void tasks_list(list_t* x);
#else
#define ETIMER_printf(...)
#define ETIMER_putchar(x)
#define ETIMER_puts(x)
#define ETIMER_print_tasks(x)
#endif

bool_t ETimer_ISR (void* data);

void etimer_system_init(timer_t* source_clock){
    etimer_device* self = &system_etimer;
    self->clock = source_clock;

    list_init(&self->timerlist);
    /*etimer *t;
    list_iterate(t, &self->timerlist){
        list_unlink(t);
    }
    */
    user_timer_t* timer = &self->os_timer;
    mutex_lock_swi(&(timer->lock), &(self->timer_irq), &(ETimer_ISR), NULL);
    user_timer_init(timer, 0);
    mutex_unlock(&(timer->lock));
    user_timer_add(source_clock, timer);
}


inline etimer_device* etimer_dev_of(etimer *t){
    return &system_etimer;
}

/** \~russian
 * \value 0 - мутекс system_etimer.os_timer.lock - не получает событий от срабатывания таймера
 *   иначе, этот  мутекса получает сигналы на каждое событие поднятое таймером.
 * */
#define ETIMER_SIGNALING 1

bool_t
ETimer_ISR (void* data){
    etimer* event = (etimer*)data;
    etimer_device* self = etimer_dev_of(event);
    list_t* pended_timers = &(self->timerlist);
    user_timer_t* timer = &(self->os_timer);
#ifdef USEC_TIMER
    etimer_time_t timeover = timer->usec_per_tick - timer->cur_time;
#else
    etimer_time_t timeover = timer->msec_per_tick - timer->cur_time;
#endif

    do { 
        event->cur_time = -timeover;
        list_unlink(&event->item);
        if (event->lock != NULL){
            ETIMER_printf("\n!et(%x){+%d}\n", (unsigned)(event), timeover );
            mutex_awake(event->lock, event->handle.lock_message);
        }
        else {
            ETIMER_printf("\n!et(%x){+%d} go*->t(%x)\n"
                    , (unsigned)(event)
                    , timeover
                    , (unsigned)(event->handle.activate_task)
                    );
            task_activate(event->handle.activate_task);
        }
        ETIMER_print_tasks(&task_active);
    
        if (list_is_empty(pended_timers)) {
            timer->cur_time = 0;
            break;
        }
        etimer* newtime = (etimer*)list_first(pended_timers);
        ETIMER_printf("\n>et(%x)\n", (unsigned)(newtime) );
    
        //assign next event time to etimer expiration
        etimer_time_t elapsed = newtime->cur_time;
        if ( timeover < elapsed ) {
            timer->cur_time      = elapsed - timeover;
#ifdef USEC_TIMER
            timer->usec_per_tick = newtime->interval;
#else
            timer->msec_per_tick = newtime->interval;
#endif
            self->timer_irq.arg = (void*)newtime;
            break;
        }
        //else
        
            //новый таймер настолько близко что успел сработать, поэтому 
            // внесу поправку его таймаута и активирую его
            timeover -= elapsed;
            newtime->cur_time      = timeover;
            event = newtime;
            ETIMER_putchar('/');
    } while (!list_is_empty(pended_timers));

    return ETIMER_SIGNALING;
}

void tasks_list(list_t* l){
    task_t *t;
    ETIMER_puts("   t:");
    list_iterate (t, l){
        ETIMER_printf("-(%x)", (unsigned)(t) );
    }
    ETIMER_putchar('\n');
}

/*---------------------------------------------------------------------------*/
#define ETIMER_SOFT_LOCK 1

inline void etimer_seq_aqure(etimer_device* self){
    mutex_lock(&(self->list_access));
}

inline void etimer_seq_release(etimer_device* self){
    mutex_unlock(&(self->list_access));
}

static void
add_timer(etimer *timer)
{
    etimer_device* self = &system_etimer;
    etimer_time_t timeout = timer->interval + timer->cur_time;
    etimer *t;

    assert(self->clock != NULL);

    etimer_seq_aqure(self);
    mutex_t* clock_lock = &(self->os_timer.lock);
    mutex_lock(clock_lock);
        if (list_is_empty(&self->timerlist)){
            timer->cur_time = timeout;
            list_append(&self->timerlist, &timer->item);
            
            // требуется на него настроить текущее  событие
            // assign next event time expiration
            user_timer_t* enow = &(self->os_timer);
            enow->cur_time      = timeout;
#ifdef USEC_TIMER
            enow->usec_per_tick = timer->interval;
#else
            enow->msec_per_tick = timer->interval;
#endif
            self->timer_irq.arg = (void*)timer;
            mutex_unlock(clock_lock);
            ETIMER_printf("\n@et(%x).%d\n", (unsigned)(timer), timeout );
        }
        else {
            user_timer_t* enow = &(self->os_timer);
            t = (etimer*)list_first(&self->timerlist);
            etimer_time_t spent1 = enow->cur_time;
            if (spent1 > timeout){
                //новый таймер  - наиболее скорый, и его вставим в начало списка событий
                t->cur_time     = spent1 - timeout;
                timer->cur_time = timeout;
                list_prepend(&self->timerlist, &timer->item);
    
                // требуется на него настроить текущее  событие
                // assign next event time expiration
                enow->cur_time      = timeout;
#ifdef USEC_TIMER
                enow->usec_per_tick = timer->interval;
#else
                enow->msec_per_tick = timer->interval;
#endif
                self->timer_irq.arg = (void*)timer;
                mutex_unlock(clock_lock);
                ETIMER_printf("\n{%d}^et(%x)->(%x).%d\n"
                        , timeout
                        , (unsigned)(timer)
                        , (unsigned)(t), t->cur_time
                        );
            }
            else {
                //внесу поправку на прошедшее время текущего таймера
                ETIMER_printf("\n{%d}(%x).%d ->", timeout, (unsigned)t, spent1);
                timeout -= spent1;

                etimer_time_t safe_timeout = t->cur_time;
#ifdef USEC_TIMER
                safe_timeout += self->clock->usec_per_tick*2;
#else
                safe_timeout += self->clock->msec_per_tick*2;
#endif
                list_iterate_from(t, t->item.next, &self->timerlist){
                    etimer_time_t elapsed = t->cur_time;
                    if (timeout < elapsed)
                        break;
                    timeout -= elapsed;
                    ETIMER_printf("(%x).%d ->", (unsigned)t, t->cur_time);
                    if (ETIMER_SOFT_LOCK)
                    if (safe_timeout > 0) {
                        safe_timeout -= elapsed;
                        if (safe_timeout <= 0)
                            mutex_unlock(clock_lock);
                    }
                }
                timer->cur_time = timeout;
                list_append(&t->item, &timer->item);//insert before t 
                if ( (&t->item) != (&self->timerlist) ) {
                    t->cur_time -= timeout;
                }
                if ( (safe_timeout > 0) || (!ETIMER_SOFT_LOCK) )
                    mutex_unlock(clock_lock);
                ETIMER_printf("+et(%x).%d\n"
                        , (unsigned)(timer), timer->cur_time
                        );
            }
        }// else if (list_empty(self->timerlist))
    etimer_seq_release(self);
}

/*---------------------------------------------------------------------------*/
void
etimer_set(etimer *et, clock_time_t interval)
{
    etimer_stop(et);
    et->interval = interval;
    et->cur_time = 0;
    add_timer(et);
}
/*---------------------------------------------------------------------------*/
void
etimer_reset(etimer *et)
{
    if (etimer_not_active(et)){
        add_timer(et);
    }
}
/**
 * \brief      Reset an event timer with a new interval.
 * \param et   A pointer to the event timer.
 * \param interval The interval before the timer expires.
 *
 *             This function very similar to etimer_reset. Opposed to
 *             etimer_reset it is possible to change the timout.
 *             This allows accurate, non-periodic timers without drift.
 *
 * \sa etimer_reset()
 */
void etimer_reset_with_new_interval(etimer *et, etimer_time_t interval){
    if (etimer_not_active(et)){
        et->interval = interval;
        add_timer(et);
    }
    else{
        long diff = interval - et->interval;
        etimer_time_t to = etimer_expiration_time(et);
        etimer_stop(et);
        et->interval = interval;
        et->cur_time = to + diff;
        add_timer(et);
    }
}

/*---------------------------------------------------------------------------*/
void
etimer_restart(etimer *et)
{
    etimer_stop(et);
    et->cur_time = 0;
    add_timer(et);
}

/*---------------------------------------------------------------------------*/
inline bool_t etime_is_over(etimer_time_t t){
    return t > (TIME_INFINITE>>1);
}

/*---------------------------------------------------------------------------*/
etimer_time_t
etimer_expiration_time(etimer *et)
{
    etimer_device* self = etimer_dev_of(et);
    etimer *t = NULL;
    etimer_time_t elapse = 0;

    assert(self->clock != NULL);

    etimer_seq_aqure(self);
    mutex_t* clock_lock = &(self->os_timer.lock);
    mutex_lock(clock_lock);
#ifdef USEC_TIMER
        etimer_time_t safe_time = self->clock->usec_per_tick*2;
#else
        etimer_time_t safe_time = self->clock->msec_per_tick;
        //if (safe_time > 2) safe_time = 2;
#endif

        list_iterate(t, &(self->timerlist) ){
            etimer_time_t elapsed = t->cur_time;
            elapse += elapsed;
            if (t == et)
                break;

            if (ETIMER_SOFT_LOCK)
            if (safe_time > 0) {
                safe_time -= elapsed;
                if (safe_time <= 0)
                    mutex_unlock(clock_lock);
            }
        }
        if (t != et)
            elapse = 0;
    if ((safe_time > 0) || (!ETIMER_SOFT_LOCK))
        mutex_unlock(clock_lock);
    etimer_seq_release(self);
    return elapse;
}
/*---------------------------------------------------------------------------*/
clock_time_t
etimer_start_time(etimer *et)
{
  return etimer_expiration_time(et) - et->interval;
}
/*---------------------------------------------------------------------------*/
void
etimer_stop(etimer *et)
{
    if (!etimer_is_wait(et))
        return;

    etimer_device* self = etimer_dev_of(et);
    etimer *t;
    mutex_t* clock_lock = &(self->os_timer.lock);

    assert(self->clock != NULL);

    etimer_seq_aqure(self);
    mutex_lock(clock_lock);
        t = (etimer*)list_first(&self->timerlist);
        if (et == t){
            //надо удалить текущий таймер
            user_timer_t* enow = &(self->os_timer);
            et->cur_time = enow->cur_time;
            list_unlink(&t->item);

            if (list_is_empty(&self->timerlist)) {
                user_timer_stop(enow);
            }
            else {
                t = (etimer*)list_first(&self->timerlist);
                // требуется на него настроить текущее  событие
                // assign next event time expiration
                enow->cur_time      += t->cur_time;
#ifdef USEC_TIMER
                enow->usec_per_tick = t->interval;
#else
                enow->msec_per_tick = t->interval;
#endif
                self->timer_irq.arg = (void*)t;
            }
        }
        else {
            //!!! стоилобы проверить что требуемый таймер именно в списке своих
            //  событий self->timerlist
            t = (etimer*)et->item.next;
            list_unlink(&et->item);
            if ( &(t->item) != &self->timerlist )
                t->cur_time += et->cur_time;
        }// else if (list_empty(self->timerlist))
    mutex_unlock(clock_lock);
    etimer_seq_release(self);
}
/*---------------------------------------------------------------------------*/
/** @} */

#endif// USER_TIMERS

