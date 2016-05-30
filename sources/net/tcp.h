#ifndef __TCP_H_
#define __TCP_H_ 1

#include <net/ip.h>
#include <buf/buf-queue-header.h>

#ifdef __cplusplus
extern "C" {
#endif



/*
 * TCP options
 */
#ifndef TCP_WND
#define TCP_WND                         2048
#endif

#ifndef TCP_MAXRTX
#define TCP_MAXRTX                      12
#endif

#ifndef TCP_SYNMAXRTX
#define TCP_SYNMAXRTX                   6
#endif

/* TCP Maximum segment size. */
#ifndef TCP_MSS
#define TCP_MSS				256	/* conservative default */
#endif

/* TCP sender buffer space (bytes). */
#ifndef TCP_SND_BUF
#define TCP_SND_BUF                     1024
#endif

/* TCP sender buffer space (pbufs).
 * This must be at least = 2 * TCP_SND_BUF/TCP_MSS for things to work. */
#ifndef TCP_SND_QUEUELEN
#define TCP_SND_QUEUELEN                4 * TCP_SND_BUF/TCP_MSS
#endif


/* Maximum number of retransmissions of data segments. */

/* Maximum number of retransmissions of SYN segments. */

/* TCP writable space (bytes). This must be less than or equal
   to TCP_SND_BUF. It is the amount of space which must be
   available in the tcp snd_buf for select to return writable */
#ifndef TCP_SNDLOWAT
#define TCP_SNDLOWAT                    TCP_SND_BUF/2
#endif

typedef enum _tcp_state_t {
	CLOSED		= 0,
	LISTEN		= 1,
	SYN_SENT	= 2,
	SYN_RCVD	= 3,
	ESTABLISHED	= 4,
	FIN_WAIT_1	= 5,
	FIN_WAIT_2	= 6,
	CLOSE_WAIT	= 7,
	CLOSING		= 8,
	LAST_ACK	= 9,
	TIME_WAIT	= 10,
} tcp_state_t;

#ifndef TCP_TMR_INTERVAL
#define TCP_TMR_INTERVAL	100	/* TCP timer interval in msec. */
#endif

#ifndef TCP_FAST_INTERVAL
#define TCP_FAST_INTERVAL	200	/* fine grained timeout in msec */
#endif

#ifndef TCP_SLOW_INTERVAL
#define TCP_SLOW_INTERVAL	500	/* coarse grained timeout in msec */
#endif

#define TCP_STUCK_TIMEOUT	5000	/* milliseconds */

#define TCP_MSL			60000  /* maximum segment lifetime in usec */

#ifndef TCP_LOCAL_PORT_RANGE_START
#define TCP_LOCAL_PORT_RANGE_START 4096
#define TCP_LOCAL_PORT_RANGE_END   0x7fff
#endif

#ifndef TCP_LOCK_STYLE
/**
 * tcp_enqueue, tcp_output affects an socket internals vs ip-input thread
 * by TCP_LOCK_LEGACY  - ones routines requres locked ip, and rely on assumption 
 *              that in locked ip, socket is not concurented by other threads
 *              it was relyes that socked is unlocked at call,   and suposed that 
 *                  socket lock ensures by locked ip
 *              it is for selfcommenting code (same as TCP_LOCK_STYLE not defined) - not actualy used
 * by TCP_LOCK_SURE    - like a legacy + tcp_output ensures that it have locks ip
 * !!!  styles above must be with IP_LOCK_STYLE_BASE 
 * NOTE since it is use more locks for operation, modes below is slower vs TCP_LOCK_SURE
 *      them interest if need more sockets or protocols concurence vs tcp 
 * by TCP_LOCK_RELAXED  - lock ip less time - only on group segments are posted,
 * by TCP_LOCK_RELAXED2 - lock ip less time - only on every segment posted to netif
 * */
#define TCP_LOCK_LEGACY     -1
#define TCP_LOCK_SURE       0
#define TCP_LOCK_RELAXED    1
#define TCP_LOCK_RELAXED2   2
#define TCP_LOCK_STYLE      TCP_LOCK_SURE
#endif

#if (IP_LOCK_STYLE > IP_LOCK_STYLE_BASE)
#   if (TCP_LOCK_STYLE < TCP_LOCK_RELAXED)
#       warning "TCP_LOCK_STYLE forced to TCP_LOCK_RELAXED due inapropiate IP_LOCK_STYLE"
#       undef TCP_LOCK_STYLE
#       define TCP_LOCK_STYLE TCP_LOCK_RELAXED
#   endif
#endif

//* TCP_IP_HCACHE - активирует опцию TF_IP_NOCACHE, включает кеширование заголовка
//*     IP-пакета для ТСП сокетов. добавляет 7-10% производительности за счет расхода памяти
//*     и раздувания кода
//#define TCP_IP_HCACHE 0

/*
 * TCP header.
 */
typedef enum{
      TCP_FIN         = 0x01
    , TCP_SYN         = 0x02
    , TCP_RST         = 0x04
    , TCP_PSH         = 0x08
    , TCP_ACK         = 0x10
    , TCP_URG         = 0x20

    , TCP_FLAGS       = 0x3f
} tcph_flags;
typedef unsigned char tcph_flags_byte;

struct _tcp_hdr {
	unsigned short src;
	unsigned short dest;
	unsigned long seqno;
	unsigned long ackno;
#define TCP_SEQ_LT(a,b)		((long)((a)-(b)) < 0)
#define TCP_SEQ_LEQ(a,b)	((long)((a)-(b)) <= 0)
#define TCP_SEQ_GT(a,b)		((long)((a)-(b)) > 0)
#define TCP_SEQ_GEQ(a,b)	((long)((a)-(b)) >= 0)

	unsigned char offset;
	tcph_flags_byte flags; //tcph_flag_set

	unsigned short wnd;

	unsigned short chksum;
	unsigned short urgp;
} __attribute__ ((packed));

typedef struct _tcp_hdr tcp_hdr_t;

/* Length of the TCP header, excluding options. */
#define TCP_HLEN		20
#define TCP_CAPLEN       (TCP_HLEN + IP_HLEN + MAC_HLEN)
#define TCP_HRESERVE     IP_ALIGNED(TCP_CAPLEN)
#define TCP_SEG_RESERVE  IP_ALIGNED(TCP_HRESERVE+NETIO_OVERLAP_HLEN)

#ifdef ETH_MTU
//* check MSS vs MTU - tcp-packet len must not exceed eth MTU limit
#if (ETH_MTU - TCP_CAPLEN) < TCP_MSS
#undef TCP_MSS
#define TCP_MSS (ETH_MTU - TCP_CAPLEN)
#endif
#endif


struct _tcp_segment_t;
typedef struct _tcp_segment_t tcp_segment_t;
struct _tcp_socket_t;
typedef struct _tcp_socket_t tcp_socket_t;


//* this is raw events handler for tcp-segments. this handle intends for 
//* proper buffer utilisation. 
//* see tcp_write_raw
typedef enum{
    //after eth output called, and from this seg->hsave.seqno can be used for
    //  rexmit data recovery
      teSENT  
    , teFREE    //after seg acked, and can be free
    , teDROP    //seg aborted< and buffer can be free
    , teREXMIT  //on ack loose, need provide data for seg
} tcp_cb_event;
typedef void (*tcp_callback)(tcp_cb_event ev
                            , tcp_segment_t* seg
                            , tcp_socket_t* s
                            );

/*
 * This structure is used to repressent TCP segments when queued.
 */
struct _tcp_segment_t {
	struct _tcp_segment_t *next; /* used when putting segements on a queue */
	struct _buf_t *p;	/* buffer containing data + TCP header */
	//! TF_TRAP_LOOSE use this pointer =NULL, to denote that frame should be loose
	void *dataptr;		/* pointer to the TCP data in the buf_t */
	tcp_hdr_t *tcphdr;	/* the TCP header */
	small_uint_t len;	/* the TCP length of this segment */
	small_uint_t datacrc;// crc16_inet of data at dataptr, ==0 if no crc
                         // spare_segs use it for segTimeLive ticks

#ifdef UTCP_RAW
	tcp_callback    handle;
	long            harg;
	struct hdr_save{
	    //* this saves used for rexmit tcp hdr restoration, when socket event handle
	    //*     provides data
	    unsigned long seqno;
	    unsigned long ackno;    //* this is just for debug interest 
	    unsigned short urgp;
	} hsave;
#endif
};

typedef enum{
      TF_ACK_DELAY  = 0x01        /* Delayed ACK. */
    , TF_ACK_NOW    = 0x02        /* Immediate ACK. */
    , TF_INFR       = 0x04        /* In fast recovery. */
    , TF_RESET      = 0x08        /* Connection was reset. */
    , TF_CLOSED     = 0x10        /* Connection was sucessfully closed. */
    , TF_GOT_FIN    = 0x20        /* Connection closed by remote end. */
    , TF_NOCORK     = 0x100         //* refuse TCP segments optimiation - don`t combine small segments into big one
    , TF_NOBLOCK    = 0x400       //*< tcp_write/read returns imidiately, not waiting for all data enqueued

#if TCP_IP_HCACHE > 0
    , TF_IP_NOCACHE = 0x4000      //* disables ip router resolution cache, always route ip-frames
#else
    , TF_IP_NOCACHE = 0           //* disables ip router resolution cache, always route ip-frames
#endif
    //*< emulates segment loose - with tcp_write_buf post segment, 
    //*     that not actualy transmited 
    , TF_TRAP_LOOSE = 0x8000
} tcps_flags;
typedef unsigned short tcps_flag_set;
typedef tcps_flag_set  tcph_flag_set;
/*
 * The TCP protocol control block
 */
struct _tcp_socket_t {//: base_socket_t
    //! lock demarcate access between send/recv, and it must not use to demarcate
    //  concurent recv or sends - their can be interleaved
	mutex_t lock;
    
	struct _ip_t *ip;
	struct _tcp_socket_t *next;	/* for the linked list */

	ip_addr_const  local_ip;
	unsigned short local_port;
	//remote_ip for ordinary sockets only, LISTEN socket not demand it
    ip_addr remote_ip;
    unsigned short remote_port;

	tcp_state_t state;		/* TCP state */

	/* queue of received packets */
#define TCP_SOCKET_QUEUE_SIZE	16
	buf_queue_header_t inq;
	struct _buf_t *queue [TCP_SOCKET_QUEUE_SIZE];

	/*
	 * Only above data are valid for sockets in LISTEN state.
	 * All the following data are for ordinary sockets only.
	 */

	/* receiver varables */
	unsigned long rcv_nxt;		/* next seqno expected */
	unsigned short rcv_wnd;		/* receiver window */

	/* Timers */
	unsigned long tmr;

	/* Retransmission timer. */
	unsigned short rtime;

	unsigned short mss;		/* maximum segment size */

	/* RTT estimation variables. */
	unsigned short rttest;		/* RTT estimate in 500ms ticks */
	unsigned long rtseq;		/* sequence number being timed */
	short sa, sv;

	unsigned short rto;		/* retransmission time-out */
	unsigned char nrtx;		/* number of retransmissions */

	/* fast retransmit/recovery */
    unsigned char dupacks;
	unsigned long lastack;		/* Highest acknowledged seqno. */

	/* congestion avoidance/control variables */
	unsigned short cwnd;
	unsigned short ssthresh;

	/* sender variables */
	unsigned long snd_nxt,		/* next seqno to be sent */
		snd_max,		/* Highest seqno sent. */
		snd_wnd,		/* sender window */
		snd_wl1, snd_wl2,	/* Sequence and acknowledgement numbers
					 * of last window update. */
		snd_lbb;		/* Sequence number of next byte
					 * to be buffered. */
	unsigned short acked;

	unsigned short snd_buf;		/* Available bytes for sending. */
	unsigned char snd_queuelen;	/* Available tcp_segment's for sending. */

	tcps_flag_set    flags;

	/* These are ordered by sequence number: */
	tcp_segment_t *unsent;		/* Unsent (queued) segments. */
	tcp_segment_t *unacked;		/* Sent but unacknowledged segments. */
    tcp_segment_t *spare_segs;        /* segments not busy, and can be used for new data */
	
	ip_header_cache*    iph_cache;
};



/***************************************************************************
 * Application program's interface (API):
 ***************************************************************************/

/*
 * Connect to another host. Wait until connection established.
 * Return 1 on success, 0 on error.
 */
tcp_socket_t *tcp_connect (ip_t *ip, unsigned char *ipaddr, unsigned short port);

/*
 * Create socket and start connection to another host.
 * ! not wait until connection established.
 * \return socket in connection state
 * \return      = SExxx - some error
 */
tcp_socket_t *tcp_connect_start (ip_t *ip,  ip_addr ipaddr, unsigned short port);

/*
 * Set the state of the connection to be LISTEN, which means that it
 * is able to accept incoming connections. The protocol control block
 * is reallocated in order to consume less memory. Setting the
 * connection to LISTEN is an irreversible process.
 */
tcp_socket_t *tcp_listen (ip_t *ip, unsigned char *ipaddr,
	unsigned short port);

/* It extracts the first connection request on the queue of pending connections 
 *  for the listening socket s, creates a new connected socket, 
 *  and returns a new tcp_socket. 
 *  The newly created socket is not in the listening state.
 *  \return      = NULL if socket s aborted of failed
 */
tcp_socket_t *tcp_accept (tcp_socket_t *s);

/* It extracts the first connection request on the queue of pending connections 
 *  for the listening socket s, creates a new connected socket, 
 *  and returns a new tcp_socket. 
 *  The newly created socket is not in the listening state.
 *   
 * \param waitarg - if (waitfor==0) and (waitarg!=0) - this assumes as nonblocking operation
 * \return      = NULL  - if timedout with no data
 * \return      = SExxx - some error
 * */
tcp_socket_t *tcp_accept_until (tcp_socket_t *s
                                , scheduless_condition waitfor, void* waitarg);

/*
 * Closes the connection held by the PCB.
 * Return 1 on success, 0 on error.
 */
int tcp_close (tcp_socket_t *s);

/*
 * Aborts a connection by sending a RST to the remote host and deletes
 * the local protocol control block. This is done when a connection is
 * killed because of shortage of memory.
 */
void tcp_abort (tcp_socket_t *s);

/* \~russian ожидает появления данных в приемнике сокета
 * \return = 0 - if socket have some dats in receiver
 *         = -1 - if timedout with no data
 *         = SEerror_code - on error
 * */
int tcp_wait_avail(tcp_socket_t *s
                    , scheduless_condition waitfor, void* waitarg);

/*
 * Blocking receive.
 * Return a number of received bytes >0.
 * Return <0 on error.
 */
int tcp_read (tcp_socket_t *s, void *dataptr, unsigned short len);

/*
 * Receive len>0 bytes. Return <0 on error.
 * When nonblock flag is zero, blocks until data get available (never returns 0).
 * When nonblock flag is nonzero, returns 0 if no data is available.
 */
int tcp_read_poll (tcp_socket_t *s, void *dataptr, unsigned short len, int nonblock);

/** alternative tcp_read_poll with condition test function waitfor
 * \arg waitfor - simple test function that must not affects mutex_xxx and schedule functionality
 * \arg waitarg - if (waitfor==0) and (waitarg!=0) - this assumes as nonblocking operation
 * */
int tcp_read_until (tcp_socket_t *s, void *dataptr, unsigned short len
                , scheduless_condition waitfor, void* waitarg);

/* reads 1 received socket buffer - vs tcp_read_until do not internal copy to user buffer
 * \arg waitarg - if (waitfor==0) and (waitarg!=0) - this assumes as nonblocking operation
 * */
buf_t* tcp_read_buf_until (tcp_socket_t *s
                , scheduless_condition waitfor, void* waitarg);

//* non blocking version of tcp_read_buf_until
buf_t* tcp_take_buf(tcp_socket_t *s);

/*
 * Send len>0 bytes.
 * Return a number ob transmitted bytes, or -1 on error.
 */
int tcp_write (tcp_socket_t *s, const void *dataptr, unsigned short len);


#ifdef UTCP_RAW

//* \brief passes buffer for tcp send, with extra events-handling 
//* \arg evhandle invokes from ip-thread and allows data-bufer manipulation 
//*         during segment life, allows more stricted memory use.
//*         main goal is - free buffer after teSENT, forget sended data on teFREE, 
//*         and provide buffer with data on teREXMIT event.
//* \arg data   buffer with user data, must have preserved header TCP_SEG_RESERVE size
int tcp_write_buf (tcp_socket_t *s, buf_t* data, tcps_flag_set opts
                    , tcp_callback evhandle, unsigned long harg);

//* uTCP segment handle helper - assign data bufer to segment.
//*     intended for use on teREXMIT
int tcp_segment_assign_buf(tcp_socket_t *s, tcp_segment_t * seg, buf_t* p);

#endif // UTCP_RAW

/*
 * Return the period of socket inactivity, in seconds.
 */
unsigned long tcp_inactivity (tcp_socket_t *s);



typedef enum _tcp_state_f {
    tcpfCLOSED     = (1<<CLOSED),
    tcpfLISTEN     = (1<<LISTEN),
    tcpfSYN_SENT    = (1<<SYN_SENT),
    tcpfSYN_RCVD    = (1<<SYN_RCVD),
    tcpfESTABLISHED = (1<<ESTABLISHED),
    tcpfFIN_WAIT_1  = (1<<FIN_WAIT_1),
    tcpfFIN_WAIT_2  = (1<<FIN_WAIT_2),
    tcpfCLOSE_WAIT  = (1<<CLOSE_WAIT),
    tcpfCLOSING     = (1<<CLOSING),
    tcpfLAST_ACK    = (1<<LAST_ACK),
    tcpfTIME_WAIT   = (1<<TIME_WAIT),
} tcp_state_f;
typedef /*tcp_state_f*/ unsigned tcp_states_set;

INLINE
bool_t   tcp_socket_is_state(tcp_socket_t *s, tcp_states_set states){
    return (((1<<s->state) & states) != 0);
}

#define TCP_STATES_ONLINE   (((tcpfESTABLISHED<<1)-1) ^ (tcpfLISTEN-1))
#define TCP_STATES_TRANSFER (((tcpfESTABLISHED<<1)-1) ^ ((tcpfLISTEN<<1)-1))

INLINE
bool_t   tcp_socket_is_online(tcp_socket_t *s){
    return tcp_socket_is_state(s, TCP_STATES_ONLINE);
}

static int tcp_queue_is_empty (tcp_socket_t *q);

INLINE
bool_t tcp_socket_is_avail(tcp_socket_t *s){
    return !tcp_queue_is_empty(s);
}



/*********************************************************************
 *      TCP stream :
 **********************************************************************/

#if defined(to_stream)
/*
 * Stream interface adapter for TCP socket.
 * To use it, you must include stream/stream.h before net/tcp.h.
 */
typedef struct _tcp_stream_t {
	stream_t stream;
	tcp_socket_t *socket;

	unsigned char *inptr;
	struct _buf_t *inbuf;

	unsigned char *outptr;
	unsigned char outdata [1000];
} tcp_stream_t;

stream_t *tcp_stream_init (tcp_stream_t *u, tcp_socket_t *sock);
#endif /* to_stream */



/*********************************************************************
 * Lower layer interface (LLI) to TCP:
 **********************************************************************/
void tcp_slowtmr (ip_t *ip) __attribute__((weak));
void tcp_fasttmr (ip_t *ip) __attribute__((weak));
void tcp_input (struct _ip_t *ip, struct _buf_t *p, struct _netif_t *inp,
	struct _ip_hdr_t *iph) __attribute__((weak));

/* Used within the TCP code only: */
tcp_socket_t *tcp_alloc (ip_t *ip);

/* tcp_output_poll - pull tcp segments until netif accept them
 * */
int tcp_output_poll (tcp_socket_t *s);

/* tcp_output - pull tcp segments until all unsent passed to netif
 * */
int tcp_output (tcp_socket_t *s);

void tcp_rexmit (tcp_socket_t *s);
void tcp_parseopt (tcp_socket_t *s, tcp_hdr_t *h);
struct _buf_t *tcp_queue_get (tcp_socket_t *q);
void tcp_queue_put (tcp_socket_t *q, struct _buf_t *p);
void tcp_queue_free (tcp_socket_t *q);

static inline int
tcp_queue_is_full (tcp_socket_t *q)
{
	/*debug_printf ("tcp_queue_is_full: returned %d\n", (q->count == TCP_SOCKET_QUEUE_SIZE));*/
	return (q->inq.count == TCP_SOCKET_QUEUE_SIZE);
}

static inline int
tcp_queue_is_empty (tcp_socket_t *q)
{
	/*debug_printf ("tcp_queue_is_full: returned %d\n", (q->count == TCP_SOCKET_QUEUE_SIZE));*/
	return (q->inq.count == 0);
}



/***********************************************************************************
 * Internal functions and global variables:
 ************************************************************************************/
tcp_socket_t *tcp_socket_copy (tcp_socket_t *s);
void tcp_socket_purge (tcp_socket_t *s);
void tcp_socket_remove (tcp_socket_t **socklist, tcp_socket_t *s);
void tcp_set_socket_state (tcp_socket_t *s, tcp_state_t newstate);

unsigned char tcp_segments_free (tcp_socket_t *s, tcp_segment_t *seg);
unsigned char tcp_segment_free (tcp_socket_t *s, tcp_segment_t *seg);
//* забирает сегмент из кучи доступных сегментов (ранее созданных)
//* \return NULL - нет свободных сегментов, !!! сокет может захватиться
//* \return  - возвращает новый сегмент, и сокет (s->lock) в захваченом состоянии
tcp_segment_t *tcp_segment_takespare_maylock (tcp_socket_t *s);
//* берет свободный сегмент, или создает новый.
//  !!! может вернуть сокет, может захваченым 
tcp_segment_t *tcp_segment_alloc_maylock(tcp_socket_t *s);

#define TCP_TCPLEN(seg) ((seg)->len + (((seg)->tcphdr->flags & \
            (TCP_FIN | TCP_SYN)) ? 1 : 0))

