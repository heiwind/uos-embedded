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
 * \file
 * Event timer header file.
 * \author
 * alexrayne <alexraynepe196@gmail.com>
 */

/** \addtogroup timer
 * @{ */

/**
 * \defgroup etimer Event timers
 *
 * Event timers provides a way to generate timed events. An event
 * timer will post an event to the process that set the timer when the
 * event timer expires.
 *
 * An event timer is declared as a \c struct \c etimer and all access
 * to the event timer is made by a pointer to the declared event
 * timer.
 *
 * \sa \ref timer "Simple timer library"
 * \sa \ref clock "Clock library" (used by the timer library)
 *
 * @{
 */

#ifndef UOS_ETIMER_H_
#define UOS_ETIMER_H_

#include <timer/timer.h>
#include <timer/timeout.h>
#include <stddef.h>

#ifndef TIMER_TIMEOUTS
#   error "etimer requres USER_TIMERS be allowed!"
#endif

#ifdef __cplusplus
extern "C" {
#endif



#define ETIMER_MS TIMEOUT_TIMER_MS

/**
 * A timer.
 *
 * This structure is used for declaring a timer. The timer must be set
 * with etimer_set() before it can be used.
 */
typedef timeout_time_t etimer_time_t;

typedef struct etimer_s {
    list_t  item;
    volatile long   cur_time;//least timeout
    etimer_time_t   interval;
    mutex_t*        lock;
    union {
        task_t*         activate_task;
        void*           lock_message;
    } handle;

#if ETIMER_SAFE > 0
#ifdef __cplusplus
    INLINE
    ~etimer_s();
#endif
#endif
} etimer;

INLINE bool_t etimer_not_active (const etimer *t){
    return list_is_empty(&(t->item));
}

INLINE etimer* etimer_assign_task(etimer *t, task_t* task){
    t->lock = NULL;
    t->handle.activate_task = task;
    return t;
}

INLINE etimer* etimer_assign_mutex(etimer *t, mutex_t* m, void* msg){
    t->lock = m;
    t->handle.lock_message = msg;
    return t;
}

INLINE void etimer_init (etimer *t){
    list_init(&t->item);
    t->lock = NULL;
    t->handle.activate_task = NULL;
}


typedef timeout_t etimer_event_t;
struct _etimer_device {
    timer_t*     clock;
    etimer_event_t  os_timer;
    mutex_t         os_timer_signal;
    mutex_irq_t     os_timer_isr;
    list_t       timerlist;
    mutex_t      list_access;
};
typedef struct _etimer_device etimer_device;

extern etimer_device system_etimer;

void etimer_system_init(timer_t* source_clock);

INLINE
bool_t etimer_system_online(){
    return (system_etimer.clock != 0);
}

/**
 * \name Functions called from application programs
 * @{
 */

/**
 * \brief      Set an event timer.
 * \param et   A pointer to the event timer
 * \param interval The interval before the timer expires.
 *
 *             This function is used to set an event timer for a time
 *             sometime in the future. When the event timer expires,
 *             the event PROCESS_EVENT_TIMER will be posted to the
 *             process that called the etimer_set() function.
 *
 */
void etimer_set(etimer *et, etimer_time_t interval);

/**
 * \brief      Reset an event timer with the same interval as was
 *             previously set.
 * \param et   A pointer to the event timer.
 *
 *             This function resets the event timer with the same
 *             interval that was given to the event timer with the
 *             etimer_set() function. The start point of the interval
 *             take into account time-drift of previous time-event expiration.
 *             Therefore, this function will cause the timer
 *             to be stable over time, unlike the etimer_restart()
 *             function.
 *             !!!But reset action must be executed right
 *             after timer event, within current clock cycle.
 *
 * \sa etimer_restart()
 */
void etimer_reset(etimer *et);

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
void etimer_reset_with_new_interval(etimer *et, etimer_time_t interval);

/**
 * \brief      Restart an event timer from the current point in time
 * \param et   A pointer to the event timer.
 *
 *             This function restarts the event timer with the same
 *             interval that was given to the etimer_set()
 *             function. The event timer will start at the current
 *             time.
 *
 *             \note A periodic timer will drift if this function is
 *             used to reset it. For periodic timers, use the
 *             etimer_reset() function instead.
 *
 * \sa etimer_reset()
 */
void etimer_restart(etimer *et);

/**
 * \brief      Get the expiration time for the event timer.
 * \param et   A pointer to the event timer
 * \return     The expiration time for the event timer.
 *
 *             This function returns the expiration time for an event timer.
 */
etimer_time_t etimer_expiration_time(etimer *et);

/**
 * \brief      Get the start time for the event timer.
 * \param et   A pointer to the event timer
 * \return     The start time for the event timer.
 *
 *             This function returns the start time (when the timer
 *             was last set) for an event timer.
 */
etimer_time_t etimer_start_time(etimer *et);

/**
 * \brief      Check if an event timer has expired.
 * \param et   A pointer to the event timer
 * \return     Non-zero if the timer has expired, zero otherwise.
 *
 *             This function tests if an event timer has expired and
 *             returns true or false depending on its status.
 */
INLINE 
int etimer_expired(etimer *et)
{
    return (etimer_not_active(et) && (et->cur_time < 0));
}

INLINE 
int etimer_is_wait(etimer *et)
{
    return (!etimer_not_active(et)); //  && (et->cur_time >= 0)
}

/**
 * \brief      Stop a pending event timer.
 * \param et   A pointer to the pending event timer.
 *
 *             This function stops an event timer that has previously
 *             been set with etimer_set() or etimer_reset(). After
 *             this function has been called, the event timer will not
 *             emit any event when it expires.
 *
 */
void etimer_stop(etimer *et);

/** @} */

/**
 * \name Functions called from timer interrupts, by the system
 * @{
 */

/**
 * \brief      Make the event timer aware that the clock has changed
 *
 *             This function is used to inform the event timer module
 *             that the system clock has been updated. Typically, this
 *             function would be called from the timer interrupt
 *             handler when the clock has ticked.
 */
INLINE
void etimer_request_poll(void) {;};

/**
 * \brief      Check if there are any non-expired event timers.
 * \return     True if there are active event timers, false if there are
 *             no active timers.
 *
 *             This function checks if there are any active event
 *             timers that have not expired.
 */
INLINE 
int etimer_pending(void)
{
    etimer_device* self = &system_etimer;
    return (!timeout_expired(&self->os_timer));
}

/**
 * \brief      Get next event timer expiration time.
 * \return     Next expiration time of all pending event timers.
 *             If there are no pending event timers this function
 *	       returns 0.
 *
 *             This functions returns next expiration time of all
 *             pending event timers.
 */
INLINE 
etimer_time_t etimer_next_expiration_time(void)
{
    etimer_device* self = &system_etimer;
    return timeout_expiration_timeout(&self->os_timer);
}


/** @} */



//* prints debug dump of etimer que
#include <stream/stream.h>
void etimer_dump(stream_t* io);
bool_t etimer_dump_event(stream_t* io, const etimer *et);

#ifdef __cplusplus
}

#if ETIMER_SAFE > 0
INLINE
etimer_s::~etimer_s()
{
    etimer_stop(this);
}
#endif // ETIMER_SAFE > 0

#endif

#endif /* UOS_ETIMER_H_ */
/** @} */
/** @} */
