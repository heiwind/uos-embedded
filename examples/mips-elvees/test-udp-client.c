/*
 * Testing TCP protocol: server side.
 */
#include <runtime/lib.h>
#include <stream/stream.h>
#include <mem/mem.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/udp.h>
#include <net/arp.h>
#include <buf/buf.h>
#include <timer/timer.h>
#include <elvees/eth.h>




void init_trace(void);

void trace_pin0_on();
void trace_pin0_off();
void trace_pin1_on();
void trace_pin1_off();
void trace_pin2_on();
void trace_pin2_off();
void trace_pin3_on();
void trace_pin3_off();

#define trace_probe_udp_work 1
#define trace_probe_udp_recv 2
#define trace_probe_udp_error_break 3

#define trace_probe0 0
#define trace_probe1 0

#if trace_probe0 == trace_probe_udp_work
#define trace_udp_work_on()     trace_pin0_on()
#define trace_udp_work_off()    trace_pin0_off()
#elif trace_probe1 == trace_probe_udp_work
#define trace_udp_work_on()     trace_pin1_on()
#define trace_udp_work_off()    trace_pin1_off()
#else
#define trace_udp_work_on()
#define trace_udp_work_off()
#endif

#if trace_probe0 == trace_probe_udp_recv
#define trace_udp_recv_on()     trace_pin0_on()
#define trace_udp_recv_off()    trace_pin0_off()
#elif trace_probe1 == trace_probe_udp_recv
#define trace_udp_recv_on()     trace_pin1_on()
#define trace_udp_recv_off()    trace_pin1_off()
#else
#define trace_udp_recv_on()
#define trace_udp_recv_off()
#endif

#if trace_probe0 == trace_probe_udp_error_break
#define trace_udp_error_break_on()     trace_pin0_on()
#define trace_udp_error_break_off()    trace_pin0_off()
#elif trace_probe1 == trace_probe_udp_error_break
#define trace_udp_error_break_on()     trace_pin1_on()
#define trace_udp_error_break_off()    trace_pin1_off()
#else
#define trace_udp_error_break_on()
#define trace_udp_error_break_off()
#endif



ARRAY (stack_udp, 1500);
ARRAY (group, sizeof(mutex_group_t) + 4 * sizeof(mutex_slot_t));
ARRAY (arp_data, sizeof(arp_t) + 10 * sizeof(arp_entry_t));
mem_pool_t pool;
arp_t *arp;
eth_t eth_data, *eth = &eth_data;
route_t route;
timer_t timer;
ip_t ip;
udp_socket_t sock;
task_t *console_task = 0;
task_t *h_udp_task;

#define PORT 		0xBBBB
#define BUF_SIZE	    365
unsigned buf [BUF_SIZE];
unsigned char server_ip [] = { 172, 0, 0, 10 };

unsigned count = 0;
struct {
     unsigned udps_ip;
} trace_udp;

void udp_task (void *data)
{
	int i;
	buf_t *p;
	unsigned old_count = 0;
	unsigned long start, end, elapsed;
	unsigned long long bytes;
	
	start = timer_milliseconds (&timer);

	debug_printf ("Press ENTER to start sending to %d.%d.%d.%d:%d\n", server_ip[0],
		server_ip[1], server_ip[2], server_ip[3], PORT);
	debug_getchar ();
	
	udp_socket (&sock, &ip, PORT);
	trace_udp.udps_ip = (unsigned)sock.ip;

	for (;;) {
		p = buf_alloc (&pool, BUF_SIZE * 4, 42);

		for (i = 0; i < BUF_SIZE; ++i) 
			buf[i] = count++;
		memcpy (p->payload, buf, sizeof(buf));

        if (!udp_sendto (&sock, p, server_ip, PORT)){
            count -= BUF_SIZE;
            debug_printf ("Press ENTER to start sending to %@.4D:%d\n", server_ip, PORT);
            while (debug_peekchar () < 0)
                task_yield();
            debug_getchar ();
        }
		
		if (timer_passed (&timer, start, 1000)) {
			end = timer_milliseconds (&timer);
			elapsed = end - start;
			bytes = (count - old_count) << 2;
			old_count = count;
			debug_printf ("snd rate: %lu (bytes/sec)\n", (unsigned) (bytes * 1000 / elapsed));
			start = end;
		}
	}
}

