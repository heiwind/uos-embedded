#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <timer/timer.h>
#include <errno.h>
#include <posix-port.h>
/*#include <dirent.h>*/
#if UOS_USLEEP_STYLE == UOS_USLEEP_STYLE_ETIMER_SLEEP
#include <timer/etimer_threads.h>
#endif

#ifndef INLINE_STDC
//  когда сторонний код требует линковаться процедурам заявленным как инлайн, и ему надо предоставить
//  объект с экземплярами этих процедур.(контрибут POSIX генерирует свой врап stdlib чтобы подключить stdc++.)
//  в коде объекта этот инлайн отключаю чтобы получить нормальный код, линкуемый извне
#define INLINE_STDC INLINE
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline int getpid()
	{ return (int) task_current; }

static inline int geteuid()
	{ return 1; }

static inline int setpgid (int pid, int pgid)
	{ return 0; }

static inline void sync()
	{}

static inline int access (const char *pathname, int mode)
	{ return -1; }

static inline int unlink (const char *pathname)
	{ return -1; }

/* Values for the second argument to access.
   These may be OR'd together.  */
#define	R_OK	4		/* Test for read permission.  */
#define	W_OK	2		/* Test for write permission.  */
#define	X_OK	1		/* Test for execute permission.  */
#define	F_OK	0		/* Test for existence.  */

static inline int close (int fd)
	{ return 0; }

static inline size_t read (int fd, void *buf, size_t count)
	{ return 0; }

#if __UOS_STDIO__ ==__UOS_STDIO_IS_NULL

INLINE_STDC
size_t write (int fd, const void *buf, size_t count)
	{ return count; }

#elif (__UOS_STDIO__ ==__UOS_STDIO_IS_STREAM) || (UOS_POSIX_NEWLIB_IO > 0)

size_t write (int fd, const void *buf, size_t count);

#endif

#define PF_LOCAL 0
#define SOCK_STREAM 0
static inline int socketpair (int d, int type, int protocol, int sv[2])
{
	errno = EOPNOTSUPP;
	return -1;
}

#if UOS_USLEEP_STYLE == UOS_USLEEP_STYLE_DELAY

#   ifdef POSIX_timer

INLINE void usleep (unsigned long usec)
{
    timer_delay (&POSIX_timer, usec / 1000);
}

INLINE void sleep (unsigned long sec)
{
    while (sec > 0)
        timer_delay (&POSIX_timer, 1000);
}

#   endif//POSIX_timer

#elif UOS_USLEEP_STYLE == UOS_USLEEP_STYLE_ETIMER_SLEEP
#define usleep(usec) etimer_usleep(usec)
#define sleep(sec)   etimer_usleep(sec*1000000ul)
#endif

static inline char *getcwd (char *buf, size_t size)
{
	buf[0] = '/';
	buf[1] = 0;
	return buf;
}

static inline char *getwd (char *buf)
{
	buf[0] = '/';
	buf[1] = 0;
	return buf;
}


#define sysconf(x) x

#ifdef _SC_PAGE_SIZE
INLINE __CONST int getpagesize(void) {
    return sysconf(_SC_PAGE_SIZE);
}

#define STDIN_FILENO    0       /* standard input file descriptor */
#define STDOUT_FILENO   1       /* standard output file descriptor */
#define STDERR_FILENO   2       /* standard error file descriptor */

#endif


#ifdef __cplusplus
}
#endif

#endif
