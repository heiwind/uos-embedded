/*
 * The output functions of TCP.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <buf/buf.h>
#include <mem/mem.h>
#include <crc/crc16-inet.h>
#include <net/netif.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/tcp.h>

/*
#undef tcp_debug
#define tcp_debug			wdog_alive(); debug_printf
*/

#if RECURSIVE_LOCKS > 0
INLINE mutex_t* tcpo_lock_ensure(mutex_t* m){
    mutex_lock(s->lock);
    return 1;
}
INLINE void tcpo_unlock_ensure(mutex_t* m){
    if (m)
        mutex_unlock(m);
}
#else
INLINE mutex_t* tcpo_lock_ensure(mutex_t* m){
    if (m == 0)
        return 0;
    if (mutex_is_my(m))
        return (mutex_t*)0;
    mutex_lock(m);
    return m;
}
INLINE void tcpo_unlock_ensure(mutex_t* m){
    if (m != 0)
    mutex_unlock(m);
}
#endif



//* забирает сегмент из кучи доступных сегментов (ранее созданных)
//* \return NULL - нет свободных сегментов, !!! сокет может захватиться
//* \return  - возвращает новый сегмент, и сокет (s->lock) в захваченом состоянии
tcp_segment_t *tcp_segment_takespare_maylock (tcp_socket_t *s){
    if (s->spare_segs == 0)
        return NULL;
    if (!mutex_is_my(&s->lock))
        mutex_lock(&s->lock);
    tcp_segment_t * seg = s->spare_segs;
    if (seg != 0){
        s->spare_segs = seg->next;
        //tcp_debug("tcp_spare:get seg<< $%x\n", seg);
        memsetw(seg, 0 ,sizeof(tcp_segment_t));
    }
    return seg;
}

//* берет свободный сегмент, или создает новый.
//  !!! может вернуть сокет, может захваченым 
tcp_segment_t *tcp_segment_alloc_maylock(tcp_socket_t *s){
    tcp_segment_t *res = tcp_segment_takespare_maylock(s);
    if (res != 0)
        return res;
    res = (tcp_segment_t *)mem_alloc (s->ip->pool, sizeof (tcp_segment_t));
    return res;
}



//***************************    TCP RAW handling   ***************************
#ifdef UTCP_RAW
void tcp_event_seg_prepare(tcp_segment_t* seg, tcp_socket_t* s)
{
    if (seg->handle != 0){
        tcp_hdr_t* tcphdr = seg->tcphdr;
        seg->hsave.ackno = NTOHL(tcphdr->ackno);
        seg->hsave.seqno = NTOHL(tcphdr->seqno);
        seg->hsave.urgp  = tcphdr->urgp;
    }
}

void tcp_event_seg_recover(tcp_segment_t* seg)
{
    if (seg->handle != 0){
        tcp_hdr_t* tcphdr = seg->tcphdr;
        tcphdr->seqno = HTONL(seg->hsave.seqno);
        tcphdr->urgp = seg->hsave.urgp;
    }
}

#else
#define tcp_event_seg_prepare(seg, s)
#define tcp_event_seg_recover(seg)
#endif



int tcp_segment_assign_buf(tcp_socket_t *s, tcp_segment_t * seg, buf_t* p)
{
    seg->p = p;
    seg->dataptr = p->payload;
    seg->len = p->tot_len;

    /* Build TCP header. */
    if (! buf_add_header (p, TCP_HLEN)) {
        tcp_debug ("tcp_enqueue: no room for TCP header\n");
        return 0;
    }
    tcp_hdr_t* tcphdr = (tcp_hdr_t*) p->payload;
    tcphdr->src     = HTONS (s->local_port);
    tcphdr->dest    = HTONS (s->remote_port);
    tcphdr->offset  = 5 << 4;
    tcphdr->flags   = 0;
    tcphdr->chksum  = 0;
    tcphdr->urgp    = 0;
    seg->tcphdr = tcphdr;
    tcp_event_seg_recover(seg);

    //overlap this buffer, to avoid unnesesary buf copy at tcp_output_segment
    netif_io_overlap* over = netif_overlap_init(p);
    over->options = nioo_ActionMutex;
    over->action.signal = 0;

    return 1;
}

/* alloc tcp seg and eth-frame buffer with <len> data allocated  
 * !!! may lock s->lock
 * */
