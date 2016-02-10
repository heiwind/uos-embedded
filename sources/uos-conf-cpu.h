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
// делаю стаб для образа во флеши
//#undef MIPS_NOBEV

/* IDLE_TASK_STACKSZ - стек всякой задачи должен справляться исполнением прерывания
 * для ЭЛВИС использование USER_TIMERS добавит накладных на стек 
 * */
#define IDLE_TASK_STACKSZ   (512+512+MIPS_FSPACE)

//suspects that commands lw, mtc0, tfc0 are completed after next tackt,
//      so inserts nop to sure thir value
//#define ELVEES_SAFE_LW_MXC0

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



#endif /* UOS_CONF_H_ */
