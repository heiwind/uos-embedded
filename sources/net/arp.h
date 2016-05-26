#ifndef __ARP_H_
#define	__ARP_H_ 1

#include <net/ip.h>
#include <stdint.h>

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

INLINE
mac_addr __CONST macadr_4l(unsigned h, uint32_t l)
{
    mac_addr res;
    res.val.l = l;
    res.val.h = h;
    return res;
}

INLINE
mac_addr __PURE macadr_4ucs(const unsigned char* x)
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
#if UOS_FOR_SIZE > 0
    memcpy(res.ucs, x, 6);
#elif defined (__AVR__)
    unsigned char* tmp = res.ucs;
    *tmp++ = *x++;
    *tmp++ = *x++;
    *tmp++ = *x++;
    *tmp++ = *x++;
    *tmp++ = *x++;
    *tmp++ = *x++;
#else
    res.ucs[0] = x[0];
    res.ucs[1] = x[1];
    res.ucs[2] = x[2];
    res.ucs[3] = x[3];
    res.ucs[4] = x[4];
    res.ucs[5] = x[5];
#endif
    return res;
}

INLINE
mac_addr macadr_assign(      mac_addr* __restrict__ dst
                            , const mac_addr* __restrict__ src
                            )
{
    dst->val.l = src->val.l;
    dst->val.h = src->val.h;
    return *dst;
}

INLINE
unsigned char* macadr_assign_ucs(      unsigned char* __restrict__ dst
                                , const unsigned char* __restrict__ src
                                )
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
#if UOS_FOR_SIZE > 0
    memcpy(dst, src, 6);
#elif defined (__AVR__)
    unsigned char* tmp = res.ucs;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
#else
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
#endif
    return dst;
}

INLINE
bool_t __PURE macadr_is_same(    const mac_addr* a
                                , const mac_addr* b
                                )
{
        return (a->val.l == b->val.l) && (a->val.h == b->val.h);
}

INLINE __NOTHROW
bool_t __PURE macadr_is_same_ucs( const unsigned char* __restrict__ a
                        ,   const unsigned char* __restrict__ b
                        ) __noexcept
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( (((uintptr_t)a|(uintptr_t)b)&CPU_ACCESSW_ALIGNMASK) == 0){
        const mac_addr* __restrict__ maca = (const mac_addr*) a;
        const mac_addr* __restrict__ macb = (const mac_addr*) b;
        return macadr_is_same(maca, macb);
    }
#endif
#if UOS_FOR_SIZE > 0
    return (memcmp(a,b,6) == 0);
#elif defined (__AVR__)
    if (*a++ == *b++)
    if (*a++ == *b++)
    if (*a++ == *b++)
    if (*a++ == *b++)
    if (*a++ == *b++)
    if (*a++ == *b++)
        return true;
    return false;
#else //ifdef MIPS32
    unsigned tmp; 
    tmp =  (a[0] - b[0]);
    tmp |= (a[1] - b[1]);
    tmp |= (a[2] - b[2]);
    tmp |= (a[3] - b[3]);
    tmp |= (a[4] - b[4]);
    tmp |= (a[5] - b[5]);
    return (tmp == 0);
#endif
}

INLINE
bool_t __CONST macadr_not0(const mac_addr a)
{
        return ((a.val.l|a.val.h) != 0)? true : false;
}

INLINE __NOTHROW
bool_t __PURE macadr_not0_ucs(const unsigned char* a) __noexcept
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( ((uintptr_t)a&CPU_ACCESSW_ALIGNMASK) == 0){
        const mac_addr* tmp = (const mac_addr*)a;
        return macadr_not0(*tmp);
    }
#endif
#if UOS_FOR_SIZE > 0
    unsigned l = 6;
    for(l = 6; l > 0; l--, a++)
        if (*a != 0)
            return true;
    return false;
#elif defined (__AVR__)
    if (*a++ == 0)
    if (*a++ == 0)
    if (*a++ == 0)
    if (*a++ == 0)
    if (*a++ == 0)
    if (*a++ == 0)
        return false;
    return true;
#else
    unsigned tmp = a[0] | a[1] | a[2] | a[3] | a[4] | a[5];
    return tmp != 0;
#endif
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
	//changes on every table change
	unsigned        stamp;
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
#define MAC_HLEN     14      /* ETH MAC header length */

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

struct ethip_hdr {
	/* Ethernet header */
	struct eth_hdr	eth;

	/* IP header */
	unsigned char	ip_hdr0 [4];
	unsigned char	ip_hdr1 [4];
	unsigned char	ip_hdr2 [4];
	ip_addr         ip_src;
	ip_addr         ip_dst;
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