void uos_init (void)
{
	/* Configure 16 Mbyte of external Flash memory at nCS3. */
	MC_CSCON3 = MC_CSCON_WS (4);		/* Wait states  */

	/* Выделяем место для динамической памяти */
	extern unsigned __bss_end[];
#ifdef ELVEES_DATA_SDRAM
	/* Динамическая память в SDRAM */
	if (((unsigned) __bss_end & 0xF0000000) == 0x80000000)
		mem_init (&pool, (unsigned) __bss_end, 0x82000000);
	else
		mem_init (&pool, (unsigned) __bss_end, 0xa2000000);
#else
	/* Динамическая память в CRAM */
	extern unsigned _estack[];
	mem_init (&pool, (unsigned) __bss_end, (unsigned) _estack - 256);
#endif

	timer_init (&timer, KHZ, 20);

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
	eth_init (eth, "eth0", 80, &pool, arp, my_macaddr);

	unsigned char my_ip[] = { 172, 0, 0, 18 };
	route_add_netif (&ip, &route, my_ip, 24, &eth->netif);
	
	h_udp_task = task_create (udp_task, 0, "udp", 65,	stack_udp, sizeof (stack_udp));
    init_trace();
}


#include <kernel/internal.h>
#include <net/ip.h>
#include <net/udp.h>
#include <elvees/eth.h>
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
#define TSK_PIN_TX      (1<<3)
//LDAT2
#define TRACE_PIN_2     (1<<4)
//LDAT3
#define TRACE_PIN_3     (1<<5)
//LDAT4
#define TSK_PIN_IP      (1<<6)
//LDAT5
#define TSK_PIN_UDP     (1<<7)
//LDAT6
#define TRACE_PIN_0     (1<<8)
//LDAT7
#define TRACE_PIN_1     (1<<9)
//LDAT8
#define TSK_PIN_IDLE    (1<<10)
//LDAT9
#define TSK_PIN_RX      (1<<11)

#define TSK_PIN_CON     (1<<10)

/*task_t **/ unsigned trace_tasks[8] = {0,0,0,0,0,0,0,0};

bool_t trace_timer(void*);

void init_trace(void){
    SYS_REG.CLK_EN.bits.CLKEN_MFBSP = 1;
    asm volatile("nop");
    asm volatile("nop");
    TSK_PORT_DIR.data =  TSK_PIN_TIMER
                        | TSK_PIN_IDLE
                        | TSK_PIN_TX
                        | TSK_PIN_RX
                        | TSK_PIN_IP
                        | TSK_PIN_UDP
                        | TSK_PIN_CON
                        | TRACE_PIN_0
                        | TRACE_PIN_1
                        | TRACE_PIN_2
                        | TRACE_PIN_3
                        ;
    ethtx_task = (task_t*)(eth->tstack);
    ethrx_task = (task_t*)(eth->stack);
    //trace_tasks[0] = (unsigned)task_idle;
    trace_tasks[0] = (unsigned)ethtx_task;
    //trace_tasks[2] = (unsigned)ethrx_task;
    ip4_task = (task_t*)(ip.stack);
    trace_tasks[3] = (unsigned)ip4_task;
    trace_tasks[4] = (unsigned)h_udp_task;
    trace_tasks[7] = (unsigned)console_task;
}

void uos_on_task_switch(task_t *t)
{
    int i;
    unsigned m = TSK_PIN_TIMER<<1;

    if (0)
    if (task_is_active(h_udp_task))
    if (h_udp_task != t)
    {
        debug_printf("%s(%d)\n", t->name, t->prio);
        debug_putchar(0, '$');
    }
    else
        debug_putchar(0, '4');

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
    trace1[0] = MC_CSR_EMAC(1);
    trace1[1] = MC_QSTR0;
    trace1[2] = MC_MASKR0;
    TSK_PORT_IO.data |= TRACE_PIN_1;
}

void trace_pin1_off(){
    trace1e[0] = MC_CSR_EMAC(1);
    trace1e[1] = MC_QSTR0;
    trace1e[2] = MC_MASKR0;
    TSK_PORT_IO.data &= ~TRACE_PIN_1;
}

void trace_pin2_on(){
    TSK_PORT_IO.data |= TRACE_PIN_2;
}

void trace_pin2_off(){
    TSK_PORT_IO.data &= ~TRACE_PIN_2;
}

void trace_pin3_on(){
    TSK_PORT_IO.data |= TRACE_PIN_3;
}

void trace_pin3_off(){
    TSK_PORT_IO.data &= ~TRACE_PIN_3;
}
