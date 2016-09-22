/*
 * uos-conf.h
 *  Created on: 06.10.2015
 *      Author: a_lityagin <alexraynepe196@gmail.com>
 *                         <alexraynepe196@hotbox.ru>
 *                         <alexraynepe196@mail.ru>
 *                         <alexrainpe196@hotbox.ru>
 *  
 *  *\~russian UTF8
 *  это дефолтовый конфигурационный файл уОС. здесь сведены настройки модулей.
 *  для сборки своей оси, скопируйте этот файл в папку своего проекта и 
 *      переопределите настройки.
 */

#ifndef UOS_CONF_H_
#define UOS_CONF_H_

/**************************************************************************
 *                              runtime
 ************************************************************************** */
/* UOS_FOR_SPEED выбирает более скоростной код против более компактного
 *  1   - выбирает оптимизации серьезные
 *  2   - включает оптимизации предельные
 * */
#define UOS_FOR_SPEED   2

/* UOS_FOR_SPEED выбирает даже незначительно компактный код против более скоростного
 * */
#define UOS_FOR_SIZE    0

/* выбирает runtime halt - более компактный код, против более полного kernel uos_halt
 * */
//#define RT_HALT

/** добавляет к интерфейсу stream_t функции захвата потоков вх/вых.
 * используется функциями print/scan для сохранения целостности строк
 *
 *  при конкурентном доступе к потоку, будут перемешиваться строки выводимые разными
 *  потоками. чтобы обеспечить их целостность надо вводить блокировку доступа потока
 *  этим занимается mt_stream_t
 * */
#define STREAM_HAVE_ACCEESS 1
//*****************************  MIPS  ***************************************
//*  MIPS_NOBEV  -  декларирует что код работает не во флеши а в РАМ
//#undef MIPS_NOBEV

//* внутрений инициализатор может работать только с бинарным образом!!!
//* если используется внешний загрузчик ELF, который загружает образ в ОЗУ
//* нельзя использовать внутрений инициализатор сегмента данных, ибо он не ведает
//* где размещен образ.
#define UOS_START_MODE_BINARY   0
#define UOS_START_MODE_FLASH    UOS_START_MODE_BINARY
#define UOS_START_MODE_LOADED   1

//#define UOS_START_MODE  UOS_START_MODE_LOADED

/**************************************************************************
 *                              uos.h
 ************************************************************************** */
/**\~russian
 * FASTER_LOCKS добавляет код ускоряющий захват уже захваченого мутекса
 *  = 0 - всякий захват требует выхода в защищенный от шедулера режим 
 *  = 1 - если текущая задача уже захватила мутекс, все выполняется бустрее.
 *      но чуть разбухает размер кода
 * */
//#define FASTER_LOCKS

/* по умолчанияю захват/ожидание мутеха реализуется через универсальные процедуры
 * mutex_lock/wait_until (mutex_t *m, scheduless_condition waitfor, void* waitarg) 
 * UOS_MUTEX_FASTER редуцирует код mutex_lock в собственные версии процедур, чуть более 
 *      быстрые, однако если в проекте используются mutex_ХХХ_until - то это добавит лишний код
 * */
//#define UOS_MUTEX_FASTER

/* по умолчанияю сигнал мутеху приводит к переключению на активированые задачи 
 *  если они более приоритетны
 * UOS_SIGNAL_SMART подключает проверку блокировки активированой нитки другим мутехом, 
 *  чтобы не делать лишнего переключения. эта проверка требует использовать дополнительное
 *     описание состояния нитки, и реализовано за счет ухудшения безопасности
 * */
#define UOS_SIGNAL_SMART_SAFE  1
#define UOS_SIGNAL_SMART_SMALL 2
#define UOS_SIGNAL_SMART UOS_SIGNAL_SMART_SMALL

/**\~russian
 * RECURSIVE_LOCKS задает стиль мутекса
 *  = 0 - ближайший unlock освобождает мутекс
 *  = 1 - мутекс отслеживает количество блокировок в текущей задаче и 
 *          на каждый вызов lock должен быть произведен unlock  
 * */
