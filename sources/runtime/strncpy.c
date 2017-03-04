/*
   Copyright (C) 1999 Marek Michalkiewicz <marekm@linux.org.pl>

   Permission to use, copy, modify, and distribute this software and
   its documentation for any purpose and without fee is hereby granted,
   without any conditions or restrictions.  This software is provided
   "as is" without express or implied warranty.
 */

#include <runtime/lib.h>

unsigned char *
strncpy(unsigned char *dest, const unsigned char *src, size_t n)
{
	register unsigned char *tmp = dest;

	if (n) {
		while ((*tmp++ = *src++) != '\0')
			if (--n == 0)
				return dest;
		do {
			*tmp++ = '\0';
		} while (--n);
	}
	return dest;
}

/* Copy no more than N-1 characters of SRC to DEST. DEST always NUL terminates.
 * \return strlen(src), if retval >= n truncation occured
 * */
small_int_t strlcpy (unsigned char *dest, const unsigned char *src, size_t l)
{
    register unsigned char *tmp = dest;
    register size_t count = l;

    if (count) {
        while ((*tmp++ = *src++) != '\0')
            if (--count == 0) {
                --tmp;
                *tmp = '\0';
                return l-count;
            }
    }
    return l-count;
}
