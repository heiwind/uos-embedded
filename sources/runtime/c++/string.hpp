#ifndef UOS_STRINGS_HPP
#define UOS_STRINGS_HPP

/*
  *  Created on: 06.10.2015
 *      Author: a_lityagin <alexraynepe196@gmail.com>
 *                         <alexraynepe196@hotbox.ru>
 *                         <alexraynepe196@mail.ru>
 *                         <alexrainpe196@hotbox.ru>

 * This file is proted from the Contiki operating system.
 *      Author: Adam Dunkels <adam@sics.se>
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 
 * this file provides c++ wraps on <string> routines with char* type  
 */

#ifdef __cplusplus
extern "C++" {
namespace std {
  namespace string {

    /** C++ have no deafult cast char* <-> uchar*
     * */

    /* Return the length of S.  */
    inline 
    size_t strlen (const char *s)
        {return ::strlen((unsigned char*)s);};

    /* Find the length of STRING, but scan at most MAXLEN characters.
       If no '\0' terminator is found in that many characters, return MAXLEN.  */
    inline 
    size_t strnlen (const char *string, size_t maxlen)
        {return ::strnlen((const unsigned char*)string, maxlen);};

    /* Copy SRC to DEST.  */
    inline 
    char * strcpy (char *dest, const char *src)
        {return (char*)::strcpy((unsigned char*)dest, (const unsigned char*)src);};

    /* Copy no more than N characters of SRC to DEST.  */
    inline 
    char * strncpy (char *dest, const char *src, size_t n)
        {return (char*)::strncpy((unsigned char*)dest, (const unsigned char*)src, n);};

    /* Append SRC onto DEST.  */
    inline 
    char * strcat (char *dest, const char *src)
        {return (char*)::strcat((unsigned char*)dest, (const unsigned char*)src);};

    /* Append no more than N characters from SRC onto DEST.  */
    inline 
    char * strncat (char *dest, const char *src, size_t n)
        {return (char*)::strncat((unsigned char*)dest, (const unsigned char*)src, n);};

    /* Compare S1 and S2.  */
    inline 
    small_int_t strcmp (const char *s1, const char *s2)
        {return ::strcmp((const unsigned char*)s1, (const unsigned char*)s2);};

    inline
    small_int_t strcasecmp (const char *s1, const char *s2)
        {return ::strcasecmp((const unsigned char*)s1, (const unsigned char*)s2);};

    /* Compare N characters of S1 and S2.  */
    inline 
    small_int_t strncmp (const char *s1, const char *s2, size_t n)
        {return ::strncmp((const unsigned char*)s1, (const unsigned char*)s2, n);};

    inline 
    small_int_t strnzcmp (const char *s1, const char *s2, size_t n)
        {return ::strnzcmp((const unsigned char*)s1, (const unsigned char*)s2, n);};

    inline
    small_int_t strncasecmp (const char *s1, const char *s2, size_t n)
        {return ::strncasecmp((const unsigned char*)s1, (const unsigned char*)s2, n);};

    /* Find the first occurrence of C in S.  */
    inline 
    char * strchr (const char *s, char c)
        {return (char*)::strchr((const unsigned char*)s, (unsigned char)c);};

    /* Find the last occurrence of C in S.  */
    inline 
    char * strrchr (const char *src, char c)
        {return (char*)::strrchr((const unsigned char*)src, (unsigned char)c);};

    inline 
    char *strstr (const char *haystack, const char *needle)
        {return (char*)::strstr((const unsigned char*)haystack
                              , (unsigned char*)needle
                              );
        };

    inline 
    small_int_t strspn (const char *s, const char *accept)
        {return ::strspn((const unsigned char*)s
                              , (const unsigned char*)accept
                              );
        };

    inline 
    small_int_t strcspn (const char *s, const char *reject)
        {return ::strspn((const unsigned char*)s
                              , (const unsigned char*)reject
                              );
        };

    inline 
    const char *strmatch (const char* string, const char* pattern)
        {return (const char*)::strmatch((unsigned char*)string
                                    , (const unsigned char*)pattern
                                    );
        };
    
  }// namespace string
  using namespace string;
} // namespace std::string
} // extern "C++"
using namespace std::string;
#endif

#endif  // UOS_STRINGS_HPP

