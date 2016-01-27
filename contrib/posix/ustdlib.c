/*
 * UOS stdlibc wraper library.cpp
 *
 *  Created on: 07.10.2015
 *      Author: a_lityagin
 *
 *  ru UTF8
 *
 *  сдесь прокладки для линковки с newlib stdc stdc++
 */
#define INLINE_STDC

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#ifndef __cplusplus

// см stream/stream.h - эти обертки должны предоставить сторнним библиотекам
// нормальные врапы вместо макросов

#undef  fopen
#undef 	fclose
#undef	feof
#undef	fflush
#undef	fputs
#undef	fgets
#undef	vfprintf


/*
FILE *fopen (const char *path, const char *mode)
    { return 0; }

int fclose (FILE *file)
    { return 0; }

int feof (FILE *stream)
    { return 1; }

int fflush (FILE *stream)
    { return 0; }

char *fgets (char *s, int size, FILE *stream)
    { return 0; }

int vfprintf (FILE *stream, const char *format, va_list ap)
    { return 0; }
*/

#if __UOS_STDIO__ ==__UOS_STDIO_IS_NULL

int fputs (const char *s, FILE *stream)
    { return 0; }

#elif __UOS_STDIO__ ==__UOS_STDIO_IS_STREAM

int fputs (const char *s, FILE *stream)
{
	if (stream == (FILE*)stdout)
		return stream_puts (to_stream (stdout), s);
	return 0;
}

#endif  //__uos_have_stdio_stream__



#endif //~__cplusplus



#undef sprintf
#undef vsprintf

#ifdef __cplusplus
extern "C"{
int vsprintf (char *buf, const char *fmt, va_list args) __THROW;
int sprintf (char *buf, const char *fmt, ...) __THROW;
}
#endif // __cplusplus

int vsprintf (char *buf, const char *fmt, va_list args) __THROW
{
	return vsnprintf((unsigned char*)buf, INT_MAX, fmt, args);
}

//* для совместимости с newlib как минимум надо реализовать этоти функции
int sprintf (char *buf, const char *fmt, ...) __THROW
{
    va_list args;
    int err;

    va_start (args, fmt);
    err = vsnprintf((unsigned char*)buf, INT_MAX, fmt, args);
    va_end (args);
    return err;
}




#include <sys/newlib.h>
struct _reent *_impure_ptr __ATTRIBUTE_IMPURE_PTR__;