#ifdef UTCP_RAW
INLINE
unsigned long tcp_segment_seqno(tcp_segment_t* seg){
    return seg->hsave.seqno;
}

#else
INLINE
unsigned long tcp_segment_seqno(tcp_segment_t* seg){
    return NTOHL (seg->tcphdr->seqno);
}
#endif

INLINE
unsigned long tcp_segment_seqafter(tcp_segment_t* seg){
    return tcp_segment_seqno(seg) + TCP_TCPLEN(seg);
}

static inline void
tcp_ack_now (tcp_socket_t *s)
{
	s->flags |= TF_ACK_NOW;
	tcp_output (s);
}

static inline void
tcp_ack (tcp_socket_t *s)
{
	if (s->flags & TF_ACK_DELAY) {
		s->flags &= ~TF_ACK_DELAY;
		tcp_ack_now (s);
	} else {
		s->flags |= TF_ACK_DELAY;
	}
}

/*
 * Calculates an initial sequence number for new connections.
 */
static inline unsigned long
tcp_next_seqno (ip_t *ip)
{
	ip->tcp_seqno += ip->tcp_ticks + 1;
	return ip->tcp_seqno;
}

/** !!! it ensures that socket is lock, and leave it locked after return!
 *  \arg flags - TCP_FLAGS and if TCP_SOCK_LOCK - leaves s locked after return  
 * \return  = amount of passed data
*/
int tcp_enqueue (tcp_socket_t *s
                , const void *dataptr, small_uint_t len
                , tcph_flag_set flags);

/** !!! it ensures that socket is lock, and leave it locked after return!
 *  \arg flags - TCP_FLAGS and if TCP_SOCK_LOCK - leaves s locked after return  
 * \return  = amount of passed data
*/
int tcp_enqueue_option4 (tcp_socket_t *s
                , tcph_flag_set flags
                , unsigned long optdata);

void tcp_rexmit_seg (tcp_socket_t *s, tcp_segment_t *seg);

void tcp_rst (ip_t *ip, unsigned long seqno, unsigned long ackno,
	unsigned char *local_ip, unsigned char *remote_ip,
	unsigned short local_port, unsigned short remote_port);

unsigned long tcp_next_seqno (ip_t *ip);

void tcp_debug_print_header (tcp_hdr_t *tcphdr);
void tcp_debug_print_flags (tcph_flag_set flags);
const char *tcp_state_name (tcp_state_t s);
void tcp_debug_print_sockets (ip_t *ip);
int tcp_debug_verify (ip_t *ip);

#ifdef TCP_DEBUG
extern const unsigned char tcp_flags_dumpfmt[];
#   ifndef TCP_PRINTF
#define tcp_debug			debug_printf
#   else
#define tcp_debug(...)      TCP_PRINTF(__VA_ARGS__)
#   endif
#else
#define tcp_debug(...)			/* void */
#define tcp_debug_print_header(tcphdr)	/* void */
#define tcp_debug_print_flags(flags)	/* void */
#define tcp_debug_print_sockets(ip)	/* void */
#define tcp_debug_verify(ip)		1
#endif /* TCP_DEBUG */

