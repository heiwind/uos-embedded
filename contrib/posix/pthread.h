#ifndef __PTHREAD_H__
#define __PTHREAD_H__

/*
 * Implement Posix pthread functions using uOS primitives.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <posix-port.h>
#include <stdlib.h>
#include <sched.h>
#if UOS_USLEEP_STYLE == UOS_USLEEP_STYLE_ETIMER_SLEEP
#include <timer/etimer_threads.h>
#include <errno.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif



typedef task_t* pthread_t;
#define pthread_self()				task_current

typedef mutex_t pthread_mutex_t;

#define pthread_mutex_lock(lock)		({ mutex_lock (lock); 0; })
#define pthread_mutex_trylock(lock)		( mutex_trylock (lock) > 0 ? 0 : -1 )
#define pthread_mutex_unlock(lock)		({ mutex_unlock (lock); 0; })
static inline int pthread_mutex_destroy (pthread_mutex_t *mutex)
	{ return 0; }

#define pthread_mutexattr_t			int

#define pthread_mutexattr_init(a)		( *a = 0 )
#define pthread_mutexattr_destroy(a)		/* empty */
#define pthread_mutexattr_settype(a,t)		/* empty */
#define pthread_mutex_init(m,a)			0

/* TODO - no recursive mutexs in uOS. */
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP	{ 0 }

typedef int pthread_key_t;
#define pthread_key_create(kp,x)		{ *kp = 0; }
#define pthread_getspecific(key)		task_current->privatep
#define pthread_setspecific(key,val)		task_set_private (task_current, val)

#define PTHREAD_STACK_SIZE			0x4000	/* 16 kilobytes */
#define PTHREAD_PRIO				0x4000	/* 16 kilobytes */

typedef int pthread_attr_t;
#define pthread_attr_init(x)
#define pthread_attr_destroy(x)
#define pthread_setcanceltype(t, oldt)

#define pthread_create(taskp,attr,func,arg)	{ \
	int stacksz = PTHREAD_STACK_SIZE;	  \
	char *stack = (char *)calloc (1, stacksz);	  \
	if (stack) \
	    *taskp = task_create ((void(*)(void*)) &(func), \
		arg, "taskpthread", PTHREAD_PRIO, (array_t*)stack, stacksz);}

/* Set thread name visible in the kernel and its interfaces.  */
INLINE  
int pthread_setname_np (pthread_t __target_thread, const char *__name) 
{
    __target_thread->name = __name;
    return 0;
}

#define pthread_cancel(t)			{ \
	if (t->base_prio) task_delete (t, 0); 	  \
	t->base_prio = 0;			}

#define pthread_join(t,statusp)			{ \
	if (t->base_prio) task_wait (t);	  \
	t->base_prio = 0;			}

#define pthread_kill(t,msg)			{ \
	if (t->base_prio) task_delete (t, 0); 	  \
	t->base_prio = 0;			  \
	free (t);				}

/*The registered cleanup handlers are called via exception handling
so we cannot mark this function with __THROW.*/
INLINE __attribute__ ((__noreturn__)) 
void pthread_exit (void *__retval)
{ task_exit(__retval);}

#define pthread_testcancel(t)			/* empty */
#define pthread_equal(t1, t2)			((t1) == (t2))

#define setpriority(a,b,c)			/* empty */

/* Set the scheduling parameters for TARGET_THREAD according to POLICY
   and *PARAM.  */
INLINE __NOTHROW
int pthread_setschedparam (pthread_t __target_thread, int __policy,
                  const struct sched_param *__param)
__noexcept //__nonnull((3))
{
    task_set_priority(__target_thread, __param->__sched_priority);
    return 0;
}

/* Set the scheduling priority for TARGET_THREAD.  */
INLINE __NOTHROW
int pthread_setschedprio (pthread_t __target_thread, int __prio)
__noexcept
{
    task_set_priority(__target_thread, __prio);
    return 0;
}

/* Return in *PARAM the scheduling parameters of *ATTR.  */
INLINE __NOTHROW
int pthread_attr_getschedparam (pthread_t __target_thread
                        , int* __policy
                        , struct sched_param *__restrict __param)
__noexcept //__nonnull ((2, 3))
{
    __param->__sched_priority = task_priority(__target_thread);
    *__policy = SCHED_RR;
    return 0;
}



typedef mutex_t*    pthread_cond_t;

#define pthread_cond_init(cond,x)		{ *cond = 0; }

#define pthread_cond_wait(cond,lock)		{ \
	*cond = lock;				  \
	mutex_wait (lock);			}

#define pthread_cond_signal(cond)		mutex_signal (*cond, 0)

#define pthread_cond_broadcast(cond)		mutex_signal (*cond, 0)

#define pthread_cond_destroy(cond)		{ *cond = 0; }

#if UOS_USLEEP_STYLE == UOS_USLEEP_STYLE_ETIMER_SLEEP
INLINE 
int pthread_cond_timedwait(pthread_cond_t* __restrict__ cond
                          , pthread_mutex_t* __restrict__ lock
                          , unsigned timo
                          )
{
    *cond = lock;
    if (mutex_etimedwait (lock, timo))
        return 0;
    else
        return ETIMEDOUT;
}
#else
#define pthread_cond_timedwait(cond,lock,timo)	({ \
	*cond = lock;				  \
	mutex_wait (lock);			  \
	0;					})
#endif //UOS_USLEEP_STYLE



#ifdef __cplusplus
}
#endif

#endif
