#ifndef __IP_H_
#define __IP_H_ 1

//need types.h
#include <runtime/arch.h>
#include <runtime/sys/uosc.h>
#include <kernel/uos.h>
#include <uos-conf-net.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



#ifndef IP_STACKSZ
#   ifdef __AVR__
#      define IP_STACKSZ	0x400
#   endif
#   ifdef MSP430
#      define IP_STACKSZ	500
#   endif
#   ifdef I386
#      define IP_STACKSZ	0x800
#   endif
#   ifdef MIPS32
#      define IP_STACKSZ	1500
#   endif
#   ifdef LINUX386
#      define IP_STACKSZ	4000
#   endif
#   if defined (__arm__) || defined (__thumb__)
#      define IP_STACKSZ	0x600
#   endif
#endif



//*****************************************************************************
//                               IP4 adress
//*****************************************************************************
typedef uint32_t ip_addr_t;
typedef union  __attribute__ ((packed,aligned(4))) _ip_addr{
    ip_addr_t       val;
    char            cs[4];
    unsigned char   ucs[4];
#ifdef __AVR__
    //на малых платформах, 16бит-адрес выгоднее использовать чем 32бит значение
    ip_addr_t       var[1];
#else
    ip_addr_t       var;
#endif
} ip_addr;


//ссылки и константы - это ссылки на ип-адреса. на малых платформах (16бит-адрес)
//  в этом качестве используеются указатель, на нормальных - значение. 
#ifdef __AVR__
//это рекомендуемое выравнивание для данных отсылаемого пакета 
//  с этим же выравниванием выдаются принятые данные  
#define IP_PAYLOAD_ALIGN    1
#define IPREF_IS_ADDR
typedef unsigned char* ip_addr_ref;
typedef const unsigned char* ip_addr_const;
#define ipref_as_ucs(x) x
#else
//это рекомендуемое выравнивание для данных отсылаемого пакета 
//  с этим же выравниванием выдаются принятые данные  
#define IP_PAYLOAD_ALIGN    4U
#define IPREF_IS_VAL
typedef ip_addr_t       ip_addr_ref;
typedef ip_addr_t       ip_addr_const;

#define ipref_as_ucs(x) ((unsigned char*)&x)

#define ipref_4ucs(x)   ipadr_4ucs(x).var
#define ipref_4l(x)     (x)

#define ipadr_assignref_ucs(dst, x) ipadr_assignl_ucs(dst, x)
#define ipadr_assignref(dst, x)     ipadr_assign_l(dst, x)
#define ipadr_is_same_ref(a,x)      ipadr_is_same_l(a,x)
#endif

#define IP_ALIGNED(x) ( ((x) + IP_PAYLOAD_ALIGN-1U) & ~(IP_PAYLOAD_ALIGN-1U) ) 

//ip adress in network byte-order
typedef ip_addr ip_naddr;
typedef const unsigned char* const_ip4_ucs;

INLINE
ip_addr __CONST ipadr_4l(uint32_t x)
{
    ip_addr res;
    res.val = x;
    return res;
}

INLINE
ip_addr __PURE ipadr_4ucs(const unsigned char* x)
{
    ip_addr res;
#if CPU_ACCESSW_ALIGNMASK > 0
    if (( (uintptr_t)x &CPU_ACCESSW_ALIGNMASK) == 0){
        res.val = *((const ip_addr_t*)x);
        return res;
    }
#endif
    // на платформе с адресацией слов с обязательным выраниванием
    //  невыравненые данные вызовут исключение 
    //res.val = x[0]|(x[1]<<8)|(x[2]<<16)|(x[3]<<24);
    res.val = (x[3]<<8) | x[2];
    res.val = (res.val <<8) | x[1];
    res.val = (res.val <<8) | x[0];
    return res;
}

