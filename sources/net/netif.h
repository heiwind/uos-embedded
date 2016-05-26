#ifndef __NETIF_H_
#define	__NETIF_H_ 1


#include <net/arp.h>
#include <buf/buf.h>

#ifdef __cplusplus
extern "C" {
#endif



struct _buf_t;

/*
 * Интерфейс к драйверу сетевого адаптера (Etnernet, SLIP и прочее).
 * Состоит из четырех процедур: передача пакета,
 * регистрация обработчика принятых пакетов,
 * опрос MAC-адреса, установка MAC-адреса.
 */
typedef struct _netif_t {
	mutex_t lock;
    //* TODO в этот лок сыпятся все события интерфейса. возможно имеет смысл разделить
	//* события приемника и передатчика
	struct _netif_interface_t *interface;
	

	const char *name;
	struct _arp_t *arp;
	mac_addr        ethaddr;	/* MAC-адрес */
	unsigned short mtu;		/* max packet length */
	unsigned char type;		/* SNMP-compatible */
	unsigned long bps;		/* speed in bits per second */
	unsigned short out_qlen;	/* number of packets in output queue */

	/* Statistics. */
	unsigned long in_bytes;
	unsigned long in_packets;
	unsigned long in_mcast_pkts;
	unsigned long in_errors;
	unsigned long in_discards;
	unsigned long in_unknown_protos;

	unsigned long out_bytes;
	unsigned long out_packets;
	unsigned long out_mcast_pkts;	/* not implemented yet */
	unsigned long out_errors;
	unsigned long out_discards;
	unsigned long out_collisions;
} netif_t;

/*
 * SNMP-compatible type of interface.
 */
#define NETIF_OTHER			1	/* none of the following */
#define NETIF_REGULAR1822		2
#define NETIF_HDH1822			3
#define NETIF_DDN_X25			4
#define NETIF_RFC877_X25		5
#define NETIF_ETHERNET_CSMACD		6
#define NETIF_ISO88023_CSMACD		7
#define NETIF_ISO88024_TOKENBUS		8
#define NETIF_ISO88025_TOKENRING	9
#define NETIF_ISO88026_MAN		10
#define NETIF_STARLAN			11
#define NETIF_PROTEON_10MBIT		12
#define NETIF_PROTEON_80MBIT		13
#define NETIF_HYPERCHANNEL		14
#define NETIF_FDDI			15
#define NETIF_LAPB			16
#define NETIF_SDLC			17
#define NETIF_DS1			18	/* T-1 */
#define NETIF_E1			19	/* european equiv. of T-1 */
#define NETIF_BASICISDN			20
#define NETIF_PRIMARYISDN		21	/* proprietary serial */
#define NETIF_PROPPOINTTOPOINTSERIAL	22
#define NETIF_PPP			23
#define NETIF_SOFTWARELOOPBACK		24
#define NETIF_EON			25	/* CLNP over IP [11] */
#define NETIF_ETHERNET_3MBIT		26
#define NETIF_NSIP			27	/* XNS over IP */
#define NETIF_SLIP			28	/* generic SLIP */
#define NETIF_ULTRA			29	/* ULTRA technologies */
#define NETIF_DS3			30	/* T-3 */
#define NETIF_SIP			31	/* SMDS */
#define NETIF_FRAME_RELAY		32

//! см. netif_free_buf
typedef struct _netif_interface_t {
	/* Передача пакета. */
	bool_t (*output) (netif_t *u, struct _buf_t *p,
		small_uint_t prio);

	/* Выборка пакета из очереди приема. */
	struct _buf_t *(*input) (netif_t *u);

	/* Установка MAC-адреса. */
	void (*set_address) (netif_t *u, unsigned char *address);
} netif_interface_t;

bool_t netif_output (netif_t *netif, struct _buf_t *p,
        ip_addr_const ipdest, ip_addr_const ipsrc);
bool_t netif_output_prio (netif_t *netif, struct _buf_t *p,
        ip_addr_const ipdest, ip_addr_const ipsrc
	, small_uint_t prio);
struct _buf_t *netif_input (netif_t *netif);
void netif_set_address (netif_t *netif, unsigned char *ethaddr);



#ifdef __cplusplus
}
#endif

