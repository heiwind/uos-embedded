#ifndef UOS_STDLIB_H
#define UOS_STDLIB_H
/*
 * Standard numeric routines for MIPS architecture.
 *
 * Copyright (C) 2008-2010 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 *
 * As a special exception to the GPL, permission is granted for additional
 * uses of the text contained in this file.  See the accompanying file
 * "COPY-UOS.txt" for details.
 */

#ifndef INLINE
#include <runtime/sys/uosc.h>
#endif

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

INLINE int
abs (int __x)
{
	return (__x < 0) ? -__x : __x;
}

INLINE long
labs (long __x)
{
	return (__x < 0) ? -__x : __x;
}

extern unsigned long strtoul (const unsigned char *, unsigned char **, int);

INLINE long
strtol (const unsigned char *__p, unsigned char **__ep, int b)
{
	return (long) strtoul (__p, __ep, b);
}

INLINE int
atoi (const unsigned char *__p)
{
	return (int) strtol (__p, (unsigned char **) 0, 10);
}

INLINE long
atol (const unsigned char *__p)
{
	return strtol (__p, (unsigned char **) 0, 10);
}

extern const unsigned char *strmatch (const unsigned char*, const unsigned char*);
extern void watchdog_alive (void);

void mips_debug_dump_stack (const char *caption, void *sp, void* frame, void *callee);
#define ARCH_debug_dump_stack mips_debug_dump_stack

#ifdef __cplusplus
}
#endif

#endif //UOS_STDLIB_H