INLINE
ip_addr ipadr_assign(        ip_addr* __restrict__ dst
                            , const ip_addr* __restrict__ src
                            )
{
    dst->val = src->val;
    return *dst;
}

INLINE
ip_addr ipadr_assign_l(        ip_addr*  dst
                            , const ip_addr_t x
                            )
{
    dst->val = x;
    return *dst;
}

INLINE __NOTHROW
unsigned char* ipadr_assign_ucs(        unsigned char* __restrict__ dst
                                , const unsigned char* __restrict__ src
                                )
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ((((uintptr_t)dst|(uintptr_t)src)&CPU_ACCESSW_ALIGNMASK) == 0){
        ((ip_addr*)dst)->val = ((ip_addr*)src)->val;
        return dst;
    }
#endif
#if UOS_FOR_SIZE > 0
    memcpy(dst, src, 4);
#elif defined (__AVR__)
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
#else //ifdef MIPS32
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
#endif
    return dst;
}

INLINE __NOTHROW
unsigned char* ipadr_assignl_ucs( unsigned char* dst
                                        , ip_addr_t x
                                ) __noexcept
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( ((uintptr_t)dst & CPU_ACCESSW_ALIGNMASK) == 0){
        ((ip_addr*)dst)->val = x;
        return dst;
    }
#endif
    dst[0] = x&0xff;
    x >>= 8;
    dst[1] = x&0xff;
    x >>= 8;
    dst[2] = x&0xff;
    x >>= 8;
    dst[3] = x&0xff;
    return dst;
}

//!!! ip==0 - дает всегда true в сравнении
#ifdef IPREF_IS_VAL
INLINE
bool_t __CONST ipadr_is_same(     ip_addr_const a_or0
                                , ip_addr_const b
                                )
{
        if (a_or0 != 0)
        if (b != 0)
            return (a_or0 == b);
        return true;
}

INLINE
bool_t __CONST ipadr_is_same_l(   ip_addr_const a
                                , const ip_addr_t b
                                )
{
        if (a != 0)
        if (b != 0)
            return (a == b);
        return true;
}
#endif

INLINE __NOTHROW
bool_t __PURE ipadr_is_same_ucs( const unsigned char* __restrict__ a
                        , const unsigned char* __restrict__ b
                        )  __noexcept
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( (((uintptr_t)a|(uintptr_t)b)&CPU_ACCESSW_ALIGNMASK) == 0){
#       ifdef IPREF_IS_VAL
        ip_addr_const* __restrict__ ipa = (ip_addr_const*) a;
        ip_addr_const* __restrict__ ipb = (ip_addr_const*) b;
        return ipadr_is_same(*ipa, *ipb);
#       endif
    }
#endif
#if UOS_FOR_SIZE > 0
    return (memcmp(a,b,4) == 0);
#elif defined (__AVR__)
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
    return (tmp == 0);
#endif
}

#ifdef IPREF_IS_VAL
// тоже самое, но false eсли  a==NULL
INLINE __NOTHROW
bool_t __CONST ipadr_or0_is_same( ip_addr_const  a_or0
                        , ip_addr_const  b
                        ) __noexcept
{
    if (a_or0 != 0)
        return ipadr_is_same(a_or0, b);
    else
        return false;
}

// тоже самое, но true eсли  a==NULL
INLINE __NOTHROW
bool_t __CONST ipadr_is_same_or0( ip_addr_const  a_or0
                        , ip_addr_const  b
                        ) __noexcept
{
    if (a_or0 != 0)
        return ipadr_is_same(a_or0, b);
    else
        return true;
}
#endif

INLINE __NOTHROW
bool_t __PURE ipadr_or0_is_same_ucs( const unsigned char* __restrict__ a_or0
                            , const unsigned char* __restrict__ b
                            ) __noexcept
{
    if (a_or0 != NULL)
        return ipadr_is_same_ucs(a_or0, b);
    else
        return false;
}


