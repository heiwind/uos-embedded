#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <runtime/lib.h>
#include <mem/mem.h>

#ifndef NULL
#define NULL 0
#endif

#ifdef __cplusplus
extern "C" {
#endif



extern mem_pool_t *uos_memory;
#define malloc(size)		mem_alloc (uos_memory, size)
#define calloc(n,size)		mem_alloc (uos_memory, (n)*(size))
#define realloc(ptr,size)	mem_realloc (ptr, size)
#define free(ptr)		mem_free (ptr)
#define strdup(str)		mem_strdup (uos_memory, str)

/* Allocate SIZE bytes on a page boundary.  The storage cannot be freed.  */
INLINE __attribute__((warning("uos can`t aligned memory alloc!!!")))
void *valloc (size_t __size)  __THROW
{
    return mem_alloc (uos_memory, __size);
}

/* Allocate memory of SIZE bytes with an alignment of ALIGNMENT.  */
INLINE __attribute__((warning("uos can`t aligned memory alloc!!!")))
int posix_memalign (void **__memptr, size_t __alignment, size_t __size) __THROW
{
    void* res = mem_alloc (uos_memory, __size);
    if ( res != NULL){
        *__memptr = res;
        return 0;
    }
    return -1;
}

/* ISO C variant of aligned allocation.  */
INLINE __attribute__((warning("uos can`t aligned memory alloc!!!")))
void *aligned_alloc (size_t __alignment, size_t __size) __THROW
{
    return mem_alloc (uos_memory, __size);
}

static inline int atexit (void (*function)(void))
	{ return 0; }

static inline char *getenv (const char *name)
	{ return 0; }

static inline int putenv (char *name)
	{ return -1; }

static inline void exit(int status)
	{ uos_halt(0); }



#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <new>

inline void operator delete (void *ptr)
    { mem_free (ptr); }

inline void operator delete[] (void *ptr)
    { mem_free (ptr); }

inline void *operator new (unsigned size)
    { return mem_alloc (uos_memory, size); }

inline void *operator new[] (unsigned size)
    { return mem_alloc (uos_memory, size); }

#endif

#endif
