/*
 * Testing TCP protocol: server side.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
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

#include <utcp_flow.h>

#include <trace.h>

//nvcom02tem-3u evboard memory size 
#define SRAM_BANK_SIZE (64<<20)

ARRAY (stack_tcp, 2800);
ARRAY (stack_con, 1800);
ARRAY (group, sizeof(mutex_group_t) + 4 * sizeof(mutex_slot_t));
ARRAY (arp_data, sizeof(arp_t) + 10 * sizeof(arp_entry_t));

mem_pool_t pool;
mem_pool_t posix_pool;
uintptr_t  posix_pool_start;
uintptr_t  posix_pool_end;

RawSource::BrickAllocator rawpool;

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

#ifdef RT_PRINTF
#define DEBUG_BUFS
#define DEBUG_BUFPOLL
#endif

unsigned count = 0;
unsigned old_count = 0;
static MDMA_Descriptor    udma;

extern "C" {
void tcp_task (void *data);
void uos_on_task_switch(task_t *t);
void uos_on_halt(int errcode);
void uos_on_timer_hook(timer_t *t);
}

void tcp_task (void *data)
{
    static utcptx_dma_flow   flow;
    static RawSource         raw;

	int i;
	int countdown = 0;
	unsigned blen      = 1;

	MDMA_handle       hdma;
	hdma = MDMA_requestChanel(MDMA_clearDescriptor(&udma), DMA_mem_ch3);

	unsigned bos_pool_start = posix_pool_end-SRAM_BANK_SIZE/2;
    rawpool.init_banks(bos_pool_start, SRAM_BANK_SIZE/2);
    debug_printf("bos: pool $%x ... $%x\n", bos_pool_start, posix_pool_end);
	rawpool.chs(0xff);
    unsigned bricks_num = rawpool.Declare_samples(1);
    debug_printf("flow: %d bricks allocated size $%x \n", bricks_num, rawpool.sizeof_brick() );
    raw.Pool(&rawpool);
	
	for (;;) {
		debug_printf ("Press SPACE to connect to %d.%d.%d.%d:%d\n"
		              "      l - emulate loose sended segment (next l - for continue)\n"
                      "      s - send single segment (next l - for continue)\n"
                      "      1..5 - dump last 100*n log entries\n"
                      "      c - clear runtime log\n"
		        , server_ip[0], server_ip[1], server_ip[2], server_ip[3], PORT
		        );
		i = 0;
		while (i != ' '){
	        i = debug_peekchar();
		    switch(i){
		    case ' ' : debug_getchar (); break;
            case 'l' : i = ' '; break;
            case 's' : i = ' '; break;
		    }
            task_yield();
		}
        debug_puts ("go\n");

		user_socket = tcp_connect (&ip, server_ip, PORT);
		if (! user_socket) {
			debug_printf ("Failed to connect!\n");
			continue;
		}

		debug_printf ("Connected to server\n");
		count = 0;
		old_count = 0;
		//raw.reset();
	    flow.init(&raw, hdma, user_socket);

		countdown = -1;
        //countdown = 5;
		for (;;) {
            unsigned flags = 0;
            if (debug_peekchar() == 'l'){
                debug_getchar ();
                flags |= TF_TRAP_LOOSE;
                debug_putchar(0,'@');
                //countdown = 5;
            }

            if (debug_peekchar() == 's'){
                debug_getchar ();
                debug_putchar(0,'S');
                countdown = 6;
            }

            if (debug_peekchar() == '8'){
                debug_getchar ();
                blen = 8;
            }
            if (debug_peekchar() == '0'){
                debug_getchar ();
                blen = 1;
            }
            unsigned sent = flow.post_banks(blen, flags);
			/* send a message to the server PORT on machine HOST */
			if (sent != blen) {
			    debug_printf("flow: terminate by posted %u != expected %u\n", sent, blen);
			    flow.terminate();
				break;
			}
			count = raw.tail_stamp;
            RT_PRINTF("utcp:posted %d at stamp %u\n", sent, count);

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
	
        RT_PRINTF("Disconnect\n");
        flow.release();
        RT_PRINTF("close\n");
		tcp_close (user_socket);
        debug_printf ("Disconnected\n");
		mem_free (user_socket);
		user_socket = 0;
	}
	task_exit(0);
}

