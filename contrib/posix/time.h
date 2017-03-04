#ifndef __TIME_H__
#define __TIME_H__

#include <posix-port.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif



struct tm {
	int     tm_sec;         /* seconds */
	int     tm_min;         /* minutes */
	int     tm_hour;        /* hours */
	int     tm_mday;        /* day of month */
	int     tm_mon;         /* month */
	int     tm_year;        /* year */
	int     tm_wday;        /* day of week */
	int     tm_yday;        /* day of year */
	int     tm_isdst;       /* daylight saving time */
};

static inline char *asctime (const struct tm *tm)
	{ return 0; }

static inline char *ctime (const time_t *timep)
	{ return 0; }

static inline struct tm *gmtime (const time_t *timep)
	{ return 0; }

static inline struct tm *localtime (const time_t *timep)
	{ return 0; }

static inline time_t mktime (struct tm *tm)
	{ return 0; }

#ifndef UOS_HAVE_UNIXTIME
#define UOS_HAVE_UNIXTIME
#include <timer/timer.h>
INLINE time_t time(time_t* t) __THROW 
{
    time_t res = timer_seconds(POSIX_timer);
    if (t != NULL)
        *t = res;
    return res;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