tcp_segment_t * tcp_segment_new(tcp_socket_t *s, const void *arg, small_uint_t seglen)
{
    tcp_segment_t * seg;

    /* Allocate memory and copy data into it.
     * If optdata is != NULL, we have options instead of data. */
    buf_t *p = buf_alloc (s->ip->pool, seglen, TCP_SEG_RESERVE );
    if (p == 0) {
        tcp_debug ("tcp_new_seg: could not allocate %u bytes\n", seglen);
        return 0;
    }

    /* Allocate memory for tcp_segment, and fill in fields. */
    seg = tcp_segment_alloc_maylock(s);
    if (seg == 0) {
        buf_free(p);
        return 0;
    }
    seg->p = p;

    if (tcp_segment_assign_buf(s, seg, p) == 0){
        tcp_segment_free(s, seg);
        return 0;
    }

    seg->datacrc = 0;
    if (arg != 0) {
        unsigned short sum = memcpy_crc16_inet(CRC16_INET_INIT, seg->dataptr, arg, seglen);
        if (seglen & 1) {
            /* Invert checksum bytes. */
            sum = (sum << 8) | (unsigned char) (sum >> 8);
        }
        seg->datacrc = sum;
    }
    else
        seg->len = 0;

    return seg;
}

void //int
tcp_enqueue_segments (tcp_socket_t *s, tcp_segment_t* queue, tcph_flag_set flags);


//* !!! it ensures that socket is lock, and leave it locked after return!
inline 
int tcp_pass_segments (tcp_socket_t *s, tcp_segment_t* queue, tcph_flag_set flags)
{
    //mutex_t* s_locked =
            tcpo_lock_ensure(&s->lock);
    tcp_enqueue_segments(s, queue, flags);
    return 1;
    /*
    if ( __glibc_unlikely(res == 0) ){
        tcp_segments_free(s, queue);
    }
    */
}

/*
 * Send data or options or flags.
 * Allocate a new buf and append it to socket send queue.
 * !!! it ensures that socket is lock, and leave it locked after return! 
 * \return  = amount of passed data
 * !!!      = 0 - may leave by error with no locked socket !!! 
 */
int
tcp_enqueue (tcp_socket_t *s, const void *arg, small_uint_t len, tcph_flag_set flags)
{
	tcp_segment_t *seg, *useg, *queue;
	unsigned long left;
	unsigned short seglen;
	const void *ptr;
	unsigned char queuelen;

    assert_task_good_stack(task_current);
	tcp_debug ("tcp_enqueue(s=%p, arg=%p, len=%u, flags=%x) queuelen = %u\n",
		(void*) s, arg, len, flags, s->snd_queuelen);
	left = len;
	ptr = arg;
	/* fail on too much data */
	if (len > s->snd_buf) {
		tcp_debug ("tcp_enqueue: too much data (len=%u > snd_buf=%u)\n",
			len, s->snd_buf);
		return 0;
	}

	/* Check if the queue length exceeds the configured maximum queue
	 * length. If so, we return an error. */
	queue = 0;
	queuelen = 0;
	if (s->snd_queuelen >= TCP_SND_QUEUELEN) {
		tcp_debug ("tcp_enqueue: too long queue %u (max %u)\n",
			queuelen, TCP_SND_QUEUELEN);
		return 0;
	}

	seg = 0;
	seglen = 0;

	/* First, break up the data into segments and tuck them together in
	 * the local "queue" variable. */
	while (queue == 0 || left > 0) {

		/* The segment length should be the MSS if the data to be
		 * enqueued is larger than the MSS. */
		seglen = left > s->mss ? s->mss : left;

		/* Allocate memory for tcp_segment, and fill in fields. */
		seg = tcp_segment_new(s, ptr, seglen);
		if (seg == 0) {
			tcp_debug ("tcp_enqueue: cannot allocate tcp_segment\n");
			break;
		}

		/* Build TCP header. */
		tcp_hdr_t* tcphdr = (tcp_hdr_t*) seg->p->payload;
		tcphdr->urgp = 0;
		tcphdr->flags = flags;
		/* don't fill in tcphdr->ackno and tcphdr->wnd until later */

        left -= seglen;
        ptr = (const void*) ((char*) ptr + seglen);

	    ++queuelen;
        if (queue == 0) {
            queue = seg;
            //! to enforce faster start of tcp transmission, just stops on 1st segment
            // for pass it to tcp_output
            break;
        } else {
            /* Attach the segment to the end of the queued segments. */
            for (useg = queue; useg->next != 0; useg = useg->next)
                ;
            useg->next = seg;
        }
        int qlen_actual = *(volatile unsigned char*)(&s->snd_queuelen);
        if ((qlen_actual+queuelen) >= TCP_SND_QUEUELEN) 
            break;
	}//while (queue == 0 || left > 0)
	
	len -= left;

    if (queue == 0) {
        ++s->ip->tcp_out_errors;
        tcp_debug ("tcp_enqueue: no allocated tcp_segments\n");
        return 0;
    }

    /* Set the PSH flag in the last segment that we enqueued, but only
    if the segment has data (indicated by seglen > 0). */
    if (seg != 0 && seglen > 0 && seg->tcphdr != 0) {
        seg->tcphdr->flags |= TCP_PSH;
    }

    return (tcp_pass_segments(s, queue, flags) != 0)? len : 0;
}

