/*
 * Example of telnet server.
 * Author: Serge Vakulenko, <vak@cronyx.ru>
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <mem/mem.h>
#include <buf/buf.h>
#include <timer/timer.h>
#include <uart/uart.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/arp.h>
#include <net/tcp.h>
#include <net/telnet.h>
#include <elvees/eth.h>

#define TASKSZ		1800		/* Task: telnet menu */
#define MAXSESS		2		/* Up to 4 telnet sessions */

ARRAY (stack_telnet, TASKSZ);
ARRAY (stack_console, TASKSZ);
ARRAY (stack_poll, TASKSZ);

task_t *tasktab [MAXSESS];
stream_t *streamtab [MAXSESS];

/*
 * Priorities for tasks.
 */
#define PRIO_POLL	1
#define PRIO_SHELL	10
#define PRIO_CONSOLE	20
#define PRIO_TELNET	30
#define PRIO_IP		40
#define PRIO_ETH	70
#define PRIO_UART	90

mem_pool_t pool;
timer_t timer;
uart_t uart;
arp_t *arp;
route_t route;
ip_t ip;
eth_t eth;

ARRAY (group, sizeof(mutex_group_t) + 4 * sizeof(mutex_slot_t));
ARRAY (arp_data, sizeof(arp_t) + 10 * sizeof(arp_entry_t));

static void
mem_cmd (stream_t *stream)
{
	int n;

	putchar (stream, '\n');
	printf (stream, "Free memory: %u bytes\n", mem_available (&pool));

	putchar (stream, '\n');
	task_print (stream, 0);
	task_print (stream, (task_t*) stack_console);
	task_print (stream, (task_t*) stack_telnet);
	for (n=0; n<MAXSESS; ++n)
		if (streamtab[n])
			task_print (stream, (task_t*) tasktab[n]);
	task_print (stream, (task_t*) ip.stack);
	task_print (stream, (task_t*) eth.stack);
	//task_print (stream, (task_t*) uart.rstack);

	putchar (stream, '\n');
}

static void
eth_cmd (stream_t *stream)
{
	long speed;
	int full_duplex;

	putchar (stream, '\n');
    if (eth_get_carrier (&eth)) {
        speed = eth_get_speed (&eth, &full_duplex);
        printf (stream, "Ethernet: %ld Mbit/sec, %s Duplex\n",
            speed / 1000000, full_duplex ? "Full" : "Half");
	} else
		printf (stream, "Ethernet: No cable\n");

	putchar (stream, '\n');
	printf (stream, "   Rx packets: %ln\n", eth.netif.in_packets);
	printf (stream, "   Tx packets: %ln\n", eth.netif.out_packets);
	printf (stream, "    Rx errors: %ln\n", eth.netif.in_errors);
	printf (stream, "    Tx errors: %ln\n", eth.netif.out_errors);
	printf (stream, "  Rx discards: %ln\n", eth.netif.in_discards);
	printf (stream, "Tx collisions: %ln\n", eth.netif.out_collisions);
	printf (stream, "   Interrupts: %ln\n", eth.intr);

	/* Print Ethernet hardware registers. */
	puts (stream, "Ethernet hardware registers:\n");
    eth_debug (&eth, stream);

	putchar (stream, '\n');
}

static const char *
state_name (tcp_state_t state)
{
	switch (state) {
	case CLOSED:	  return "CLOSED";	break;
	case LISTEN:	  return "LISTEN";	break;
	case SYN_SENT:	  return "SYN_SENT";	break;
	case SYN_RCVD:	  return "SYN_RCVD";	break;
	case ESTABLISHED: return "ESTABLISHED";	break;
	case FIN_WAIT_1:  return "FIN_WAIT_1";	break;
	case FIN_WAIT_2:  return "FIN_WAIT_2";	break;
	case CLOSE_WAIT:  return "CLOSE_WAIT";	break;
	case CLOSING:	  return "CLOSING";	break;
	case LAST_ACK:	  return "LAST_ACK";	break;
	case TIME_WAIT:	  return "TIME_WAIT";	break;
	}
	return "???";
}

