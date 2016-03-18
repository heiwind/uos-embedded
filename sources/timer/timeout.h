#ifndef _UOS_TIMEOUT_H_
#define _UOS_TIMEOUT_H_

/*  UTF8 ru  */

/**\~russian
 * !!! чтобы функционал таймаутов был активен на объектах таймеров надо его подключить макро TIMER_TIMEOUTS
 * см. <timer/timer.h>
 * */

#include <kernel/uos.h>
#include <timer/timer.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef struct _timeout_t timeout_t;
typedef void (* timeout_handler)(timeout_t *to, void *arg);
typedef unsigned long timeout_time_t;


struct _timeout_t
{
    list_t item;
    
    timer_t *timer;
    
    mutex_t *mutex;
    void *signal;
    
    timeout_handler handler;
    void *handler_arg;
    
    timeout_time_t interval;
    volatile long cur_time;
    
    int autoreload;
};

void timeout_clear(timeout_t *to);

#ifdef TIMER_TIMEOUTS

/**\~russian
 * создает чистый таймаут и сразу подключает его к таймеру timer
 * */
void timeout_init (timeout_t *to, timer_t *timer, mutex_t *mutex);

/**\~russian
 * подключает/отключает таймаут к таймеру
 * */
void timeout_add (timer_t *t,       timeout_t *ut);
void timeout_remove (timeout_t *ut);

void timeout_start (timeout_t *to);

INLINE
void timeout_stop (timeout_t *to)
{
    timeout_remove(to);
}

#endif //TIMER_TIMEOUTS

INLINE
void timeout_wait (timeout_t *ut)
{
    mutex_wait (ut->mutex);
}

/**\~russian
 * перезапускает периодический таймер с текущего момента.
 * этот функционал не подключает таймаут к таймеру, он не расчитан на работу с режимом tsLoadRemove
 * */
INLINE
void timeout_restart (timeout_t *to)
{
    while (to->cur_time != (long)to->interval)
        to->cur_time = (long)to->interval;
}

/**\~russian
 * останавливает активность таймера.
 * этот функционал не отключает таймаут от таймера.
 * в режимом tsLoadRemove таймер отключает прерваный таймаут в ближайший отсчет.
 * */
INLINE
void timeout_break (timeout_t *to)
{
    while (to->cur_time != 0)
        to->cur_time = 0;
}

INLINE
void timeout_set_signal (timeout_t *to, void *signal){
    to->signal = signal;
}

INLINE
void timeout_set_mutex (timeout_t *to, mutex_t *mutex, void *signal){
    to->mutex = mutex;
    to->signal = signal;
}

enum timeout_atoreload_style{
    //* stops timeout after arming, but leave it in timer check-list with inactive state
     tsLoadOnce      = -1
        //*remove timeout from timer check-list after arming
        //*this is default timeout behaviour
    , tsLoadRemove    = 0
    //* automaticaly restart timeout after arming
    , tsLoadAuto      = 1
};

INLINE
void timeout_set_autoreload (timeout_t *to, int autoreload) {
    to->autoreload = autoreload;
}

INLINE
void timeout_set_handler (timeout_t *to, timeout_handler handler, void *arg) {
    to->handler = handler;
    to->handler_arg = arg;
}

#ifdef USEC_TIMER
#define TIMEOUT_TIMER_MS   (1000UL)

INLINE
void timeout_set_value_us (timeout_t *to, unsigned long interval_usec){
    to->interval = interval_usec;
}

INLINE
void timeout_set_value (timeout_t *to, unsigned long interval_msec){
    timeout_set_value_us(to, interval_msec*TIMEOUT_TIMER_MS);
}

/**\~russian
 * перезапускает периодический таймер с момента предыдущего события.
 * xsec_interval - новый период событий таймера
 * вызов   user_timer_restart_interval(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void timeout_restart_interval_us (timeout_t *ut, timeout_time_t interval_us);
INLINE
void timeout_restart_interval (timeout_t *to, unsigned long interval_msec){
    timeout_restart_interval_us(to, interval_msec*TIMEOUT_TIMER_MS);
}

/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void timeout_arm_us  (timeout_t *ut, timeout_time_t interval_us);
INLINE
void timeout_arm (timeout_t *to, unsigned long interval_msec){
    timeout_arm_us(to, interval_msec*TIMEOUT_TIMER_MS);
}

//* тоже user_timer_arm_us, но не защищен от конкурентности,
//  можно вызывать из прерываиий, если заведомо нет конкуренции
INLINE
void timeout_arm_us_nomt (timeout_t *ut, timeout_time_t interval){
    ut->interval = interval;
    ut->cur_time = interval;
    //ut->autoreload = tsLoadOnce;
}
INLINE
void timeout_arm_nomt (timeout_t *to, unsigned long interval_msec){
    timeout_arm_us_nomt(to, interval_msec*TIMEOUT_TIMER_MS);
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается
 * */
bool_t timeout_rearm_us  (timeout_t *ut, timeout_time_t interval);
INLINE
bool_t timeout_rearm (timeout_t *to, unsigned long interval_msec){
    return timeout_rearm_us(to, interval_msec*TIMEOUT_TIMER_MS);
}

INLINE
bool_t timeout_rearm_us_nomt (timeout_t *ut, timeout_time_t interval){
    ut->interval = interval;
    ut->cur_time += interval;
    //ut->autoreload = tsLoadOnce;
    return ut->cur_time <=0;
}
INLINE
bool_t timeout_rearm_nomt (timeout_t *to, unsigned long interval_msec){
    return timeout_rearm_us_nomt(to, interval_msec*TIMEOUT_TIMER_MS);
}

#else //USEC_TIMER
#define TIMEOUT_TIMER_MS   (1UL)

INLINE
void timeout_set_value (timeout_t *to, unsigned long interval_msec){
    to->interval = interval_msec;
}

/**\~russian
 * перезапускает периодический таймер с момента предыдущего события.
 * xsec_interval - новый период событий таймера
 * вызов   user_timer_restart_interval(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void timeout_restart_interval (timeout_t *ut, timeout_time_t interval_ms);

/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void timeout_arm  (timeout_t *ut, timeout_time_t interval_ms);

//* тоже user_timer_arm_us, но не защищен от конкурентности,
//  можно вызывать из прерываиий, если заведомо нет конкуренции
INLINE
void timeout_arm_nomt (timeout_t *ut, timeout_time_t interval){
    ut->cur_time = interval;
    ut->autoreload = 0;
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается
 * */
bool_t timeout_rearm  (timeout_t *ut, timeout_time_t interval);

INLINE
bool_t timeout_rearm_nomt (timeout_t *ut, timeout_time_t interval){
    ut->cur_time += interval;
    ut->autoreload = 0;
    return ut->cur_time <=0;
}

#endif //USEC_TIMER



/**\~russian
 * true - если таймер остановлен.
 * !!!не действителен для периодического таймера, работает только с одноразовыми.
 * */
INLINE
bool_t timeout_expired (const timeout_t *ut)
{
    return (ut->cur_time <= 0);
}

/**\~russian
 * возвращает интервал времени до следующего события таймера
 * */
INLINE
timeout_time_t timeout_expiration_timeout(const timeout_t *ut)
{
    return ut->cur_time;
}



#ifdef __cplusplus
}
#endif

#endif //_UOS_TIMEOUT_H_
