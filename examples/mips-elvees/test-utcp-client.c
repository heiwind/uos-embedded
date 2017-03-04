/*
 * Testing TCP protocol: server side.
 */
#include <runtime/lib.h>
#include <stdint.h>
#include <stream/stream.h>
#include <mem/mem.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/arp.h>
#include <timer/timer.h>
#ifdef ELVEES_MCB03
#   include <elvees/eth-mcb.h>
#   define MCB_COMMON_IRQ          34  /* Прерывание от MCB */
#else
#   include <elvees/eth.h>
#endif

#include <buf/buf.h>
#include <buffers/rtlog_buf.h>

#include <trace.h>

//nvcom02tem-3u evboard memory size 
#define SRAM_BANK_SIZE (64<<20)

ARRAY (stack_tcp, 2200);
ARRAY (stack_con, 1800);
ARRAY (group, sizeof(mutex_group_t) + 4 * sizeof(mutex_slot_t));
ARRAY (arp_data, sizeof(arp_t) + 10 * sizeof(arp_entry_t));

mem_pool_t pool;
mem_pool_t posix_pool;
uintptr_t  posix_pool_start;
uintptr_t  posix_pool_end;

arp_t *arp;
#ifdef ELVEES_MCB03
eth_mcb_t eth_data, *eth = &eth_data;
#else
eth_t eth_data, *eth = &eth_data;
#endif
route_t route;
timer_t timer;
ip_t ip;
tcp_socket_t *user_socket;
task_t *console_task;
task_t *h_task;

static const unsigned tcp_loglimit = 1024*sizeof(rtlog_node);
rtlog   tcp_log;

unsigned char server_ip [] = { 172, 0, 0, 20 };
#define PORT 		0xBBBB
#define BUF_SIZE    (300*sizeof(unsigned))
#define BUF_LIMIT   8
typedef enum {
    bsFREE, bsBUSY
} buf_status;
typedef struct {
    buf_t*      p;
    volatile buf_status  state;
} pool_node;

#ifdef RT_PRINTF
#define DEBUG_BUFS
#endif
pool_node  bufs[BUF_LIMIT];
mutex_t    bufs_signal;

int lock_buf();
int leave_buf(buf_t* p);
unsigned fill_buf(buf_t* p, unsigned stamp);
int wait_free_buf();
void buf_tcp_callback(tcp_cb_event ev
                            , tcp_segment_t* seg
                            , tcp_socket_t* s
                            );
unsigned count = 0;
unsigned old_count = 0;

void tcp_task (void *data)
{
	int i;
	int countdown = 0;
	mutex_init(&bufs_signal);
    for (i = 0; i < BUF_LIMIT; ++i){
        bufs[i].p = buf_alloc(&pool, BUF_SIZE, TCP_SEG_RESERVE);
        bufs[i].state = bsFREE;
    }

	for (;;) {
		debug_printf ("Press SPACE to connect to %d.%d.%d.%d:%d\n"
		              "      l - emulate loose sended segment (next l - for continue)\n"
                      "      s - send single segment (next l - for continue)\n"
                      "      1..5 - dump last 100*n log entries\n"
                      "      c - clear runtime log\n"
		        , server_ip[0], server_ip[1], server_ip[2], server_ip[3], PORT
		        );
		while (debug_peekchar() != ' ')
		    task_yield();
		debug_getchar ();
        debug_puts ("go\n");

		user_socket = tcp_connect (&ip, server_ip, PORT);
		if (! user_socket) {
			debug_printf ("Failed to connect!\n");
			continue;
		}

		debug_printf ("Connected to server\n");
		count = 0;
		old_count = 0;

		countdown = -1;
        //countdown = 5;
		for (;;) {
		    int idx = wait_free_buf();
		    while (idx < 0)
		        idx = wait_free_buf();
		    buf_t* p = bufs[idx].p;

		    //trace_pin1_on();
		    unsigned newcount = fill_buf(p, count);
            //trace_pin1_off();

            unsigned flags = 0;

            if (debug_peekchar() == 'l'){
                debug_getchar ();
                flags |= TF_TRAP_LOOSE;
                debug_putchar(0,'@');
                countdown = 2;
            }

            if (debug_peekchar() == 's'){
                debug_getchar ();
                debug_putchar(0,'S');
                countdown = 0;
            }

			/* send a message to the server PORT on machine HOST */
			if (tcp_write_buf (user_socket, p, flags, buf_tcp_callback, count) < 0) {
			    leave_buf(p);
				break;
			}
			count = newcount;

            if (countdown > 0)
                --countdown;
			while (countdown == 0){
		        int c = debug_peekchar();
		        switch (c){
                case 'l' : 
                    debug_getchar ();
		        case ' ' :
                    debug_putchar(0,'>');
                case 's' :
                    countdown = -1;
                    break;
		        }
		        task_yield();
            }
			
			if (debug_peekchar() == ' '){
		        debug_getchar ();
			    break;
			}

			if (count % 1000000 == 0)
		        mdelay (1);
		}
	
        debug_printf ("Disconnected\n");
		tcp_close (user_socket);
		mem_free (user_socket);
		user_socket = 0;
	}
	task_exit(0);
}

