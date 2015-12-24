/**\namespace timer \brief \~english
 * Module "timer" - system time management.
 *
 * Full description of file timer.c.
 */

/**\namespace timer \brief \~russian
 * Модуль "timer" - управление системным временем.
 *
 * Полное описание файла timer.c.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <timer/timer.h>

#if defined (ARM_CORTEX_M3) || defined (ARM_CORTEX_M4)
extern volatile uint32_t __timer_ticks_uos;
#endif

#ifndef CODE_ISR
#define CODE_ISR
#endif

#if I386
#   include <runtime/i386/i8253.h>
#   define TIMER_IRQ        0   /* IRQ0 */
#endif

#if __AVR__             /* Timer 1 compare A */
#   if defined(__AVR_ATmega2561__) || defined(__AVR_ATmega2560__)
#      define TIMER_IRQ     16
#   endif
#   if defined (__AVR_ATmega103__) || defined (__AVR_ATmega128__)
#      define TIMER_IRQ     11
#   endif
#   ifdef __AVR_ATmega161__
#      define TIMER_IRQ     6
#   endif
#   ifdef __AVR_ATmega168__
#      define TIMER_IRQ     10
#   endif
#endif  /*__AVR__*/

#if ARM_S3C4530
#   define TIMER_IRQ        10  /* Timer 0 interrupt */
#endif

#if ARM_AT91SAM
#   define TIMER_IRQ    AT91C_ID_SYS
#endif

#if ARM_OMAP44XX
#   define TIMER_IRQ        29  /* Private Timer interrupt */
#endif

#if ELVEES_MC24
#   define TIMER_IRQ        29  /* Interval Timer interrupt */
#endif

#if ELVEES_MC0226
#   define TIMER_IRQ        29  /* Interval Timer interrupt */
#endif

#if ELVEES_MC24R2
#   define TIMER_IRQ        22  /* Interval Timer interrupt */
#endif

#if ELVEES_NVCOM01
#   define TIMER_IRQ        22  /* Interval Timer interrupt */
#endif

#if ELVEES_NVCOM02
#   define TIMER_IRQ        22  /* TODO: Interval Timer interrupt */
#endif

#if ELVEES_MCT02
#   define TIMER_IRQ        22  /* Interval Timer interrupt */
#endif

#if ELVEES_MCT03P
#   define TIMER_IRQ        22  /* Interval Timer interrupt */
#endif

#if ELVEES_MC0428
#   define TIMER_IRQ        22  /* Interval Timer interrupt */
#endif

#if ELVEES_MC30SF6
#   define TIMER_IRQ		22	/* Interval Timer interrupt */
#endif

#if PIC32MX
#   define TIMER_IRQ    PIC32_VECT_CT   /* Core Timer Interrupt */
#endif

#if ARM_STM32F4
#   define TIMER_IRQ        82  /* Systick */
#endif

#if ARM_STM32L151RC || ARM_STM32L152RC
#   if defined(RTC_TIMER)
#       define TIMER_IRQ        IRQ_RTC_WKUP    /* RTC */
#   else
#       define TIMER_IRQ        ARCH_TIMER_IRQ  /* Systick */
#   endif
#endif

#if ARM_1986BE9
#   define TIMER_IRQ        32  /* Systick */
#endif

