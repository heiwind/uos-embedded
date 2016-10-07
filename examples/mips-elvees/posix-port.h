#ifndef __POSIX_PORT__
#define __POSIX_PORT__

#include <runtime/sys/uosc.h>

#ifdef __cplusplus
extern "C" {
#endif


//#include <main.h>
#include <mem/mem.h>
extern mem_pool_t posix_pool;
#define POSIX_memory  &posix_pool

#define UOS_USLEEP_STYLE_DELAY          0
#define UOS_USLEEP_STYLE_ETIMER_SLEEP   1
#define UOS_USLEEP_STYLE    UOS_USLEEP_STYLE_ETIMER_SLEEP

//#include <console/console-io.h>



#include <timer/timer.h>
extern timer_t timer;
#define POSIX_timer timer




//#define TASK_PRIORITY_MAX 100
//#define TASK_PRIORITY_MIN 0

#define _SC_PAGE_SIZE   0x100

#define UOS_POSIX_NEW_LIBC  0
#define UOS_POSIX_NEW_MY    1
#define UOS_POSIX_NEW       UOS_POSIX_NEW_MY

#ifdef __cplusplus
}
#endif

#endif //__POSIX_PORT__