static void print_tcp_socket (stream_t *stream, tcp_socket_t *s)
{
	if (ipref_as_ucs(s->local_ip)[0])
		printf (stream, "%@.4D\t", ipref_as_ucs(s->local_ip) );
	else
		puts (stream, "*\t\t");

	printf (stream, "%u\t", s->local_port);

	if (s->remote_ip.ucs[0])
		printf (stream, "%@.4D\t", s->remote_ip.ucs);
	else
		puts (stream, "*\t\t");

	if (s->remote_port)
		printf (stream, "%u\t", s->remote_port);
	else
		puts (stream, "*\t");

	puts (stream, state_name (s->state));
	putchar (stream, '\n');
}

static void
net_cmd (stream_t *stream, int arg)
{
	putchar (stream, '\n');
	if (arg == 'a') {
		arp_entry_t *e;

		puts (stream, "ARP table\n\n");
		for (e=arp->table; e<arp->table+arp->size; ++e) {
			if (! e->netif)
				continue;
			printf (stream, "%@.4D at %#6D age %d on %s\n",
				e->ipaddr.ucs, e->ethaddr.ucs,
				e->age, e->netif->name);
		}
	} else if (arg == 'i') {
		puts (stream, "IP statistics\n");

		puts (stream, "\nIP input:\n");
		printf (stream, "    Received packets: %ln\n", ip.in_receives);
		printf (stream, "       Header errors: %ln\n", ip.in_hdr_errors);
		printf (stream, "      Address errors: %ln\n", ip.in_addr_errors);
		printf (stream, "   Discarded packets: %ln\n", ip.in_discards);
		printf (stream, "      Unknown protos: %ln\n", ip.in_unknown_protos);
		printf (stream, "   Delivered packets: %ln\n", ip.in_delivers);

		puts (stream, "\nIP output:\n");
		printf (stream, "     Output requests: %ln\n", ip.out_requests);
		printf (stream, "   Discarded packets: %ln\n", ip.out_discards);
		printf (stream, "      Unknown routes: %ln\n", ip.out_no_routes);
		printf (stream, "   Forwarded packets: %ln\n", ip.forw_datagrams);

	} else if (arg == 'c') {
		puts (stream, "ICMP statistics\n");

		puts (stream, "\nICMP input:\n");
		printf (stream, "    Input packets: %ln\n", ip.icmp_in_msgs);
		printf (stream, "  Errored packets: %ln\n", ip.icmp_in_errors);
		printf (stream, "     Echo queries: %ln\n", ip.icmp_in_echos);

		puts (stream, "\nICMP output:\n");
		printf (stream, "   Output packets: %ln\n", ip.icmp_out_msgs);
		printf (stream, "    Output errors: %ln\n", ip.icmp_out_errors);
		printf (stream, "Dest.unreachables: %ln\n", ip.icmp_out_dest_unreachs);
		printf (stream, "     Time exceeds: %ln\n", ip.icmp_out_time_excds);
		printf (stream, "     Echo replies: %ln\n", ip.icmp_out_echo_reps);

	} else if (arg == 't') {
		puts (stream, "TCP statistics\n\n");

		printf (stream, "    Input packets: %ln\n", ip.tcp_in_datagrams);
		printf (stream, "  Errored packets: %ln\n", ip.tcp_in_errors);
		printf (stream, "Discarded packets: %ln\n", ip.tcp_in_discards);
		printf (stream, "   Output packets: %ln\n", ip.tcp_out_datagrams);
		printf (stream, "    Output errors: %ln\n", ip.tcp_out_errors);

	} else {
		tcp_socket_t *s;

		puts (stream, "TCP sockets\n\n");
		puts (stream, "Local address   Port    Peer address    Port    State\n");
		for (s=ip.tcp_sockets; s; s=s->next)
			print_tcp_socket (stream, s);
		for (s=ip.tcp_closing_sockets; s; s=s->next)
			print_tcp_socket (stream, s);
		for (s=ip.tcp_listen_sockets; s; s=s->next)
			print_tcp_socket (stream, s);
	}
	putchar (stream, '\n');
}