#if ARM_1986BE1
#   if (ARM_SYS_TIMER==1)
#       define TIMER_IRQ    TIMER1_IRQn
#       define SYS_TIMER    ARM_TIMER1
#       define PER_CLOCK_EN ARM_PER_CLOCK_TIMER1
#       define TIM_CLK_EN   ARM_TIM_CLOCK_EN1
#   elif (ARM_SYS_TIMER==2)
#       define TIMER_IRQ    TIMER2_IRQn
#       define SYS_TIMER    ARM_TIMER2
#       define PER_CLOCK_EN ARM_PER_CLOCK_TIMER2
#       define TIM_CLK_EN   ARM_TIM_CLOCK_EN2
#   elif (ARM_SYS_TIMER==3)
#       define TIMER_IRQ    TIMER3_IRQn
#       define SYS_TIMER    ARM_TIMER3
#       define PER_CLOCK_EN ARM_PER_CLOCK_TIMER3
#       define TIM_CLK_EN   ARM_TIM_CLOCK_EN3
#   elif (ARM_SYS_TIMER==4)
#       define TIMER_IRQ    TIMER4_IRQn
#       define SYS_TIMER    ARM_TIMER4
#       define PER_CLOCK_EN ARM_PER_CLOCK_TIMER4
#       define TIM_CLK_EN   ARM_UART_CLOCK_TIM4_EN
#   else
#       warning "ARM_SYS_TIMER is not defined in CFLAGS (target.cfg). Using TIMER1 for system timer."
#       define TIMER_IRQ    TIMER1_IRQn
#       define SYS_TIMER    ARM_TIMER1
#       define PER_CLOCK_EN ARM_PER_CLOCK_TIMER1
#       define TIM_CLK_EN   ARM_TIM_CLOCK_EN1
#   endif
#endif

#if MSP430
#   ifdef TIMERA0_VECTOR
#      define TIMER_IRQ     (TIMERA0_VECTOR / 2)
#   else
#      define TIMER_IRQ     (TIMER0_A0_VECTOR / 2)
#      define TACTL         TA0CTL
#      define TACCR0        TA0CCR0
#      define TAEX0         TA0EX0
#   endif
#endif

#if LINUX386
#   include <sys/time.h>
#   include <signal.h>
#   define TIMER_IRQ        SIGALRM
#endif

/**\~english
 * Check that `msec' milliseconds have passed.
 * `Interval' is the time interval, probably rolled over the day.
 *
 * \~russian
 * Проверка, прошло ли указанное количество миллисекунд `msec'.
 * Параметр `interval' содержит интервал времени, возможно, переходящий границу суток.
 */
CODE_ISR 
bool_t 
interval_greater_or_equal (long interval, long msec)
{
#ifndef TIMER_NO_DAYS
    if (interval < 0)
        interval += TIMER_MSEC_PER_DAY;
    else if (interval >= (long)TIMER_MSEC_PER_DAY)
        interval -= TIMER_MSEC_PER_DAY;
    return (interval >= msec);
#else
    if (interval < 0)
        interval = -interval;
    return (interval >= msec);
#endif
}



/*
 * Timer update function.
 */
CODE_ISR 
inline void timer_mutex_note(mutex_t* t, unsigned long message){
    mutex_awake (t, (void*) message);
}

