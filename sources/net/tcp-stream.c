#include <runtime/lib.h>
#include <kernel/uos.h>
#include <stream/stream.h>
#include <buf/buf.h>
#include <mem/mem.h>
#include <net/ip.h>
#include <net/tcp.h>

/*
 * Flush output buffer.
 */
static void
stream_flush (tcp_stream_t *u)
{
/*debug_printf ("tstream output"); buf_print_data (u->outdata, len);*/
    tcp_socket_t*  s = u->socket;
	while (u->outptr != u->outdata){
	    int len = u->outptr - u->outdata;
	    int sent = tcp_enqueue (u->socket, (void*) u->outdata, len, 0);
	    tcp_debug("tcp-stream: sent %d bytes\n", sent);
	    u->outptr   += sent;
	    if (sent != len) {
#         if TCP_LOCK_STYLE < TCP_LOCK_RELAXED
	        mutex_unlock (&s->lock);
#         endif
	        tcp_output (s);
#         if TCP_LOCK_STYLE < TCP_LOCK_RELAXED
	        mutex_lock (&s->lock);
#         endif
	        mutex_wait (&s->lock);
	    }
	}
}

static void
socket_flush (tcp_socket_t *s)
{
	/* Force IP level to send a packet. */
	tcp_output (s);
}

static void
tcp_stream_putchar (tcp_stream_t *u, short c)
{
    tcp_socket_t*  s = u->socket;
	if (! s)
		return;
	mutex_lock (&s->lock);
	if (!tcp_socket_is_state(s, TCP_STATES_TRANSFER | tcpfCLOSE_WAIT)) {
		/* Connection was closed. */
		mutex_unlock (&s->lock);
		return;
	}
	/* Put byte into output buffer. */
	*u->outptr++ = c;
	if (u->outptr < u->outdata + sizeof(u->outdata)) {
		mutex_unlock (&s->lock);
		return;
	}
	stream_flush (u);
	mutex_unlock (&s->lock);

	/* Force IP level to send a packet. */
	socket_flush (s);
}

static unsigned short
tcp_stream_getchar (tcp_stream_t *u)
{
	unsigned short c;
	tcp_socket_t*  s = u->socket;

	if (!s)
		return -1;
	mutex_lock (&s->lock);

	/* Flush output buffer. */
	if (u->outptr > u->outdata) {
		stream_flush (u);
		mutex_unlock (&s->lock);
		socket_flush (s);
		mutex_lock (&s->lock);
	}

	/* Wait for data. */
	if (u->inbuf)
		mutex_unlock (&s->lock);
	else {
		while (tcp_queue_is_empty (s)) {
			if (!tcp_socket_is_state(s, TCP_STATES_TRANSFER)) {
				mutex_unlock (&s->lock);
				return -1;
			}
			mutex_wait (&s->lock);
		}
		u->inbuf = tcp_queue_get (s);
		u->inptr = u->inbuf->payload;
		mutex_unlock (&s->lock);
/*debug_printf ("tstream input"); buf_print (u->inbuf);*/

		mutex_lock (&s->ip->lock);
		if (! (s->flags & TF_ACK_DELAY) &&
		    ! (s->flags & TF_ACK_NOW)) {
			tcp_ack (s);
		}
		mutex_unlock (&s->ip->lock);
	}

	/* Get byte from buffer. */
	c = *u->inptr++;
	if (u->inptr >= u->inbuf->payload + u->inbuf->len) {
		buf_t *old = u->inbuf;

		u->inbuf = old->next;
		if (u->inbuf) {
			u->inptr = u->inbuf->payload;
			old->next = 0;
		}
		buf_free (old);
	}
	return c;
}

static int
tcp_stream_peekchar (tcp_stream_t *u)
{
    tcp_socket_t*  s = u->socket;
	if (! s)
		return -1;
	mutex_lock (&s->lock);

	/* Flush output buffer. */
	if (u->outptr > u->outdata) {
		stream_flush (u);
		mutex_unlock (&s->lock);
		socket_flush (s);
		mutex_lock (&s->lock);
	}

	/* Any data available? */
	if (u->inbuf)
		mutex_unlock (&s->lock);
	else {
		if (tcp_queue_is_empty (s)) {
			mutex_unlock (&s->lock);
			return -1;
		}
		u->inbuf = tcp_queue_get (s);
		u->inptr = u->inbuf->payload;
		mutex_unlock (&s->lock);

		mutex_lock (&s->ip->lock);
		if (! (s->flags & TF_ACK_DELAY) &&
		    ! (s->flags & TF_ACK_NOW)) {
			tcp_ack (s);
		}
		mutex_unlock (&s->ip->lock);
	}
	return *u->inptr;
}

static void
tcp_stream_flush (tcp_stream_t *u)
{
    tcp_socket_t*  s = u->socket;
	if (! s)
		return;
	mutex_lock (&s->lock);

	/* Flush output buffer. */
	if (u->outptr <= u->outdata) {
		mutex_unlock (&s->lock);
		return;
	}
	stream_flush (u);
	mutex_unlock (&s->lock);

	/* Force IP level to send a packet. */
	socket_flush (s);
}

static bool_t
tcp_stream_eof (tcp_stream_t *u)
{
	bool_t ret;
    tcp_socket_t*  s = u->socket;

	if (! s)
		return 1;
	mutex_lock (&s->lock);
	ret = (!tcp_socket_is_state(s, TCP_STATES_TRANSFER));
	mutex_unlock (&s->lock);
	return ret;
}

static void
tcp_stream_close (tcp_stream_t *u)
{
    tcp_socket_t*  s = u->socket;
	if (! s)
		return;
	mutex_lock (&s->lock);
	mutex_signal (&s->lock, 0);
	mutex_unlock (&s->lock);

	tcp_close (s);
	mem_free (s);
	u->socket = 0;
}

/*
 * Returns an address of a lock, which is signaled on data receive.
 */
static mutex_t *
tcp_stream_receiver (tcp_stream_t *u)
{
	if (! u->socket)
		return 0;
	return &u->socket->lock;
}

#ifdef __cplusplus
#define idx(i)
#define item(i)
#else
#define idx(i) [i] =
#define item(i) .i =
#endif

static stream_interface_t tcp_interface = {
	.putc = (void (*) (stream_t*, short))	tcp_stream_putchar,
	.getc = (unsigned short (*) (stream_t*))tcp_stream_getchar,
	.peekc = (int (*) (stream_t*))		tcp_stream_peekchar,
	.flush = (void (*) (stream_t*))		tcp_stream_flush,
	.eof = (bool_t (*) (stream_t*))		tcp_stream_eof,
	.close = (void (*) (stream_t*))		tcp_stream_close,
	.receiver = (mutex_t *(*) (stream_t*))	tcp_stream_receiver,
#if STREAM_HAVE_ACCEESS > 0
    //* позволяют потребовать монопольного захвата потока
    item(access_rx)                             0
    , item(access_tx)                           0
#endif
};

/*
 * Initialize a stream adapter for TCP socket.
 */
stream_t *tcp_stream_init (tcp_stream_t *u, tcp_socket_t *sock)
{
	u->stream.interface = &tcp_interface;
	u->socket = sock;
	u->outptr = u->outdata;
	return &u->stream;
}