INLINE __NOTHROW
bool_t __PURE ipadr_is_same_or0_ucs( const unsigned char* __restrict__ a_or0
                            , const unsigned char* __restrict__ b
                            ) __noexcept
{
    if (a_or0 != NULL)
        return ipadr_is_same_ucs(a_or0, b);
    else
        return true;
}

#ifdef IPREF_IS_VAL
INLINE
bool_t __CONST ipadr_not0(ip_addr_const a)
{
        return (a != 0)? true : false;
}

INLINE
bool_t __CONST ipadr_is_broadcast(ip_addr_const a)
{
        return ((a == 0) || (~a == 0)) ? true : false;
}
#endif

INLINE
bool_t __PURE ipadr_not0_ucs(const unsigned char* a)
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( ((uintptr_t)a&CPU_ACCESSW_ALIGNMASK) == 0){
        return (((ip_addr*)a)->val != 0)? true : false;
    }
#endif
#if defined (__AVR__)
    unsigned tmp = *a++ | *a++ | *a++ | *a++;
#else
    unsigned tmp = a[0] | a[1] | a[2] | a[3];
#endif
    return tmp != 0;
}

INLINE
bool_t __PURE ipadr_is_broadcast_ucs(const unsigned char* a)
{
#if CPU_ACCESSW_ALIGNMASK > 0
    if ( ((uintptr_t)a&CPU_ACCESSW_ALIGNMASK) == 0){
        return (((ip_addr*)a)->val != 0)? true : false;
    }
#endif
#if defined (__AVR__)
    unsigned tmp = *a++ | *a++ | *a++ | *a++;
#else
    unsigned tmp0 = a[0];
    unsigned tmpf = a[0];
    tmp0 |= a[1];
    tmpf &= a[1];
    tmp0 |= a[2];
    tmpf &= a[2];
    tmp0 |= a[3];
    tmpf &= a[3];
#endif
    return (tmp0 == 0) || (tmpf == 0xff);
}

/** надо стараться придерживаться этого шаблона сокета, для создания протокольных сокетов
 * */

struct _ip_t;

#define UOSIP_BASE_SOCKET(sock_type) \
        mutex_t     lock;\
        struct _ip_t          *ip;\
        sock_type*          next;\
\
        ip_addr_const   local_ip;\
        unsigned short  local_port;\
        ip_addr         peer_ip;\
        unsigned short  peer_port;

typedef struct _base_socket_t {
    UOSIP_BASE_SOCKET(struct _base_socket_t);
} base_socket_t;

#include <net/errors.h> 