#ifndef SW_TIMER
CODE_ISR 
static inline
#endif
void timer_update (timer_t *t)
{
#if defined (ARM_CORTEX_M3) || defined (ARM_CORTEX_M4)
	__timer_ticks_uos++;

	if (__timer_ticks_uos==0)
		__timer_ticks_uos++;
#endif
/*debug_printf ("<ms=%ld> ", t->milliseconds);*/
#if defined (ELVEES)
    /* Clear interrupt. */
    MC_ITCSR &= ~MC_ITCSR_INT;
#endif
#if ARM_AT91SAM
    /* Clear interrupt. */
    *AT91C_PITC_PIVR;
#endif
#if ARM_OMAP44XX
    /* Clear interrupt. */
    ARM_PRT_INT_STATUS = ARM_PRT_EVENT;
#endif
#if ARM_1986BE1
    SYS_TIMER->TIM_STATUS &= ~ARM_TIM_CNT_ARR_EVENT;
#endif
#if PIC32MX
    /* Increment COMPARE register. */
    unsigned compare = mips_read_c0_register (C0_COMPARE);
    do {
        compare += t->compare_step;
        mips_write_c0_register (C0_COMPARE, compare);
    } while ((int) (compare - mips_read_c0_register (C0_COUNT)) < 0);
#endif
#if defined (RTC_TIMER)
#   if defined (ARM_STM32L151RC) || defined (ARM_STM32L152RC)
        RTC->ISR &= ~RTC_WUTF;
        PWR->CR |= PWR_CWUF;
        EXTI->PR = EXTI_RTC_WKUP;
#   endif
#endif

#ifdef USEC_TIMER
    const unsigned long interval = t->usec_per_tick;
#else
    const unsigned long interval = t->msec_per_tick;
#endif
    unsigned long msec = t->milliseconds;
#ifndef TIMER_NO_DECISEC
    unsigned long nextdec = t->next_decisec;
#else
    unsigned long nextdec = msec;
#endif

    /* Increment current time. */
#ifdef USEC_TIMER
    unsigned long usec = t->usec_in_msec;
    usec += interval;
    while (usec > t->usec_per_tick_msprec){
        usec -= t->usec_per_tick_msprec;
        msec += t->msec_per_tick;
    }
    while (usec > TIMER_USEC_PER_MSEC) {
        msec++;
        usec -= TIMER_USEC_PER_MSEC;
    }
    t->usec_in_msec = usec;
#else
    msec += interval;
#endif //USEC_TIMER

#ifndef TIMER_NO_DAYS
    if (msec >= TIMER_MSEC_PER_DAY) {
        t->days++;
        msec -= TIMER_MSEC_PER_DAY;
        nextdec -= TIMER_MSEC_PER_DAY;
    }
#endif

    t->milliseconds = msec;

#ifndef TIMER_NO_DECISEC
    /* Send signal every 100 msec. */
#ifdef USEC_TIMER
    if ((interval >= (TIMER_DECISEC_MS*1000ul)) ||
#else
    if ((interval >= TIMER_DECISEC_MS) ||
#endif
        (msec >= nextdec)) 
    {
        nextdec += TIMER_DECISEC_MS;
/*debug_printf ("<ms=%lu,nxt=%lu> ", t->milliseconds, t->next_decisec);*/
        timer_mutex_note(&t->decisec, msec);
    }
    t->next_decisec = nextdec;
#endif

#ifdef USER_TIMERS
    if (! list_is_empty (&t->user_timers)) {
        user_timer_t *ut;
        list_iterate (ut, &t->user_timers) {
            long now = ut->cur_time;
            if (now <= 0) continue;
            now  -= interval;
            if (now <= 0) {
#ifdef USEC_TIMER
                    now += ut->usec_per_tick;
#else
                    now += ut->msec_per_tick;
#endif
                ut->cur_time = now;
                timer_mutex_note(&ut->lock, msec);
            }
            else
            ut->cur_time = now;
        }
    }
#endif //USER_TIMERS
}

/*
 * Timer interrupt handler.
 */
#ifndef UOS_ON_TIMER
#define UOS_ON_TIMER(t)
#else
__attribute__((weak, noinline))
void uos_on_timer_hook(timer_t *t)
{}
#endif

CODE_ISR 
bool_t 
timer_handler (timer_t *t)
{
    UOS_ON_TIMER(t);
    timer_update (t);

    arch_intr_allow (TIMER_IRQ);

    /* Must signal a lock, for timer_wait().
     * TODO: Optimize timer_delay, keeping a sorted
     * queue of delay values. */
    return 0;
}

/**\~english
 * Return the (real) time in days and milliseconds since uOS start.
 *
 * \~russian
 * Запрос времени в сутках и миллисекундах.
 */
#ifndef TIMER_NO_DAYS
unsigned int
timer_days (timer_t *t, unsigned long *milliseconds)
{
    unsigned short val;

    if (!milliseconds){
        val = t->days;
    }
    else{
        *milliseconds = t->milliseconds;
        val = t->days;
        while (*milliseconds != t->milliseconds){
            *milliseconds = t->milliseconds;
            val = t->days;
        } 
    }
    return val;
}
#endif

/**\~english
 * Delay the current task by the given time in milliseconds.
 *
 * \~russian
 * Задержка выполнения текущей задачи.
 */
void
timer_delay (timer_t *t, unsigned long msec)
{
    unsigned long t0;

    t0 = t->milliseconds;
    while (! interval_greater_or_equal (t->milliseconds - t0, msec)) {
        mutex_wait (&t->lock);
    }
}

void
timer_delay_ticks (timer_t *t, clock_time_t ticks)
{
    unsigned long t0;

    t0 = t->tick;
    while ((t->tick - t0) < ticks) {
        mutex_wait (&t->lock);
    }
}

/**\~english
 * Check that `msec' milliseconds passed since the `t0' moment.
 *
 * \~russian
 * Проверка временного события.
 */
bool_t
timer_passed (timer_t *t, unsigned long t0, unsigned int msec)
{
    unsigned long now = timer_milliseconds(t);
    return interval_greater_or_equal (now - t0, msec);
}

#ifdef USEC_TIMER

static inline unsigned long umuldiv1000(unsigned long khz, unsigned long usec_per_tick){
    //res = khz*usec_per_tick /1000
    unsigned long long res = khz>>3;
    res = res * usec_per_tick;
    //const long Nmod = 128*128; 
    //res = res*(Nmod/125);
    //return res/Nmod;
    res = res / 125;
    return res;
} 

static inline unsigned long timer_period_byus(unsigned long khz, unsigned long usec_per_tick){
    return umuldiv1000(khz, usec_per_tick);
}

unsigned long timer_seconds (timer_t *t){
    unsigned long ms;
    unsigned long days = 0;
#ifndef TIMER_NO_DAYS
    days = timer_days (t, &ms);
#else
    ms = timer_miliseconds(t);
#endif
    unsigned long secs = umuldiv1000(TIMER_MSEC_PER_DAY, days) + umuldiv1000(ms, 1);
    return secs;
}

/**\~english
 * Nanosecond Timer initialization.
 *
 * \~russian
 * Инициализация наносекудного таймера.
 */
void
timer_init_us (timer_t *t, unsigned long khz, unsigned long usec_per_tick)
{
    t->usec_per_tick = usec_per_tick;
    unsigned tick_ms = usec_per_tick/1000;
    t->msec_per_tick = tick_ms;
    if (tick_ms > 0)
        t->usec_per_tick_msprec = tick_ms*1000;
    else
        t->usec_per_tick_msprec = ~0;
    t->khz = khz;

#ifndef SW_TIMER
    /* Attach fast handler to timer interrupt. */
    mutex_attach_irq (&t->lock, TIMER_IRQ, (handler_t) timer_handler, t);

    /* Initialize the hardware. */
#if ARM_1986BE9
    ARM_SYSTICK->CTRL = 0;
    ARM_SYSTICK->VAL = 0;
    ARM_SYSTICK->LOAD = timer_period_byus(khz, usec_per_tick) - 1;
    ARM_SYSTICK->CTRL = ARM_SYSTICK_CTRL_ENABLE |
                ARM_SYSTICK_CTRL_TICKINT |
                ARM_SYSTICK_CTRL_HCLK;
//#endif // ARM_1986BE9

#elif ARM_1986BE1
    ARM_RSTCLK->PER_CLOCK |= PER_CLOCK_EN;
#if (ARM_SYS_TIMER==4)
    ARM_RSTCLK->UART_CLOCK |= TIM_CLK_EN;
#else
    ARM_RSTCLK->TIM_CLOCK |= TIM_CLK_EN;
#endif
    SYS_TIMER->TIM_CNT = 0;
    SYS_TIMER->TIM_PSG = 0;
    SYS_TIMER->TIM_ARR = timer_period_byus(khz, usec_per_tick) - 1;
    SYS_TIMER->TIM_IE = ARM_TIM_CNT_ARR_EVENT_IE;
    SYS_TIMER->TIM_CNTRL = ARM_TIM_CNT_EN;
//#endif

#elif defined (ELVEES)
    /* Use interval timer with prescale 1:1. */
    MC_ITCSR = 0;
    MC_ITSCALE = 0;
    MC_ITPERIOD = timer_period_byus(khz, usec_per_tick) - 1;
    MC_ITCSR = MC_ITCSR_EN;
//#endif

#else
#   error "TIMER cant initialise unsupported chip\n"
#endif


#endif // SW_TIMER

#ifdef USER_TIMERS
    list_init (&t->user_timers);
#endif
}

#else //USEC_TIMER

/**\~english
 * Create timer task.
 *
 * \~russian
 * Инициализация таймера.
 */
void CODE_ISR
timer_init (timer_t *t, unsigned long khz, small_uint_t msec_per_tick)
{
    t->msec_per_tick = msec_per_tick;
    t->khz = khz;

#ifndef SW_TIMER
    /* Attach fast handler to timer interrupt. */
    mutex_attach_irq (&t->lock, TIMER_IRQ, (handler_t) timer_handler, t);

    /* Initialize the hardware. */
#if __AVR__
    TCCR1A = 0;
    TCCR1B = 0;
    OCR1A = (t->khz * t->msec_per_tick) / 8 - 2;
    TCNT1 = 0;
    TCCR1B = 0x0A;  /* clock source CK/8, clear on match A */
#endif
#if I386
    {
    /* For I386, t->khz contains actually Hz (=1193182). */
    unsigned short count = (t->khz * t->msec_per_tick + 500) / 1000;
    outb (I8253_MODE_SEL0 | I8253_MODE_RATEGEN | I8253_MODE_16BIT,
        I8253_MODE);
    outb (count & 0xff, I8253_CNTR0);
    outb (count >> 8, I8253_CNTR0);
    }
#endif
#if ARM_S3C4530
    /* Use timer 0 in toggle mode. */
    ARM_TMOD &= ~(ARM_TMOD_TE0 | ARM_TMOD_TCLR0);
    ARM_TDATA(0) = (t->khz * t->msec_per_tick) - 1;
    ARM_TCNT(0) = 0;
    ARM_TMOD |= ARM_TMOD_TE0 | ARM_TMOD_TMD0;
#endif
#if ARM_AT91SAM
    *AT91C_PITC_PIMR = (((t->khz * t->msec_per_tick + 8) >> 4) - 1) |
        AT91C_PITC_PITEN | AT91C_PITC_PITIEN;
#endif
#if ARM_OMAP44XX
    ARM_PRT_LOAD = khz * msec_per_tick - 1;
    ARM_PRT_CONTROL = ARM_PRT_TIMER_EN | ARM_PRT_AUTO_RELOAD |
        ARM_PRT_IRQ_EN;
#endif
#if PIC32MX
    /* Use core timer. */
    unsigned count = mips_read_c0_register (C0_COUNT);
    t->compare_step = (t->khz * t->msec_per_tick + 1) / 2;
    mips_write_c0_register (C0_COMPARE, count + t->compare_step);
#endif
#if defined (ELVEES)
    /* Use interval timer with prescale 1:1. */
    MC_ITCSR = 0;
    MC_ITSCALE = 0;
    MC_ITPERIOD = t->khz * t->msec_per_tick - 1;
    MC_ITCSR = MC_ITCSR_EN;
#endif
#if ARM_CORTEX_M3 || ARM_CORTEX_M4
#   if defined (RTC_TIMER)
#       if defined (ARM_STM32L151RC) || defined (ARM_STM32L152RC)
            /* Clear write protection for RTC registers */
            RCC->APB1ENR |= RCC_PWREN;
            PWR->CR |= PWR_DBP;
            RTC->WPR = 0xCA;
            RTC->WPR = 0x53;
            /* Enable LSE and set it as clock source for RTC */
            RCC->CSR |= RCC_LSEON;
            while (! (RCC->CSR & RCC_LSERDY));
            RCC->CSR |= RCC_RTCEN | RCC_RTCSEL_LSE;
            /* Enable RTC Wakeup interrupt in the EXTI */
            EXTI->PR = EXTI_RTC_WKUP;
            EXTI->RTSR |= EXTI_RTC_WKUP;
            EXTI->IMR |= EXTI_RTC_WKUP;
            /* HZ_CLKIN_RTC is divided by 2 by WUT Prescaler */
            RTC->CR = 0;
            while (! (RTC->ISR & RTC_WUTWF));
            RTC->WUTR = (HZ_CLKIN_RTC / 2) * t->msec_per_tick / 1000 - 1;
            RTC->CR = RTC_WUCKSEL_DIV2 | RTC_WUTE | RTC_WUTIE;
#       endif
#   else
        ARM_SYSTICK->CTRL = 0;
        ARM_SYSTICK->VAL = 0;
#       ifdef SETUP_HCLK_HSI
            /* Max 2130 msec/tick at 8 MHz. */
            ARM_SYSTICK->LOAD = 8000 * t->msec_per_tick - 1;
#       else
            /* Max 213 msec/tick at 80 MHz. */
            ARM_SYSTICK->LOAD = t->khz * t->msec_per_tick - 1;
#       endif
            ARM_SYSTICK->CTRL = ARM_SYSTICK_CTRL_ENABLE |
                        ARM_SYSTICK_CTRL_TICKINT |
                        ARM_SYSTICK_CTRL_HCLK;
#   endif
#endif
#if ARM_1986BE1
    ARM_RSTCLK->PER_CLOCK |= PER_CLOCK_EN;
#if (ARM_SYS_TIMER==4)
    ARM_RSTCLK->UART_CLOCK |= TIM_CLK_EN;
#else
    ARM_RSTCLK->TIM_CLOCK |= TIM_CLK_EN;
#endif
    SYS_TIMER->TIM_CNT = 0;
    SYS_TIMER->TIM_PSG = 0;
    SYS_TIMER->TIM_ARR = t->khz * t->msec_per_tick - 1;
    SYS_TIMER->TIM_IE = ARM_TIM_CNT_ARR_EVENT_IE;
    SYS_TIMER->TIM_CNTRL = ARM_TIM_CNT_EN;
#endif
#if MSP430
    {
    unsigned long divider = t->msec_per_tick * t->khz;
#ifdef TAEX0
    /* Setup divider expansion. */
    small_uint_t nx = (divider - 4) >> (16 + 3);
    assert (nx <= 7);
    TAEX0 = nx;
    if (nx)
        divider /= nx + 1;
#endif
    TACTL = TACLR;              /* Stop timer. */
    TACTL = TASSEL_SMCLK | ID_DIV8;     /* Source clock SMCLK divided by 8. */
    divider = (divider + 4) >> 3;
    assert (divider <= 65536);
    TACCR0 = divider - 1;           /* Set tick rate. */
    TACTL |= MC_1;              /* Start timer in up mode. */
    }
#endif
#if LINUX386
    {
    struct itimerval itv;

    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = t->msec_per_tick * 1000L;
    itv.it_value = itv.it_interval;
    setitimer (ITIMER_REAL, &itv, 0);
    }
#endif

#endif // SW_TIMER

#ifdef USER_TIMERS
    list_init (&t->user_timers);
#endif // USER_TIMERS
}
#endif // USEC_TIMER

#ifdef USER_TIMERS

#ifdef USEC_TIMER
void user_timer_init_us (user_timer_t *ut, unsigned long usec_per_tick)
{
    ut->usec_per_tick = usec_per_tick;
    ut->cur_time = usec_per_tick;
    list_init (&ut->item);
}

/**\~russian
 * запускает периодический таймер с текущего момента.
 * xsec_interval - период событий таймера
 * вызов   user_timer_set(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_set_us  (user_timer_t *ut, usertimer_time_t usec_interval)
{
    arch_state_t x;
    arch_intr_disable (&x);
    ut->usec_per_tick = usec_interval;
    ut->cur_time = usec_interval;
    arch_intr_restore (x);
}

/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_arm_us  (user_timer_t *ut, usertimer_time_t usec_time)
{
    arch_state_t x;
    arch_intr_disable (&x);
    ut->usec_per_tick = 0;
    ut->cur_time = usec_time;
    arch_intr_restore (x);
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается  
 * */
bool_t user_timer_rearm_us  (user_timer_t *ut, usertimer_time_t usec_time){
    arch_state_t x;
    bool_t res;
    arch_intr_disable (&x);
    ut->usec_per_tick = 0;
    ut->cur_time += usec_time;
    res = ut->cur_time <=0;
    arch_intr_restore (x);
    return res;
}

//void user_timer_reset   (user_timer_t *ut);

/**\~russian
 * перезапускает периодический таймер с момента предыдущего события.
 * xsec_interval - новый период событий таймера
 * вызов   user_timer_restart_interval(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_restart_interval_us (user_timer_t *ut, usertimer_time_t usec_interval){
    arch_state_t x;
    arch_intr_disable (&x);
    if (ut->cur_time > 0){
        long delta = ut->usec_per_tick - usec_interval;
        ut->usec_per_tick += delta;
        ut->cur_time += delta;
    }
    else{
        ut->cur_time += usec_interval;
        ut->usec_per_tick = usec_interval;
    }
    arch_intr_restore (x);
}

#else
void user_timer_init (user_timer_t *ut, small_uint_t msec_per_tick)
{
    ut->msec_per_tick = msec_per_tick;
    ut->cur_time = msec_per_tick;
    list_init (&ut->item);
}

/**\~russian
 * запускает периодический таймер с текущего момента.
 * xsec_interval - период событий таймера
 * вызов   user_timer_set(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_set  (user_timer_t *ut, usertimer_time_t msec_interval)
{
    arch_state_t x;
    arch_intr_disable (&x);
    ut->msec_per_tick = msec_interval;
    ut->cur_time = msec_interval;
    arch_intr_restore (x);
}


/**\~russian
 * запускает одноразовый таймер с текущего момента.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_arm  (user_timer_t *ut, usertimer_time_t msec_time)
{
    arch_state_t x;
    arch_intr_disable (&x);
    ut->msec_per_tick = 0;
    ut->cur_time = msec_time;
    arch_intr_restore (x);
}

/**\~russian
 * запускает одноразовый таймер с момента предыдущего события.
 * xsec_time - время задержки события таймера
 * вызов   user_timer_arm(, 0) - останавливает таймер, аналогичен user_timer_stop
 * !!! если вновь выставленное событие уже просрочено - возвращается true
 * return false - если запущеное событие еще ожидается  
 * */
bool_t user_timer_rearm  (user_timer_t *ut, usertimer_time_t msec_time){
    arch_state_t x;
    bool_t res;
    arch_intr_disable (&x);
    ut->msec_per_tick = 0;
    ut->cur_time += msec_time;
    res = ut->cur_time <=0;
    arch_intr_restore (x);
    return res;
}

//void user_timer_reset   (user_timer_t *ut);

/**\~russian
 * перезапускает периодический таймер с момента предыдущего события.
 * xsec_interval - новый период событий таймера
 * вызов   user_timer_restart_interval(, 0) - останавливает таймер, аналогичен user_timer_stop
 * */
void user_timer_restart_interval (user_timer_t *ut, usertimer_time_t msec_interval){
    arch_state_t x;
    arch_intr_disable (&x);
    if (ut->cur_time > 0){
        long delta = ut->msec_per_tick - msec_interval;
        ut->msec_per_tick += delta;
        ut->cur_time += delta;
    }
    else{
        ut->cur_time += msec_interval;
        ut->msec_per_tick = msec_interval;
    }
    arch_intr_restore (x);
}
#endif // no USEC_TIMER

void user_timer_add (timer_t *t, user_timer_t *ut)
{
    mutex_lock (&t->lock);
    list_append (&t->user_timers, &ut->item);
    mutex_unlock (&t->lock);
}

void user_timer_remove (timer_t *t, user_timer_t *ut){
    mutex_lock (&t->lock);
    list_unlink (&ut->item);
    mutex_unlock (&t->lock);
}

#endif // USER_TIMERS

