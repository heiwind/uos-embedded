#ifndef __STDIO_H__
#define __STDIO_H__

#include <runtime/lib.h>
#include <posix-port.h>
#include <stream/stream.h>

#ifndef INLINE_STDC
//	когда сторонний	код требует линковаться процедурам заявленным как инлайн, и ему надо предоставить
//	объект с экземплярами этих процедур.(контрибут POSIX генерирует свой врап stdlib чтобы подключить stdc++.)
//	в коде объекта этот инлайн отключаю чтобы получить нормальный код, линкуемый извне
#define INLINE_STDC INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifndef NULL
#define NULL 0
#endif

#ifndef EOF
#define EOF -1
#endif

#ifndef BUFSIZ
#define BUFSIZ 512
#endif

#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */

// <stream.h> declares this routines
#if (!defined(to_stream))
int sscanf (const char *buf, const char *fmt, ...);
int snprintf (char *buf, int size, const char *fmt, ...);
int vsnprintf (char *buf, int size, const char *fmt, va_list args);
#endif

#define FILE			int
#define fpos_t			long

#define __UOS_STDIO_IS_NULL    0
#define __UOS_STDIO_IS_STREAM  1

#ifndef __UOS_STDIO__
#define __UOS_STDIO__   __UOS_STDIO_IS_NULL
#define stdin           0
#define stdout          0
#define stderr          0
#endif

INLINE_STDC
FILE *fdopen (int fildes, const char *mode)
	{ return 0; }

INLINE_STDC
FILE *freopen (const char *path, const char *mode, FILE *stream)
	{ return 0; }

INLINE_STDC
int fseek (FILE *stream, long offset, int whence)
	{ return -1; }

INLINE_STDC
long ftell (FILE *stream)
	{ return 0L; }

INLINE_STDC
void rewind (FILE *stream)
	{}

INLINE_STDC
int fgetpos (FILE *stream, fpos_t *pos)
	{ return -1; }

INLINE_STDC
int fsetpos (FILE *stream, fpos_t *pos)
	{ return -1; }

INLINE_STDC
size_t fread (void *ptr, size_t size, size_t nmemb, FILE *stream)
	{ return 0; }

INLINE_STDC
size_t fwrite (const void *ptr, size_t size, size_t nmemb,
    FILE *stream)
	{ return 0; }

INLINE_STDC
void clearerr (FILE *stream)
	{}

INLINE_STDC
int ferror (FILE *stream)
	{ return 0; }

INLINE_STDC
int fileno (FILE *stream)
	{ return 0; }

INLINE_STDC
void setbuf (FILE *stream, char *buf)
	{}

INLINE_STDC
void setbuffer (FILE *stream, char *buf, size_t size)
	{}

INLINE_STDC
void setlinebuf (FILE *stream)
	{}

INLINE_STDC
int setvbuf (FILE *stream, char *buf, int mode, size_t size)
	{ return 0; }

INLINE_STDC
int fputc (int c, FILE *stream)
	{ return c; }

INLINE_STDC
int putc (int c, FILE *stream)
	{ return c; }

INLINE_STDC
int getc (FILE *stream)
    { return -1; }

INLINE_STDC
int ungetc (int c, FILE *stream)
    { return c; }

INLINE_STDC
int fgetc (FILE *stream)
    { return -1; }

INLINE_STDC
int fscanf (FILE *fd, const char *fmt, ...)
	{ return 0; }

INLINE_STDC
int fprintf (FILE *fd, const char *fmt, ...) __THROW
{
	va_list args;
	int err;

	va_start (args, fmt);
	err = debug_vprintf (fmt, args);
	va_end (args);
	return err;
}

#if (!defined(to_stream)) || defined(__cplusplus) 
/** \~russian
 * эти функции конфликтуют с макросами объявлеными в <stream/stream.h>
 * для сохранения обратной совместимости со старым кодом, убираю их объявление
 * в с++ - они доступны как перегруженые
 * \ref stream/stream.h
 * */

INLINE_STDC
FILE *fopen (const char *path, const char *mode)
    { return 0; }

INLINE_STDC int fclose (FILE *file)
    { return 0; }

INLINE_STDC int feof (FILE *stream)
    { return 1; }

INLINE_STDC int fflush (FILE *stream)
    { return 0; }

INLINE_STDC int fputs (const char *s, FILE *stream)
    { return 0; }

INLINE_STDC char *fgets (char *s, int size, FILE *stream)
    { return 0; }

//freceiver

INLINE_STDC int vfprintf (FILE *stream, const char *format, va_list ap)
    { return 0; }

//vscanf

#endif //to_stream

#if __UOS_STDIO__ ==__UOS_STDIO_IS_NULL

#if (!defined(to_stream)) || defined(__cplusplus) 

INLINE_STDC
int putchar (int c)
    { return c; }

INLINE_STDC
int getchar (void)
    { return -1; }

//peekchar

INLINE_STDC
int puts (const char *s)
    { return 0; }

INLINE_STDC
char *gets (char *s)
    { return 0; }

INLINE_STDC
int printf (const char *fmt, ...) __THROW
{
    va_list args;
    int err;

    va_start (args, fmt);
    err = debug_vprintf (fmt, args);
    va_end (args);
    return err;
}

#endif //to_stream

#elif __UOS_STDIO__ ==__UOS_STDIO_IS_STREAM

#if (defined(to_stream))
// remove stream.h wraps macros since used stdio
#undef putchar
#undef getchar
#undef gets
#undef puts
#undef printf
#endif

INLINE_STDC
int putchar (int c) __THROW
    { stdout->interface->putc(stdout, c); return 0 ;}

INLINE_STDC
int getchar (void) __THROW
    { return stdin->interface->getc(stdin); }

//peekchar

INLINE_STDC
int puts (const char *s) __THROW
    { return stream_puts (to_stream (stdout), s);}

//this is posix-incompatible, use fgets better  
INLINE_STDC
char *gets (char *s, int len) __THROW
    { return (char*) stream_gets(to_stream (stdin), (unsigned char*)s, len);}

INLINE_STDC
int printf (const char *fmt, ...) __THROW
{
    va_list args;
    int err;

    va_start (args, fmt);
    err = vfprintf (stdout, fmt, args);
    va_end (args);
    return err;
}
#endif  //__uos_have_stdio_stream__



#include <limits.h>

//* для совместимости с newlib как минимум надо реализовать этоти функции
#define sprintf(buf, fmt, ...) 		snprintf(buf, INT_MAX, fmt, __VA_ARGS__)
#define vsprintf(buf, fmt, args) 	vsnprintf(buf, INT_MAX, fmt, args)




#ifdef __cplusplus
}
#endif

#endif
