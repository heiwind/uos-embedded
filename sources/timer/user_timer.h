#ifndef _UOS_USERTIMER_H_
#define _UOS_USERTIMER_H_

/*  UTF8 ru
 * DO NOT USE THIS HEADER!. this header used by timer/timer.h internaly
 * */
/**\~russian
 * пользовательский таймер.
 * он тикает от системного таймера, если его период не кратен системному, то время
 *  тика выставляется с погрешностью  - с шагом системного таймера в наиближайшей
 *  точке к пользовательскому интервалу.
 * в отличие от системного таймера - пользовательский таймер может останавливаться
 *
 * deprecated - now use <timer/timeouts.h>
 * */

#include <kernel/uos.h>
#include <timer/timeout.h>

#ifndef __deprecated
#define __deprecated
#endif

#ifndef INLINE
#define INLINE static inline
#endif

#ifdef __cplusplus
extern "C" {
#endif


struct _user_timer_t {
    timeout_t   to;
    mutex_t     lock;
};

typedef struct _user_timer_t  user_timer_t;
typedef timeout_time_t        usertimer_time_t;



#ifdef USEC_TIMER

#define USER_TIMER_MS   (1000UL)

__deprecated
void user_timer_init_us (user_timer_t *ut, usertimer_time_t usec_per_tick);

__deprecated
void user_timer_init (user_timer_t *ut, usertimer_time_t msec_per_tick);


__deprecated
INLINE
void user_timer_set_us  (user_timer_t *ut, usertimer_time_t usec_interval){
    timeout_t* to = &(ut->to);
    timeout_set_autoreload(to, tsLoadAuto);
    timeout_set_value_us(to, usec_interval);
    timeout_restart(to);
}


/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
__deprecated
INLINE
void user_timer_arm_us  (user_timer_t *ut, usertimer_time_t usec_interval){
    timeout_arm_us(&(ut->to), usec_interval);
}

//* тоже user_timer_arm_us, но не защищен от конкурентности,
//  можно вызывать из прерываиий, если заведомо нет конкуренции
__deprecated
INLINE
void user_timer_arm_us_nomt (user_timer_t *ut, usertimer_time_t usec_interval){
    timeout_arm_us_nomt(&(ut->to), usec_interval);
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается
 * */
__deprecated
INLINE
bool_t user_timer_rearm_us  (user_timer_t *ut, usertimer_time_t usec_interval){
    return timeout_rearm_us(&(ut->to), usec_interval);
}

__deprecated
INLINE
bool_t user_timer_rearm_us_nomt (user_timer_t *ut, usertimer_time_t usec_interval){
    return timeout_rearm_us_nomt(&(ut->to), usec_interval);
}


__deprecated
INLINE
void user_timer_restart_interval_us (user_timer_t *ut, usertimer_time_t usec_interval){
    timeout_restart_interval_us(&(ut->to), usec_interval);
}

#else
#define USER_TIMER_MS   1ul

__deprecated
void user_timer_init (user_timer_t *ut, usertimer_time_t msec_per_tick);
#endif  //USEC_TIMER

/**\~russian
 * запускает периодический таймер с текущего момента.
 * xsec_interval - период событий таймера
 * вызов   user_timer_set(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
__deprecated
INLINE
void user_timer_set  (user_timer_t *ut, usertimer_time_t msec_interval){
    timeout_t* to = &(ut->to);
    timeout_set_autoreload(to, tsLoadAuto);
    timeout_set_value(to, msec_interval);
    timeout_restart(to);
}

/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
__deprecated
INLINE
void user_timer_arm  (user_timer_t *ut, usertimer_time_t msec_interval){
    timeout_arm(&(ut->to), msec_interval);
}

//* тоже user_timer_arm_us, но не защищен от конкурентности,
//  можно вызывать из прерываиий, если заведомо нет конкуренции
__deprecated
INLINE
void user_timer_arm_nomt (user_timer_t *ut, usertimer_time_t msec_time){
    timeout_arm_nomt(&(ut->to), msec_time);
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается
 * */
__deprecated
INLINE
bool_t user_timer_rearm  (user_timer_t *ut, usertimer_time_t msec_interval){
    return timeout_rearm(&(ut->to), msec_interval);
}

__deprecated
INLINE
bool_t user_timer_rearm_nomt (user_timer_t *ut, usertimer_time_t msec_time){
    return timeout_rearm_nomt(&(ut->to), msec_time);
}

/**\~russian
 * перезапускает периодический таймер с момента предыдущего события.
 * xsec_interval - новый период событий таймера
 * вызов   user_timer_restart_interval(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
__deprecated
INLINE
void user_timer_restart_interval (user_timer_t *ut, usertimer_time_t msec_interval){
    timeout_restart_interval(&(ut->to), msec_interval);
}

__deprecated
INLINE
void user_timer_add (timer_t *t, user_timer_t *ut){
    timeout_add(t, &(ut->to));
}

__deprecated
INLINE
void user_timer_remove (timer_t *t, user_timer_t *ut){
    timeout_remove(&(ut->to));
}

__deprecated
INLINE
void user_timer_wait (user_timer_t *ut)
{
    mutex_wait (&ut->lock);
}

/**\~russian
 * останавливает активность таймера.
 * при этом теряются его параметры - точка старта, период
 * */
__deprecated
INLINE
void user_timer_stop (user_timer_t *ut){
    timeout_break(&(ut->to));
}

//void user_timer_reset   (user_timer_t *ut);

/**\~russian
 * перезапускает периодический таймер с текущего момента.
 * */
__deprecated
INLINE
void user_timer_restart (user_timer_t *ut)
{
    timeout_restart(&(ut->to));
}

/**\~russian
 * true - если таймер остановлен.
 * !!!не действителен для периодического таймера, работает только с одноразовыми.
 * */
__deprecated
INLINE
bool_t user_timer_expired (user_timer_t *ut)
{
    return timeout_expired(&(ut->to));
}

/**\~russian
 * возвращает время следующего события таймера
 * */
//usertimer_time_t user_timer_expiration_time(user_timer_t *ut)

/**\~russian
 * возвращает интервал времени до следующего события таймера
 * */
__deprecated
INLINE
usertimer_time_t user_timer_expiration_timeout(user_timer_t *ut)
{
    return timeout_expiration_timeout(&(ut->to));
}



#ifdef __cplusplus
}
#endif

#endif //_UOS_USERTIMER_H_
