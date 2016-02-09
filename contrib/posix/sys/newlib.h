/*
 * UOS newlib wraper library
 *
 *  Created on: 07.10.2015
 *      Author: a_lityagin
 *
 *  ru UTF8
 *
 *  сдесь прокладки для линковки с newlib
 */
#ifndef __UOS_POSIX_NEWLIB_H
#define __UOS_POSIX_NEWLIB_H

#include <kernel/uos.h>
#include <kernel/internal.h>
#include <reent.h>



#ifndef INLINE_STDC
//  когда сторонний код требует линковаться процедурам заявленным как инлайн, и ему надо предоставить
//  объект с экземплярами этих процедур.(контрибут POSIX генерирует свой врап stdlib чтобы подключить stdc++.)
//  в коде объекта этот инлайн отключаю чтобы получить нормальный код, линкуемый извне
#define INLINE_STDC INLINE
#endif


#ifdef __cplusplus
extern "C" {
#endif

//! этот хук на переключение задач УОС, его надо назначить на uos-conf:UOS_ON_SWITCH_TOTASK(t) или
//	использовать в своем хуке
//  в текущей реализации использует task->privatep в качестве newlib-контекста нитки,
//	!!! позаботьтесь сами о том чтобы он указывал на _reent нитки - его надо создавать для каждой нитки свой
INLINE_STDC
void newlib_on_task_switch(task_t *t)
{
	_impure_ptr = (struct _reent *)(t->privatep);
}

#ifdef __cplusplus
}
#endif

#endif//__UOS_POSIX_NEWLIB_H