//#undef RECURSIVE_LOCKS
//#define RECURSIVE_LOCKS 0

/* mutex_unlock вызывает шедулер всякий раз когда ждет более приоритетная задача
 * MUTEX_LASY_SCHED > 0 - позволяет избежать этих преключений. система станет менее динамичной
 *  зато реже переключаться будет.   
 * !!! ленивый режим может привести к заморозке переключения задач в циклах.
 * */
//#define MUTEX_LASY_SCHED 1

/**\~russian
 * UOS_MGROUP_SMART определяет стиль поведения mutex_group:
 *  UOS_MGROUP_SMART_BASIC - базовый вариант не расчитан на динамичное перестроение групп.
 *          группа один раз собирается и запускается на прослушивание
 *  UOS_MGROUP_SMART_DINAMIC - группы могут перестраиваться в процессе работы.
 *          чуть разбухает размер кода. на скорость влияния нет.
 * */
#define UOS_MGROUP_SMART_BASIC      0
#define UOS_MGROUP_SMART_DINAMIC    1
#define UOS_MGROUP_SMART    UOS_MGROUP_SMART_DINAMIC

/**\~russian
 * INLINE задает модификатор для инлайн функций уОС.
 * */
//#ifndef INLINE
//#define INLINE inline
//#endif

#define DEBUG_UARTBAUD 921600

/**************************************************************************
 *                              IRQ
 ************************************************************************** */

/**************************************************************************
 *                              timer.h
 ************************************************************************** */

/**\~russian
 * макро SW_TIMER отключает инициализацию обработчика прерывания системного таймера. в этом случае настраивает и запускает таймер
 * пользовательский код. обновление таймера производится вызовом timer_update
 * */
//#define SW_TIMER 1

/**\~russian
 * макро USER_TIMERS включает функционал множественных пользовательских таймеров на общем системном таймере.
 * см. функции user_timer_XXX
 * !!! DEPRECATED. этот функционал реализуется теперь таймаутами см. TIMER_TIMEOUTS, <timer/timeout.h>
 * */
//#define USER_TIMERS 1

/**\~russian
 * макро TIMER_TIMEOUTS включает в таймер функционал множественных пользовательских таймаутов.
 * см. функции <timer/timeout.h>:timeout_XXX
 * */
//#define TIMER_TIMEOUTS 1

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


//* отключает контроль целостности списка событий. по умолчанию прерывание контролирует что начало списка исправно
//*     и если это не так - громко падает
//#define ETIMER_SAFE   0

/**************************************************************************
 *                              time.h
 ************************************************************************** */
/**\~russian
 * UOS_LEAP_SECONDS включает функционал вычисления tz_time_t.sec в функции tz_time
 * */
//#define UOS_LEAP_SECONDS 1


/**************************************************************************
 *                              debug console
 ************************************************************************** */
/** \~russian
 * NDEBUG отключает дебажный вывод функций assert
 */
//#define NDEBUG

/** \~russian
 * NO_DEBUG_PRINT отключает дебажный вывод функций debug_XXX
 */
//#define NO_DEBUG_PRINT

/** \~russian
 * DEBUG_SIMULATE заглушает дебажный ввод/вывод функций debug_getchar
 */
//#define DEBUG_SIMULATE

//#define MEM_DEBUG 1

//* отключает возможность переключения задач при простое на порте отладки. важно для поведения
//  debug_getchar - конкуретный вариант не занимает время процессора, неконкурентный
//      блокируется на этом вызове.
//  нужно чтобы отвязать debug_XXX от менегера задач, на этом можно сэкономить код
#define DEBUG_IO_NOCONCURENCE

//* включает использование ФИФО модулем uart.c. задаю используемый максимум размера
//* фифо вывода
//#define UART_FIFO_LEN   8


/**************************************************************************
 *                              debug tests and checks
 ************************************************************************** */
