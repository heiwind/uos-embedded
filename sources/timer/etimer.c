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
#include <runtime/lib.h>
#include <runtime/arch.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <stddef.h>

#ifdef TIMER_TIMEOUTS

#include <timer/etimer.h>

etimer_device system_etimer = {NULL};

//#define DEBUG_ETIMER 1
#ifndef ETIMER_SAFE_MEM
#define ETIMER_SAFE_ARGS    1
#define ETIMER_SAFE_BASE    2
#define ETIMER_SAFE_MEM     4
#endif

#ifndef ETIMER_SAFE
#define ETIMER_SAFE         0
#endif

#define ET_STRICT( level, body ) if ((ETIMER_SAFE & ETIMER_SAFE_##level) != 0) body



#if DEBUG_ETIMER
#include <runtime/lib.h>
#ifdef ETIMER_PRINTF
#define ETIMER_printf(...) ETIMER_PRINTF(__VA_ARGS__)
#define ETIMER_putchar(x)  ETIMER_PRINTF(x)
#define ETIMER_puts(x)     ETIMER_PRINTF(x)
#else
#define ETIMER_printf(...) debug_printf(__VA_ARGS__)
#define ETIMER_putchar(x)  debug_putchar(0, x)
#define ETIMER_puts(x)     debug_puts(x)
#endif
#define ETIMER_print_tasks(x)
void tasks_list(list_t* x);
#else
#define ETIMER_printf(...)
#define ETIMER_putchar(x)
#define ETIMER_puts(x)
#define ETIMER_print_tasks(x)
#endif

bool_t ETimer_ISR (void* data);
void ETimer_Handle (timeout_t *to, void *arg);

void etimer_system_init(timer_t* source_clock){
    etimer_device* self = &system_etimer;
    self->clock = source_clock;
    ET_STRICT(BASE, ) assert2( (source_clock->lock.item.next != 0)
                        , "etimer: init from uninitialised clock are danger on timerlist operations!\n"
                        , 0);

    list_init(&self->timerlist);
    /*etimer *t;
    list_iterate(t, &self->timerlist){
        list_unlink(t);
    }
    */
    timeout_t* timer = &self->os_timer;
    timeout_clear(timer);
    mutex_attach_swi(&(self->os_timer_signal), &(self->os_timer_isr), ETimer_ISR, 0);
    timeout_set_mutex(timer, &(self->os_timer_signal), self);
    //timeout_set_handler(timer, &(ETimer_Handle), 0);
    timeout_set_autoreload(timer, tsLoadOnce);
    timeout_add(source_clock, timer);
}


inline etimer_device* etimer_dev_of(etimer *t){
    return &system_etimer;
}

inline
void signal_new_etime(etimer_device* self
        , long  timeout, timeout_time_t interval
        , etimer* event)
{
    timeout_t *now = &self->os_timer;
    now->cur_time   = timeout;
    now->interval   = interval;
    now->signal     = event;
    self->os_timer_isr.arg = event;
}

/** \~russian
 * \value 0 - мутекс system_etimer.os_timer.lock - не получает событий от срабатывания таймера
 *   иначе, этот  мутекса получает сигналы на каждое событие поднятое таймером.
 * */
#define ETIMER_SIGNALING 1

bool_t
ETimer_ISR (void* data){
    ETimer_Handle (0, data);
    return ETIMER_SIGNALING;
}

