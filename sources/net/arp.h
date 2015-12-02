#ifndef __ARP_H_
#define	__ARP_H_ 1

#include <net/ip.h>

#ifdef __cplusplus
extern "C" {
#endif



//*****************************************************************************
//                               MAC adress
//*****************************************************************************
typedef union __attribute__ ((packed,aligned(4))) _mac_addr{
    struct {
        uint32_t    l;
        uint16_t    h;
    }               val;
    char            cs[6];
    unsigned char   ucs[6];
} mac_addr;

INLINE __CONST 
mac_addr macadr_4l(unsigned h, uint32_t l) __THROW 
{
    mac_addr res;
    res.val.l = l;
    res.val.h = h;
    return res;
}

INLINE __CONST 
mac_addr macadr_4ucs(const unsigned char* __restrict__ x) __THROW 
{
    mac_addr res;
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( ((uintptr_t)x&CPU_ACCESSW_ALIGNMASK) == 0){
        const mac_addr* tmp = (const mac_addr*)x;
        res.val.l = tmp->val.l;
        res.val.h = tmp->val.h;
        return res;
    }
#endif
    res.ucs[0] = x[0];
    res.ucs[1] = x[1];
    res.ucs[2] = x[2];
    res.ucs[3] = x[3];
    res.ucs[4] = x[4];
    res.ucs[5] = x[5];
    return res;
}

INLINE mac_addr macadr_assign(      mac_addr* __restrict__ dst
                            , const mac_addr* __restrict__ src
                            ) __THROW
{
    dst->val.l = src->val.l;
    dst->val.h = src->val.h;
    return *dst;
}

INLINE unsigned char* macadr_assign_ucs(      unsigned char* __restrict__ dst
                                , const unsigned char* __restrict__ src
                                ) __THROW
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( (((uintptr_t)dst|(uintptr_t)src)&CPU_ACCESSW_ALIGNMASK) == 0){
        mac_addr* __restrict__ dmac = (mac_addr*)dst; 
        mac_addr* __restrict__ smac = (mac_addr*)src; 
        dmac->val.l = smac->val.l;
        dmac->val.h = smac->val.h;
        return dst;
    }
#endif
    memcpy(dst,src,6);
    return dst;
}

INLINE 
bool_t __CONST macadr_is_same(    const mac_addr a
                                , const mac_addr b
                                ) __THROW
{
        return (a.val.l == b.val.l) && (a.val.h == b.val.h);
}

INLINE 
bool_t macadr_is_same_ucs( const unsigned char* __restrict__ a
                        ,   const unsigned char* __restrict__ b
                        ) __THROW
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( (((uintptr_t)a|(uintptr_t)b)&CPU_ACCESSW_ALIGNMASK) == 0){
        const mac_addr* __restrict__ maca = (const mac_addr*) a;
        const mac_addr* __restrict__ macb = (const mac_addr*) b;
        return macadr_is_same(*maca, *macb);
    }
#endif
    return (memcmp(a,b,6) == 0)?true:false;
}

INLINE  
bool_t __CONST macadr_not0(const mac_addr a) __THROW
{
        return ((a.val.l|a.val.h) != 0)? true : false;
}

INLINE  
bool_t macadr_not0_ucs(const unsigned char* a) __THROW
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( ((uintptr_t)a&CPU_ACCESSW_ALIGNMASK) == 0){
        const mac_addr* tmp = (const mac_addr*)a;
        return macadr_not0(*tmp);
    }
#endif
    unsigned l = 6;
    for(l = 6; l > 0; l--, a++)
        if (*a != 0)
            return true;
    return false;
}



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
	unsigned char	size;
	unsigned char	timer;
	arp_entry_t	table [1];
} arp_t;

/* MUST be compiled with "pack structs" or equivalent! */
struct eth_hdr {
	unsigned char	dest [6];	/* destination MAC address */
	unsigned char	src [6];	/* source MAC address */

	unsigned short	proto;		/* protocol type */
#define PROTO_ARP	HTONS (0x0806)
#define PROTO_IP	HTONS (0x0800)
} __attribute__ ((packed));

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

struct ethip_hdr {
	/* Ethernet header */
	struct eth_hdr	eth;

	/* IP header */
	unsigned char	ip_hdr0 [4];
	unsigned char	ip_hdr1 [4];
	unsigned char	ip_hdr2 [4];
	unsigned char	ip_src [4];
	unsigned char	ip_dst [4];
} __attribute__ ((packed));

arp_t *arp_init (array_t *buf, unsigned bytes, struct _ip_t *ip);
struct _buf_t *arp_input (struct _netif_t *netif, struct _buf_t *p);
bool_t arp_request (struct _netif_t *netif, struct _buf_t *p,
	const unsigned char *ipdest, const unsigned char *ipsrc);
bool_t arp_add_header (struct _netif_t *netif, struct _buf_t *p,
	const unsigned char *ipdest
	, const unsigned char *ethdest);
unsigned char *arp_lookup (struct _netif_t *netif, const unsigned char *ipaddr);
void arp_timer (arp_t *arp);



#ifdef __cplusplus
}
#endif

#endif /* !__ARP_H_ */
