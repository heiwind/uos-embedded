/*
 * This file contains TCP functions, called by user.
 * All of them must acquire/release socket lock or ip->lock,
 * as appropriate. (But never both!)
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <buf/buf.h>
#include <mem/mem.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <timer/timer.h>

/*
 * A nastly hack featuring 'goto' statements that allocates a
 * new TCP local port.
 */
static unsigned short
tcp_new_port (ip_t *ip)
{
	tcp_socket_t *s;
again:
	++ip->tcp_port;
	if (ip->tcp_port > TCP_LOCAL_PORT_RANGE_END) {
		ip->tcp_port = TCP_LOCAL_PORT_RANGE_START;
	}
	for (s = ip->tcp_sockets; s != 0; s = s->next) {
		if (s->local_port == ip->tcp_port) {
			goto again;
		}
	}
	for (s = ip->tcp_closing_sockets; s != 0; s = s->next) {
		if (s->local_port == ip->tcp_port) {
			goto again;
		}
	}
	for (s = ip->tcp_listen_sockets; s != 0; s = s->next) {
		if (s->local_port == ip->tcp_port) {
			goto again;
		}
	}
	return ip->tcp_port;
}



/*
 * Create socket and start connection to another host.
 * ! not wait until connection established.
 * \return socket in connection state
 * \return      = NULL, or SExxx - some error
 */
tcp_socket_t *tcp_connect_start (ip_t *ip,  ip_addr ipaddr, unsigned short port)
{
	tcp_socket_t *s;

	tcp_debug ("tcp_connect to port %u\n", port);
	if (ipaddr.val == 0)
		return 0;
	mutex_lock (&ip->lock);

	s = tcp_alloc (ip);
	if (s == 0){
        mutex_unlock (&ip->lock);
	    return 0;
	}
	
	s->remote_ip.val = ipaddr.val;
	s->remote_port = port;
	if (s->local_port == 0) {
		s->local_port = tcp_new_port (ip);
	}
    mutex_unlock (&ip->lock);
    tcp_socket_t* res = tcp_connect_restart(s);
    if (SEANYERROR(res)){
        mem_free(s);
    }
    return res;
}

/* takes socket at state CLOSED, and start connect to s->remote
 *      if s not CLOSED, return null
 * \return socket in connection state
 * \return      = SExxx - some error
 *
 * */
tcp_socket_t *tcp_connect_restart (tcp_socket_t *s)
{
    unsigned long optdata;
    ip_t *ip = s->ip;
    tcp_debug ("tcp reconnect to port %u\n", s->remote_port);

	s->lastack = s->snd_nxt - 1;
	s->snd_lbb = s->snd_nxt - 1;
	s->snd_wnd = TCP_WND;
	s->ssthresh = s->mss * 10;
	s->state = SYN_SENT;

	/* Build an MSS option */
	optdata = HTONL (((unsigned long)2 << 24) |
		((unsigned long)4 << 16) |
		(((unsigned long)s->mss / 256) << 8) |
		(s->mss & 255));

	if (! tcp_enqueue_option4 (s, TCP_SYN, optdata)) {
		return 0;
	}
    mutex_lock (&ip->lock);
    s->tmr = ip->tcp_ticks;
	tcp_list_add (&ip->tcp_sockets, s);
#if TCP_LOCK_STYLE <= TCP_LOCK_SURE
    tcp_output (s);
    mutex_unlock (&ip->lock);
#elif TCP_LOCK_STYLE >= TCP_LOCK_RELAXED
    mutex_unlock (&ip->lock);
    tcp_output (s);
#endif
    mutex_unlock (&s->lock);
	return s;
}

/*
 * Connect to another host. Wait until connection established.
 * Return 1 on success, 0 on error.
 */
