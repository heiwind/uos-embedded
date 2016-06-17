/*
 * Testing TCP protocol: server side.
 */
#include <runtime/lib.h>
#include <stream/stream.h>
#include <mem/mem.h>
#include <buf/buf.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/arp.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <timer/timer.h>
#include <gpanel/gpanel.h>
#include <milandr/eth.h>

ARRAY (stack_tcp, 1000);
ARRAY (stack_udp, 1000);
ARRAY (stack_console, 1000);
ARRAY(stack_local_idle, 500); /* Task: local idle for eth led */
ARRAY (stack_poll, 1000);
ARRAY (group, sizeof(mutex_group_t) + 4 * sizeof(mutex_slot_t));
ARRAY (arp_data, sizeof(arp_t) + 10 * sizeof(arp_entry_t));
mem_pool_t pool;
arp_t *arp;
eth_t eth;
route_t route;
timer_t timer;
ip_t ip;
tcp_socket_t *user_socket;
unsigned char buf [1024];

gpanel_t display;
extern gpanel_font_t font_fixed6x8;

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
	if (s->local_ip[0])
		printf (stream, "%d.%d.%d.%d\t", s->local_ip[0],
			s->local_ip[1], s->local_ip[2], s->local_ip[3]);
	else
		puts (stream, "*\t\t");

	printf (stream, "%u\t", s->local_port);

	if (s->remote_ip[0])
		printf (stream, "%d.%d.%d.%d\t", s->remote_ip[0],
			s->remote_ip[1], s->remote_ip[2], s->remote_ip[3]);
	else
		puts (stream, "*\t\t");

	if (s->remote_port)
		printf (stream, "%u\t", s->remote_port);
	else
		puts (stream, "*\t");

	puts (stream, state_name (s->state));
	putchar (stream, '\n');
}

static void print_socket_data (stream_t *stream, tcp_socket_t *s)
{
	printf (stream, "User socket: snd_queuelen=%u, ", s->snd_queuelen);
	printf (stream, "snd_buf=%u, ", s->snd_buf);
	printf (stream, "snd_lbb=%u, ", s->snd_lbb);
	printf (stream, "snd_nxt=%u,\n     ", s->snd_nxt);
	printf (stream, "snd_max=%u, ", s->snd_max);
	printf (stream, "snd_wnd=%u, ", s->snd_wnd);
	printf (stream, "cwnd=%u, ", s->cwnd);
	printf (stream, "mss=%u, ", s->mss);
	printf (stream, "ssthresh=%u\n", s->ssthresh);
}

void display_refresh ()
{
	unsigned sec = timer_milliseconds (&timer) / 1000;
	unsigned min = sec / 60;
	unsigned hour = min / 60;
	sec -= min*60;
	min -= hour*60;

	gpanel_clear (&display, 0);
	puts (&display, "--Работает 5600ВГ1У--\r\n");
	printf (&display, "Время теста:%3u:%02u:%02u\r\n", hour, min, sec);
	printf (&display, " TX пакетов:%9lu\r\n", eth.netif.out_packets);
	printf (&display, "     ошибок:%9lu\r\n", eth.netif.out_errors);
	printf (&display, " RX пакетов:%9lu\r\n", eth.netif.in_packets);
	printf (&display, "     ошибок:%9lu\r\n", eth.netif.in_errors);
	printf (&display, " Прерываний:%9lu\r\n", eth.intr);
	printf (&display, "Своб.байтов:%9u\r\n", mem_available (&pool));
}