/*
 * Send options or flags.
 * Allocate a new buf and append it to socket send queue.
 * !!! it ensures that socket is lock, and leave it locked after success return!
 * \return 0 on error.
 */
int
tcp_enqueue_option4 (tcp_socket_t *s
                , tcph_flag_set flags
                , unsigned long optdata)
{
    const unsigned optlen = 4;
    tcp_segment_t *seg, *queue;
    unsigned char queuelen;

    tcp_debug ("tcp_enqueue_option(s=%p, flags=%x, op=0x%x) queuelen = %u\n",
        (void*) s, flags, optdata,s->snd_queuelen);

    /* Check if the queue length exceeds the configured maximum queue
     * length. If so, we return an error. */
    queue = 0;
    queuelen = s->snd_queuelen;
    if (queuelen >= TCP_SND_QUEUELEN) {
        tcp_debug ("tcp_enqueue: too long queue %u (max %u)\n",
            queuelen, TCP_SND_QUEUELEN);
        return 0;
    }

    /* First, break up the data into segments and tuck them together in
     * the local "queue" variable. */

        /* Allocate memory for tcp_segment, and fill in fields. */
        seg = tcp_segment_new(s,  0, optlen);
        if (seg == 0) {
            tcp_debug ("tcp_enqueue_option: cannot allocate tcp_segment\n");
            return 0;
        }

        /* Build TCP header. */
        tcp_hdr_t* tcphdr = (tcp_hdr_t*) seg->tcphdr;
        tcphdr->urgp = 0;
        tcphdr->flags = flags;
        /* don't fill in tcphdr->ackno and tcphdr->wnd until later */

            /* Copy options into data portion of segment.
             * Options can thus only be sent in non data carrying
             * segments such as SYN|ACK. */
            //!assume we alloc aligned buffer 
            //memcpy (seg->dataptr, optdata, optlen);
            *(unsigned long*)seg->dataptr = optdata;
            tcphdr->offset = (5 + optlen / 4) << 4;
         queue = seg;

         return tcp_pass_segments(s, queue, flags);
}

/*
 * Send data or options or flags.
 * Allocate a new buf and append it to socket send queue.
 * \return  = 0 on error
 */
void //int
tcp_enqueue_segments (tcp_socket_t *s, tcp_segment_t* queue, tcph_flag_set flags)
{
    tcp_segment_t *useg;
    unsigned long seqno;
    unsigned len = 0;
    unsigned queuelen;

    queuelen = s->snd_queuelen;
    if (queuelen != 0) {
        assert (s->unacked != 0 || s->unsent != 0);
    }

    /* seqno will be the sequence number of the first segment enqueued
     * by the call to this function. */
    seqno = s->snd_lbb;
    useg = queue;
    while(useg != 0) {
        useg->hsave.seqno = seqno;
        tcp_hdr_t* tcphdr = useg->tcphdr;
        tcphdr->seqno = HTONL (seqno);
        tcp_debug ("tcp_enqueue: queueing %lu:%lu (flags 0x%x)\n"
                , tcp_segment_seqno(useg), tcp_segment_seqafter(useg)
                , flags);
        if (flags & TF_TRAP_LOOSE){
            tcp_debug ("tcp_enqueue: loose seg seq %lu\n", seqno);
            useg->dataptr = 0;
        }
        seqno   += useg->len;
        len     += useg->len;
        useg    = useg->next;
        queuelen++;
    }  
	
	/* Now that the data to be enqueued has been broken up into TCP
	segments in the queue variable, we add them to the end of the
	s->unsent queue. */
	if (s->unsent == 0) {
		useg = 0;
	} else {
		for (useg = s->unsent; useg->next != 0; useg = useg->next)
			;
	}

	/* If there is room in the last buf on the unsent queue,
	chain the first buf on the queue together with that. */
	if (useg != 0 && TCP_TCPLEN(useg) != 0
	    && !((flags | s->flags) & TF_NOCORK)
	    && ! (useg->tcphdr->flags & (TCP_SYN | TCP_FIN)) &&
	    ! (flags & (TCP_SYN | TCP_FIN)) &&
	    useg->len + queue->len <= s->mss)
	{
		/* Remove TCP header from first segment. */
		buf_add_header (queue->p, -TCP_HLEN);
		buf_chain (useg->p, queue->p);

		useg->len += queue->len;
		useg->next = queue->next;

		tcp_debug ("tcp_enqueue: chaining, new len %u\n", useg->len);
		mem_free (queue);
	} else {
		if (useg == 0) {
			s->unsent = queue;
		} else {
			useg->next = queue;
		}
	}
	if ((flags & TCP_SYN) || (flags & TCP_FIN)) {
		++len;
	}
	s->snd_lbb += len;
	s->snd_buf -= len;
	s->snd_queuelen = queuelen;
	tcp_debug ("tcp_enqueue: done, queuelen = %d\n", s->snd_queuelen);
	if (s->snd_queuelen != 0) {
		assert (s->unacked != 0 || s->unsent != 0);
	}

	//return 1;
}