typedef struct _ip_t {
	mutex_t		lock;
	mutex_group_t	*netif_group;	/* list of network drivers */
	struct _mem_pool_t *pool;	/* pool for memory allocation */
	struct _timer_t *timer;		/* timer driver */
	struct _route_t *route;		/* routing table */
	struct _arp_t	*arp;		/* ARP protocol data */
	bool_t		forwarding;	/* forwarding enabled */
	small_uint_t	default_ttl;	/* default time-to-live value */
	small_uint_t	tos;		/* type of service value */
	unsigned	id;		/* output packet number */

	/*
	 * UDP
	 */
	struct _udp_socket_t *udp_sockets;	/* list of UDP sockets */

	/*
	 * TCP
	 */
	struct _tcp_socket_t *tcp_listen_sockets;	/* LISTEN state */
	struct _tcp_socket_t *tcp_sockets;		/* active sockets */
	struct _tcp_socket_t *tcp_closing_sockets;	/* TIME-WAIT state */

	/* Incremented every coarse grained timer shot
	 * (typically every 500 ms, determined by TCP_COARSE_TIMEOUT). */
	unsigned long	tcp_ticks;

	unsigned char	tcp_timer;
	unsigned short	tcp_port;	/* local port number to allocate */
	unsigned long	tcp_seqno;	/* initial sequence number for
					 * new TCP connections */

	/* These variables are global to all functions involved in the input
	 * processing of TCP segments. Set by the tcp_input() function. */
	struct _tcp_socket_t *tcp_input_socket;
	unsigned short	tcp_input_len;
	unsigned char	tcp_input_flags;
	unsigned long	tcp_input_seqno;
	unsigned long	tcp_input_ackno;

	/*
	 * IP statistics.
	 */
	unsigned long	in_receives;	/* total input packets */
	unsigned long	in_hdr_errors;	/* input errors: checksum, version,
					   length, IP options, fragmentation */
	unsigned long	in_addr_errors;	/* received packet was not for us */
	unsigned long	in_discards;	/* ignored input packets, due to
					   lack of memory */
	unsigned long	in_unknown_protos; /* input packets with unsupported
					   transport protocols */
	unsigned long	in_delivers;	/* successfully delivered packets */
	unsigned long	out_requests;	/* output requests */
	unsigned long	out_discards;	/* lost output packets due
					   to not enough memory */
	unsigned long	out_no_routes;	/* lost output packets due to
					   no route to host */
	unsigned long	forw_datagrams;	/* forwarded packets */

	/*
	 * ICMP statistics.
	 */
        unsigned long   icmp_in_msgs;		/* total input packets */
        unsigned long   icmp_in_errors;		/* invalid input packets */
        unsigned long   icmp_in_dest_unreachs;	/*  */
        unsigned long   icmp_in_time_excds;	/*  */
        unsigned long   icmp_in_parm_probs;	/*  */
        unsigned long   icmp_in_src_quenchs;	/*  */
        unsigned long   icmp_in_redirects;	/*  */
        unsigned long   icmp_in_echos;		/* input echos */
        unsigned long   icmp_in_echo_reps;	/*  */
        unsigned long   icmp_in_timestamps;	/*  */
        unsigned long   icmp_in_timestamp_reps;	/*  */
        unsigned long   icmp_in_addr_masks;	/*  */
        unsigned long   icmp_in_addr_mask_reps;	/*  */
        unsigned long   icmp_out_msgs;		/* total output packets */
        unsigned long   icmp_out_errors;	/* output errors */
        unsigned long   icmp_out_dest_unreachs;	/* output destination unreachables */
        unsigned long   icmp_out_time_excds;	/* output time exceeds */
        unsigned long   icmp_out_parm_probs;	/*  */
        unsigned long   icmp_out_src_quenchs;	/*  */
        unsigned long   icmp_out_redirects;	/*  */
        unsigned long   icmp_out_echos;		/*  */
        unsigned long   icmp_out_echo_reps;	/* output echo replies */
        unsigned long   icmp_out_timestamps;	/*  */
        unsigned long   icmp_out_timestamp_reps; /*  */
        unsigned long   icmp_out_addr_masks;	/*  */
        unsigned long   icmp_out_addr_mask_reps; /*  */

	/*
	 * UDP statistics.
	 */
	unsigned long   udp_out_datagrams; /* total output packets */
	unsigned long	udp_in_datagrams; /* total input packets */
	unsigned long   udp_in_errors;	/* input packets with invalid length,
					   checksum errors or socket overflow */
	unsigned long   udp_no_ports;	/* no listener on port */

	/*
	 * TCP statistics.
	 */
	unsigned long   tcp_out_datagrams; /* total output packets */
	unsigned long   tcp_out_errors;	/* output errors */

	unsigned long	tcp_in_datagrams; /* total input packets */
	unsigned long   tcp_in_errors;	/* input packets with invalid length,
					   checksum errors or socket overflow */
	unsigned long	tcp_in_discards; /* ignored input packets, due to
					   lack of memory */

	ARRAY (stack, IP_STACKSZ);	/* task stack */
} ip_t;

typedef struct  __attribute__ ((packed)) _ip_hdr_t {
	unsigned char	version;	/* version / header length */
	unsigned char	tos;		/* type of service */
	unsigned char	len_h, len_l;	/* total length */
	unsigned char	id_h, id_l;	/* identification */

	unsigned char	offset_h, offset_l; /* fragment offset field */
#define IP_DF		0x40		/* dont fragment flag */
#define IP_MF		0x20		/* more fragments flag */
#define IP_OFFMASK	0x1f		/* mask for fragmenting bits */

	unsigned char	ttl;		/* time to live */

	unsigned char	proto;		/* protocol */
#define IP_PROTO_ICMP	1
#define IP_PROTO_TCP	6
#define IP_PROTO_UDP	17

	unsigned char	chksum_h, chksum_l; /* checksum */

	ip_addr         src;	/* source destination IP address */
	ip_addr         dest;	/* destination IP address */
} ip_hdr_t;

#define IP_HLEN		20		/* IP header length */
#define IP_MAXPACKET	1500		/* max packet size after reassemble */

#define IP_ADDR(val)	({unsigned long addr = val; (unsigned char*) &addr; })

#define IS_BROADCAST(ipaddr)	(memcmp (ipaddr, IP_ADDR(0xffffffff), 4) == 0 ||\
				 memcmp (ipaddr, IP_ADDR(0), 4) == 0)
#define IS_MULTICAST(ipaddr)	((ipaddr[0] & 0xf0) == 0xe0)

#define ICMP_ER		0	/* echo reply */
#define ICMP_DUR	3	/* destination unreachable */
#define ICMP_SQ		4	/* source quench */
#define ICMP_RD		5	/* redirect */
#define ICMP_ECHO	8	/* echo */
#define ICMP_TE		11	/* time exceeded */
#define ICMP_PP		12	/* parameter problem */
#define ICMP_TS		13	/* timestamp */
#define ICMP_TSR	14	/* timestamp reply */
#define ICMP_IRQ	15	/* information request */
#define ICMP_IR		16	/* information reply */

#define	ICMP_DUR_NET	0	/* net unreachable */
#define	ICMP_DUR_HOST	1	/* host unreachable */
#define	ICMP_DUR_PROTO	2	/* protocol unreachable */
#define	ICMP_DUR_PORT	3	/* port unreachable */
#define	ICMP_DUR_FRAG	4	/* fragmentation needed and DF set */
#define	ICMP_DUR_SR	5	/* source route failed */

#define	ICMP_TE_TTL	0	/* time to live exceeded in transit */
#define ICMP_TE_FRAG	1	/* fragment reassembly time exceeded */

typedef struct __attribute__ ((packed)) _icmp_hdr_t {
	ip_hdr_t	ip;
	unsigned char	type;
	unsigned char	code;
	unsigned short	chksum;
	unsigned short	id;
	unsigned short	seqno;
} icmp_hdr_t;

struct _buf_t;
struct _netif_t;

void ip_init (ip_t *ip, struct _mem_pool_t *pool, int prio,
	struct _timer_t *timer, struct _arp_t *arp, mutex_group_t *g);
void ip_input (ip_t *ip, struct _buf_t *p, struct _netif_t *inp);
bool_t ip_output (ip_t *ip, struct _buf_t *p, unsigned char *dest,
	unsigned char *src, small_uint_t proto);
bool_t ip_output_netif (ip_t *ip, struct _buf_t *p
        , const unsigned char *dest, const unsigned char *src
        , small_uint_t proto
        , ip_addr_const gateway
        , struct _netif_t *netif, ip_addr_const netif_ipaddr);

void icmp_echo_request (ip_t *ip, struct _buf_t *p, struct _netif_t *inp);
void icmp_dest_unreach (ip_t *ip, struct _buf_t *p, small_uint_t op);
void icmp_time_exceeded (ip_t *ip, struct _buf_t *p);



#ifdef __cplusplus
}
#endif

#endif /* __IP_H_ */
