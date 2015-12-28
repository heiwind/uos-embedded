/*
 * \file
 *  uosc.h
 *  Created on: 06.10.2015
 *      Author: a_lityagin <alexraynepe196@gmail.com>
 *                         <alexraynepe196@hotbox.ru>
 *                         <alexraynepe196@mail.ru>
 *                         <alexrainpe196@hotbox.ru>
 * Default definitions of C compiler quirk work-arounds.
 *  
 *  *\~russian UTF8
 *  этот хедер используется уОС для локализации особенностей компиляторов С и
 *  и использования их расширеного функционала
 */
/**
 *
 * This file is used for making use of extra functionality of some C
 * compilers used for Contiki, and defining work-arounds for various
 * quirks and problems with some other C compilers.
 */
 
#ifndef UOS_C_H_
#define UOS_C_H_

#include <uos-conf.h>
#ifndef INLINE
#   ifdef __cplusplus
#       define INLINE inline
#   else
//  если не делать static - то объектники начнут кофликтовать своими многочисленными 
//  копиями инлайнов при сборке. сделать инлайн слабым (weak)
#       define INLINE static inline
#   endif
#endif

#ifdef MIPS32
//архитектура МИПС не умеет адресоваться к невыравненым словам, тоесть адресацию
//таких данных надо вести побайтово!
#define CPU_ACCESSW_ALIGNMASK 3
#endif



/** linux stdlib задает и использует эти макро */

#ifndef __nonnull
#define __nonnull(seq) __attribute__((nonull seq )) 
#endif

#ifndef __THROW
#   ifdef __cplusplus
#       define __THROW throw()
#   else
#       define __THROW 
#   endif
#endif

#ifndef __CONST
#define __CONST __attribute__ ((__const__))
#endif

#define __PURE      __attribute__ ((__pure__))
#define __NORETURN  __attribute__ ((__noreturn__))

/* Convenience macros to test the versions of glibc and gcc.
   Use them like this:
   #if __GNUC_PREREQ (2,8)
   ... code requiring gcc 2.8 or later ...
   #endif
   Note - they won't work for gcc1 or glibc1, since the _MINOR macros
   were not defined then.  */
#ifndef __GNUC_PREREQ
#if defined __GNUC__ && defined __GNUC_MINOR__
# define __GNUC_PREREQ(maj, min) \
    ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define __GNUC_PREREQ(maj, min) 0
#endif
#endif

/* At some point during the gcc 3.1 development the `used' attribute
   for functions was introduced.  We don't want to use it unconditionally
   (although this would be possible) since it generates warnings.  */
#if __GNUC_PREREQ (3,1)
# define __USED __attribute__ ((__used__))
# define __NOINLINE __attribute__ ((__noinline__))
#else
# define __USED
# define __NOINLINE /* Ignore */
#endif

#ifndef __deprecated
/* gcc allows marking deprecated functions.  */
#if __GNUC_PREREQ (3,2)
# define __deprecated __attribute__ ((__deprecated__))
#else
# define __deprecated /* Ignore */
#endif
#endif

# define __WEAK     __attribute__((weak))
# define __always_inline        __attribute__((always_inline))
# define __must_check           __attribute__((warn_unused_result))

#if __GNUC__ >= 3
# define __glibc_unlikely(cond) __builtin_expect ((cond), 0)
# define __glibc_likely(cond)   __builtin_expect ((cond), 1)
#else
# define __glibc_unlikely(cond) (cond)
# define __glibc_likely(cond)   (cond)
#endif



/**\~rissian эти модификаторы предназначены для более аккуратной линковки кода:
 * \value CODE_ISR - код вызывается из обработчика прерывания,вероятно его желательно 
 *  положить в быструю память рядом с таблицей прерываний
 * \value CODE_FAST - код критичен к скорости исполения,вероятно его желательно 
 *  положить в быструю память
 * */
#ifndef CODE_FAST
#define CODE_FAST
#endif

#ifndef DATA_FAST
#define DATA_FAST
#endif

#ifndef CODE_ISR
#define CODE_ISR
#endif

#ifndef USED_ISR
#define USED_ISR
#endif

#ifndef DATA_TASK
#define DATA_TASK
#endif




#define ARRAY_LENGTH(array) (sizeof (array) / sizeof ((array)[0]))
#define ARRAY_END(array)    ((array) + ARRAY_LENGTH (array))


/*
 * Reading a byte from flash memory.
 * By default - handle it like a simple pointer.
 */
#define FETCH_BYTE(p)   (*(unsigned char*)(p))
#define FETCH_WORD(p)   (*(unsigned short*)(p))
#define FETCH_LONG(p)   (*(unsigned long*)(p))
#define FETCH_PTR(p)    ({ void *x = (void*)(p); *(void**)x; })



/*
 * Define an external name with a given value.
 */
#define ASSIGN_VIRTUAL_ADDRESS(name, val)       \
        asm volatile (              \
        ".globl " #name "\n\t"          \
        ".set " #name ", %0"            \
        :: "n" (val))



#endif /* UOS_C_H_ */