void console_task (void *data)
{
	int c, display_count = 0;
	tcp_socket_t *s;

	for (;;) {
		if (peekchar (&debug) < 0) {
			timer_delay (&timer, 50);
			if (++display_count == 10) {
				display_refresh ();
				display_count = 0;
			}
			continue;
		}
		c = getchar (&debug);
		switch (c) {
		case '\n': case '\r':
			putchar (&debug, '\n');
			printf (&debug, "Transmit: %ld packets, %ld collisions, %ld errors\n",
					eth.netif.out_packets, eth.netif.out_collisions,
					eth.netif.out_errors);
			printf (&debug, "Receive: %ld packets, %ld errors, %ld netif.in_discards\n", eth.netif.in_packets, eth.netif.in_errors, eth.netif.in_discards);

			printf (&debug, "In discards: %ld full_buff, %ld len_packet, %ld missed\n",
			        eth.in_discards_full_buff, eth.in_discards_len_packet, eth.in_missed);

			printf (&debug, "Interrupts: %ln\n", eth.intr);
			printf (&debug, "Free memory: %u bytes\n",
				mem_available (&pool));
			eth_debug (&eth, &debug);
			puts (&debug, "Local address   Port    Peer address    Port    State\n");
			for (s=ip.tcp_sockets; s; s=s->next)
				print_tcp_socket (&debug, s);
			for (s=ip.tcp_closing_sockets; s; s=s->next)
				print_tcp_socket (&debug, s);
			for (s=ip.tcp_listen_sockets; s; s=s->next)
				print_tcp_socket (&debug, s);
			if (user_socket)
				print_socket_data (&debug, user_socket);
			putchar (&debug, '\n');
			break;
		case 't':// & 037:
			task_print (&debug, 0);
			task_print (&debug, (task_t*) stack_console);
			//task_print (&debug, (task_t*) stack_poll);
			task_print (&debug, (task_t*) stack_udp);
			task_print (&debug, (task_t*) eth.stack);
			task_print (&debug, (task_t*) ip.stack);
			putchar (&debug, '\n');
			break;
		}
	}
}
#if 0
void poll_task (void *data)
{
	for (;;) {
		eth_poll (&eth);
	}
}
#endif
void tcp_task (void *data)
{
	tcp_socket_t *lsock;
	unsigned short serv_port = 2222;
	unsigned char ch;
	int n;
	int counter = 0;

	unsigned char ipaddr[] = {192,168, 1, 225};

	lsock = tcp_listen (&ip, ipaddr, serv_port);
	if (! lsock) {
		printf (&debug, "Error on listen, aborted\n");
		uos_halt (0);
	}

	/* Добавляем заголовок - длину пакета (LSB). */
	memset (buf, 0xff, sizeof (buf));
	buf[0] = (char) sizeof (buf);
	buf[1] = sizeof (buf) >> 8;

	for (;;) {
		printf (&debug, "Server waiting on port %d...\n", serv_port);
		printf (&debug, "Free memory: %d bytes\n", mem_available (&pool));
		user_socket = tcp_accept (lsock);
		if (! user_socket) {
			printf (&debug, "Error on accept\n");
			uos_halt (0);
		}

		/* Пересылаем данные, пока юзер не отключится. */

		for (;;) {
			/* Десять пакетов в секуду. */
/*			mutex_wait (&timer.decisec);*/
			printf (&debug, "send %d\n", counter++);
			for (n=0; n<2; ++n) {
				if (tcp_write (user_socket, buf, sizeof (buf)) < 0) {
					printf (&debug, "tcp_write failed\n");
					goto closed;
				}
			}

			/* Обрабатываем команды от пользователя. */
			for (;;) {
				n = tcp_read_poll (user_socket, &ch, 1, 1);
				if (n < 0) {
					printf (&debug, "close\n");
					goto closed;
				}
				if (n == 0)
					break;
				//process_command (ch);
			}
		}
closed:
		tcp_close (user_socket);
		mem_free (user_socket);
		user_socket = 0;
	}
}
#define BUF_SIZE	1024
void udp_task (void *data)
{
	udp_socket_t usock;

	unsigned short serv_port = 2220;

	memset((void*)&usock, 0, sizeof(usock));

	udp_socket (&usock, &ip, serv_port);

    unsigned char client_addr [4] = {192,168,1,1};
    unsigned short client_port = 2220;

	udp_connect(&usock, client_addr, client_port);

	printf (&debug, "Server waiting on port %d...\n", serv_port);
	printf (&debug, "Free memory: %d bytes\n", mem_available (&pool));

	/* Пересылаем данные, пока юзер не отключится. */

	for (;;) {
		/* Десять пакетов в секуду. */
			//mutex_wait (&timer.decisec);
			//  Ожидание   пакета   от   клиента .
			buf_t *p = udp_recvfrom (&usock, client_addr, &client_port);
			if (p) {
				//printf (&debug, "received from %u.%u.%u.%u packet %d\n", client_addr[0], client_addr[1], client_addr[2], client_addr[3], counter++);
				//int i;
				//for (i=0;i<p->len;i++) {
				//	printf (&debug, "%02X", p->payload[i]);
				//}
				//printf (&debug, "\n");

				// Отправка ответа клиенту.
				udp_sendto (&usock, p, client_addr, client_port);
			}
	}
	udp_close (&usock);
}

