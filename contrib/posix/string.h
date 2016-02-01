#ifndef __STDDEF_H__
#define __STDDEF_H__

#include <runtime/lib.h>
#include <mem/mem.h>

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

INLINE_STDC __NOTHROW
char *strerror (int n) __noexcept
	{ static char s[] = ""; return s; }

INLINE_STDC __NOTHROW
void bzero (void *s, size_t n) __noexcept
	{ memset (s, 0, n); }


#ifdef __cplusplus
}
#endif

#endif