// overlaped buffer seg->p seems shouldn`t be sopy, caller responded on it`s life
static bool_t
tcp_output_segment (tcp_segment_t *seg, tcp_socket_t *s)
{
	unsigned int n;
	netif_t *netif;
	buf_t *p;
	tcp_hdr_t* tcphdr = seg->tcphdr;

	/* The TCP header has already been constructed, but the ackno and
	 * wnd fields remain. */
	tcphdr->ackno = HTONL (s->rcv_nxt);

	/* silly window avoidance */
	//! TODO need FIX - avoidance can block connected side to send even small chunck 
	if (s->rcv_wnd < s->mss) {
		tcphdr->wnd = 0;
	} else {
		/* advertise our receive window size in this TCP segment */
		tcphdr->wnd = HTONS (s->rcv_wnd);
	}

	/* If we don't have a local IP address, we get one by
	 * calling ip_route(). */
	if ( !ipadr_not0(s->local_ip) ) {
	    ip_addr_const local_ip;
		netif = route_lookup (s->ip, s->remote_ip.var, 0, &local_ip);
		if (! netif)
			return 0;
		s->local_ip = local_ip;
	}

    tcp_event_seg_prepare(seg, s);
	
    bool_t res;

    if (seg->dataptr != 0){

    p = seg->p;

	n = (unsigned int) ((unsigned char*) tcphdr -
		(unsigned char*) p->payload);
    //buf_add_header(p, n);
	p->len -= n;
	p->tot_len -= n;
	p->payload = (unsigned char*) tcphdr;

	tcphdr->chksum = 0;

    unsigned hsum = crc16_inet_header (ipref_as_ucs(s->local_ip), s->remote_ip.ucs,
            IP_PROTO_TCP, p->tot_len);

    if (seg->datacrc != 0) {
        unsigned tsum = crc16_inet(hsum, p->payload, ((unsigned char*)seg->dataptr - (unsigned char*) tcphdr) );
        //if have tcp-data crc calculated, the recalc only header
        unsigned sum = tsum + seg->datacrc; 
        sum = (unsigned short)sum + (sum>>16);
        sum = (unsigned short)sum + (sum>>16);
        tcphdr->chksum = ~((unsigned short)sum);
        /*if (sum != n){
            debug_printf("tcp crc:valid %x , cached %x, cacheddata %x, data %x, tcpdat %x, tcph %x, header %x\n"
                    , n, sum, seg->datacrc, dsum, tdsum, tsum, hsum);
        }*/
    }
    else {
        n = buf_chksum (p, hsum);
        if (p->tot_len & 1) {
            /* Invert checksum bytes. */
            n = (n << 8) | (unsigned char) (n >> 8);
        }
        tcphdr->chksum = n;
    }

    netif_io_overlap* over = netif_is_overlaped(p);
    if (over == 0) {
        // overlaped buffer seems shouldn`t be sopy, caller responded on it`s life 
	p = buf_copy (p);
	if (! p)
		return 0;
    }//if (over == 0)
/*	buf_print_tcp (p);*/

#if TCP_LOCK_STYLE >= TCP_LOCK_RELAXED2
    mutex_t* s_locked = tcpo_lock_ensure(&s->ip->lock);
#endif

    if ( (TF_IP_NOCACHE) && ((s->flags & TF_IP_NOCACHE) == 0) && (s->iph_cache) )
        res = ip_output_withheader(s->ip, p, s->iph_cache);
    else
	res = ip_output (s->ip, p, s->remote_ip.ucs, ipref_as_ucs(s->local_ip), IP_PROTO_TCP);

#if TCP_LOCK_STYLE >= TCP_LOCK_RELAXED2
	tcpo_unlock_ensure(s_locked);
#endif

    }
    else { //if (seg->dataptr != 0){
        // suspects that TF_TRAP_LOOSE
        tcp_debug ("!!!tcp_output_segment: drop segment as TF_TRAP_LOOSE\n"); 
        tcp_event_seg(teSENT, seg, s);
        netif_free_buf(0, seg->p);
        res = 1;
    }


	if (res){
	    ++s->ip->tcp_out_datagrams;
        s->rtime = 0;
    
        if (s->rttest == 0) {
            s->rttest = s->ip->tcp_ticks;
            s->rtseq = tcp_segment_seqno(seg);
        }
        s->snd_nxt = tcp_segment_seqafter(seg);
        if (TCP_SEQ_LT (s->snd_max, s->snd_nxt)) {
            s->snd_max = s->snd_nxt;
        }
	} //if (res)
	else{
	    //unsent segment will be sent next later
	    if (seg->p == 0)
	        tcp_event_seg(teREXMIT, seg, s);
	}
    tcp_debug ("tcp_output_segment: %lu:%lu, snd_nxt = %u ok %d\n"
            , tcp_segment_seqno(seg), tcp_segment_seqafter(seg)
            , s->snd_nxt
        , res
        );

	return  res;
}