void console (void *unused)
{
	unsigned long start, end, elapsed;
	unsigned long long bytes;
	
	start = timer_milliseconds (&timer);

	for (;;) {
		timer_delay (&timer, 1000);
		end = timer_milliseconds (&timer);
		elapsed = end - start;
		bytes = (count - old_count) << 2;
		old_count = count;
		start = end;
		task_set_priority (console_task, 1);
		debug_printf ("snd rate: %lu (bytes/sec)\n", (unsigned) (bytes * 1000 / elapsed));
		int c = debug_peekchar();
		if (c >= 0) 
		if (c != ' ') {
		    debug_putchar(0, c);
		    switch(c){
		    case '1' : rtlog_dump_last(&tcp_log, &debug, 100);break;
            case '2' : rtlog_dump_last(&tcp_log, &debug, 200);break;
            case '3' : rtlog_dump_last(&tcp_log, &debug, 300);break;
            case '4' : rtlog_dump_last(&tcp_log, &debug, 400);break;
            case '5' : rtlog_dump_last(&tcp_log, &debug, 500);break;
            case 'c' : rtlog_clear(&tcp_log); break;
            case 'l' : c = ' ';break;
            case 's' : c = ' ';break;
		    }
		    if (c != ' ')
		        debug_getchar();
		}
		task_set_priority (console_task, 100);
	}
}

void uos_init (void)
{
	/* Configure 16 Mbyte of external Flash memory at nCS3. */
	MC_CSCON3 = MC_CSCON_WS (4);		/* Wait states  */

    extern unsigned _etext[];
    posix_pool_end = SRAM_BANK_SIZE;
    posix_pool_end |= ((unsigned) _etext & 0xF0000000);
    posix_pool_start = ((unsigned)_etext | 0xff)+1;
    /* Динамическая память в SDRAM */
    mem_init (&posix_pool, posix_pool_start, posix_pool_end);
    debug_printf("posix: pool $%x ... $%x\n", posix_pool_start, posix_pool_end);

    /* Динамическая память в CRAM */
    extern unsigned __bss_end[];
    extern unsigned _estack[];
    mem_init (&pool, (unsigned) __bss_end, (unsigned) _estack - 256);

    void* log_store = mem_alloc_dirty(&posix_pool, tcp_loglimit);
    assert(log_store != 0);
    rtlog_init(&tcp_log, log_store, tcp_loglimit);

	timer_init (&timer, KHZ, 50);

	/*
	 * Create a group of two locks: timer and eth.
	 */
	mutex_group_t *g = mutex_group_init (group, sizeof(group));
	mutex_group_add (g, &eth->netif.lock);
	mutex_group_add (g, &timer.decisec);

	arp = arp_init (arp_data, sizeof(arp_data), &ip);
	ip_init (&ip, &pool, 70, &timer, arp, g);

	/*
	 * Create interface eth0
	 */
	const unsigned char my_macaddr[] = { 0, 9, 0x94, 0xf1, 0xf2, 0xf3 };
#ifdef ELVEES_MCB03
    // nCS0 и nCS1 для MCB-03
    MC_CSCON0 = MC_CSCON_AE | MC_CSCON_E | MC_CSCON_WS(1) |
        MC_CSCON_CSBA(0x00000000) | MC_CSCON_CSMASK(0xFC000000);
    MC_CSCON1 = MC_CSCON_AE | MC_CSCON_E | MC_CSCON_WS(3) |
        MC_CSCON_CSBA(0x00000000) | MC_CSCON_CSMASK(0xFC000000);

    // Включаем обработчик прерываний от MCB
    mcb_create_interrupt_task (MCB_COMMON_IRQ, 100, 
        stack_mcb, sizeof (stack_mcb));

    eth_mcb_init (eth, "eth0", 80, &pool, arp, my_macaddr);
#else
	eth_init (eth, "eth0", 80, &pool, arp, my_macaddr);
#endif

	unsigned char my_ip[] = { 172, 0, 0, 18 };
	route_add_netif (&ip, &route, my_ip, 24, &eth->netif);
	
	h_task = task_create (tcp_task, 0, "tcp", 45,
		stack_tcp, sizeof (stack_tcp));
		
    console_task = task_create (console, 0, "con", 1,
		stack_con, sizeof (stack_con));

    init_trace();
}

