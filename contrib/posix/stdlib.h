#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <runtime/lib.h>
#include <posix-port.h>

#ifndef NULL
#define NULL 0
#endif

#ifndef INLINE_STDC
//	когда сторонний	код требует линковаться процедурам заявленным как инлайн, и ему надо предоставить
//	объект с экземплярами этих процедур.(контрибут POSIX генерирует свой врап stdlib чтобы подключить stdc++.)
//	в коде объекта этот инлайн отключаю чтобы получить нормальный код, линкуемый извне
#define INLINE_STDC INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif



INLINE_STDC
int atexit (void (*function)(void))
	{ return 0; }

INLINE_STDC
char *getenv (const char *name)
	{ return 0; }

INLINE_STDC
int putenv (char *name)
	{ return -1; }

INLINE_STDC
void exit(int status)
	{ uos_halt(0); }


INLINE_STDC
__NOTHROW
void abort() __noexcept
	{ uos_halt(0); }


#ifdef __cplusplus
}
#endif


#include <malloc.h>


#endif
