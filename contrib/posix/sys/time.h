#ifndef __SYS_TIME_H__
#define __SYS_TIME_H__
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <timer/timer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct timeval {
	long tv_sec;		/* seconds */
	long tv_usec;		/* microseconds */
};

struct timespec {
	long tv_sec;		/* seconds */
	long tv_nsec;		/* nanoseconds */
};

struct itimerspec {
  struct timespec  it_interval;  /* Timer period */
  struct timespec  it_value;     /* Timer expiration */
};

#define __time_t_defined

struct timezone {
	int tz_minuteswest;	/* Greenwitch offset */
	int tz_dsttime;		/* time correction mode */
};

static inline int gettimeofday (struct timeval *tv, struct timezone *tz)
{
	unsigned long msec, days;
	extern timer_t *uos_timer;

	if (tv) {
		days = timer_days (uos_timer, &msec);
		tv->tv_sec = msec / 1000;
		tv->tv_usec = (msec - tv->tv_sec * 1000) * 1000;
		tv->tv_sec += days * 24 * 60 * 60;
	}
	if (tz) {
		tz->tz_dsttime = 0;
		tz->tz_minuteswest = 0;
	}
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif
