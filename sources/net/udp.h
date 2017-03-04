#ifndef __UDP_H_
#define __UDP_H_ 1

#include <kernel/uos.h>
#include <buf/buf.h>
#include <net/netif.h>
#include <net/ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UDP header length */
#define UDP_HLEN    8
#define UDP_CAPLEN       (UDP_HLEN + IP_HLEN + MAC_HLEN)
#define UDP_HRESERVE     IP_ALIGNED(UDP_CAPLEN)


#define UDP_SOCKET_QUEUE_SIZE	8

typedef struct _udp_socket_queue_t {
	struct _buf_t	*buf;
	ip_addr         addr;
	unsigned short	port;
} udp_socket_queue_t;

typedef struct _udp_socket_t { //: base_socket_t
    UOSIP_BASE_SOCKET(struct _udp_socket_t);

	struct _netif_t	*netif;
	ip_addr_const   gateway;

	/* queue of received packets */
	udp_socket_queue_t queue [UDP_SOCKET_QUEUE_SIZE];
	udp_socket_queue_t *head;
	unsigned char	count;
} udp_socket_t;

struct _ip_hdr_t;

/*
 * Process the packet, received from the network.
 * Called by IP layer.
 * If there is no target socket found, just return the packet.
 */
struct _buf_t *udp_input (struct _ip_t *ip, struct _buf_t *p, struct _netif_t *inp,
	struct _ip_hdr_t *iph);

/*
 * Create the UDP socket on the given local port.
 * The newly created socket is in "unconnected" state (destination
 * address and port unspecified). Unconnected socket will receive
 * packets from any peer.
 */
void udp_socket (udp_socket_t *s, struct _ip_t *ip, unsigned short port);

/*
 * Close the socket: remove queued data.
 * Remove the UDP socket from IP list.
 */
void udp_close (udp_socket_t *s);

/*
 * Send the UDP packet to the destination address/port.
 */
bool_t udp_sendto (udp_socket_t *s, struct _buf_t *p, unsigned char *dest,
	unsigned short port);

/*
 * Get the received packet. Blocks the task until the data is available.
 * Stores also the address and port of the sender.
 * Recommended for use on unconnected sockets.
 */
struct _buf_t *udp_recvfrom (udp_socket_t *s, unsigned char *from_addr,
	unsigned short *from_port);

/*
 * Get the received packet. Returns 0 when no data is available.
 * Stores also the address and port of the sender.
 * Recommended for use on unconnected sockets.
 * Does not block the task.
 */
struct _buf_t *udp_peekfrom (udp_socket_t *s, unsigned char *from_addr,
	unsigned short *from_port);

/*
 * Connect the UDP socket to the given destination address and port.
 * Connected socket will receive packets only from the peer
 * with appropriate address/port.
 */
void udp_connect (udp_socket_t *s, const unsigned char *ipaddr, unsigned short port);

/*
 * Send the UDP packet to the connected socket.
 */
bool_t udp_send (udp_socket_t *s, struct _buf_t *p);

/*
 * Get the received packet. Blocks the task until the data is available.
 * Recommended for use on connected sockets.
 */
static inline struct _buf_t *
udp_recv (udp_socket_t *s)
{
	return udp_recvfrom (s, 0, 0);
}

/*
 * Get the received packet. Returns 0 when no data is available.
 * Recommended for use on connected sockets.
 * Does not block the task.
 */
static inline struct _buf_t *
udp_peek (udp_socket_t *s)
{
	return udp_peekfrom (s, 0, 0);
}



#ifdef __cplusplus
}
#endif

#endif /* __UDP_H_ */
