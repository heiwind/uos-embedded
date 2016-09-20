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

#ifndef UOS_CONF_CPU_H_
#define UOS_CONF_CPU_H_

/**************************************************************************
 *                              runtime
 ************************************************************************** */
#ifdef FORCE_LIB_FCPU
#undef KHZ
#undef DELVEES_CLKIN
#undef MPORT_KHZ
#undef RAM_HI
#undef ARCH_HAVE_FPU
#undef MIPS_NOBEV
#undef ELVEES_INIT_SDRAM
#endif

//* отключает настройку режима процессора при старте уос, полагается на настройку стартера
//#define EXTERNAL_SETUP

/* отключение плавающей точки может сэкономить код
 * */
//#undef ARCH_HAVE_FPU
//#define ARCH_HAVE_FPU

/* IDLE_TASK_STACKSZ - стек всякой задачи должен справляться исполнением прерывания
 * для ЭЛВИС использование USER_TIMERS добавит накладных на стек
 * */
#define IDLE_TASK_STACKSZ   (512+512+MIPS_FSPACE)


/**************************************************************************
 *                              ELVEES
 ************************************************************************** */
//suspects that commands lw, mtc0, tfc0 are completed after next tackt,
//      so inserts nop to sure thir value
//#define ELVEES_SAFE_LW_MXC0

//#define FLUSH_CACHE_IN_EXCEPTION

/** форсирует настройку памяти при старте, если происходит запуск под отладчиком или настройка
 * производится внешним загрузчиком - то этот код необязателен, можно довериться настройкам загрузчика */
//#define ELVEES_INIT_SDRAM

#ifndef KHZ
#define KHZ         2400000U
//# Frequency of installed oscillator, kHz
#define DELVEES_CLKIN 10000U
//# Frequency of memory bus, kHz
#define MPORT_KHZ   900000U
//#define RAM_HI      0xb840
#define RAM_HI      0x9801
#endif


/**************************************************************************
 *                              ELVEES PHYSICAL MEMORY SEGMENTING
 ************************************************************************** */
//* C0_STATUS.ERL affects memory mapping
#define UOS_MIPS_NOUSE_ERL      0
//* memory mapping differ on Kseg2,3 - should it be in mind?
#define UOS_MIPS_NOUSE_KSEG23   0

//* декларация того что система конфигурирует кеш, и инвалидация кеша должна учитывать
//*       эту конфигурацию.
//* caching this segments configurable, should it be in mind?
#define UOS_MIPS_NOCACHEBLE_KSEG23    0
#define UOS_MIPS_NOCACHEBLE_KUSEG     0
//* caching this segments configurable, should it be in mind?
//*     if no - ENABLE_I/DCACHE value used
#define UOS_MIPS_NOCACHEBLE_KSEG0     0


/**************************************************************************
 *                              IRQ
 ************************************************************************** */
// это макро задает стиль вызова обработчика прерывания:
//  MIPS_FSPACE == 0 - обработчик вызывается как обычная процедура task_t* hanler()
//  MIPS_FSPACE >0 - прерывание само выделяет в стеке пространство MIPS_FSPACE и 
//                   переходит в обработчик в обход преамбул начала/конца - это чуть быстрее
//  ELVEES требует размер фрейма 88 - без оптимизации, иначе 40 

//#define MIPS_FSPACE 0

//* MIPS_VEC_SECTIONS declares that vectors are plased on their position by linker
//  so verctor-handles places in it`s own sections:
//      .init.pagefault, .init.exception, .init.irq,
//      .init.* - other common stuff
//* это макро поможет отбросить неиспользуемый вектор irq линкером.
//#define  MIPS_VEC_SECTIONS

//* размещение векторов прерываний в CRAM
//#define ELVEES_VECT_CRAM



#endif /* UOS_CONF_H_ */
