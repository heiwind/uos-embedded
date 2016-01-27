#ifndef __POSIX_PORT__
#define __POSIX_PORT__

#ifdef __cplusplus
extern "C" {
#endif



//#define POSIX_memory	uos_memory

#define UOS_USLEEP_STYLE_DELAY          0
#define UOS_USLEEP_STYLE_ETIEMER_SLEEP  1
#define UOS_USLEEP_STYLE    UOS_USLEEP_STYLE_DELAY

//#define __UOS_STDIO__ __UOS_STDIO_IS_???

//#define POSIX_timer uos_timer

//INLINE unixtime32_t time(unixtime32_t* t) __THROW ???
//#define UOS_HAVE_UNIXTIME

//#define TASK_PRIORITY_MAX 100
//#define TASK_PRIORITY_MIN 0

//#define _SC_PAGE_SIZE

#ifdef __cplusplus
}
#endif

#endif //__POSIX_PORT__