bool_t uos_valid_memory_address (void *ptr)
{
    unsigned address = (unsigned) ptr;

    extern unsigned __data_start[];
    extern unsigned _estack[];

    /* Internal SRAM. */
    if (address >= (unsigned) __data_start &&
        address < (unsigned) _estack)
        return 1;

    if (address >= posix_pool_start &&  address < posix_pool_end)
        return 1;
    return 0;
}




#include <kernel/internal.h>
#include <timer/timer.h>
#include <multicore/nvcom02t.h>

task_t *ethtx_task; 
task_t *ethrx_task; 
task_t *ip4_task;

#define TSK_PORT_DIR    MFBSP1.DIR
#define TSK_PORT_IO     MFBSP1.GPIO_DR

//LDAT0
#define TSK_PIN_TIMER   (1<<2)
//LDAT1
#define TSK_PIN_IDLE    (1<<3)
//LDAT2
#define TSK_PIN_TX      (1<<4)
//LDAT3
#define TSK_PIN_RX      (1<<5)
//LDAT4
#define TSK_PIN_IP      (1<<6)
//LDAT5
#define TSK_PIN_UDP     (1<<7)
//LDAT6
#define TRACE_PIN_0     (1<<8)
//LDAT7
#define TRACE_PIN_1     (1<<9)
#define TRACE_PIN_2     TSK_PIN_IDLE

#define TSK_PIN_CON     (1<<10)

/*task_t **/ unsigned trace_tasks[8] = {0,0,0,0,0,0,0,0};

bool_t trace_timer(void*);

void init_trace(void){
    SYS_REG.CLK_EN.bits.CLKEN_MFBSP = 1;
    asm volatile("nop");
    asm volatile("nop");
    TSK_PORT_DIR.data =   TSK_PIN_TIMER | (0xff << 2);

    ethtx_task = (task_t*)(eth->tstack);
    ethrx_task = (task_t*)(eth->stack);
    trace_tasks[0] = 0;//(unsigned)task_idle;
    trace_tasks[1] = (unsigned)ethtx_task;
    trace_tasks[2] = (unsigned)ethrx_task;
    ip4_task = (task_t*)(ip.stack);
    trace_tasks[3] = (unsigned)ip4_task;

    trace_tasks[4] = (unsigned)h_task;
    trace_tasks[7] = (unsigned)console_task;
}

void uos_on_task_switch(task_t *t)
{
    int i;
    unsigned m = TSK_PIN_IDLE;

    //debug_printf("@%s\n", t->name);
    for (i = 0; i < 8; i++, m = m<<1 ){
        if (trace_tasks[i] != 0){
            if ((unsigned)t == trace_tasks[i]){
                TSK_PORT_IO.data |= m;
                //debug_putchar(0,'0'+i);
                //debug_printf("%x[%d]\n", (unsigned)t, i);
            }
            else
                TSK_PORT_IO.data &= ~m;
        }
    }
}

