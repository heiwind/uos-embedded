#ifndef __TELNET_H_
#define __TELNET_H_ 1

#include <stream/stream.h>
#include <net/tcp.h>

#ifdef __cplusplus
extern "C" {
#endif



/*
 * Telnet protocol implementation, with stream interface.
 */
typedef struct _telnet_t {
	stream_t stream;
	tcp_stream_t ts;

	unsigned char local_option [256/8];
	unsigned char remote_option [256/8];
} telnet_t;

stream_t *telnet_init (tcp_socket_t *sock);
stream_t *telnet_init4buf (tcp_socket_t *sock
                        , void* outdata_buf, unsigned size);
stream_t *telnet_init4pool (tcp_socket_t *sock
                        , mem_pool_t* region, unsigned size);


#ifdef __cplusplus
}
#endif

#endif /* __TELNET_H_ */