void ETimer_Handle (timeout_t *to, void *data){

    etimer* event = (etimer*)data;
    etimer_device* self = etimer_dev_of(event);
    if (self == 0)
        return;
    ET_STRICT(BASE, assert(self->os_timer.timer == self->clock));
    if (self->os_timer.timer != self->clock)
        return;

    list_t* pended_timers = &(self->timerlist);
    timeout_t *timer      = &(self->os_timer);
    /*etimer_time_t*/long timeover = -timer->cur_time;

    if (timer->cur_time > 0)
        return;

    do { 
        ET_STRICT(MEM, ) assert2(uos_valid_memory_address(event)
                                , "etimer:ISR: bad event $%x\n", event
                                );
        event->cur_time = -timeover;
        ET_STRICT(BASE,) {
        assert( event->item.prev == pended_timers);
        assert( pended_timers->next == &event->item);
        assert2( &event->item == event->item.next->prev,
                "etimer($%x)->($%x.<-$%x):"
                , event, event->item.next, event->item.next->prev);
        }
        list_unlink(&event->item);
        if (event->lock != NULL){
            ETIMER_printf("\n!et(%x){+%d}", (unsigned)(event), timeover );
            mutex_awake(event->lock, event->handle.lock_message);
        }
        else {
            ETIMER_printf("\n!et(%x){+%d} go*->t(%x:%s)"
                    , (unsigned)(event)
                    , timeover
                    , (unsigned)(event->handle.activate_task), task_name(event->handle.activate_task)
                    );
            task_awake(event->handle.activate_task);
        }
        //ETIMER_print_tasks(&task_active);
    
        if (list_is_empty(pended_timers)) {
            timer->cur_time = 0;
            break;
        }
        etimer* newtime = (etimer*)list_first(pended_timers);
        ETIMER_printf("\n>et(%x)\n", (unsigned)(newtime) );
        ET_STRICT(MEM, ) assert2(uos_valid_memory_address(newtime)
                                , "etimer:ISR: bad next event $%x\n", newtime
                                );
    
        //assign next event time to etimer expiration
        etimer_time_t elapsed = newtime->cur_time;
        if ( timeover < elapsed ) {
            signal_new_etime(self, (elapsed - timeover), newtime->interval, newtime);
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

    ET_STRICT(BASE, assert(self->os_timer.timer == self->clock));
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
// TODO нужно углубленое тестирование этой фичи - на предмет конфликта с прерыванием
//      или конкуренции
#ifndef ETIMER_SOFT_LOCK
#define ETIMER_SOFT_LOCK 0
#endif

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
    etimer *t;

    ET_STRICT(ARGS, assert(timer != 0));
    ET_STRICT(ARGS, assert(timer->item.next != 0) );
    ET_STRICT(BASE, assert(self->clock != NULL) );
    ET_STRICT(BASE, assert(self->os_timer.timer == self->clock) );
    ET_STRICT(MEM, ) assert2(uos_valid_memory_address(timer)
                            , "etimer:start: bad timer $%x\n", timer
                            );
    //* not initialised timer
    if (timer->item.next == 0)
        return;

#ifdef USEC_TIMER
    if (timer->interval <= self->clock->usec_per_tick)
#else
    if (timer->interval <= self->clock->msec_per_tick)
#endif
    {
        // wakeup timer imidiately
        timer->cur_time = 0;
        if (timer->lock != NULL){
            ETIMER_printf("\n!et(%x){0} ", (unsigned)(timer) );
            mutex_awake(timer->lock, timer->handle.lock_message);
        }
        return;
    }

    etimer_seq_aqure(self);
    mutex_t* clock_lock = self->os_timer.mutex;
    mutex_lock(clock_lock);
    while (1) {
        etimer_time_t timeout = timer->interval + timer->cur_time;
        if (list_is_empty(&self->timerlist)){
            timer->cur_time = timeout;
            list_append(&self->timerlist, &timer->item);
            
            // требуется на него настроить текущее  событие
            // assign next event time expiration
            signal_new_etime(self, timeout, timer->interval, timer);
            if (list_is_empty(&self->os_timer.item))
                list_append (&self->clock->timeouts, &self->os_timer.item);
            mutex_unlock(clock_lock);
            ETIMER_printf("\n@et(%x:%s).%d\n", (unsigned)(timer), task_name(task_current), timeout );
        }
        else {
            etimer_event_t* enow = &(self->os_timer);
            t = (etimer*)list_first(&self->timerlist);
            ET_STRICT(MEM, ) assert2(uos_valid_memory_address(t)
                                    , "etimer:start: bad 1st timer $%x\n", t
                                    );
            etimer_time_t spent1 = enow->cur_time;
            assert2(!list_is_empty(&t->item), "et(%x).%d", t, t->cur_time);
            if (spent1 > timeout){
                //новый таймер  - наиболее скорый, и его вставим в начало списка событий
                t->cur_time     = spent1 - timeout;
                timer->cur_time = timeout;
                list_prepend(&self->timerlist, &timer->item);
    
                // требуется на него настроить текущее  событие
                // assign next event time expiration
                signal_new_etime(self, timeout, timer->interval, timer);
                mutex_unlock(clock_lock);
                ETIMER_printf("\n{%d}^et(%x:%s)->(%x).%d\n"
                        , timeout
                        , (unsigned)(timer), task_name(task_current)
                        , (unsigned)(t), t->cur_time
                        );
            }
            else {
                //внесу поправку на прошедшее время текущего таймера
                ETIMER_printf("\n{%d}(%x).%d->", timeout, (unsigned)t, spent1);
                timeout -= spent1;

                etimer_time_t safe_timeout = t->cur_time;
#ifdef USEC_TIMER
                safe_timeout += self->clock->usec_per_tick*2;
#else
                safe_timeout += self->clock->msec_per_tick*2;
#endif
                list_iterate_from(t, t->item.next, &self->timerlist){
                    ET_STRICT(BASE,)
                        assert2( (t != timer)
                                , "etimer: try insert alredy active etimer et(%x)\n", timer);
                    ET_STRICT(MEM, ) assert2(uos_valid_memory_address(t)
                                        , "etimer:start: bad timer $%x\n", t
                                        );
                    ET_STRICT(BASE,)
                        assert2(!list_is_empty(&t->item)
                                    , "etimer: broken chain at et(%x).%d\n", t, t->cur_time);
                    if (list_is_empty(&t->item)){
                        //this is impossible - chain is broken.
                        halt(1);
                    }
                    ET_STRICT(BASE,) assert2( (void*)t == t->item.prev->next
                                    , "(%x.->%x)<-et(%x)"
                                    , t->item.prev, t->item.prev->next, t);
                    if ((void*)t != t->item.prev->next){
                        //this is impossible/unsync collision. try again on it
                        t = 0;
                        break;
                    }
                    etimer_time_t elapsed = t->cur_time;
                    if (timeout < elapsed)
                        break;
                    timeout -= elapsed;
                    ETIMER_printf("(%x).%d->", (unsigned)t, t->cur_time);
                    if (ETIMER_SOFT_LOCK)
                    if (safe_timeout > 0) {
                        if (safe_timeout >= elapsed)
                            safe_timeout -= elapsed;
                        else {
                            safe_timeout = 0;
                            mutex_unlock(clock_lock);
                        }
                    }
                }
                if (t == 0)
                    continue;
                if (list_is_empty(&t->item))    //this is impossible/unsync collision. try again on it
                    continue;
                timer->cur_time = timeout;
                list_append(&t->item, &timer->item);//insert before t 
                if ( (&t->item) != (&self->timerlist) ) {
                    t->cur_time -= timeout;
                }
                if ( (safe_timeout > 0) || (!ETIMER_SOFT_LOCK) )
                    mutex_unlock(clock_lock);
                if ( (&t->item) != (&self->timerlist) )
                    ETIMER_printf("+et(%x:%s).%d->(%x).%d\n"
                            , (unsigned)(timer), task_name(task_current)
                            , timeout, t, t->cur_time
                            );
                else
                ETIMER_printf("+et(%x:%s).%d\n"
                        , (unsigned)(timer), task_name(task_current)
                        , timeout
                        );
            }
        }// else if (list_empty(self->timerlist))
        break;  //normal finish
    } // while (true) - cycle attempts on collision
    ET_STRICT(BASE, assert(self->os_timer.timer == self->clock) );
    etimer_seq_release(self);
}

/*---------------------------------------------------------------------------*/
void
etimer_set(etimer *et, etimer_time_t interval)
{
    etimer_stop(et);
    if (et->item.next == 0)
        etimer_init(et);
    et->interval = interval;
    et->cur_time = 0;
    add_timer(et);
}
/*---------------------------------------------------------------------------*/
void
etimer_reset(etimer *et)
{
    ET_STRICT(ARGS, assert(et != 0));
    if (et->item.next == 0)
        etimer_init(et);
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
    ET_STRICT(ARGS, assert(et != 0));
    if (et->item.next == 0)
        etimer_init(et);
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
    ET_STRICT(ARGS, assert(et != 0));
    ET_STRICT(ARGS, assert(et->item.next != 0) ); //* not initialised timer
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
    ET_STRICT(ARGS, assert(et != 0));
    ET_STRICT(ARGS, assert(et->item.next != 0) ); //* not initialised timer
    etimer_device* self = etimer_dev_of(et);
    etimer *t = NULL;
    etimer_time_t elapse = 0;

    assert(self->clock != NULL);
    assert(self->os_timer.timer == self->clock);

    etimer_seq_aqure(self);
    mutex_t* clock_lock = self->os_timer.mutex;
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
etimer_time_t
etimer_start_time(etimer *et)
{
  return etimer_expiration_time(et) - et->interval;
}
/*---------------------------------------------------------------------------*/
void
etimer_stop(etimer *et)
{
    ET_STRICT(ARGS, assert(et != 0));
    if (et->item.next == 0)
        return;
    if (etimer_not_active(et))
        return;

    etimer_device* self = etimer_dev_of(et);
    etimer *t;
    mutex_t* clock_lock = self->os_timer.mutex;

    assert(self->clock != NULL);
    ET_STRICT(BASE, assert(self->os_timer.timer == self->clock));

    etimer_seq_aqure(self);
    mutex_lock(clock_lock);
    if (!etimer_not_active(et)){
        t = (etimer*)list_first(&self->timerlist);
        ET_STRICT(BASE, assert(t != 0) );
        ET_STRICT(MEM, ) assert2(uos_valid_memory_address(t)
                                , "etimer:stop: bad timeout $%x\n", t
                                );
        if (et == t){
            ETIMER_printf("#et(%x:%s)\n", (unsigned)(et), task_name(task_current) );
            //надо удалить текущий таймер
            etimer_event_t* enow = &(self->os_timer);
            et->cur_time = enow->cur_time;
            list_unlink(&t->item);

            if (list_is_empty(&self->timerlist)) {
                timeout_break(enow);//stop
            }
            else {
                t = (etimer*)list_first(&self->timerlist);
                // требуется на него настроить текущее  событие
                // assign next event time expiration
                signal_new_etime(self, (enow->cur_time+t->cur_time),  t->interval, t);
            }
        }
        else {
            //!!! стоилобы проверить что требуемый таймер именно в списке своих
            //  событий self->timerlist
            ETIMER_printf("#et(%x:%s).%d\n", (unsigned)(et), task_name(task_current), et->cur_time);
            t = (etimer*)et->item.next;
            list_unlink(&et->item);
            if ( &(t->item) != &self->timerlist )
                t->cur_time += et->cur_time;
        }// else if (list_empty(self->timerlist))
    }//if (!etimer_not_active(et))
    mutex_unlock(clock_lock);
    ET_STRICT(BASE, assert(self->os_timer.timer == self->clock));
    etimer_seq_release(self);
}
/*---------------------------------------------------------------------------*/
/** @} */

//* prints debug dump of etimer que
void etimer_dump(stream_t* io){
    etimer_device* self = etimer_dev_of(0);
    if (io == 0)
        io = &debug;
    printf(io, "etimer device($%x):clock $%x\n", self, self->clock);
    if (!list_contains(&self->clock->timeouts, &self->os_timer.item)){
        fputs("      etimer is not clocked!\n", io);
    }
    if (self->os_timer_isr.lock != &self->os_timer_signal){
        printf(io, "      etimer not signals: $%x expect $%x!\n"
                , self->os_timer_isr.lock, &self->os_timer_signal);
    }
    if (self->os_timer_isr.handler != ETimer_ISR){
        printf(io, "      etimer strange ISR: $%x expect $%x!\n"
                , self->os_timer_isr.handler, ETimer_ISR);
    }
    if (self->os_timer.mutex != &self->os_timer_signal){
        printf(io, "      etimer strange signaling mutex: $%x expect $%x!\n"
                , self->os_timer.mutex, &self->os_timer_signal);
    }
    mutex_print(io, &self->os_timer_signal);

    const etimer *et;
    list_iterate(et, &self->timerlist){
        if (!etimer_dump_event(io, et)){
            break;
        }
        list_t* prev = et->item.prev;
        if (uos_valid_memory_address(prev)){
            if (prev->next != &et->item){
                printf(io, "    broken list chain: prev->$%x\n", prev->next);
            }
        }
        else {
            printf(io, "    broken list chain: damaged prev = $%x\n", prev);
        }
        if (list_is_empty(et)){
            printf(io, "    broken list chain: event is empty\n");
            break;
        }
    }
}

bool_t etimer_dump_event(stream_t* io, const etimer *et){
    if (!uos_valid_memory_address(et)){
        printf(io, "event($%x): invalid!\n", et);
        return 0;
    }
    if (et->lock != NULL){
        printf(io, "event($%x): now %lu interval %lu:  mutex($%x) msg=$%x\n"
                , et
                , et->cur_time, et->interval
                , et->lock, et->handle.lock_message
                );
        mutex_print(io, et->lock);
    }
    else{
        printf(io, "event($%x): now %lu interval %lu:  task($%x)\n"
                , et
                , et->cur_time, et->interval
                , et->handle.activate_task
                );
        task_print(io, 0);
        task_print(io, et->handle.activate_task);
    }
    return 1;
}

#endif// USER_TIMERS

