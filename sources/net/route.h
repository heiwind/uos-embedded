#ifndef __ROUTE_H_
#define	__ROUTE_H_ 1

#include <net/netif.h>
#include <net/ip.h>

#ifdef __cplusplus
extern "C" {
#endif



struct _netif_t;
struct _ip_t;

typedef struct _route_t {
	struct _route_t *next;
	struct _netif_t	*netif;
	unsigned char	ipaddr [4];
	unsigned char	broadcast [4];
	unsigned char	netaddr [4];
	unsigned char	gateway [4];
	unsigned char	gwifaddr [4];
	unsigned char	masklen;
} route_t;

struct _netif_t *route_lookup (struct _ip_t *ip, const unsigned char *ipaddr
        , const unsigned char **gateway
        , const unsigned char **netif_ipaddr);
struct _netif_t *route_lookup_self (struct _ip_t *ip, unsigned char *ipaddr,
	unsigned char *broadcast);
const unsigned char *route_lookup_ipaddr (struct _ip_t *ip, const unsigned char *ipaddr,
	struct _netif_t *netif);

/*
 * Add interface record to the list.
 */
void route_add_netif (struct _ip_t *ip, struct _route_t *r,
	const unsigned char *ipaddr, unsigned char masklen
	, struct _netif_t *netif);
bool_t route_add_gateway (struct _ip_t *ip, struct _route_t *r,
	unsigned char *ipaddr, unsigned char masklen, unsigned char *gateway);

/*
 * There are two types of routing records:
 * 1) For every real network interface IP address (alias);
 * 2) For every additional route.
 * Records of type 1 have gateway[0] = 0.
 * Records of type 2 have real gateway addresses.
 */
void route_setup (struct _ip_t *ip, struct _route_t *r
                , const unsigned char *ipaddr
                , unsigned char masklen
	            , const unsigned char *gateway);



#ifdef __cplusplus
}
#endif

#endif /* !__ROUTE_H_ */