tcp_socket_t *
tcp_connect (ip_t *ip, unsigned char *ipaddr, unsigned short port)
{
    tcp_socket_t* s = tcp_connect_start(ip, ipadr_4ucs(ipaddr), port);
    if ((s == 0) || SEANYERROR(s))
        return s;

	mutex_lock (&s->lock);
	for (;;) {
		if (s->state == ESTABLISHED) {
			mutex_unlock (&s->lock);
			return s;
		}
		if (s->state == CLOSED) {
			mutex_unlock (&s->lock);
			mem_free (s);
			return 0;
		}
        mutex_wait (&s->lock);
	}
}

/*
 * Send len>0 bytes.
 * Return a number ob transmitted bytes, or -1 on error.
 */
int
tcp_write (tcp_socket_t *s, const void *arg, unsigned short len)
{
	tcp_debug ("tcp_write(s=%p, arg=%p, len=%u)\n",
		(void*) s, arg, len);

	if (!tcp_socket_is_state(s, TCP_STATES_TRANSFER)) {
		tcp_debug ("tcp_write() called in invalid state\n");
		return -1;
	}
	if (len == 0) {
		return -1;
	}
	mutex_group_t *g = 0;
	ARRAY (group, sizeof(mutex_group_t) + 2 * sizeof(mutex_slot_t));

	const char* ptr = (const char*)arg;
	unsigned left = len;
	while (left > 0){

	    int sent = tcp_enqueue (s, ptr, len, 0);
	    left    -= sent;
        ptr     += sent;

        if (left == 0)
            break;

#       if TCP_LOCK_STYLE <= TCP_LOCK_SURE
        if (mutex_is_my(&s->lock))
        mutex_unlock (&s->lock);
        tcp_output_poll (s);
#       elif TCP_LOCK_STYLE <= TCP_LOCK_RELAXED
        tcp_output_poll (s);
        mutex_unlock (&s->lock);
#       endif

        /* Не удалось поставить пакет в очередь - мало памяти. */
        if (! g) {
            memset (group, 0, sizeof(group));
            g = mutex_group_init (group, sizeof(group));
            mutex_group_add (g, &s->lock);
            mutex_group_add (g, &s->ip->timer->decisec);
            mutex_group_listen (g);
        }
        /* Каждые 100 мсек делаем повторную попытку. */
        mutex_group_wait (g, 0, 0);
        /* Проверим, не закрылось ли соединение. */
        if (!tcp_socket_is_state(s, TCP_STATES_TRANSFER)) {
            if (g)
                mutex_group_unlisten (g);
            return -1;
        }
	}

	if (g)
		mutex_group_unlisten (g);

#       if TCP_LOCK_STYLE <= TCP_LOCK_SURE
        mutex_unlock (&s->lock);
        tcp_output (s);
#       elif TCP_LOCK_STYLE <= TCP_LOCK_RELAXED
        tcp_output (s);
        mutex_unlock (&s->lock);
#       endif
	return len;
}

/* \~russian ожидает появления данных в приемнике сокета
 * \return = 0 - if socket have some dats in receiver
 *         = -1 - if timedout with no data
 *         = SEerror_code - on error
 * */
int tcp_lock_avail(tcp_socket_t *s, unsigned allow_states
                    , scheduless_condition waitfor, void* waitarg)
{
    bool_t nonblock = (waitfor == (scheduless_condition)0) && (waitarg != (void*)0);
    if (nonblock && tcp_queue_is_empty (s))
        return 0;
    mutex_lock (&s->lock);
    while (tcp_queue_is_empty (s)) {
        int res = -1;
        if ( __glibc_unlikely(nonblock) )
            ;
        else if( __glibc_unlikely( ((1 << s->state) & allow_states) == 0) )
            res = SENOTCONN;
        else {
            if (mutex_wait_until (&s->lock, waitfor, waitarg))
                continue;
            if (!mutex_is_my(&s->lock))
                return res;
        }
        //else
        //    res = -1; //SETIMEDOUT;

        mutex_unlock (&s->lock);
        return res;
    }
    return 0;
}

int tcp_wait_avail(tcp_socket_t *s
                    , scheduless_condition waitfor, void* waitarg)
{
    int res = tcp_lock_avail(s, TCP_STATES_ONLINE, waitfor, waitarg);
    if (res == 0)
        mutex_unlock (&s->lock);
    return res;
}

/*
 * Receive len>0 bytes. Return <0 on error.
 * When nonblock flag is zero, blocks until data get available (never returns 0).
 * When nonblock flag is nonzero, returns 0 if no data is available.
 */
int
tcp_read_poll (tcp_socket_t *s, void *arg, unsigned short len, int nonblock)
{
    if (len == 0) {
        return -1;
    }
    int res = tcp_read_until (s, arg, len, 0, (void*)nonblock);
    return res;
}

/** alternative tcp_read_poll with condition test function waitfor
 * \arg waitfor - simple test function that must not affects mutex_xxx and schedule functionality
 * */
buf_t* tcp_read_buf_until (tcp_socket_t *s
                , scheduless_condition waitfor, void* waitarg)
{
	buf_t *p;
	bool_t nonblock = (waitfor == (scheduless_condition)0) && (waitarg != (void*)0);
	if (nonblock && tcp_queue_is_empty (s))
	    return 0;
	int ok = tcp_lock_avail(s, TCP_STATES_TRANSFER, waitfor, waitarg);
	if (ok != 0)
	    return (buf_t*)ok;
	
	p = tcp_queue_get (s);

	tcp_debug ("tcp_read: received %u bytes, wnd %u (%u).\n",
	       p->tot_len, s->rcv_wnd, TCP_WND - s->rcv_wnd);
	mutex_unlock (&s->lock);

    if (p == 0)
        return 0;

	mutex_lock (&s->ip->lock);
	if (! (s->flags & TF_ACK_DELAY) && ! (s->flags & TF_ACK_NOW)) {
		tcp_ack (s);
	}
	mutex_unlock (&s->ip->lock);
	return p;
}

//* non blocking version of tcp_read_buf_until
buf_t* tcp_take_buf(tcp_socket_t *s)
{
    return tcp_read_buf_until(s, 0, (void*)1);
}

int tcp_read_until (tcp_socket_t *s, void *arg, unsigned short len
                , scheduless_condition waitfor, void* waitarg)
{
    buf_t *p;
    int n;

    tcp_debug ("tcp_read_until(s=%p, arg=%p, len=%u)\n",
        (void*) s, arg, len);
    p = tcp_read_buf_until(s, waitfor, waitarg);
    if (p == 0)
        return 0;
    if ((unsigned)p > SESOCKANY)
        return (int)p;

	/* Copy all chunks. */
	n = buf_copy_continous(arg, p, len);
	//! TODO need fix - if reads less then buffer len - loose rest of buf
	buf_free (p);
	return n;
}

/*
 * Blocking receive.
 * Return a number of received bytes >0.
 * Return <0 on error.
 */
int
tcp_read (tcp_socket_t *s, void *arg, unsigned short len)
{
    if (len == 0) {
        return -1;
    }
	return tcp_read_until (s, arg, len, 0, 0);
}

/*
 * Set the state of the connection to be LISTEN, which means that it
 * is able to accept incoming connections. The protocol control block
 * is reallocated in order to consume less memory. Setting the
 * connection to LISTEN is an irreversible process.
 */
tcp_socket_t *tcp_listen (ip_t *ip, unsigned char *ipaddr,
	unsigned short port)
{
	tcp_socket_t *s, *cs;
	ip_addr targetip;
	

	if (ipaddr) {
	    targetip = ipadr_4ucs(ipaddr);
	}
	else
	    targetip.val = 0;

	s = tcp_alloc (ip);
	if (s == 0) {
		return 0;
	}

	/* Bind the connection to a local portnumber and IP address. */
	mutex_lock (&ip->lock);
	if (port == 0) {
		port = tcp_new_port (ip);
	}
	tcp_debug ("tcp_listen: port %u\n", port);

	/* Check if the address already is in use. */
	for (cs = ip->tcp_listen_sockets; cs != 0; cs = cs->next) {
		if (cs->local_port == port) {
			if ( ipadr_is_same(targetip.var, cs->local_ip) ) {
				mutex_unlock (&ip->lock);
				mem_free (s);
				return 0;
			}
		}
	}
	for (cs = ip->tcp_sockets; cs != 0; cs = cs->next) {
		if (cs->local_port == port) {
			if (ipadr_is_same(targetip.var, cs->local_ip) ) {
				mutex_unlock (&ip->lock);
				mem_free (s);
				return 0;
			}
		}
	}

	if ( ipadr_not0(targetip.var) ) {
		s->local_ip = targetip.var;
	}
	s->local_port = port;
	s->state = LISTEN;
    buf_queueh_init(&s->inq, sizeof(s->queue));

	tcp_list_add (&ip->tcp_listen_sockets, s);
	mutex_unlock (&ip->lock);
	return s;
}

