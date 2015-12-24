#ifndef _TIMER_H_
#define _TIMER_H_

/*	UTF8 ru  */

#include <uos-conf.h>
#include <kernel/uos.h>

#ifdef __cplusplus
extern "C" {
#endif



/**\~english
 * Number of milliseconds per day.
 *
 * \~russian
 * Количество миллисекунд в дне.
 */
#define TIMER_MSEC_PER_DAY  (24UL*60*60*1000)
#define TIMER_USEC_PER_MSEC (1000UL)

/**\~russian
 * макро SW_TIMER отключает инициализацию обработчика прерывания системного таймера. в этом случае настраивает и запускает таймер
 * пользовательский код. обновление таймера производится вызовом timer_update
 * */
//#define SW_TIMER 1

/**\~russian
 * макро USER_TIMERS включает функционал множественных пользовательских таймеров на общем системном таймере.
 * см. функции user_timer_XXX
 * */
//#define USER_TIMERS 1

/**\~russian
 * макро USEC_TIMER дает возможность использовать прецизионный таймер сразрешением в [us]. см. timer_init_us
 * */
//#define USEC_TIMER 1

/**\~russian
 * макро TIMER_NO_DAYS опускает реализацию _timer_t.days - экономит время обработчика прерывания
 *  используется модулями: smnp
 * */
//#define TIMER_NO_DAYS 1 

/**\~russian
 * макро TIMER_NO_DECISEC опускает реализацию _timer_t.decisec - экономит время обработчика прерывания
 *  , это поле требуется для протокола TCP!!! 
 * */
//#define TIMER_NO_DECISEC 1 

/**\~russian
 * макро TIMER_DECISEC_MS задает период редкого таймера _timer_t.decisec
 * */
#ifndef TIMER_DECISEC_MS
#define TIMER_DECISEC_MS 100
#endif


typedef small_uint_t clock_time_t;
#ifdef UINT_MAX
#define TIME_MAX     UINT_MAX
#else
#define TIME_MAX     ((clock_time_t)(-1))
#endif

#define TIME_INFINITE (TIME_MAX>>1)

struct _timer_t {
    mutex_t lock;
#ifndef TIMER_NO_DECISEC
    mutex_t decisec;        /* every 0.1 second is signalled here */
    volatile unsigned long next_decisec; /* when next decisecond must be signalled */
#endif
    volatile clock_time_t tick;      //this is timer stamp increments on every tick
    unsigned long khz;      /* reference clock */

#ifdef USEC_TIMER
    unsigned long usec_per_tick;
    volatile unsigned long usec_in_msec;
    small_uint_t usec_per_tick_msprec;
#endif
    small_uint_t msec_per_tick;
    volatile unsigned long milliseconds; /* real time counter */

#if PIC32MX
    unsigned compare_step;
#endif

#ifndef TIMER_NO_DAYS
    volatile unsigned int days;      /* days counter */
#endif

#ifdef USER_TIMERS
    list_t user_timers;
#endif
};



#ifdef USER_TIMERS
typedef small_uint_t usertimer_time_t;

struct _user_timer_t {
    list_t item;
    mutex_t lock;
#ifdef USEC_TIMER
    usertimer_time_t usec_per_tick;
    volatile long int cur_time;
#else
    small_uint_t msec_per_tick;
    volatile long int cur_time;
#endif
};
#endif

/**\~english
 * Data structure of a timer driver.
 *
 * \~russian
 * Структура данных для драйвера таймера.
 */
typedef struct _timer_t timer_t;

typedef struct _user_timer_t user_timer_t;


#ifdef USEC_TIMER
// Микросекудный таймер. Поддерживается пока только для миландровских контроллеров
void timer_init_us (timer_t *t, unsigned long khz, unsigned long usec_per_tick);
INLINE 
void timer_init (timer_t *t, unsigned long khz, small_uint_t msec_per_tick){
    timer_init_us (t, khz, msec_per_tick*TIMER_USEC_PER_MSEC);
};
#else
void timer_init (timer_t *t, unsigned long khz, small_uint_t msec_per_tick);
#endif

#ifdef SW_TIMER
void timer_update (timer_t *t);
#endif

/* Delay current task. */
void timer_delay (timer_t *t, unsigned long msec);

/**\~english
 * Return the (real) time in milliseconds since uOS start.
 *
 * \~russian
 * Запрос времени в миллисекундах.
 */
INLINE unsigned long timer_milliseconds (timer_t *t){
    return t->milliseconds;
};

unsigned long timer_seconds (timer_t *t);

/**\~english
 * Check that `msec' milliseconds passed since the `t0' moment.
 *
 * \~russian
 * Проверка временного события.
 */
bool_t timer_passed (timer_t *t, unsigned long t0, unsigned int msec);

#ifndef TIMER_NO_DAYS
unsigned int timer_days (timer_t *t, unsigned long *milliseconds);
#endif

/**\~english
 * Check that `msec' milliseconds have passed.
 * `Interval' is the time interval, probably rolled over the day.
 *
 * \~russian
 * Проверка, прошло ли указанное количество миллисекунд `msec'.
 * Параметр `interval' содержит интервал времени, возможно, переходящий границу суток.
 */
bool_t interval_greater_or_equal (long interval, long msec);



void timer_delay_ticks (timer_t *t, clock_time_t ticks);

/**\~english
 * Return the (real) time in ticks since uOS start.
 *
 * \~russian
 * Запрос времени в тиках.
 */
INLINE unsigned long timer_tick(timer_t *t){
    return t->tick;
};

/**\~english
 * Check that `ticks' passed since the `t0' moment.
 *
 * \~russian
 * Проверка временного события.
 */
INLINE bool_t timer_passed_tick(timer_t *t, clock_time_t t0, clock_time_t ticks)
{
    unsigned long now = timer_tick(t);
    return (now - t0) > ticks;
}



#ifdef USER_TIMERS

/**\~russian
 * пользовательский таймер.
 * он тикает от системного таймера, если его период не кратен системному, то время
 *  тика выставляется с погрешностью  - с шагом системного таймера в наиближайшей 
 *  точке к пользовательскому интервалу.
 * в отличие от системного таймера - пользовательский таймер может останавливаться
 * */
#ifdef USEC_TIMER
void user_timer_init_us (user_timer_t *ut, unsigned long usec_per_tick);
void user_timer_set_us  (user_timer_t *ut, usertimer_time_t usec_interval);

/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_arm_us  (user_timer_t *ut, usertimer_time_t usec_time);

//* тоже user_timer_arm_us, но не защищен от конкурентности, 
//  можно вызывать из прерываиий, если заведомо нет конкуренции
INLINE 
void user_timer_arm_us_nomt (user_timer_t *ut, usertimer_time_t usec_time){
    ut->usec_per_tick = 0;
    ut->cur_time = usec_time;
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается  
 * */
bool_t user_timer_rearm_us  (user_timer_t *ut, usertimer_time_t usec_interval);

INLINE 
bool_t user_timer_rearm_us_nomt (user_timer_t *ut, usertimer_time_t usec_time){
    ut->usec_per_tick = 0;
    ut->cur_time += usec_time;
    return ut->cur_time <=0;
}


void user_timer_restart_interval_us (user_timer_t *ut, usertimer_time_t usec_interval);

INLINE 
void user_timer_init (user_timer_t *ut, unsigned long msec_per_tick){
    user_timer_init_us (ut, msec_per_tick*TIMER_USEC_PER_MSEC);
};

INLINE 
void user_timer_set     (user_timer_t *ut, usertimer_time_t msec_interval){
    user_timer_set_us(ut, msec_interval*1000ul);
}

/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
INLINE 
void user_timer_arm  (user_timer_t *ut, usertimer_time_t msec_interval){
    user_timer_arm_us(ut, msec_interval*1000ul);
}

INLINE 
void user_timer_arm_nomt  (user_timer_t *ut, usertimer_time_t usec_time){
    user_timer_arm_us_nomt(ut, usec_time*1000ul);
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается  
 * */
INLINE 
bool_t user_timer_rearm  (user_timer_t *ut, usertimer_time_t msec_interval){
    return user_timer_rearm_us(ut, msec_interval*1000ul);
}

INLINE 
bool_t user_timer_rearm_nomt  (user_timer_t *ut, usertimer_time_t usec_time){
    return user_timer_rearm_us_nomt(ut, usec_time*1000ul);
}

/**\~russian
 * перезапускает периодический таймер с момента предыдущего события.
 * xsec_interval - новый период событий таймера
 * вызов   user_timer_restart_interval(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
INLINE 
void user_timer_restart_interval (user_timer_t *ut, usertimer_time_t msec_interval){
    user_timer_restart_interval_us(ut, msec_interval*1000ul);
}

#else
void user_timer_init (user_timer_t *ut, small_uint_t msec_per_tick);

/**\~russian
 * запускает периодический таймер с текущего момента.
 * xsec_interval - период событий таймера
 * вызов   user_timer_set(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_set  (user_timer_t *ut, usertimer_time_t msec_interval);

/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_arm  (user_timer_t *ut, usertimer_time_t msec_interval);

//* тоже user_timer_arm_us, но не защищен от конкурентности, 
//  можно вызывать из прерываиий, если заведомо нет конкуренции
INLINE 
void user_timer_arm_nomt (user_timer_t *ut, usertimer_time_t msec_time){
    ut->msec_per_tick = 0;
    ut->cur_time = usec_time;
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается  
 * */
bool_t user_timer_rearm  (user_timer_t *ut, usertimer_time_t msec_interval);

INLINE 
bool_t user_timer_rearm_nomt (user_timer_t *ut, usertimer_time_t msec_time){
    ut->msec_per_tick = 0;
    ut->cur_time += msec_time;
    return ut->cur_time <=0;
}

/**\~russian
 * перезапускает периодический таймер с момента предыдущего события.
 * xsec_interval - новый период событий таймера
 * вызов   user_timer_restart_interval(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_restart_interval (user_timer_t *ut, usertimer_time_t msec_interval);

#endif
void user_timer_add (timer_t *t, user_timer_t *ut);
void user_timer_remove (timer_t *t, user_timer_t *ut);

INLINE 
void user_timer_wait (user_timer_t *ut)
{
    mutex_wait (&ut->lock);
}

/**\~russian
 * останавливает активность таймера.
 * при этом теряются его параметры - точка старта, период  
 * */
INLINE 
void user_timer_stop (user_timer_t *ut){
    while (ut->cur_time != 0)
        ut->cur_time = 0;
}

//void user_timer_reset   (user_timer_t *ut);

/**\~russian
 * перезапускает периодический таймер с текущего момента.
 * */
INLINE
void user_timer_restart (user_timer_t *ut)
{
#ifdef USEC_TIMER
    while (ut->cur_time != (long)ut->usec_per_tick)
        ut->cur_time = ut->usec_per_tick;
#else
    while (ut->cur_time != ut->msec_per_tick)
        ut->cur_time = ut->msec_per_tick;
#endif
}

/**\~russian
 * true - если таймер остановлен. 
 * !!!не действителен для периодического таймера, работает только с одноразовыми. 
 * */
INLINE 
bool_t user_timer_expired (user_timer_t *ut)
{
    return (ut->cur_time <= 0);
}

/**\~russian
 * возвращает время следующего события таймера 
 * */
//usertimer_time_t user_timer_expiration_time(user_timer_t *ut)

/**\~russian
 * возвращает интервал времени до следующего события таймера 
 * */
INLINE 
usertimer_time_t user_timer_expiration_timeout(user_timer_t *ut)
{
    return ut->cur_time;
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* _TIMER_H_ */