void console (void *unused)
{
	unsigned long start, end, elapsed;
	unsigned long long bytes;
	int lastc = 0;
	
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
		    case '1' : rtlog_dump_last(&tcp_log, &debug, 1000);break;
            case '2' : rtlog_dump_last(&tcp_log, &debug, 200);break;
            case '3' : rtlog_dump_last(&tcp_log, &debug, 300);break;
            case '4' : rtlog_dump_last(&tcp_log, &debug, 400);break;
            case '5' : rtlog_dump_last(&tcp_log, &debug, 500);break;
            case 'c' : rtlog_clear(&tcp_log); break;
            case 'l' : c = ' ';break;
            case 's' : c = ' ';break;
		    }
		    if (c != ' ') {
                lastc = 0;
		        debug_getchar();
		    }
		    else if (c == lastc){
                debug_getchar();
		    }
		    else
		        lastc = c;
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
    unsigned bos_pool_start = posix_pool_end - SRAM_BANK_SIZE/2;
    posix_pool_start = ((unsigned)_etext | 0xff)+1;
    /* Динамическая память в SDRAM */
    mem_init (&posix_pool, posix_pool_start, bos_pool_start);
    debug_printf("posix: pool $%x ... $%x\n", posix_pool_start, bos_pool_start);

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
	
	h_task = task_create (tcp_task, 0, "tcp", 30,
		stack_tcp, sizeof (stack_tcp));
		
    console_task = task_create (console, 0, "con", 45,
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


void uos_on_halt(int errcode)
{
    //залочу task_yield
    int prio = task_current->prio;
    task_current->prio = PRIO_MAX;
    asm volatile ("nop");
    rtlog_dump_last(&tcp_log, &debug, 1000);
    fflush((stream_t*)&debug);
    task_current->prio = prio;
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



//*****************************************************************************
// этот кольцевой буфер в хРАМ, заполняется нарастающим числом по AtRead, и
//      отбрасывает прочитанное содержимое по NextRead

//class CBosRaw : public CBaseRaw {
RawSource::RawSource()
{
    ReadBrick        = &_ReadBrick;
    for (unsigned i = 0; i < bricks_limit; i++){
        bricks[i] = (Brick)BrickAllocator::brickNULL;
    }
    ring_uindex_init(&brick_idx, bricks_limit);
    tail_stamp  =0;
    ReadIdx = brick_idx.read;
    BUFS_printf("bufs:alloc %d bufs sized %d\n", bricks_limit, brick_size);
}

RawSource::~RawSource(){
    for (unsigned i = 0; i < bricks_limit; i++){
        mem_free(bricks[i]);
        bricks[i] = 0;
    }
}

void  RawSource::Pool(RawSource::BrickAllocator* x) {
    pool = x;
    for (unsigned i = 0; i < bricks_limit; i++){
        bricks[i] = pool->Alloc( pool->sizeof_brick() );
    }
    _block_samples = pool->brick_len();
};

unsigned RawSource::sizeof_frame(unsigned mss){
    if ((mss%frame_size) > sample_size)
        return sample_size;
    else
        return frame_size;
}

bool RawSource::AtRead(RawSource::SoftBrick* dst, unsigned next_no){
    if (next_no >= brick_idx.mask){
        dst->data    = (Brick)pool->brickNULL;
        dst->limit   = 0;
        dst->size    = 0;
        dst->pos     = 0;
        return false;
    }
    while (next_no >= ring_uindex_avail(&brick_idx))
    {
        unsigned idx = brick_idx.write;
        unsigned* data  = (unsigned*)bricks[idx];
        BUFS_printf("bufs:fill buf[%d:$%x] at $%x\n", idx, data, tail_stamp);

        unsigned stamp_limit = fill_brick(bricks[idx], tail_stamp);

        ring_uindex_put(&brick_idx);
        tail_stamp = stamp_limit;
    }
    if (next_no <= ring_uindex_avail(&brick_idx)){
        unsigned idx = (brick_idx.read+next_no) & brick_idx.mask; 
        BUFS_printf("bufs:access buf[+%d=%d:$%x]\n", next_no, idx, bricks[idx]);
        dst->data    = bricks[idx];
        dst->limit   = pool->sizeof_brick();
        dst->size    = pool->sizeof_brick();
        dst->pos     = 0;
    }
    return true;
}

unsigned RawSource::fill_brick(RawSource::Brick dst, unsigned stamp){
    brick_navigator b(dst, this);
    brick_navigator::inBrickPtr ofs = 0;

    for (unsigned ti = 0; ti < pool->chcount(); ti++){
        unsigned* data  = (unsigned*)b.data_at(ofs);
        unsigned tsize = b.len_at(ofs);
        unsigned tlen  = tsize/sizeof(unsigned);
        BUFS_printf("bufs:trek[%d:$%x]:fill sample $%x len $%x\n"
                , ti, data
                , stamp, tlen
                );
        for(unsigned x = stamp; x < stamp + tlen; x++)
            *data++ = x;
        stamp += tlen;
        ofs = b.succ(ofs, tsize);
    }
    return stamp;
}

bool RawSource::NextRead()
{         //отпускает текущий блок, берет следующий
    ReadIdx = brick_idx.read;
    if (!ring_uindex_avail(&brick_idx)){
        _ReadBrick.data    = (Brick)pool->brickNULL;
        _ReadBrick.limit   = 0;
        _ReadBrick.size    = 0;
        _ReadBrick.pos     = 0;
        BUFS_printf("bufs:start buf[%d:$%x]\n", ReadIdx, bricks[ReadIdx]);
        return AtRead(&_ReadBrick, 0);
    }
    ring_uindex_get(&brick_idx);
    ReadIdx = brick_idx.read;
    BUFS_printf("bufs:next buf[%d:$%x]\n", ReadIdx, bricks[ReadIdx]);
    {
        _ReadBrick.data    = bricks[ReadIdx];
        _ReadBrick.limit   = brick_size;
        _ReadBrick.size    = brick_size;
        _ReadBrick.pos     = 0;
    }
    return true;
}



//*****************************************************************************************
//                          RawSource::BrickAllocator
#ifdef DEBUG_BUFPOLL
#define POOL_printf(...)    RT_PRINTF(__VA_ARGS__)
//#define BUFS_printf(...)    debug_printf(__VA_ARGS__)
#else
#define POOL_printf(...)
#endif

RawSource::BrickAllocator::BrickAllocator()
: chs_count(0)
, frame_size(0)
{
    page_size    = 0;
    bank[0].size = 0;
    bank[1].size = 0;
}

RawSource::BrickAllocator::~BrickAllocator()
{
    mem_free((void*)bank[0].org);
    if (mem_size((void*)bank[0].org) <= bank[0].size)
        mem_free((void*)bank[1].org);
    bank[0].clear();
    bank[1].clear();
}

void
RawSource::BrickAllocator::init_banks(size_t org, size_t size)
{
    page_size = sample_len *sizeof(RawSource::raw_sample);
    org = (size_t)MDMA_coherent((void*)org);

    dsp_bank& bank0 = bank[0];
    dsp_bank& bank1 = bank[1];
    bank0.assign_mem(org, size/2);
    bank1.assign_mem(org+bank0.size, size/2);
}


void RawSource::BrickAllocator::dsp_bank::assign_mem(size_t _org, size_t _size){
    size = _size;
    org  = _org;
    POOL_printf("pool:org $%x size $%x\n", org, size);
}


//уведомление о том сколько предстоит размещать кирпичей
unsigned
RawSource::BrickAllocator::Declare(size_t brick_size, unsigned bricks){
    unsigned _fs = Measures::Raw::frame_size(chs());
    unsigned samples = brick_size / _fs;
    return Declare_samples(samples);
}

unsigned
RawSource::BrickAllocator::Declare_samples(size_t _brick_samples)
{
    unsigned bricks = 0;
    treks_init(_brick_samples);
    bricks  = bank_init(bank[0]);
    bricks += bank_init(bank[1]);
    POOL_printf("pool: declared %d bricks with size $%x samples %d\n"
            , bricks, brick_size, brick_samples);
    return bricks;
}

void
RawSource::BrickAllocator::treks_init(unsigned _brick_samples){
    brick_samples = _brick_samples;
    frame_size    = 0;
    brick_size    = 0;
    unsigned tid = 0;
    unsigned chset = chs();
    for (unsigned ci= 0; ci < ch_limit; ci++, chset = chset >> 1){
        if ((chset & 1) != 0){
            trek_info& t = trek[tid];
            t.ch = ci;
            t.sizeof_sample = Measures::Raw::sizeof_sample(0);
            t.sizeof_brick  = t.sizeof_sample * brick_samples;
            frame_size += t.sizeof_sample;
            brick_size += t.sizeof_brick;
            POOL_printf("pool:trek[%d]:init ch %d sample $%x brick $%x\n"
                    , tid, ci
                    , t.sizeof_sample, t.sizeof_brick
                    );
            tid++;
        }
    }
    POOL_printf("pool:treks:frame $%x brick: size $%x samples %d\n"
                , frame_size, brick_size, brick_samples
            );
}

unsigned
RawSource::BrickAllocator::bank_init(dsp_bank& b)
{
    b.alloc_sample = 0;
    b.bricks = (b.size / brick_size);
    b.samples = b.bricks * brick_samples;
    POOL_printf("pool:bank[$%x]: bricks $%x samples $%x\n"
            , b.org, b.bricks, b.samples
            );
    uintptr_t torg = b.org;
    for (unsigned ti = 0; ti < ch_limit; ti++){
        dsp_bank::trek_info& t = b.trek[ti];
        if (ti < chs_count){
            t.org = torg;
            t.size = trek[ti].sizeof_brick*b.bricks;
            torg += t.size;
            POOL_printf("       :trek[%d]:org $%x size $%x\n"
                    , ti, t.org, t.size
                    );
        }
        else {
            t.org   = 0;
            t.size  = 0;
        }
    }
    return b.bricks;
}

RawSource::Brick
RawSource::BrickAllocator::Alloc(size_t size){
    for (unsigned bi = 0; bi < banks_limit; bi++){
        dsp_bank& b = bank[bi];
        if ( (b.alloc_sample + brick_samples) < b.samples ) {
            Brick res = as_brick(bi, b.alloc_sample);
            b.alloc_sample += brick_samples;
            return res;
        }
    }
    return (Brick)brickNULL;
}

void
RawSource::BrickAllocator::Free(RawSource::BrickAllocator::Brick x, size_t size){
}

void
RawSource::BrickAllocator::chs(RawSource::BrickAllocator::ChMask chs){
    _chs = chs;
    chs_count = pop(chs);
}

void
RawSource::BrickAllocator::brick_fill(RawSource::BrickAllocator::Brick brick, RawSource::RawSampleId x)
{
    for (unsigned ti = 0; ti < chcount(); ti++){
        void* t = brick_sample(brick, ti);
        memsetw(t, x, sizeof_trek(brick, ti) );
    }
}



//*******************************************************************************************
//                      brick_navigator

RawSource::brick_navigator::inBrickPtr
RawSource::brick_navigator::succ(RawSource::brick_navigator::inBrickPtr p, unsigned ofs) const
{
    unsigned tlen = pool->sizeof_brick_trek(treck_of(p));
    unsigned t = treck_of(p);
    assert(t < treks());
    while (ofs >= tlen){
        ofs -= tlen;
        ++t;
        if (t < treks())
            tlen = pool->sizeof_brick_trek(t);
        else
            return null;
    }
    return as_ptr(t, ofs);
};
