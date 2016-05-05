/*
   Copyright (C) 1999 Marek Michalkiewicz <marekm@linux.org.pl>

   Permission to use, copy, modify, and distribute this software and
   its documentation for any purpose and without fee is hereby granted,
   without any conditions or restrictions.  This software is provided
   "as is" without express or implied warranty.
 */

#include <runtime/lib.h>

small_int_t
strnzcmp(const unsigned char *s1, const unsigned char *s2, size_t limit)
{
    size_t n = limit;
    if (n) {
        register unsigned char ret;
        register unsigned char tmp2;
        while(n > 0) {
            ret  = *s1++;
            tmp2 = *s2++;
            if (__glibc_unlikely((ret != tmp2) || (tmp2 == 0))){
                if ((tmp2 == 0) || (ret == 0))
                    return limit - n;
                return n - limit;
            }
            n--;
        };
        return limit;
    }
    return 0;
}