tcp_socket_t *
tcp_accept (tcp_socket_t *s)
{
    tcp_socket_t * res;
    do {
        res = tcp_accept_until(s, (scheduless_condition)0, (void*)0);
        if ((unsigned)res == SENOMEM)
            //для совместимости со старым поведением
            continue;
        if (SEANYERROR(res))
            return res;
    } while (res == 0);
    return res;
}

tcp_socket_t *tcp_accept_until (tcp_socket_t *s
                                , scheduless_condition waitfor, void* waitarg)
{
	tcp_socket_t *ns;
	buf_t *p;
	ip_hdr_t *iph;
	tcp_hdr_t *h;
	unsigned long optdata;
    int ok = tcp_lock_avail(s, (1<<LISTEN)
                              , waitfor, waitarg);
    if (ok != 0){
        if (s->state != LISTEN){
            tcp_debug ("tcp_accept: called in invalid state\n");
            return (tcp_socket_t *)SEINVAL;
        }
        if (ok == -1)
            return 0;
        return (tcp_socket_t *)ok;
    }

	/* Create a new PCB, and respond with a SYN|ACK.
	 * If a new PCB could not be created (probably due to lack of memory),
	 * we don't do anything, but rely on the sender will retransmit
	 * the SYN at a time when we have more memory available. */
	mutex_lock (&s->ip->lock);
	ns = tcp_alloc (s->ip);
	if (ns == 0) {
        mutex_unlock (&s->lock);
		++(s->ip->tcp_in_discards);
		mutex_unlock (&s->ip->lock);
        tcp_debug ("tcp_accept: could not allocate PCB\n");
		return (tcp_socket_t *)SENOMEM;
	}
    p = tcp_queue_get (s);

	h = (tcp_hdr_t*) p->payload;
	iph = ((ip_hdr_t*) p->payload) - 1;

	/* Set up the new PCB. */
	ns->local_ip = iph->dest.var;
	ns->local_port = s->local_port;
	ns->remote_ip  = iph->src;
	ns->remote_port = h->src;
	ns->state = SYN_RCVD;
	ns->rcv_nxt = s->ip->tcp_input_seqno + 1;
	ns->snd_wnd = h->wnd;
	ns->ssthresh = ns->snd_wnd;
	ns->snd_wl1 = s->ip->tcp_input_seqno;
	ns->ip = s->ip;
    mutex_unlock (&s->lock);

	/* Register the new PCB so that we can begin receiving
	 * segments for it. */
	tcp_list_add (&s->ip->tcp_sockets, ns);

	/* Parse any options in the SYN. */
	tcp_parseopt (ns, h);

	/* Build an MSS option. */
	optdata = HTONL (((unsigned long)2 << 24) | ((unsigned long)4 << 16) |
		(((unsigned long)ns->mss / 256) << 8) | (ns->mss & 255));
	buf_free (p);

	/* Send a SYN|ACK together with the MSS option.
	 * there is impossible fail of tcp_enqueue_option4
	 * */
	tcp_enqueue_option4 (ns, TCP_SYN | TCP_ACK, optdata);
#if TCP_LOCK_STYLE <= TCP_LOCK_SURE
    tcp_output (ns);
    mutex_unlock (&ns->ip->lock);
#elif TCP_LOCK_STYLE >= TCP_LOCK_RELAXED
    mutex_unlock (&ns->ip->lock);
    tcp_output (ns);
#endif
    mutex_unlock (&ns->lock);
	return ns;
}