//overlaped buffer can rearm suspended transfer when reaches s->snd_nxt mark
void tcp_output_segment_arm_handle(buf_t *p, unsigned arg){
    tcp_socket_t *s = (tcp_socket_t *)arg; 
    netif_io_overlap* over = netif_is_overlaped(p);
    tcp_segment_t *   seg = (tcp_segment_t *)over->arg3;

    tcp_debug("tcp_output segment($%x:$%x): sent status $%x by th %d\n"
                , s, seg
                , (int)over->status, over->arg2
                );

    if ((over->status & nios_IPOK) != 0)
    if (TF_IP_NOCACHE && ((s->flags & TF_IP_NOCACHE) == 0))
    {
        unsigned len = ((unsigned char*)seg->tcphdr - p->payload);
        if (s->iph_cache == 0){
            s->iph_cache = buf_alloc(s->ip->pool, 0, IPH_CACHE_HRESERVE(len));
        }
        ip_refresh_hcache(s->ip, 0, s->iph_cache, p, len);
    }

    //* неотправленый сегмент будет останется в очереди отправки, и будет новая попытка отправки
    if ((over->status & nios_IPOK) != 0)
        tcp_event_seg(teSENT, seg, s);

    if (TCP_SEQ_LT(s->snd_nxt , over->arg2)){
      if (!mutex_is_my(&s->lock))
      //if (mutex_is_wait(&s->lock))
        mutex_signal(&s->lock, s);
    }
}

void tcp_output_segment_arm(tcp_socket_t *s, tcp_segment_t *seg){
      netif_io_overlap* over = netif_is_overlaped(seg->p);
      if (over == 0)
          return;
      //overlaped buffer can rearm suspended transfer when reaches s->snd_nxt mark
      over->action.callback = &(tcp_output_segment_arm_handle);
      over->options &= ~nioo_ActionMASK;
      over->options |= nioo_ActionCB;
      over->arg  = (unsigned long)s;
      over->arg2 = s->snd_nxt + TCP_TCPLEN (seg);
      over->arg3 = (unsigned long)seg;
}

/*
 * Find out what we can send and send it.
 * Must be called with ip locked        if TCP_LOCK_LEGACY.
 * or ensures that socket locked        if TCP_LOCK_RELAXED
 * Return 0 on error.
 */