//* включает тесты целостности группы
#define UOS_STRICT_MGROUP       1
//* включает тест успешного захвата мутеха в вызовах mutex_lock()/mutex_signal()
#define UOS_STRICT_MUTEX_LOCK   2
//* включает тесты выхода за границы стека
#define UOS_STRICT_STACK        4

/* UOS_STRICT_xxx - задают перечень включаемых тестов aasert в модулях ядра
 * */
#define UOS_STRICTS         (0)



//*****************************************************************************
/* buf/rt_log может собираться с рядом проверок
 * */
//#define RTLOG_ARGS_LIMIT 6

//* включает валидацию журнала перед использвоанием
//#define RTLOG_STRICT_MEM       2
//* валидация корректности аргументов (количество аргументов принтера ограничено)
//#define RTLOG_STRICT_ARGS      8

//#define RTLOG_STRICTNESS       (RTLOG_STRICT_ARGS)
//#define RTLOG_STRICTNESS       0



 /**************************************************************************
 *                              Task Hooks
 ************************************************************************** */
//#define UOS_ON_NEW_TASK(t)
//#define UOS_ON_DESTROY_TASK(t, message)
#define UOS_ON_HALT(errcode)  uos_on_halt(errcode)

//#define UOS_WITH_NEWLIB
//!	Newlib port: см. posix/sys/newlib.h
//этот хук на переключение задач УОС, его надо назначить на uos-conf:UOS_ON_SWITCH_TOTASK(t) или
//	использовать в своем хуке
#ifdef UOS_WITH_NEWLIB
//	NewLib использует _impure_ptr для связи с контекстом нитки.
//	!!! позаботьтесь сами о том чтобы каждая нитка имела свой newlib-контест -  _reent,
//		задача этого хука - получать этот контекст в _impure_ptr
#define UOS_ON_SWITCH_TOTASK(t) newlib_on_task_switch(t)
#else
//#define UOS_ON_SWITCH_TOTASK(t) uos_on_task_switch(t)
#endif

//#define UOS_ON_TIMER(t)         uos_on_timer_hook(t)


/**\~russian
  * эти атрибуты используются для указания размещения кода линкеру для систем с отдельной памятью для прерываний
  *	и быстрого кода
*/
//#define CODE_FAST __attribute__((section(".text.hot")))
//#define CODE_ISR  __attribute__((section(".text.isr_used")))
//#define USED_ISR  __attribute__((section(".text.isr_used")))



//************************   MIPS    *****************************************
/* * MIPS have user exception handling of style
 *      bool uos_on_exception(unsigned context[])
 *      \return - 0 caused default general halt generation
 *      \return - 1 cause normal interupt return? with task switch 
    */
//#define UOS_ON_EXCEPTION(context)  uos_on_exception(context)
//#define UOS_ON_SEGFAULT(context)   uos_on_segfault(context)

/* * MIPS have user IRQ hook:
 *  void uos_on_irq(int nirq)
 * before ISR call it pass IRQno and after ISRservice it finishes call with -1
 */
//#define UOS_ON_IRQ(nirq)        uos_on_irq(nirq)


 /**************************************************************************
 *                  global static initializer support
 ************************************************************************** */
//* gcc c++ use it for static class init.
/*  you should place such code to linkes script to provide initialisers table
     __CTOR_LIST__ = .;
      LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
      KEEP(*(.ctors))
      LONG(0)
      __CTOR_END__ = .;
      __DTOR_LIST__ = .;
      LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
      KEEP(*(.dtors))
      LONG(0)
      __DTOR_END__ = .;
 * */
#define UOS_HAVE_CTORS

//* gcc ARM target use it for static initialise.
/** you should place such code to linkes script to provide initialisers table
   .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
*/
#if defined (__arm__) || defined (__thumb__) \
    || defined(I386) || defined(LINUX386)
#define UOS_HAVE_INITFINI
#endif


#if !defined(UOS_HAVE_CTORS) && !defined(UOS_HAVE_INITFINI)
#ifdef UOS_WITH_NEWLIB
#define UOS_HAVE_CTORS
#endif
#endif



#endif /* UOS_CONF_H_ */