/*
 * The TCP socket lists are defined in ip_t:
 * tcp_listen_sockets	-- List of all TCP sockets in LISTEN state.
 * tcp_sockets		-- List of all TCP sockets that are in a state
 *			   in which they accept or send data.
 * tcp_closing_sockets	-- List of all TCP sockets in TIME-WAIT state.
 *
 * Axioms about TCP socket lists:
 * 1) Every TCP socket that is not CLOSED is in one of the lists.
 * 2) A socket is only in one of the lists.
 * 3) All sockets in the tcp_listen_sockets list is in LISTEN state.
 * 4) All sockets in the tcp_closing_sockets list is in TIME-WAIT state.
 *
 * Define two macros, that register a TCP socket
 * with a socket list or remove a socket from a list.
 */
static inline void
tcp_list_add (tcp_socket_t **socklist, tcp_socket_t *ns)
{
	ns->next = *socklist;
	*socklist = ns;
}

static inline void
tcp_list_remove (tcp_socket_t **socklist, tcp_socket_t *ns)
{
	if (*socklist == ns) {
		*socklist = ns->next;
	} else {
		tcp_socket_t *p;

		for (p = *socklist; p; p = p->next) {
			if (p->next && p->next == ns) {
				p->next = ns->next;
				break;
			}
		}
	}
	ns->next = 0;
}



#ifdef UTCP_RAW
INLINE
void tcp_event_seg(tcp_cb_event ev
                , tcp_segment_t* seg
                , tcp_socket_t* s
                ) 
{
    if (seg->handle != 0)
        seg->handle(ev, seg, s);
}
#else
#define tcp_event_seg(ev, seg, sock)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TCP_H_ */