void init_leds() {
	ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_GPIOD;
	ARM_GPIOD->FUNC = (ARM_GPIOD->FUNC & ~ARM_FUNC_MASK(7)) | ARM_FUNC_PORT(7);
	ARM_GPIOD->ANALOG |= (1 << 7);
	ARM_GPIOD->PWR = (ARM_GPIOD->PWR & ~ARM_PWR_MASK(7)) | ARM_PWR_FASTEST(7);
	ARM_GPIOD->OE |= (0x1 << 7);
	/* Светодиоды для Ethernet*/
	/* Green led PB15
	 * Yellow led PB14
	 */
	ARM_GPIOB->FUNC = (ARM_GPIOB->FUNC
			& ~( ARM_FUNC_MASK(14) | ARM_FUNC_MASK(15)))
			| (ARM_FUNC_PORT(14) | ARM_FUNC_PORT(15));
	ARM_GPIOB->ANALOG |= ARM_DIGITAL(
			14) | ARM_DIGITAL(15);
	ARM_GPIOB->DATA = 0;
	ARM_GPIOB->OE |= ARM_GPIO_OUT(
			14) | ARM_GPIO_OUT(15);
	ARM_GPIOB->PWR = ( ARM_GPIOB->PWR & ~( ARM_PWR_MASK(14) | ARM_PWR_MASK(15)))
			| (ARM_PWR_FASTEST(14) | ARM_PWR_FASTEST(15));
}

void local_idle(void *data) {
	unsigned i = 0;
	init_leds();
	for (;;) {
		if (i == 10)
			ARM_GPIOD->CLRTX = (1 << 7);
		if (i == 20) {
			ARM_GPIOD->SETTX = (1 << 7);
			i = 0;
		}
		i++;
		if (!(ARM_ETH->PHY_STAT & ARM_ETH_PHY_LED_CRS))
			ARM_GPIOB->SETTX = 1 << 15;	//отображение сигнала LINK
		else
			ARM_GPIOB->CLRTX = 1 << 15;
		if (!(ARM_ETH->PHY_STAT & ARM_ETH_PHY_LED_HD))
			ARM_GPIOB->SETTX = 1 << 14;	//Full Duplex Mode
		else
			ARM_GPIOB->CLRTX = 1 << 14;
		mutex_wait(&timer.lock);
	}
}

void uos_init (void)
{
	printf (&debug, "\nCPU speed is %d MHz\n", KHZ/1000);

	/* Используем только внутреннюю память.
	 * Оставляем 256 байтов для задачи "idle". */
extern unsigned __hi_data_end[], _hstack[];
	mem_init(&pool, (unsigned) __hi_data_end, (unsigned) _hstack );

	timer_init (&timer, KHZ, 50);
	gpanel_init (&display, &font_fixed6x8);
	gpanel_clear (&display, 0);
	puts (&display, "Testing TCP.\r\n");

	/*
	 * Create a group of two locks: timer and eth.
	 */
	mutex_group_t *g = mutex_group_init (group, sizeof(group));
	mutex_group_add (g, &eth.netif.lock);
	mutex_group_add (g, &timer.decisec);

	arp = arp_init (arp_data, sizeof(arp_data), &ip);
	ip_init (&ip, &pool, 70, &timer, arp, g);

	/*
	 * Create interface eth0
	 */
	const unsigned char my_macaddr[] = { 0, 9, 0x94, 0xf1, 0xf2, 0xf3 };
	eth_init (&eth, "eth0", 80, &pool, arp, my_macaddr);

	unsigned char my_ip[] = { 192, 168, 1, 225 };
	route_add_netif (&ip, &route, my_ip, 24, &eth.netif);

//task_create (tcp_task, 0, "tcp", 10, stack_tcp, sizeof (stack_tcp));
	task_create (udp_task, 0, "udp", 10, stack_udp, sizeof (stack_udp));
//	task_create (poll_task, 0, "poll", 1, stack_poll, sizeof (stack_poll));
	task_create (console_task, 0, "cons", 20, stack_console, sizeof (stack_console));
    task_create (local_idle, 0, "local idle", 21, stack_local_idle, sizeof(stack_local_idle));
}