static void
help_cmd (stream_t *stream)
{
	puts (stream, "Available commands:\n");
	puts (stream, "    mem\n");
	puts (stream, "    eth\n");
	puts (stream, "    ip\n");
	puts (stream, "    arp\n");
	puts (stream, "    tcp\n");
	puts (stream, "    icmp\n");
	puts (stream, "    sock\n");
}

/*
 * Read a newline-terminated string from stream.
 */
static unsigned char *
getline (stream_t *stream, unsigned char *buf, int len)
{
	int c;
	unsigned char *s;

	s = buf;
        while (--len > 0) {
		c = getchar (stream);
		if (feof (stream))
			return 0;
		if (c == '\b' || c == 0177) {
			if (s > buf) {
				--s;
				puts (stream, "\b \b");
			}
			continue;
		}
		if (c == '\r')
			c = '\n';
		putchar (stream, c);
		*s++ = c;
		if (c == '\n')
			break;
	}
	*s = '\0';
	return buf;
}

void user_shell (void *arg)
{
	stream_t *stream = (stream_t*) arg;
	unsigned char line [200];
	int n;

	/* Give telnet some time to negotiate. */
	timer_delay (&timer, 500);
	fflush (stream);

	puts (stream, "\n\nTesting Telnet\n");
	puts (stream, "~~~~~~~~~~~~\n");
	printf (stream, "Free memory: %u bytes\n", mem_available (&pool));
	puts (stream, "\nEnter \"help\" for a list of commands\n\n");

	for (;;) {
		puts (stream, "% ");
		if (! getline (stream, line, sizeof (line)))
			break;
		if (line[0] == '\n')
			continue;

		if (strncmp ((unsigned char*) "help", line, 4) == 0) {
			help_cmd (stream);
		} else if (strncmp ((unsigned char*) "quit", line, 4) == 0) {
			break;
		} else if (strncmp ((unsigned char*) "mem", line, 3) == 0) {
			mem_cmd (stream);
		} else if (strncmp ((unsigned char*) "eth", line, 3) == 0) {
			eth_cmd (stream);
		} else if (strncmp ((unsigned char*) "ip", line, 2) == 0) {
			net_cmd (stream, 'i');
		} else if (strncmp ((unsigned char*) "arp", line, 3) == 0) {
			net_cmd (stream, 'a');
		} else if (strncmp ((unsigned char*) "tcp", line, 3) == 0) {
			net_cmd (stream, 't');
		} else if (strncmp ((unsigned char*) "icmp", line, 4) == 0) {
			net_cmd (stream, 'c');
		} else if (strncmp ((unsigned char*) "sock", line, 4) == 0) {
			net_cmd (stream, 0);
		} else
			printf (stream, "Unknown command: %s\n", line);
	}

	for (n=0; n<MAXSESS; ++n)
		if (streamtab[n] == stream) {
			fclose (stream);
			streamtab[n] = 0;
			task_exit (0);
		}
}

void start_session (tcp_socket_t *sock)
{
	int n;
	array_t *t;
	stream_t *stream;

	for (n=0; n<MAXSESS; ++n)
		if (! streamtab[n])
			break;
	if (n >= MAXSESS) {
		debug_printf ("Too many sessions\n");
		tcp_close (sock);
		return;
	}
	if (tasktab[n] != 0) {
		/* Free the memory of the previous session. */
		mem_free (tasktab[n]);
		tasktab[n] = 0;
	}
	t = (array_t*) mem_alloc (&pool, TASKSZ);
	if (! t) {
		debug_printf ("No memory for task\n");
		tcp_close (sock);
		return;
	}
	stream = telnet_init (sock);
	if (! stream) {
		debug_printf ("Error initializing telnet\n");
		tcp_close (sock);
		return;
	}
	streamtab[n] = stream;
	tasktab[n] = task_create (user_shell, stream, "shell", PRIO_SHELL, t, TASKSZ);
}

