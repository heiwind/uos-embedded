#ifndef	_UOS_ARPA_INET_H
#define	_UOS_ARPA_INET_H	1

#include <net/ip.h>

#ifdef __cplusplus
extern "C" {
#endif



/** returns the dots-and-numbers string in a static buffer that is overwritten 
 * with each call to the function.
 * */
char*   inet_iptoasn( ip_addr ip, char *cp, unsigned cplen) __THROW;
int     inet_atoip(const char *cp, ip_addr *ip) __THROW;



#ifdef __cplusplus
}
#endif

#endif