//empty overlaped buffer seg->p managed by netif_output to free or hold in segment
//  after tx 
int
tcp_output_poll (tcp_socket_t *s)
{
	buf_t *p;
	tcp_hdr_t *tcphdr;
	tcp_segment_t *seg, *useg;
	unsigned long wnd;

    assert_task_good_stack(task_current);

#if TCP_LOCK_STYLE >= TCP_LOCK_RELAXED
	mutex_t* s_locked = tcpo_lock_ensure(&s->lock);
#elif TCP_LOCK_STYLE <= TCP_LOCK_SURE
    mutex_t* s_locked = iptx_lock_ensure(s->ip);
#endif
    
    wnd = (s->snd_wnd < s->cwnd) ? s->snd_wnd : s->cwnd;
    seg = s->unsent;
	
	/* If the TF_ACK_NOW flag is set, we check if there is data that is
	 * to be sent. If data is to be sent out, we'll just piggyback our
	 * acknowledgement with the outgoing segment. If no data will be
	 * sent (either because the ->unsent queue is empty or because the
	 * window doesn't allow it) we'll have to construct an empty ACK
	 * segment and send it. */
	if ((s->flags & TF_ACK_NOW) && (seg == 0 ||
	    tcp_segment_seqno(seg) - s->lastack + seg->len > wnd)) 
	{
		p = buf_alloc (s->ip->pool, TCP_HLEN, 16 + IP_HLEN);
		if (p == 0) {
			tcp_debug ("tcp_output: (ACK) could not allocate buf\n");
	        tcpo_unlock_ensure(s_locked);
			return 0;
		}
		tcp_debug ("tcp_output: sending ACK for %lu\n", s->rcv_nxt);

		tcphdr = (tcp_hdr_t*) p->payload;
		tcphdr->src = HTONS (s->local_port);
		tcphdr->dest = HTONS (s->remote_port);
		tcphdr->seqno = HTONL (s->snd_nxt);
		tcphdr->ackno = HTONL (s->rcv_nxt);
		tcphdr->flags = TCP_ACK;
		tcphdr->wnd = HTONS (s->rcv_wnd);
		tcphdr->urgp = 0;
		tcphdr->offset = 5 << 4;

		tcphdr->chksum = 0;
		tcphdr->chksum = buf_chksum (p,
			crc16_inet_header (ipref_as_ucs(s->local_ip),	s->remote_ip.ucs,
			IP_PROTO_TCP, p->tot_len));

      #if (TCP_LOCK_STYLE >= TCP_LOCK_RELAXED) && (IP_LOCK_STYLE != IP_LOCK_STYLE_OUT1)
		mutex_t* ip_locked = iptx_lock_ensure(s->ip);
      #endif
		if (ip_output (s->ip, p, s->remote_ip.ucs, ipref_as_ucs(s->local_ip), IP_PROTO_TCP) != 0)
		{
		    s->flags &= ~(TF_ACK_DELAY | TF_ACK_NOW);
		}
      #if (TCP_LOCK_STYLE >= TCP_LOCK_RELAXED) && (IP_LOCK_STYLE != IP_LOCK_STYLE_OUT1)
		iptx_unlock_ensure(ip_locked);
      #endif
	    tcpo_unlock_ensure(s_locked);
		return 1;
	}

	if (seg == 0) {
		tcp_debug ("tcp_output: nothing to send, snd_wnd %lu, cwnd %lu, wnd %lu, ack %lu\n",
			s->snd_wnd, s->cwnd, wnd, s->lastack);
	}
	else {

#if TCP_LOCK_STYLE == TCP_LOCK_RELAXED
	mutex_t* ip_locked = iptx_lock_ensure(s->ip);
#endif
	
	while (seg != 0) 
	{
	    int               tcpseglen = 0;
	    netif_io_overlap* over = 0;
	    //tcp_debug ("tcp_output seg $%x:\n", seg);

        //with utcp-raw support segment can loose its data
        if ( (seg->tcphdr == 0) || (seg->p == 0) )
        if (seg->len > 0)
        {
            //this situation preserved for REXMIT segment that was released after sent
            assert(tcp_seg_with_event(seg));
            //do not execute rexmit event from ip-input processing
            if (s->ip->tcp_input_socket == s) {
                break;
            }

            tcp_event_seg(teREXMIT, seg, s);
        }

        if ( (seg->tcphdr != 0) && (seg->p != 0) )
	    {
            if (tcp_segment_seqno(seg) - s->lastack + seg->len > wnd){
                tcp_debug("tcp_output : break seg $%x seq %lu by window\n", seg, tcp_segment_seqno(seg));
                break;
            }

		tcp_debug ("tcp_output: snd_wnd %lu, cwnd %lu, wnd %lu, effwnd %lu, seq %lu, ack %lu\n",
			s->snd_wnd, s->cwnd, wnd,
			tcp_segment_seqno(seg) + seg->len - s->lastack,
			tcp_segment_seqno(seg), s->lastack);

		if (s->state != SYN_SENT) {
			seg->tcphdr->flags |= TCP_ACK;
			s->flags &= ~(TF_ACK_DELAY | TF_ACK_NOW);
		}

	    over = netif_is_overlaped(seg->p);
		tcpseglen = TCP_TCPLEN (seg);
		if (tcpseglen == 0){
            //empty buffer not acked, so we can free it right in after send
	        if (over != 0) {
	            //empty overlaped buffer should with no action, to manage netif_output
	            //  to free it after tx
	            over->options &= ~nioo_ActionMASK;
	            over->options |= nioo_ActionNone;
	        }
		}
		else { //if (seg->next != 0) 
		      tcp_output_segment_arm(s, seg);
		}

		if (!tcp_output_segment (seg, s)){
		    tcp_debug("tcp_output : break seg $%x seq %lu by net\n", seg, tcp_segment_seqno(seg));
		    break;
		}
	    }
		else //if ( (seg->tcphdr != 0) && (seg->p != 0) )
	        assert(seg->len == 0);

		s->unsent = seg->next;
		/* put segment on unacknowledged list if length > 0 */
		if (tcpseglen > 0) {
			seg->next = 0;
			if (s->unacked == 0) {
				s->unacked = seg;
			} else {
				for (useg = s->unacked; useg->next != 0; useg = useg->next)
					;
				useg->next = seg;
			}
		} else {
		    if (over != 0)
                //empty overlaped buffer managed by netif_output to free it after tx
		        seg->p = 0;
			tcp_segment_free (s, seg);
		}
		seg = s->unsent;
	}//while (seg != 0 
	
#if TCP_LOCK_STYLE == TCP_LOCK_RELAXED
	iptx_unlock_ensure(ip_locked);
#endif
	
	} //else if (seg == 0)

    tcpo_unlock_ensure(s_locked);

	return 1;
}