void poll_task (void *data)
{
    stream_t * console = data;
	unsigned last_sec = 0;
    if (console == (void*)0)
        console = &debug;

	for (;;) {
		unsigned sec = timer_milliseconds (&timer) / 1000;
		if (sec == last_sec)
			continue;
		last_sec = sec;

		unsigned min = sec / 60;
		unsigned hour = min / 60;
		sec -= min*60;
		min -= hour*60;
		puts (console, "--Работает nvcom01 --\r\n");
		printf (console, "Время теста:%3u:%02u:%02u\r\n", hour, min, sec);
		printf (console, " TX пакетов:%9lu\r\n", eth.netif.out_packets);
		printf (console, "     ошибок:%9lu\r\n", eth.netif.out_errors);
		printf (console, " RX пакетов:%9lu\r\n", eth.netif.in_packets);
		printf (console, "     ошибок:%9lu\r\n", eth.netif.in_errors);
		printf (console, " Прерываний:%9lu\r\n", eth.intr);
		printf (console, "Своб.байтов:%9u\r\n", mem_available (&pool));
		timer_delay (&timer, 2000);
	}
}

void
console_task (void *data)
{
    if (data == (void*)0)
        data = &debug;
	for (;;)
		user_shell ((stream_t*) data);
}

void telnet_task (void *data)
{
	tcp_socket_t *lsock, *sock;
	unsigned short serv_port = 23;

	lsock = tcp_listen (&ip, 0, serv_port);
	if (! lsock) {
		debug_printf ("Error on listen, aborted\n");
		uos_halt (1);
	}
	for (;;) {
		sock = tcp_accept (lsock);
		if (! sock) {
			debug_printf ("Error on accept\n");
			continue;
		}
		else
            debug_printf ("telnet accept\n");
		start_session (sock);
	}
}

void uos_init (void)
{
	/* Используем только внутреннюю память.
	 * Оставляем 256 байтов для задачи "idle". */
	extern unsigned __bss_end[], _estack[];
	mem_init (&pool, (unsigned) __bss_end, (unsigned) _estack - 256);

	timer_init (&timer, KHZ, 50);
	//uart_init (&uart, 1, PRIO_UART, KHZ, 115200);
	/*
	 * Create a group of two locks: timer and eth.
	 */
	mutex_group_t *g = mutex_group_init (group, sizeof(group));
	mutex_group_add (g, &eth.netif.lock);
	mutex_group_add (g, &timer.decisec);

	arp = arp_init (arp_data, sizeof(arp_data), &ip);
	ip_init (&ip, &pool, PRIO_IP, &timer, arp, g);

	/*
	 * Create interface eth0
	 */
	const unsigned char mac_addr [6] = { 0, 9, 0x94, 0xf1, 0xf2, 0xf3 };
	eth_init (&eth, "eth0", PRIO_ETH, &pool, arp, mac_addr);

	static unsigned char ip_addr [4] = { 172, 0, 0, 18 };
	route_add_netif (&ip, &route, ip_addr, 24, &eth.netif);

	task_create (console_task, &debug, "console", PRIO_CONSOLE,
		stack_console, sizeof (stack_console));
	//task_create (poll_task, 0, "poll", PRIO_POLL, stack_poll, sizeof (stack_poll));
	task_create (telnet_task, 0, "telnet", PRIO_TELNET,
		stack_telnet, sizeof (stack_telnet));
}



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

#define TSK_PIN_CON     (1<<10)

/*task_t **/ unsigned trace_tasks[8] = {0,0,0,0,0,0,0,0};

bool_t trace_timer(void*);

void init_trace(void){
    SYS_REG.CLK_EN.bits.CLKEN_MFBSP = 1;
    asm volatile("nop");
    asm volatile("nop");
    TSK_PORT_DIR.data =   TSK_PIN_TIMER | (0xff << 2);

    ethtx_task = (task_t*)(eth.tstack);
    ethrx_task = (task_t*)(eth.stack);
    trace_tasks[0] = (unsigned)task_idle;
    trace_tasks[1] = (unsigned)ethtx_task;
    trace_tasks[2] = (unsigned)ethrx_task;
    ip4_task = (task_t*)(ip.stack);
    trace_tasks[3] = (unsigned)ip4_task;

//    trace_tasks[4] = (unsigned)h_udp_task;
//    trace_tasks[7] = (unsigned)console_task;
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
