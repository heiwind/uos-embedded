#ifndef	_UOS_ARPA_INET_H
#define	_UOS_ARPA_INET_H	1

#include <net/ip.h>

#ifdef __cplusplus
extern "C" {
#endif



/** returns the dots-and-numbers string in a static buffer that is overwritten 
 * with each call to the function.
 * */
__noexcept char*   inet_iptoasn( ip_addr ip, char *cp, unsigned cplen) __NOTHROW;
__noexcept int     inet_atoip(const char *cp, ip_addr *ip) __NOTHROW;



#ifdef __cplusplus
}
#endif

#endif
