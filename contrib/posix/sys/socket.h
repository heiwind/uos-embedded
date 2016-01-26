#ifndef __POSIX_SYS_SOCKET_H__
#define __POSIX_SYS_SOCKET_H__

#include <runtime/sys/uosc.h>
#include <net/ip.h>
#include <netinet/in.h>
#include <net/tcp.h>


typedef unsigned int socklen_t;
typedef struct sockaddr_in sockaddr;

INLINE 
int getpeername(int/* base_socket_t* */ socket, sockaddr *address, socklen_t *address_len) __THROW
{
    base_socket_t* s = (base_socket_t*)socket;
    if (s->ip == NULL) return -1;
    //if (s->netif == NULL) return -1;
    address->sin_addr.s_addr = s->peer_ip.val;
    address->sin_port = s->peer_port;
    if(address_len)
        *address_len = sizeof(address);
    return 0;
}

/* Await a connection on socket FD.
   When a connection arrives, open a new socket to communicate with it,
   set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
   peer and *ADDR_LEN to the address's actual length, and return the
   new socket's descriptor, or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
INLINE
int accept (int /* base_socket_t* */ socket
            , sockaddr *address, socklen_t *address_len) __THROW
{
    tcp_socket_t* s = (tcp_socket_t*)socket;
    if (s->ip == NULL) return -1;
    tcp_socket_t *res = tcp_accept(s);
    return (res)? (int)res : -1;
}


#endif// __POSIX_SYS_SOCKET_H__