/*
 * Find out what we can send and send it. poll sending until unsent queue empty.
 * Must be called with ip locked        if TCP_LOCK_LEGACY.
 * suposed that sock is unlocked        if TCP_LOCK_SURE
 *      and suposed that socket lock ensures by locked ip
 * or ensures that socket locked        if TCP_LOCK_RELAXED
 * Return 0 on error.
 */
int
tcp_output (tcp_socket_t *s)
{
    /* First, check if we are invoked by the TCP input processing code.
     * If so, we do not output anything. Instead, we rely on the input
     * processing code to call us when input processing is done with. */
    if (s->ip->tcp_input_socket == s) {
        if (mutex_is_my(&s->ip->lock))
            return 1;
    }

    if ((s->unsent == 0) && !(s->flags & TF_ACK_NOW))
        return 1;

#if (TCP_LOCK_STYLE >= TCP_LOCK_RELAXED) || (IP_LOCK_STYLE == IP_LOCK_STYLE_OUT1)
    mutex_t* s_locked = tcpo_lock_ensure(&s->lock);
#elif TCP_LOCK_STYLE <= TCP_LOCK_SURE
    mutex_t* s_locked = iptx_lock_ensure(s->ip);
#endif

    while ((s->unsent != 0) || (s->flags & TF_ACK_NOW))
    {
        tcp_output_poll(s);
        if (s->unsent == 0)
            break;

#if (TCP_LOCK_STYLE >= TCP_LOCK_RELAXED) || (IP_LOCK_STYLE == IP_LOCK_STYLE_OUT1)
        mutex_wait(&s->lock);
#elif TCP_LOCK_STYLE <= TCP_LOCK_SURE
        mutex_t* sock_locked = tcpo_lock_ensure(&s->lock);
        //if (!s_locked) //caller have alredy locked iplock
        iptx_unlock_ensure(IP_TX_LOCK(s->ip));
        mutex_wait(&s->lock);
        tcpo_unlock_ensure(sock_locked);
        iptx_lock_ensure(s->ip);
#endif
        /* TODO Может Проверим, не закрылось ли соединение??? */
    }

    tcpo_unlock_ensure(s_locked);
    return 1;
}

void
tcp_rst (ip_t *ip, unsigned long seqno, unsigned long ackno,
	unsigned char *local_ip, unsigned char *remote_ip,
	unsigned short local_port, unsigned short remote_port)
{
	buf_t *p;
	tcp_hdr_t *tcphdr;

	p = buf_alloc (ip->pool, TCP_HLEN, 16 + IP_HLEN);
	if (p == 0) {
		tcp_debug ("tcp_rst: could not allocate memory\n");
		return;
	}

	tcphdr = (tcp_hdr_t*) p->payload;
	tcphdr->src = HTONS (local_port);
	tcphdr->dest = HTONS (remote_port);
	tcphdr->seqno = HTONL (seqno);
	tcphdr->ackno = HTONL (ackno);
	tcphdr->flags = TCP_RST | TCP_ACK;
	tcphdr->wnd = HTONS (TCP_WND);
	tcphdr->urgp = 0;
	tcphdr->offset = 5 << 4;

	tcphdr->chksum = 0;
	tcphdr->chksum = buf_chksum (p, crc16_inet_header (local_ip,
		remote_ip, IP_PROTO_TCP, p->tot_len));

	++ip->tcp_out_datagrams;
	ip_output (ip, p, remote_ip, local_ip, IP_PROTO_TCP);
	tcp_debug ("tcp_rst: seqno %lu ackno %lu.\n", seqno, ackno);
}

void
tcp_rexmit (tcp_socket_t *s)
{
	tcp_segment_t *seg;

	if (s->unacked == 0) {
		return;
	}

	/* Move all unacked segments to the unsent queue. */
	for (seg = s->unacked; seg->next != 0; seg = seg->next);

	seg->next = s->unsent;
	s->unsent = s->unacked;

	s->unacked = 0;

	s->snd_nxt = tcp_segment_seqno(s->unsent);
	tcp_debug ("tcp_rexmit: snd_nxt = %u\n", s->snd_nxt);

	++s->nrtx;

	/* Don't take any rtt measurements after retransmitting. */
	s->rttest = 0;

	/* Do the actual retransmission. */
	//* if this call not post all segments, transmitions can hung, and it will
	//* continue by sender task when it polls tcp_output
	tcp_output_poll(s);
}

