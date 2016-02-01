#ifndef	_ARPA_INET_H
#define	_ARPA_INET_H	1

#include <runtime/sys/uosc.h>
#include <net/ip.h>
#include <net/arpa/inet.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif



/** returns the dots-and-numbers string in a static buffer that is overwritten 
 * with each call to the function.
 * */
INLINE __NOTHROW
char*   inet_ntoa( in_addr ip) __noexcept
{
    return inet_iptoasn(ipadr_4l(ip.s_addr) , (char*)NULL, 0);
}

INLINE __NOTHROW
int     inet_aton(const char *cp, in_addr *ip) __noexcept
{
    ip_addr res;
    res.val = 0;
    in_addr_t ok = inet_atoip(cp, &res);
    if (ok != 0)
        ip->s_addr = res.val;
    return res.val;
}

INLINE __NOTHROW
in_addr_t inet_addr (const char *__cp) __noexcept
{
    ip_addr res;
    res.val = 0;
    in_addr_t ok = inet_atoip(__cp, &res);
    if (ok != 0)
        return HTONL(res.val);
    else
        return -1;
}



#ifdef __cplusplus
}
#endif

#endif
