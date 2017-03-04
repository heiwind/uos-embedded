/*
 * Processor-dependent data types for MIPS architecture.
 * Do not include this file directly, use runtime/lib.h instead.
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
#ifndef __MACHINE_TYPES_H_
#define __MACHINE_TYPES_H_ 1

#include <stdint.h>
//! TODO have to deside use standart GCC headers or not? should we allow use local std-types?
//#include <stddef.h>


#define INT_SIZE 4

typedef int small_int_t;
typedef unsigned int small_uint_t;

typedef int bool_t;

/*
 * An integer type, large enough to keep a memory address.
 * On ARM, data pointers have 4-byte size.
 */
#ifndef _SIZE_T_DEFINED_ 
typedef unsigned long size_t;
typedef signed long     ssize_t;
typedef signed long     __off_t;
#define _SIZE_T_DEFINED_
#endif

typedef long jmp_buf [10];

#ifdef ARCH_HAVE_FPU
#include <float.h>
#endif

#endif /* __MACHINE_TYPES_H_ */
