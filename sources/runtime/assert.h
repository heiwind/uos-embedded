#ifndef __UOS_ASSERT__H_
#define __UOS_ASSERT__H_ 1

#include <runtime/sys/uosc.h>



#ifdef __cplusplus
extern "C" {
#endif

/* This prints an "Assertion failed" message and aborts. */
#ifdef NDEBUG
#   define __assert_fail(a,b,c,d) __assert_fail_ndebug()
#   define __assert_msg(msg, ...)
    void __assert_fail_ndebug ();
#else
    void __assert_fail (const char *expr, const char *file,
                        unsigned line, const char *func);// __NORETURN;
    void __assert_msg(const char *msg, ...);
#endif

#ifdef __cplusplus
}
#endif

/*
 * Permanent assertion, independent of NDEBUG macro.
 * Example:
 *	assert_always (ptr != NULL);
 */

#define assert_always(condition) do {			\
	if (__builtin_expect (! (condition), 0))	\
	    { __assert_fail (#condition, __FILE__,	\
			__LINE__, __PRETTY_FUNCTION__);	\
          __builtin_unreachable ();         \
	    }\
	} while (0)

#define assertmsg_always(condition, msg, ...) do {           \
    if (__builtin_expect (! (condition), 0))    \
         {__assert_msg  (msg, __VA_ARGS__);      \
         __assert_fail (#condition, __FILE__,   \
            __LINE__, __PRETTY_FUNCTION__);     \
         __builtin_unreachable ();              \
         }\
    } while (0)

/*
 * void assert (int expression);
 *
 * If NDEBUG is defined, do nothing.
 * If not, and EXPRESSION is zero, print an error message and abort.
 */
#ifdef NDEBUG
#   define assert(expr)
#   define assert2(expr, msg, ...)
#else
#   define assert(expr)	assert_always(expr)
#   define assert2(expr, msg, ...) assertmsg_always(expr, msg, __VA_ARGS__)
#endif

#endif /* __UOS_ASSERT__H_ */
