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
INLINE 
char*   inet_ntoa( in_addr ip) __THROW
{
    return inet_iptoasn(ipadr_4l(ip.s_addr) , (char*)NULL, 0);
}

INLINE 
int     inet_aton(const char *cp, in_addr *ip) __THROW
{
    ip_addr res;
    res.val = 0;
    in_addr_t ok = inet_atoip(cp, &res);
    if (ok != 0)
        ip->s_addr = res.val;
    return res.val;
}

in_addr_t inet_addr (const char *__cp) __THROW
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
