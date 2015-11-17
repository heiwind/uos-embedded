#ifndef __SCHED_H__
#define __SCHED_H__

#include <kernel/uos.h>
#include <runtime/sys/uosc.h>
#include <posix-port.h>

#ifdef __cplusplus
extern "C" {
#endif



/* Definitions of constants and data structure for POSIX 1003.1b-1993
   scheduling interface.

 * Implement Posix sheduler functions using uOS primitives.
 */

/* Scheduling algorithms.  */
#define SCHED_OTHER     0
#define SCHED_FIFO      1
#define SCHED_RR        2
//#ifdef __USE_GNU
# define SCHED_BATCH        3
# define SCHED_IDLE     5

# define SCHED_RESET_ON_FORK    0x40000000
//#endif //__USE_GNU

struct sched_param
  {
    int __sched_priority;
  };

/* Get maximum priority value for a scheduler.  */
INLINE __CONST
int sched_get_priority_max (int __algorithm) __THROW
{
#ifdef TASK_PRIORITY_MAX
    return TASK_PRIORITY_MAX;
#else
    return 100;
#endif
}

/* Get minimum priority value for a scheduler.  */
INLINE __CONST 
int sched_get_priority_min (int __algorithm) __THROW
{
#ifdef TASK_PRIORITY_MIN
    return TASK_PRIORITY_MIN;
#else
    return 0;
#endif
}

#define sched_yield()               task_yield()


#ifdef __cplusplus
}
#endif

#endif //SCHED
