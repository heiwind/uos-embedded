#ifndef __STREAM_H_
#define __STREAM_H_ 1

// requested mutex_t
#include <kernel/uos.h>

#ifdef __cplusplus
extern "C" {
#endif



/*
 * Поток данных.
 * Имеет интерфейс, состоящий из четырех процедур:
 * выдача байта, опрос последнего выданного байта,
 * прием байта, и опрос первого байта во входящей очереди без удаления.
 */
typedef struct _stream_t {
	struct _stream_interface_t *interface;
} stream_t;

typedef struct _stream_interface_t {
	void (*putc) (stream_t *u, short c);
	unsigned short (*getc) (stream_t *u);
	int (*peekc) (stream_t *u);
	void (*flush) (stream_t *u);
	bool_t (*eof) (stream_t *u);
	void (*close) (stream_t *u);
	struct _mutex_t *(*receiver) (stream_t *u);
} stream_interface_t;

#define to_stream(x)   ((stream_t*)&((x)->interface))

void drain_input (stream_t *u); /* LY: чистит забуферизированный в потоке ввод. */
int stream_puts (stream_t *u, const char *str);
unsigned char *stream_gets (stream_t *u, unsigned char *str, int len);
int stream_printf (stream_t *u, const char *fmt, ...);
int stream_vprintf (stream_t *u, const char *fmt, va_list args);
int stream_scanf (stream_t *u, const char *fmt, ...);
int stream_vscanf (stream_t *u, const char *fmt, va_list argp);

/*
 * Вывод в строку как в поток.
 * Для snprintf и т.п.
 */
typedef struct {
	const stream_interface_t *interface;
	unsigned char *buf;
	int size;
} stream_buf_t;

stream_t *stropen (stream_buf_t *u, unsigned char *buf, int size);
void strclose (stream_buf_t *u);

int snprintf (unsigned char *buf, int size, const char *fmt, ...);
int vsnprintf (unsigned char *buf, int size, const char *fmt, va_list args);
int sscanf (const unsigned char *buf, const char *fmt, ...);
int vsscanf (const unsigned char *buf, const char *fmt, va_list args);

/*
 * LY: выдает результат vprintf без печати, т.е. считает кол-во символов.
 */
int vprintf_getlen (const char *fmt, va_list args);
extern stream_t null_stream;



#ifdef __cplusplus
}
#endif



//**********************************   STDIO   *******************************************
//	here is stdio/stdlib wraps on stream

#ifndef __cplusplus

#ifndef __STDIO_H__
/*
 * Методы приходится делать в виде макросов, т.к. необходимо приведение типа к родительскому.
 * в C++ снято это ограничение за счет перегрузки функций
 * !!! эти макросы конфликтуют с stdio.h, включите <stdio.h> после stream.h чтобы 
 *      учесть конфликт
 */
#define putchar(x,s)    (x)->interface->putc(to_stream (x), s)
#define getchar(x)  (x)->interface->getc(to_stream (x))
#define peekchar(x) (x)->interface->peekc(to_stream (x))
#define fflush(x)       if ((x)->interface->flush) \
            (x)->interface->flush(to_stream (x))
#define feof(x)     ((x)->interface->eof ? \
            (x)->interface->eof(to_stream (x)) : 0)
#define fclose(x)       if ((x)->interface->close) \
            (x)->interface->close(to_stream (x))
#define freceiver(x)    ((x)->interface->receiver ? \
            (x)->interface->receiver(to_stream (x)) : 0)

#define fputs(str, x) stream_puts (to_stream (x), str)
#define puts(x,str) stream_puts (to_stream (x), str)
#define fgets(str,n,x)  stream_gets (to_stream (x), str, n)
#define gets(x,str,n)   stream_gets (to_stream (x), str, n)
#define vfprintf(x,f,a) stream_vprintf (to_stream (x), f, a)
#define vprintf(x,f,a)  stream_vprintf (to_stream (x), f, a)
#define vscanf(x,f,a)   stream_vscanf (to_stream (x), f, a)

/* LY: умышленно вызываем ошибки там, где без необходимости вместо puts() используется printf() */
#define printf(x,f,...) stream_printf (to_stream (x), f, ##__VA_ARGS__)
#endif //~ __STDIO_H__

#else  // __cplusplus
extern "C++" {
namespace uos {
namespace stream {

using ::snprintf;
using ::vsnprintf;
using ::sscanf;
using ::vsscanf;

inline int putchar(stream_t *x, char c) {x->interface->putc(x, c);return 0;};
inline int getchar(stream_t *x) {return x->interface->getc(x);};
inline int peekchar(stream_t *x) {return x->interface->peekc(x);};

inline int fflush(stream_t *x) {
    stream_interface_t* i = x->interface;
    if (i->flush)
        i->flush(x);
    return 0;
};

inline int feof(stream_t *x) {
    stream_interface_t* i = x->interface;
    if (i->eof)
        return i->eof(x);
    else
        return 0;
};

inline int fclose(stream_t *x) {
    stream_interface_t* i = x->interface;
    if (i->close)
        i->close(x);
    return 0;
};

inline mutex_t * freceiver(stream_t *x) {
    stream_interface_t* i = x->interface;
    if (i->receiver)
        return i->receiver(x);
    else
        return (mutex_t*)0; //NULL
};

inline int vfprintf(stream_t *u, const char *fmt, va_list args)
    {return stream_vprintf(u, fmt, args);};

inline int fprintf (stream_t *u, const char *fmt, ...) {
    va_list args;
    int err;
    va_start (args, fmt);
    err = stream_vprintf (u, fmt, args);
    va_end (args);
    return err;
};

inline int fputs(const char* s, stream_t *x) {return stream_puts(x, s);};
inline int puts(stream_t *x, const char* s) {return stream_puts(x, s);};
inline char* fgets(char* dst, int len, stream_t *x) {return (char*)stream_gets(x, (unsigned char*)dst, len);};
inline char* gets(stream_t *x, char* dst, int len) {return (char*)stream_gets(x, (unsigned char*)dst, len);};

inline int vprintf (stream_t *u, const char *fmt, va_list args) {
        return stream_vprintf (u, fmt, args);
};

inline int printf (stream_t *u, const char *fmt, ...) {
    va_list args;
    int err;

    va_start (args, fmt);
    err = stream_vprintf (u, fmt, args);
    va_end (args);
    return err;
};

inline int vscanf (stream_t *u, const char *fmt, va_list argp){
    return stream_vscanf(u, fmt, argp);
};

//! C++ have no internal convertions char* <-> uchar*
inline int vsnprintf (char *buf, int size, const char *fmt, va_list args)
{ return ::vsnprintf((unsigned char *)buf, size, fmt, args); }

inline int snprintf (char *buf, int size, const char *fmt, ...){
    va_list args;
    int err;

    va_start (args, fmt);
    err = ::vsnprintf ((unsigned char *)buf, size, fmt, args);
    va_end (args);
    return err;
}

inline int sscanf (const char *buf, const char *fmt, ...){
    va_list args;
    int err;

    va_start (args, fmt);
    err = ::vsscanf ((unsigned char *)buf, fmt, args);
    va_end (args);
    return err;
}

};// namespace stream
};// namespace uos
} //extern "C++"

namespace std{
using namespace uos::stream;
}

using namespace uos::stream;

#endif // __cplusplus



#endif /* __STREAM_H_ */
