#ifndef __ARP_H_
#define	__ARP_H_ 1

#include <net/ip.h>
#include <net/ethernet.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



//*****************************************************************************
//                               ARP entry
//*****************************************************************************
typedef struct _arp_entry_t {
	struct _netif_t	*netif;
	ip_addr         ipaddr;
	mac_addr        ethaddr;
	unsigned char	age;
} arp_entry_t;

typedef struct _arp_t {
	struct _ip_t	*ip;
	//changes on every table change
	unsigned        stamp;
	unsigned char	size;
	unsigned char	timer;
	arp_entry_t	table [1];
} arp_t;

//*********************    ARP headers       **********************************

struct arp_hdr {
	struct eth_hdr	eth;		/* ethernet header */

	unsigned short	hwtype;		/* hardware address type */
#define HWTYPE_ETHERNET	HTONS (1)

	unsigned short	proto;		/* protocol type */

	unsigned char	hwlen;		/* hardware address length */
	unsigned char	protolen;	/* protocol address length */

	unsigned short	opcode;		/* ARP operation code */
#define ARP_REQUEST	HTONS (1)
#define ARP_REPLY	HTONS (2)

	unsigned char	src_hwaddr [6];	/* source ethernet address */
	unsigned char	src_ipaddr [4];	/* source IP address */
	unsigned char	dst_hwaddr [6];	/* dest. ethernet address */
	unsigned char	dst_ipaddr [4];	/* dest. IP address */
} __attribute__ ((packed));



arp_t *arp_init (array_t *buf, unsigned bytes, struct _ip_t *ip);
struct _buf_t *arp_input (struct _netif_t *netif, struct _buf_t *p);
bool_t arp_request (struct _netif_t *netif, struct _buf_t *p,
        ip_addr_const ipdest, ip_addr_const ipsrc);
bool_t arp_add_header (struct _netif_t *netif, struct _buf_t *p,
        ip_addr_const ipdest
	, const unsigned char *ethdest);
unsigned char *arp_lookup (struct _netif_t *netif, ip_addr_const ipaddr);
void arp_timer (arp_t *arp);



#ifdef __cplusplus
}
#endif

#endif /* !__ARP_H_ */