#ifdef UTCP_RAW
//* \brief passes buffer for tcp send, with extra events-handling 
//* \arg evhandle invokes from ip-thread and allows data-bufer manipulation 
//*         during segment life, allows more stricted memory use.
//*         main goal is - free buffer after teSENT, forget sended data on teFREE, 
//*         and provide buffer with data on teREXMIT event.
//* \arg data   buffer with user data, must have preserved header TCP_SEG_RESERVE size
int tcp_write_buf (tcp_socket_t *s, buf_t* p, tcps_flag_set flags
                    , tcp_callback evhandle, unsigned long harg)
{
    tcp_debug ("tcp_write_buf(s=%p, buf=%p, len=%u, arg=%x)\n",
        (void*) s, p, p->tot_len, harg);

    if (!tcp_socket_is_state(s, TCP_STATES_TRANSFER)) {
        tcp_debug ("tcp_write() called in invalid state $%x\n", s->state);
        return -1;
    }
    unsigned long len = p->tot_len;
    if (len == 0) {
        return -1;
    }

    unsigned segn = 1;
    if (flags & TF_SEG_CHAIN)
        segn = buf_chain_len(p);
    else
        assert(len < s->mss);

    unsigned char queuelen;

    /* Check if the queue length exceeds the configured maximum queue
     * length. If so, we return an error. */
    queuelen = s->snd_queuelen;
    if (queuelen >= (TCP_SND_QUEUELEN-segn) ) {
        tcp_debug ("tcp_write_buf: too long queue %u (max %u)\n",
            queuelen, TCP_SND_QUEUELEN);
        if (flags & TF_NOBLOCK)
            return 0;
        mutex_t* s_locked = tcpo_lock_ensure(&s->lock);
        while (s->snd_queuelen >= (TCP_SND_QUEUELEN-segn) ) {
            if ((s->unsent != 0)){
#               if TCP_LOCK_STYLE <= TCP_LOCK_SURE
                mutex_unlock (&s->lock);
                tcp_output_poll (s);
                mutex_lock (&s->lock);
#               elif TCP_LOCK_STYLE <= TCP_LOCK_RELAXED
                tcp_output_poll (s);
#               endif
            }
            else
                mutex_wait(&s->lock);
            if (!tcp_socket_is_state(s, TCP_STATES_TRANSFER)) {
                tcp_debug ("tcp_write() called in invalid state $%x\n", s->state);
                tcpo_unlock_ensure(s_locked);
                return -1;
            }
        }
        //tcpo_unlock_ensure(s_locked);
    }

    /* First, break up the data into segments and tuck them together in
     * the local "queue" variable. */

    tcp_segment_t *seg;
    tcp_segment_t *pseg = 0;
    tcp_segment_t *que  = 0;
    buf_t* nextp = 0;

    while (p != 0){
        /* Allocate memory for tcp_segment, and fill in fields. */
        seg = tcp_segment_alloc_maylock(s);
        if (seg == 0) {
            if (mutex_is_my(&s->lock))
                mutex_unlock(&s->lock);
            tcp_debug ("tcp_write_buf: cannot allocate tcp_segment\n");
            return 0;
        }
        if (que == 0)
            que = seg;
        if (pseg == 0)
            ;
        else
            pseg->next = seg;

        if (flags & TF_SEG_CHAIN){
            nextp = buf_dechain(p);
            assert(p->tot_len < s->mss);
        }

        seg->datacrc = 0;
        seg->handle  = 0;
        if (tcp_segment_assign_buf(s, seg, p) == 0){
            //отсоединю обратно в цепь буферы из цепи сегментов
            p = que->p;
            que->p = 0;
            for (seg = que->next; seg != 0; seg = seg->next){
                buf_chain(p, seg->p);
                seg->p = 0;
            }
            if (nextp != 0)
                buf_chain(p, nextp);

            tcp_segments_free(s, que);
            if (mutex_is_my(&s->lock))
                mutex_unlock(&s->lock);
            return 0;
        }

        /* Build TCP header. */
        tcp_hdr_t* tcphdr = (tcp_hdr_t*) seg->tcphdr;
        tcphdr->urgp = 0;
        tcphdr->flags = flags;
        /* don't fill in tcphdr->ackno and tcphdr->wnd until later */
        
        seg->handle = evhandle;
        seg->harg = harg;
        if (evhandle)
            flags |= TF_NOCORK;

        p = nextp;
        pseg = seg;
    }//while (p != 0)

        tcpo_lock_ensure(&s->lock);
        int res = (tcp_pass_segments(s, que, flags) != 0)? len : 0;

#       if TCP_LOCK_STYLE <= TCP_LOCK_SURE
        mutex_unlock(&s->lock);
        tcp_output (s);
#       elif TCP_LOCK_STYLE >= TCP_LOCK_RELAXED
        tcp_output (s);
        mutex_unlock(&s->lock);
#       endif
        //tcpo_unlock_ensure(s_locked);

        //assert (tcp_debug_verify (s->ip));
        return res;
}
#endif //UTCP_RAW
