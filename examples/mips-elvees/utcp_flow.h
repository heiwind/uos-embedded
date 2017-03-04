#ifndef __UTCP_FLOW_H_
#define __UTCP_FLOW_H_ 1



#include <kernel/uos.h>
#include <buf/buf.h>
#include <mem/mem.h>
#include <net/tcp.h>
#include <buffers/ring_index.h>
#include <elvees/dev/dma_mem.h>

#include <utcp_flow-port.h>



//* для отправки страниц CBosRaw здействован DMA. передача данных от ДМА в уТСП делаю через банки
struct utcptx_dma_flow{
    public:
        typedef utcptx_dma_source dma_source;

    protected:
    bool _terminate;

    public:

    utcptx_dma_flow()  __cpp_decls;
    ~utcptx_dma_flow()  __cpp_decls;

    // типовой порядок использования:
    //  init(...);
    //  for (...)
    //      post_banks(n);
    //  release();
    //
    void init(dma_source* asrc, MDMA_handle ahdma, tcp_socket_t* s) __cpp_decls;
    // \return sent src bricks
    unsigned post_banks(unsigned count, tcps_flag_set opts = 0) __cpp_decls;
    bool post_complete() __cpp_decls;           //release его вызывает сам, нужен для сброса занятых буферов и страниц
    void release()  __cpp_decls;

    // нужен для аккуратного прерывания post_banks
    void terminate();
    bool terminated() {return _terminate;};

    public:
    //контекст передачи. 
    MDMA_handle       hdma;
    tcp_socket_t*       s;
    tcps_flag_set       sopt;

    // банк размещает слоты которые заполняет ДМА из источника
    // после передачи всех слотов, банк освобождается,
    //     и может освободжиться страница RawSource
    static const unsigned slot_limit = 4;
    dma_source*              src;
    dma_source::RawSampleId  read_sample;
    unsigned              seg_size;

    dma_source::SoftBrick   post_brick;
    dma_source::RawSampleId post_sample;
    unsigned                post_idx;//after read brick

    protected:
    typedef dma_source::brick_navigator::inBrickPtr inBrickPtr;
    static const inBrickPtr brick_null = dma_source::brick_navigator::null;
    static bool is_null(inBrickPtr x) {return dma_source::brick_navigator::isNULL(x); };

    struct dma_task_compose {
        static const unsigned cmd_limit = 8;//slot_limit;
        inline static
        void* mem_coherent(void* x){
            return MDMA_coherent(x);
        }

        typedef DMAMEM_REQ_CHAIN(cmd_limit) dma_req;
        dma_req                 req;
        MDMA_request*         req_coherent(){return (MDMA_request*)mem_coherent(&req.r);};

        unsigned    ncmd;
        size_t      len;
        //inBrickPtr  brick_ofs;

        void add(void* dst, void* src, size_t len)  __cpp_decls;
        //return len of append data
        inBrickPtr add(void* dst, const dma_source::brick_navigator& src, inBrickPtr ofs, size_t limit)  __cpp_decls;
        bool go(MDMA_handle hdma)  __cpp_decls;
        bool wait()                 { return MDMA_waitReq( req_coherent() ); };
        void clear() __cpp_decls;
        void init()  __cpp_decls;
        bool check_link(unsigned len) __cpp_decls;

        dma_task_compose()  __cpp_decls;
        ~dma_task_compose(){clear();};
    };

    struct ack_waits_bank;
    struct dma_bank {

        dma_source::SoftBrick*     brick;
        dma_source::RawSampleId    brick_sample;
        unsigned                brick_ofs;
        unsigned                seg_limit;
        unsigned                ack_pt;

        typedef struct {
            buf_t*              p;
            inBrickPtr          src;
        } slot_t;
        slot_t                  slot[slot_limit];
        volatile unsigned       slots_send;

        dma_task_compose        dmatask;

        dma_bank();
        void init();
        void init(mem_pool_t* pool, unsigned seglen);
        void close();
        ~dma_bank() {close();};

        //* \return data len assigned on dma request
        bool load_go(MDMA_handle hdma) {return dmatask.go(hdma);};
        bool load_wait();
        buf_t* bufs_chain();

        //выделяет правильный буфер для обмена с ДМА
        static buf_t* alloc_buf(mem_pool_t* pool, unsigned seglen);

        bool _terminate;
        void terminate(){_terminate = true;};
        bool terminated() {return _terminate;};
    };
    void    init_bank(dma_bank* self);

    dma_bank        dma_pool[2];
    dma_bank*       dma_prepare;
    dma_bank*       dma_send;
    mutex_t         sent_signal;

    inBrickPtr      post_pos;
    unsigned    post_assign(dma_bank* bank, unsigned brick_posted);
    int     post_sent(dma_bank* bank);
    bool    wait_sent(dma_bank* bank);
    bool    onSent(dma_bank* bank,          tcp_segment_t* seg);

    bool    send_check(dma_source::RawSampleId brick_stamp); //, unsigned ofs
    bool    bank_load_ckeck(dma_bank* bank); //, unsigned ofs


    struct ack_waits_bank {
        typedef struct {
            inBrickPtr      pdata;
            unsigned long   seqno;
        } slot_t;
        slot_t                  slot[slot_limit];
        dma_source::RawSampleId brick_sample;
        dma_source::Brick       brick_data;
        unsigned                slots_unack;
        void assign(dma_bank* bank);
    };

    static const unsigned   ack_limit = 16;
    ring_uindex_t           ack_idx;
    ack_waits_bank          ack_seq[ack_limit];
    mutex_t                 ack_signal;

    bool    onSent(ack_waits_bank* bank,    tcp_segment_t* seg, unsigned slotid);
    bool    onAck(ack_waits_bank* bank,     tcp_segment_t* seg, unsigned slotid);
    bool    rexmit(ack_waits_bank* bank,    buf_t* p, unsigned slotid);

    unsigned ack_push_bank(dma_bank* bank);
    unsigned ack_release_finished();
    unsigned ack_awaiting_banks() {return ring_uindex_avail(&ack_idx);};
    void     ack_clear();

    protected:
    //void swap_prep2send();

    static void utcp_callback(tcp_cb_event ev
                               , tcp_segment_t* seg
                               , tcp_socket_t* s
                               );

    static void utcp_sent_cb(tcp_cb_event ev
                               , tcp_segment_t* seg
                               , tcp_socket_t* s
                               );

    static inline
    unsigned seg_harg2(unsigned ack_idx, unsigned slot_idx){
        return (slot_idx << 8) | ack_idx;
    };
    static inline
    unsigned seg_harg2_ack(unsigned harg2){
        return harg2 & 0xff;
    };
    static inline
    unsigned seg_harg2_slot(unsigned harg2){
        return harg2>>8;
    };
    
    static 
    bool_t is_sent(void* arg);
};



#endif //__UTCP_FLOW_H_
