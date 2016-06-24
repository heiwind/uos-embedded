#ifndef __STDLIB_MALLOC_H__
#define __STDLIB_MALLOC_H__

#include <runtime/lib.h>
#include <mem/mem.h>
#include <posix-port.h>

#ifndef NULL
#define NULL 0
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

#ifndef POSIX_memory
extern mem_pool_t *uos_memory;
#define POSIX_memory    uos_memory
#endif



#define calloc(n,size)      mem_alloc (POSIX_memory, (n)*(size))
#define strdup(str)         mem_strdup (POSIX_memory, str)



INLINE_STDC
__NOTHROW
void* realloc (void* ptr, size_t size) __noexcept {
    return mem_realloc (ptr, size);
}

INLINE_STDC
__NOTHROW
void* malloc(size_t size)  __noexcept {
    return mem_alloc (POSIX_memory, size);
}

INLINE_STDC
__NOTHROW
void free( void* ptr)  __noexcept {
    mem_free (ptr);
}

/* Allocate SIZE bytes on a page boundary.  The storage cannot be freed.  */
INLINE_STDC
__attribute__((warning("uos can`t aligned memory alloc!!!")))
__NOTHROW
void* valloc (size_t __size) __noexcept
{
    return mem_alloc (POSIX_memory, __size);
}

/* Allocate memory of SIZE bytes with an alignment of ALIGNMENT.  */
INLINE_STDC
__attribute__((warning("uos can`t aligned memory alloc!!!")))
__NOTHROW
int posix_memalign (void **__memptr, size_t __alignment, size_t __size) __noexcept
{
    void* res = mem_alloc (POSIX_memory, __size);
    if ( res != NULL){
        *__memptr = res;
        return 0;
    }
    return -1;
}

/* ISO C variant of aligned allocation.  */
INLINE_STDC
__attribute__((warning("uos can`t aligned memory alloc!!!")))
__NOTHROW
void * aligned_alloc (size_t __alignment, size_t __size) __noexcept
{
    return mem_alloc (POSIX_memory, __size);
}


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include <new>

#if UOS_POSIX_NEW > 0

INLINE_STDC
__NOTHROW
void operator delete (void *ptr) __noexcept
    { mem_free (ptr); }

INLINE_STDC
__NOTHROW
void operator delete[] (void *ptr) __noexcept
    { mem_free (ptr); }

INLINE_STDC
__NOTHROW
void *operator new (unsigned size) __noexcept
    { return mem_alloc (POSIX_memory, size); }

INLINE_STDC
__NOTHROW
void *operator new[] (unsigned size) __noexcept
    { return mem_alloc (POSIX_memory, size); }

#endif // UOS_POSIX_NEW

INLINE_STDC
__NOTHROW
void *operator new (unsigned size, mem_pool_t& pool) __noexcept
    { return mem_alloc (&pool, size); }

#endif



#endif //__STDLIB_MALLOC_H__
