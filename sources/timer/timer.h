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
struct _user_timer_t {
    list_t item;
    mutex_t lock;
#ifdef USEC_TIMER
	unsigned long usec_per_tick;
#else
    small_uint_t msec_per_tick;
#endif
    volatile long int cur_time;
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
inline void timer_init (timer_t *t, unsigned long khz, small_uint_t msec_per_tick){
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
static inline unsigned long timer_milliseconds (timer_t *t){
    return t->milliseconds;
};

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
static inline unsigned long timer_tick(timer_t *t){
    return t->tick;
};

/**\~english
 * Check that `ticks' passed since the `t0' moment.
 *
 * \~russian
 * Проверка временного события.
 */
static inline bool_t timer_passed_tick(timer_t *t, clock_time_t t0, clock_time_t ticks)
{
    unsigned long now = timer_tick(t);
    return (now - t0) > ticks;
}



#ifdef USER_TIMERS
#ifdef USEC_TIMER
void user_timer_init_us (user_timer_t *ut, unsigned long usec_per_tick);
inline void user_timer_init (timer_t *t, unsigned long khz, small_uint_t msec_per_tick){
    user_timer_init_us (t, khz, msec_per_tick*TIMER_USEC_PER_MSEC);
};
#else
void user_timer_init (user_timer_t *ut, small_uint_t msec_per_tick);
#endif
void user_timer_add (timer_t *t, user_timer_t *ut);
void user_timer_wait (user_timer_t *ut);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _TIMER_H_ */
