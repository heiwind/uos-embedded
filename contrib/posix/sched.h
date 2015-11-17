#ifndef __SCHED_H__
#define __SCHED_H__

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


#endif //SCHED