void uos_on_timer_hook(timer_t *t)
{
    TSK_PORT_IO.data = TSK_PORT_IO.data ^ TSK_PIN_TIMER;
}

void trace_pin0_on(){
    TSK_PORT_IO.data |= TRACE_PIN_0;
}

void trace_pin0_off(){
    TSK_PORT_IO.data &= ~TRACE_PIN_0;
}

uint32_t trace1[3]; 
uint32_t trace1e[3]; 

void trace_pin1_on(){
    TSK_PORT_IO.data |= TRACE_PIN_1;
    //debug_putchar(0, '#');
}

void trace_pin1_off(){
    TSK_PORT_IO.data &= ~TRACE_PIN_1;
    //debug_putchar(0, '$');
}

void trace_pin2_on(){
    TSK_PORT_IO.data |= TRACE_PIN_2;
    //debug_putchar(0, '#');
}

void trace_pin2_off(){
    TSK_PORT_IO.data &= ~TRACE_PIN_2;
    //debug_putchar(0, '$');
}



#ifdef DEBUG_BUFS
#define BUFS_printf(...)    RT_PRINTF(__VA_ARGS__)
//#define BUFS_printf(...)    debug_printf(__VA_ARGS__)
#else
#define BUFS_printf(...)
#endif

int lock_buf(){
    int i;
    for (i = 0; i < BUF_LIMIT; ++i) {
        if (bufs[i].p != 0)
        if (bufs[i].state == bsFREE){
            bufs[i].state = bsBUSY;
            BUFS_printf("buf: lock %d\n", i);
            return i;
        }
    }
    return -1;
}

int leave_buf(buf_t* p){
    int i;
    for (i = 0; i < BUF_LIMIT; ++i) {
        if (bufs[i].p == p){
            BUFS_printf("buf: leave %d\n", i);
            assert(bufs[i].state == bsBUSY);
            bufs[i].state = bsFREE;
            if (mutex_is_wait(&bufs_signal))
                mutex_signal(&bufs_signal, (void*)i);
            return i;
        }
    }
    return -1;
}

int wait_free_buf(){
    mutex_lock(&bufs_signal);
    int res = lock_buf();
    while (res < 0){
        mutex_wait(&bufs_signal);
        res = lock_buf();
    }
    mutex_unlock(&bufs_signal);
    return res;
}

unsigned fill_buf(buf_t* p, unsigned stamp)
{
    //int i;
    buf_reset_header(p, TCP_SEG_RESERVE);
    unsigned len = p->len;
    unsigned* dst = (unsigned*)p->payload;
    unsigned* limit = (unsigned*)(p->payload+len);
    //for (dst = 0; i < limit; ++i)
    for (; dst < limit;)
        *dst++ = stamp++;
    return stamp;
}


void buf_tcp_callback(tcp_cb_event ev
                            , tcp_segment_t* seg
                            , tcp_socket_t* s
                            )
{
    BUFS_printf("tcp:on buf: event %d seg $%x sock $%x\n", ev, seg, s);
    int tmp;
    switch (ev){
    case teSENT :{
            buf_t* p = seg->p;
            netif_io_overlap* over = netif_is_overlaped(p);
            assert(over != 0);
            if ((over->status & nios_IPOK) == 0)
                break;
        }

    case teFREE:    //after seg acked, and can be free
    case teDROP:    //seg aborted< and buffer can be free
        tmp = leave_buf(seg->p);
        if (tmp < 0)
            buf_free(seg->p);
        seg->p          = 0;
        seg->dataptr    = 0;
        seg->tcphdr     = 0;
        break;

    case teREXMIT: //on ack loose, need provide data for seg
        if (seg->p != 0)
            break;

        mutex_lock(&bufs_signal);
        tmp = lock_buf();
        mutex_unlock(&bufs_signal);
        {
            buf_t* p;
            if (tmp >= 0)
                p = bufs[tmp].p;
            else
                p = buf_alloc(&pool, BUF_SIZE, TCP_SEG_RESERVE);
            BUFS_printf("rexmit at %x\n", seg->harg);//BUFS_printf
            fill_buf(p, seg->harg);
            tcp_segment_assign_buf(s, seg, p);
        }
        break;

    default:;
    }
}
