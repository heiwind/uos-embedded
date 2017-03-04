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

#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "malloc.h"

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
	else if (stream == (FILE*)stderr)
		return stream_puts (to_stream (stderr), s);
	return 0;
}

#endif  //__uos_have_stdio_stream__



#endif //~__cplusplus



#undef sprintf
#undef vsprintf

#ifdef __cplusplus
extern "C"{
int __NOTHROW vsprintf (char *buf, const char *fmt, va_list args);
int __NOTHROW sprintf (char *buf, const char *fmt, ...);
}
#endif // __cplusplus

int __NOTHROW vsprintf (char *buf, const char *fmt, va_list args)
{
	return vsnprintf((unsigned char*)buf, INT_MAX, fmt, args);
}

//* для совместимости с newlib как минимум надо реализовать этоти функции
int __NOTHROW sprintf (char *buf, const char *fmt, ...)
{
    va_list args;
    int err;

    va_start (args, fmt);
    err = vsnprintf((unsigned char*)buf, INT_MAX, fmt, args);
    va_end (args);
    return err;
}



/**The __cxa_pure_virtual function is an error handler that is invoked when a pure virtual function is called.
 * If you are writing a C++ application that has pure virtual functions you must supply your own __cxa_pure_virtual
 * error handler function. For example:
 * extern "C" void __cxa_pure_virtual() { while (1); }
 */
#include <runtime/assert.h>

//__NORETURN
void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    __assert_fail(expr, file, line, func);
}

//extern "C"
void __cxa_pure_virtual() {
	assert(0);
}

//extern "C"
void __cxa_deleted_virtual() {
	assert(0);
}



#include <sys/newlib.h>

#if UOS_POSIX_NEWLIB_IO > 0

struct _reent _global_impure __ATTRIBUTE_IMPURE_PTR__
#   if __UOS_STDIO__ ==__UOS_STDIO_IS_STREAM
        = {
              0                         //int _errno;
            , (__FILE*)stdin
            , (__FILE*)stdout
            , (__FILE*)stderr     //__FILE *_stdin, *_stdout, *_stderr;
        };
#   else
        ;
#   endif

struct _reent *_CONST _global_impure_ptr __ATTRIBUTE_IMPURE_PTR__ = &_global_impure;
struct _reent *_impure_ptr __ATTRIBUTE_IMPURE_PTR__ = &_global_impure;

#else
struct _reent *_impure_ptr __ATTRIBUTE_IMPURE_PTR__;
#endif


#ifndef _REENT_ONLY
#include <errno.h>
int global_errno;
extern int *__errno(void){
    if (_impure_ptr == 0)
        return &global_errno;
    return &_impure_ptr->_errno;
}
#endif



#if __UOS_STDIO__ ==__UOS_STDIO_IS_STREAM


static FILE* stdfiles[3] = {(FILE*)stdin, (FILE*)stdout, (FILE*)stderr};

FILE *fdopen_io (int fildes){
    if (fildes <= STDERR_FILENO)
        return stdfiles[fildes];
    return 0;
}

int fileno_io (FILE *f){
    if (f == (FILE*)stdin)
        return STDIN_FILENO;
    if (f == (FILE*)stdout)
        return STDOUT_FILENO;
    if (f == (FILE*)stderr)
        return STDERR_FILENO;
    return -1;
}

#if UOS_POSIX_NEWLIB_IO > 0

FILE *fdopen (int fildes, const char *mode){
    if (_impure_ptr == 0)
        return fdopen_io(fildes);
    return &_impure_ptr->_errno;

    switch (fildes){
        case STDIN_FILENO: return (FILE*)_impure_ptr->_stdin; break;
        case STDOUT_FILENO: return (FILE*)_impure_ptr->_stdout;break;
        case STDERR_FILENO: return (FILE*)_impure_ptr->_stderr;break;
    }
    return 0;
}
int fileno (FILE * f){
    if (_impure_ptr == 0)
        return fileno_io(f);
    if (f == (FILE*)_impure_ptr->_stdin)
        return STDIN_FILENO;
    if (f == (FILE*)_impure_ptr->_stdout)
        return STDOUT_FILENO;
    if (f == (FILE*)_impure_ptr->_stderr)
        return STDERR_FILENO;
    return -1;
}

#else //UOS_POSIX_NEWLIB_IO > 0

FILE *fdopen (int fildes, const char *mode){
    return fdopen_io(fildes);
}

int fileno (FILE *f){
    return fileno_io(f);
}

#endif //UOS_POSIX_NEWLIB_IO > 0

size_t write (int fd, const void *buf, size_t count){
    FILE* f = fdopen(fd, 0);
    if (f == 0) return 0;
    return stream_write(to_stream((stream_t*)f), buf, count);
    //fclose(f);
}

#endif //__UOS_STDIO__ ==__UOS_STDIO_IS_STREAM