/*
 * Closes the connection held by the PCB.
 * Return 1 on success, 0 on error.
 */
int
tcp_close (tcp_socket_t *s)
{
	mutex_lock (&s->lock);

	tcp_debug ("tcp_close: sock $%x state=%S\n", s, tcp_state_name (s->state));

	switch (s->state) {
	default: /* Has already been closed. */
		tcp_queue_free (s);
		mutex_unlock (&s->lock);
		return 1;
	case LISTEN:
	case SYN_SENT:
		tcp_queue_free (s);
		mutex_unlock (&s->lock);
		mutex_lock (&s->ip->lock);
		tcp_socket_remove (s->state == LISTEN ?
			&s->ip->tcp_listen_sockets : &s->ip->tcp_sockets, s);
		mutex_unlock (&s->ip->lock);
		return 1;
	case SYN_RCVD:
	case ESTABLISHED:
	case CLOSE_WAIT:
		break;
	}
	for (;;) {
		if (tcp_enqueue_option4 (s, TCP_FIN, 0))
			break;
		mutex_wait (&s->lock);
	}
	s->state = (s->state == CLOSE_WAIT) ? LAST_ACK : FIN_WAIT_1;

    if (s->unsent || (s->flags & TF_ACK_NOW)) {
#if TCP_LOCK_STYLE <= TCP_LOCK_SURE
    mutex_unlock (&s->lock);
#endif
	tcp_output (s);
#if TCP_LOCK_STYLE <= TCP_LOCK_SURE
    mutex_lock (&s->lock);
#endif
    }//if (s->unsent || (s->flags & TF_ACK_NOW))

	while (s->state != CLOSED) {
		if (s->state == TIME_WAIT && ! s->unsent) {
			tcp_queue_free (s);
			mutex_unlock (&s->lock);
			mutex_lock (&s->ip->lock);
			tcp_socket_remove (&s->ip->tcp_closing_sockets, s);
			mutex_unlock (&s->ip->lock);
			return 1;
		}
		mutex_wait (&s->lock);
	}
	mutex_unlock (&s->lock);
	return 1;
}

/*
 * Aborts a connection by sending a RST to the remote host and deletes
 * the local protocol control block. This is done when a connection is
 * killed because of shortage of memory.
 */
void
tcp_abort (tcp_socket_t *s)
{
	ip_t *ip = s->ip;
	unsigned long seqno, ackno;
	unsigned short remote_port, local_port;

	mutex_lock (&s->lock);

	/* Figure out on which TCP PCB list we are, and remove us. If we
	 * are in an active state, send an RST to the remote end. */
	if (s->state == TIME_WAIT) {
		tcp_queue_free (s);
		mutex_unlock (&s->lock);
		mutex_lock (&ip->lock);
		tcp_socket_remove (&ip->tcp_closing_sockets, s);
		mutex_unlock (&ip->lock);
		return;
	}
	seqno = s->snd_nxt;
	ackno = s->rcv_nxt;
	local_port = s->local_port;
	remote_port = s->remote_port;
	tcp_queue_free (s);
	mutex_unlock (&s->lock);

	mutex_lock (&ip->lock);
	tcp_socket_remove (&ip->tcp_sockets, s);
	tcp_socket_purge(s);

	tcp_debug ("tcp_abort: sending RST\n");
	tcp_rst (ip, seqno, ackno, ipref_as_ucs(s->local_ip), s->remote_ip.ucs,
		local_port, remote_port);
	mutex_unlock (&ip->lock);
}

/*
 * Return the period of socket inactivity, in seconds.
 */
unsigned long
tcp_inactivity (tcp_socket_t *s)
{
	unsigned long sec;

	mutex_lock (&s->ip->lock);
	sec = (s->ip->tcp_ticks - s->tmr) >> 1;
	mutex_unlock (&s->ip->lock);
	return sec;
}