/** netif_io_overlap - структура оверлап-запроса - кладется в начале буфера, 
 * позволяет передать опции, колбэк, сигнал с запросом. которые отрабатываются 
 * по окончании отправки/загрузки буфера.
 * 
 * если в оверлапе задан сигнал или обработчик, то буфер не дестроится после передачи,
 *  он отдается на откуп обработчику.
 *  !!! следите за памятью.
 *  
 * */

//        mutex import
#include <kernel/uos.h>

#ifdef __cplusplus
extern "C" {
#endif

enum netif_io_overlap_option{
      nioo_ActionNone  = 0
    , nioo_ActionMutex = 0x40
    , nioo_ActionCB    = 0x80
    , nioo_ActionTask  = 0xc0
    , nioo_ActionMASK  = 0xc0
    //* задает уровень свободных слотов в очереди передачи драйвера, на которые 
    //* пакет не претендует - если в очереди осталось меньше места, драйвер отвергает его
    //* возвращая ошибку.
    , nioo_FreeLevel   = 0xf
    , nioo_ActionTerminate  = 0x20
} ;

enum netif_io_overlay_asynco{
      nios_free         = 0
      //буфер просигналил и готовится освободиться. делается проверка терминирования
    , nios_leave
    //буфер в фазе сигналинга, начиная с этой фазы терминация буфера лежит на 
    //    netio 
    , nios_inaction
    , nios_inprocess
};

enum netif_io_overlap_status{
        nios_IPOK   =   1   //< завершены заголовки ip,eth
};

typedef void (*netif_callback)(buf_t *p, unsigned arg);
#define NETIF_OVERLAP_MARK 0x5a

typedef struct _netif_io_overlap{
    union {
        mutex_t* signal;
        task_t*  task;
        netif_callback callback;
    } action;
    unsigned arg;
    unsigned arg2;
    //*внутреннее поле оверлея для организации конкурентной обработки
    volatile char asynco;
    volatile char options;
    //поля, флаги заполняемые по мере продвижения пакета по стеку
    char status;
    //контрольный маркер, чтобы убедиться что в буфере лежит именно оверлап
    char mark;
} netif_io_overlap;

#define NETIO_OVERLAP_HLEN  sizeof(netif_io_overlap)

INLINE
netif_io_overlap* netif_is_overlaped(buf_t *p){
    netif_io_overlap* over = (netif_io_overlap*)((char*)p + sizeof(buf_t));
    if( (p->payload - sizeof(netif_io_overlap)) < (unsigned char*)over )
        return 0;
    if (over->mark == NETIF_OVERLAP_MARK)
        return over;
    else
        return 0;
}

INLINE
netif_io_overlap* netif_overlap_init(buf_t *p){
    netif_io_overlap* over = (netif_io_overlap*)((char*)p + sizeof(buf_t));
    if( (p->payload - sizeof(netif_io_overlap)) < (unsigned char*)over )
        return 0;
    over->mark = NETIF_OVERLAP_MARK;
    over->options = 0;
    over->status = 0;
    over->action.signal = 0;
    return over;
}

void netif_overlap_assign_mutex(buf_t *p, unsigned options
                                , mutex_t* signal, void* msg);
void netif_overlap_assign_cb(buf_t *p, unsigned options
                                , netif_callback cb, void* msg);

//используется интерфейсами для освобождения буфера
//!!! начиная с версии "+tcp:out:nocopy" обязательно использовать этот метод, 
//    чтобы ТСП-стек работоспособен был
void netif_free_buf(netif_t *u, buf_t *p);


//прерывает передачу буфера - драйвер не отсылает буфер, а сразу его освобождает 
void netif_abort_buf(netif_t *u, buf_t *p);
//конкурентно-безопасный дестрой буфера, прерывает передачу буфера, и инициирует его 
//  дестрой. терминируемый буфер - не сигналит, а сразу дестроится!!! 
void netif_terminate_buf(netif_t *u, buf_t *p);


#ifdef __cplusplus
}
#endif

#endif /* !__NETIF_H_ */
