#include "utcp_flow.h"
#include <runtime/assert.h>

#include <trace.h>



#define UTCP_DEBUG
#define FLOW_DEBUG

#ifdef UTCP_DEBUG
#define UTCP_PRINTF(...)     RT_PRINTF(__VA_ARGS__)

#ifdef UTCP_PRINTF
#define UTCP_printf(...)    UTCP_PRINTF(__VA_ARGS__)
#else
#define UTCP_printf(...)    LogMsg(vTCP, vTrace, __VA_ARGS__);
#endif

#else
#define UTCP_printf(...)
#endif


#ifdef FLOW_DEBUG
#define FLOW_PRINTF(...)     RT_PRINTF(__VA_ARGS__)

#ifdef FLOW_PRINTF
#define FLOW_printf(...)    FLOW_PRINTF(__VA_ARGS__)
#else
#define FLOW_printf(...)    LogMsg(vTCP, vTrace, __VA_ARGS__);
#endif

#else
#define FLOW_printf(...)
#endif



inline
void* PhyCoherent_FM(void* x){
    return MDMA_coherent(x);
}



utcptx_dma_flow::utcptx_dma_flow(){
    //memsetw(this, 0, sizeof(*this));
    mutex_init(&ack_signal);
    mutex_init(&sent_signal);
    ring_uindex_init(&ack_idx, ack_limit);
    dma_pool[0].init();
    dma_pool[1].init();
}

utcptx_dma_flow::~utcptx_dma_flow(){
    post_complete();
    if (mutex_is_my(&ack_signal))
        mutex_unlock(&ack_signal);
    if (mutex_is_my(&ack_signal))
        mutex_unlock(&ack_signal);
    dma_pool[0].close();
    dma_pool[1].close();
}

void utcptx_dma_flow::init(utcptx_dma_flow::dma_source* asrc, MDMA_handle ahdma, tcp_socket_t* sock)
{
    if (terminated())
        ack_release_finished();
    ack_clear();

    src = asrc;
    s   = sock;
    read_sample = src->ReadSample();

    //размер сегмента чтобы он содержал целове число фреймов или семплов
    // исхожу из положения что размер страницы всяко больше mss
    unsigned mss = s->mss;
    unsigned sampsize = src->sizeof_frame(mss);
    seg_size = (mss / sampsize) * sampsize;
    UTCP_printf("utcp_flow: init: sock $%x(mss %d) for samle %d seg %d\n"
                , s, mss, sampsize, seg_size );

    _terminate = false;
    hdma = ahdma;
    post_idx = 0;
    src->AtRead(&post_brick, 0);
    dma_prepare = &dma_pool[0];
    dma_send    = &dma_pool[1];
    assert(dma_prepare != NULL);
    assert(dma_send != NULL);

    init_bank(dma_prepare);
    init_bank(dma_send);

    //захвачу сигналы чтобы гарантировать ожидание на них
    mutex_lock(&ack_signal);
    mutex_lock(&sent_signal);
}

void utcptx_dma_flow::release(){
    post_complete();
    mutex_unlock(&ack_signal);
    mutex_unlock(&sent_signal);
    UTCP_printf("flow:close\n");
    dma_prepare->close();
    dma_send->close();
}

void utcptx_dma_flow::terminate(){
    _terminate = true;
    dma_prepare->terminate();
    dma_send->terminate();
}

unsigned utcptx_dma_flow::post_banks(unsigned count, tcps_flag_set opts){
    assert(dma_prepare != NULL);
    assert(dma_send != NULL);
    unsigned sent = 0;
    sopt = opts;
    while (count > sent){
        src->AtRead(&post_brick, post_idx);
        post_sample = read_sample + src->BlockSamples()*post_idx;
        UTCP_printf("utcp_flow: post at sample $%x idx %u + %d\n"
                    , post_sample, src->ReadIdx, post_idx);
        if ( src->isNULL(post_brick.data)){
            UTCP_printf("utcp_flow: break by no data\n");
            break;
        }
        //post_brick = src->AtRead();
        unsigned ofs = post_brick.pos;
        unsigned post_len = 0;
        post_pos     = 0;
        if (ofs < post_brick.size) {
        while (ofs < post_brick.size){
            ack_release_finished();

            if (!tcp_socket_is_online(s)){
                UTCP_printf("utcp_flow: break by sock offline\n");
                return sent;
            }
            if (terminated()){
                UTCP_printf("utcp_flow: break by terminate\n");
                return sent;
            }

            if (dma_prepare->slots_send != 0){
                UTCP_printf("utcp_flow: wait prepared sent[$%x] ack[%d] slots $%x\n"
                        , dma_prepare, dma_prepare->ack_pt, dma_prepare->slots_send
                        );
                if ( __glibc_unlikely(!wait_sent(dma_prepare)) )
                {
                    UTCP_printf("utcp_flow: break by sent wait\n");
                    return sent;
                }
            }

            post_len = post_assign(dma_prepare, ofs);
            if (dma_prepare->slots_send != 0) {
                UTCP_printf("utcp_flow: post prepare\n");
                if (ack_push_bank(dma_prepare) > ack_idx.mask)
                {
                    UTCP_printf("utcp_flow: break by push ack\n");
                    return sent;
                }
                if (!dma_prepare->load_go(hdma))
                {
                    UTCP_printf("utcp_flow: break by dma start\n");
                    return sent;
                }
            }
            if (dma_send->slots_send != 0) {
                if (post_sent(dma_send) == 0)
                {
                    // error to post
                    UTCP_printf("utcp_flow: break by send write\n");
                    return sent;
                }
            }
            ofs += post_len;
            {
                //swap_prep2send();
                dma_bank* tmp   = dma_prepare;
                dma_bank* tmp2  = dma_send;
                dma_prepare     = tmp2;
                dma_send        = tmp;
            }
        }
        sent++;
        post_idx++;
        post_sample += src->BlockSamples();
        } // if (ofs < post_brick.size)
    }
    UTCP_printf("utcp_flow: complete %d \n", sent);
    ack_release_finished();
    return sent;
}

int utcptx_dma_flow::post_sent(dma_bank* bank){
    UTCP_printf("utcp_flow: wait prepared load\n");
    assert(bank->load_wait());
#ifdef _UTCP_DEBUG
    assert(bank_load_ckeck(bank));
    assert(send_check(post_sample));
#endif
    int res = tcp_write_buf(s, bank->bufs_chain(), sopt | TF_SEG_CHAIN
            , utcptx_dma_flow::utcp_callback, (long)(void*)this 
            );
    return res;
}

bool
utcptx_dma_flow::send_check(utcptx_dma_flow::dma_source::RawSampleId brick_stamp) //, unsigned ofs
{
#ifdef UTCP_DEBUG

    dma_bank&       bank = *dma_send;
    unsigned        m = bank.slots_send;
    unsigned        ofs = bank.brick_ofs;
    unsigned        stamp = bank.brick_sample * (src->Pool()->sizeof_frame()/sizeof(unsigned));
    for (unsigned si = 0 ;
            (si < slot_limit) && (m != 0) ;
            m = m >> 1, si++
        )
    {
        dma_bank::slot_t& slot = bank.slot[si];
        buf_t* p = slot.p;
        assert(p->len == bank.seg_limit);
        unsigned* px = (unsigned*)p->payload;
        unsigned x = stamp + ofs/sizeof(unsigned);
        FLOW_printf("flow:check slot[%d:$%x] on stamp $%x:$%x \n", si, px, brick_stamp, x);
        for (unsigned i = 0; i < p->len/sizeof(unsigned) ; i++, px++, x++ ){
            assert2( (*px == x)
                    , "buf failed copy[$%x] = $%x expected $%x:\n%16@Dlx\n"
                    , i, *px, x, px
                    );
        }
        ofs += p->len;
    }

#endif
    return true;
}

bool
utcptx_dma_flow::bank_load_ckeck(dma_bank* bank) //, unsigned ofs
{
#ifdef UTCP_DEBUG

    unsigned ofs = bank->brick_sample * (src->Pool()->sizeof_frame()/sizeof(unsigned));
    ofs += bank->brick_ofs/sizeof(unsigned);
    for (unsigned ci = 0 ; ci < bank->dmatask.ncmd; ci++ )
    {
        MC_DMA_MemCh_Settings* cmd = &(bank->dmatask.req_coherent()->cmd[ci]);
        unsigned x = ofs;
        unsigned len = cmd->csr.bits.WCX+1;
        FLOW_printf("flow:check cmd[%d] on stamp $%x:$%x \n", ci, bank->brick_sample, x);
        unsigned* px = (unsigned*)PhyCoherent_FM((void*)cmd->ir0); //src
        for (unsigned i = 0; i < len ; i++, px++, x++ ){
            assert2( (*px == x)
                    , "buf($%x) failed fill[$%x] = $%x expected $%x:\n%16@Dlx\n"
                    , cmd->ir0 , i, *px, x, px
                    );
        }
        px = (unsigned*)PhyCoherent_FM((void*)cmd->ir1); //dst
        x = ofs;
        for (unsigned i = 0; i < len ; i++, px++, x++ ){
            assert2( (*px == x)
                    , "buf($%x) failed copy[$%x] = $%x expected $%x:\n%16@Dlx\n"
                    , cmd->ir1 , i, *px, x, px
                    );
        }
        ofs += len;
    }

#endif
    return true;
}

bool utcptx_dma_flow::post_complete(){
    if (terminated())
        return false;
    if (dma_send->slots_send != 0) {
        UTCP_printf("flow:complete: wait sent\n");
        ack_waits_bank& ack = ack_seq[dma_send->ack_pt];
        if (ack.slots_unack == dma_send->slots_send){
            post_sent(dma_send);
        }
        if (__glibc_unlikely( !wait_sent(dma_send) )){
            UTCP_printf("flow: incomplete by send await\n");
            return false;
        }
    }
    while (ack_awaiting_banks() > 0){
        ack_release_finished();
        if (terminated())
            break;
        if ((ack_awaiting_banks() > 0)){
            UTCP_printf("flow:complete: wait ack[%d]\n", ack_idx.read);
            mutex_wait(&ack_signal);
        }
    }
    UTCP_printf("flow:completed\n");
    return ring_uindex_free(&ack_idx);
}

unsigned utcptx_dma_flow::ack_push_bank(dma_bank* bank)
{
    while (ring_uindex_full(&ack_idx)){
        if (!tcp_socket_is_online(s))
            return ~0;
        mutex_wait(&ack_signal);
        ack_release_finished();
    }
    unsigned idx = ack_idx.write;

    ack_waits_bank& ack = ack_seq[idx];
    bank->ack_pt = idx;
    ack.assign(bank);
    ring_uindex_put(&ack_idx);
    return idx;
}

unsigned utcptx_dma_flow::ack_release_finished()
{
    unsigned idx = ack_idx.read;
    if (!ring_uindex_avail(&ack_idx))
        return 0;
    if (ack_seq[idx].slots_unack != 0)
        return 0;

    unsigned cnt = 0;
    while (ring_uindex_avail(&ack_idx) && (ack_seq[idx].slots_unack == 0))
    {
        UTCP_printf("utcp_flow: ack_release_finished: release ack %d\n", ack_idx.read);
        ring_uindex_get(&ack_idx);
        cnt++;
        if (!ring_uindex_avail(&ack_idx))
            break;
        idx = ack_idx.read;
        ack_waits_bank& ack = ack_seq[idx];
        if (ack.brick_sample != read_sample){
            if (src->NextRead()){
                post_idx--;
                read_sample += src->BlockSamples();
                UTCP_printf("utcp_flow: ack_release_finished: read at sample %u\n", read_sample);
            }
            else{
                UTCP_printf("utcp_flow: stop by read limit\n");
                return cnt;
            }
            if (ack.brick_sample != read_sample){
                UTCP_printf("utcp_flow: break by read unsync: read sample %u != ack %u\n"
                            , read_sample, ack.brick_sample);
                terminate();
                return cnt;
            }
        }
    }
    mutex_signal(&ack_signal, this);
    return cnt;
}

void utcptx_dma_flow::ack_clear(){
    while (ring_uindex_avail(&ack_idx)) {
        ack_waits_bank& ack = ack_seq[ack_idx.read];
        ack.brick_data = (dma_source::Brick)src->brickNULL;
        for (unsigned si = 0; si < slot_limit; si++){
            ack_waits_bank::slot_t& slot = ack.slot[si];
            slot.pdata = brick_null;
        }
        if (ack.slots_unack != 0)
            UTCP_printf("flow:clear:ack[%d]\n", ack_idx.read);
        ack.slots_unack = 0;
        ring_uindex_get(&ack_idx);
    }
}



void utcptx_dma_flow::utcp_callback(tcp_cb_event ev
                           , tcp_segment_t* seg
                           , tcp_socket_t* s
                           )
{
    utcptx_dma_flow* self = (utcptx_dma_flow*)seg->harg;
    switch (ev){
        case teSENT :
        case teFREE:    //after seg acked, and can be free
        case teDROP:    //seg aborted< and buffer can be free
            if (!self->onSent(self->dma_send, seg))
                if (!self->onSent(self->dma_prepare, seg)){
                    tcp_debug("utcp:QsTcpProxy:send: uncknown segment $%x occasion!!!\n", seg);
                    //тут нельзя оказаться, это значит что получил
                    //  сегмент который не в отосланных банках
                    //просто оставлю данные сегмента на месте, чтобы избежать падения
                    //assert();
                    break;
                }
            seg->handle = &(utcptx_dma_flow::utcp_sent_cb);
            if (ev == teSENT)
                break;
            else{   //teFREE, teDROP
                utcptx_dma_flow::utcp_sent_cb(ev, seg, s);
            }
            break;

        case teREXMIT: //on ack loose, need provide data for seg
            assert2(seg->p != 0, "utcp:QsTcpProxy:send: uncknown segment $%x for REXMIT occasion!!!\n", seg);
            break;
    }
}

void utcptx_dma_flow::utcp_sent_cb(tcp_cb_event ev
                           , tcp_segment_t* seg
                           , tcp_socket_t* s
                           )
{
    utcptx_dma_flow* self = (utcptx_dma_flow*)seg->harg;
    //к этому времени инициализирован seg->harg2, потому можно воспользоваться им для отыскания
    //  его слота.
    //  если seg->p - в наличии, это скорее всего REXMIT-сегмент.
    unsigned waits_idx = utcptx_dma_flow::seg_harg2_ack(seg->harg2);
    unsigned slot_idx  = utcptx_dma_flow::seg_harg2_slot(seg->harg2);
    ack_waits_bank* ack = &(self->ack_seq[waits_idx]);

    switch (ev){
        case teSENT :
            self->onSent(ack, seg, slot_idx);
            break;

        case teFREE:    //after seg acked, and can be free
        case teDROP:    //seg aborted< and buffer can be free
            self->onAck(ack, seg, slot_idx);
            break;

        case teREXMIT: //on ack loose, need provide data for seg
            if (seg->p != 0)
                break;
            //создам новый буфер и загружу в него данные
            buf_t* p = dma_bank::alloc_buf(s->ip->pool, seg->len);
            //buf_t* p = 0;
            if (p == 0){
                UTCP_printf("utcp rexmit:sock $%x cant alloc buffer[%u] for seg $%x \n",s, seg->len, seg);
                tcp_abort(s);
                break;
            }
            ack_waits_bank::slot_t& slot = ack->slot[slot_idx];
            assert(slot.seqno == seg->hsave.seqno);
            self->rexmit(ack, p, slot_idx);
            tcp_segment_assign_buf(s, seg, p);
            break;
    }
}

bool    utcptx_dma_flow::onSent(utcptx_dma_flow::dma_bank* bank,          tcp_segment_t* seg)
{
    //*определю отосланый слот, и укажу его в сегменте.
    if (bank->slots_send == 0 )
        return false;
    dma_bank::slot_t*   slot = bank->slot;
    for (unsigned s = 0; s < slot_limit; s++){
        if (seg->p == slot[s].p){
            seg->harg2 = utcptx_dma_flow::seg_harg2(bank->ack_pt, s);
            bank->slots_send &= ~(1<<s);
            seg->p = 0;
            ack_waits_bank* ack = &(ack_seq[bank->ack_pt]);
            ack_waits_bank::slot_t& slot = ack->slot[s];
            slot.seqno = seg->hsave.seqno;
            seg->dataptr = (void*)slot.pdata;
            UTCP_printf("utcp: sent [%d].slot[%d] at $%x seq %u\n"
                        , bank->ack_pt, s, slot.pdata, slot.seqno);
            break;
        }
    }
    //* проверю что банк полностью отослан, и просигналю его освобождение
    if (bank->slots_send == 0 ){
        UTCP_printf("flow:ack sent bank complete on ack[%d]\n", bank->ack_pt);
        mutex_signal(&sent_signal, bank);
    }
    return (seg->p == 0);
}

bool    utcptx_dma_flow::onSent(utcptx_dma_flow::ack_waits_bank* bank,    tcp_segment_t* seg, unsigned slotid)
{
    ack_waits_bank::slot_t& slot = bank->slot[slotid];
    assert(seg->hsave.seqno == slot.seqno);
    UTCP_printf("utcp: sent slot[%u] at $%x[$%x] seq %u\n"
                , slotid, bank->brick_data, slot.pdata, slot.seqno);
    if (seg->p != 0){
        mem_free(seg->p);
        seg->p = 0;
        seg->dataptr = (void*)slot.pdata;
    }
    else {
        assert(seg->dataptr == (void*)slot.pdata);
    }
    return true;
}

bool    utcptx_dma_flow::onAck(utcptx_dma_flow::ack_waits_bank* bank,     tcp_segment_t* seg, unsigned slotid)
{

    if ( is_null( (inBrickPtr) seg->dataptr) )
        return false;

    assert2(seg->hsave.seqno == bank->slot[slotid].seqno
            , "utcpflow:ack[%d] strange on seq %u expect %u\n"
            , slotid, seg->hsave.seqno, bank->slot[slotid].seqno);
    assert2(seg->dataptr == (void*)bank->slot[slotid].pdata,
            "utcpflow:ack on seq %u strange data $%x\n"
            , seg->hsave.seqno, seg->dataptr
            );
    bank->slots_unack &= ~(1<<slotid);
    //bank->slot[slotid].pdata = 0;

    //* проверю что банк полностью отправлен и подтвержден, и просигналю его освобождение
    if (bank->slots_unack == 0 ){
        UTCP_printf("flow:ack slot complete\n");
        mutex_signal(&ack_signal, bank);
    }

    //слот освобожден и сегмент более не подлежит реинкарнации
    seg->dataptr = 0;//brick_null;
    seg->handle  = 0;
    return true;
}

bool    utcptx_dma_flow::rexmit(utcptx_dma_flow::ack_waits_bank* bank,    buf_t* p, unsigned slotid)
{
    //создам новый буфер и загружу в него данные
    unsigned len = p->tot_len;
    assert(seg_size == len);
    ack_waits_bank::slot_t& slot = bank->slot[slotid];
    assert( !is_null(slot.pdata) );
    UTCP_printf("utcp rexmit:sock $%x alloc REXMIT for seq %u at $%x[$%x]\n"
                , s, slot.seqno     , bank->brick_data, slot.pdata
                );

    dma_source::brick_navigator b(bank->brick_data, src);
    dma_task_compose    dmatask;
    //size_t l =
            dmatask.add(p->payload, b, slot.pdata, len);
    assert(dmatask.len == len);

    mutex_lock(&(dmatask.req_coherent()->signal));
    dmatask.go(hdma);
    dmatask.wait();
    mutex_unlock(&(dmatask.req_coherent()->signal));

    return true;
}



void utcptx_dma_flow::init_bank(utcptx_dma_flow::dma_bank* self)
{
    self->init(s->ip->pool, seg_size);
    self->brick = &post_brick;
}

utcptx_dma_flow::dma_bank::dma_bank(){
    init();
}

void utcptx_dma_flow::dma_bank::init(){
    memsetw(this, 0, sizeof(*this));
    dmatask.init();
}

void utcptx_dma_flow::dma_bank::init(mem_pool_t* pool, unsigned seglen){
    //init();
    _terminate  = false;
    slots_send  = 0;
    brick       = NULL;
    dmatask.init();

    seg_limit   = seglen;

    UTCP_printf("utcp_flow: init for seglen %d\n", seglen);

    for (unsigned si = 0; si < slot_limit; si++){
        slot_t& ds = slot[si];
        if (ds.p != 0){
            buf_reset_header(ds.p, TCP_SEG_RESERVE);
            if (ds.p->tot_len < seg_limit){
                buf_free(ds.p);
                ds.p = 0;
            }
            else if (ds.p->tot_len > seg_limit)
                buf_truncate(ds.p, seg_limit);
            UTCP_printf("utcp_flow: realloc slot[%d]\n", si);
        }
        if (ds.p == 0){
            ds.p = alloc_buf(pool, seg_limit);
            UTCP_printf("utcp_flow: alloc slot[%d].buf($%x) len %d\n", si, ds.p, ds.p->tot_len);
        }
        ds.src = brick_null;
    }
}

void utcptx_dma_flow::dma_bank::close(){
    dmatask.clear();
    slot_t* ds = slot;
    for (unsigned s = 0; s < slot_limit ; s++, ds++) //, brick_ofs < brick->size
    {
        buf_free(ds->p);
        ds->p = 0;
    }
}

buf_t* utcptx_dma_flow::dma_bank::alloc_buf(mem_pool_t* pool, unsigned seglen)
{
    //буферы картирую в некешируемой когерентной памяти 
    buf_t* res = buf_alloc(pool, seglen, TCP_SEG_RESERVE);
    res = (buf_t*)PhyCoherent_FM(res);
    res->payload = (unsigned char*)PhyCoherent_FM(res->payload);
    return res;
}

bool_t utcptx_dma_flow::is_sent(void* arg){
    utcptx_dma_flow::dma_bank* bank = (utcptx_dma_flow::dma_bank*)arg;
    return (bank->slots_send == 0) || bank->terminated();
}

bool    utcptx_dma_flow::wait_sent(dma_bank* bank){
    //while ((bank->slots_send != 0) && !terminated()) {
    //    mutex_wait(&sent_signal);
    //}
    if (!is_sent(bank))
        mutex_wait_until(&sent_signal, is_sent, bank);
    return (bank->slots_send == 0);
}

utcptx_dma_flow::dma_task_compose::dma_task_compose()
{
    memsetw(this, 0, sizeof(*this));
    init();
};

void utcptx_dma_flow::dma_task_compose::init(){
    MDMA_clearRequest(req_coherent());
    MDMA_cmdChain_link(req_coherent()->cmd, cmd_limit-1);
    assert(check_link(cmd_limit));
}

bool utcptx_dma_flow::dma_task_compose::check_link(unsigned len){
    MC_DMA_MemCh_Settings* cmd = req_coherent()->cmd;
    for (unsigned ci = 1; ci < len; ci++){
        dma_phyadr dmac = MDMA_phyaddr(cmd+ci);
        assert2(cmd[ci-1].chain == dmac, "break dma chain at %d\n", ci);
    }
    return true;
}

bool utcptx_dma_flow::dma_task_compose::go(MDMA_handle hdma) {
    FLOW_printf("flow_dma[$%x]: go %d cmd on dma $%x\n", this, ncmd, hdma);
    MDMA_cmdChain_activateAtLast(req_coherent()->cmd, ncmd-1);
#ifdef UTCP_DEBUG
    assert(check_link(ncmd));
#endif
    return MDMA_startReq(hdma, req_coherent()); //(MDMA_request*)PhyCoherent_FM(&req.r)
};

void utcptx_dma_flow::dma_task_compose::add(void* dst, void* src, size_t _len){
    MDMA_cmd_assign( &(req_coherent()->cmd[ncmd]), dst, src, _len/sizeof(long), 1, 1 );
    FLOW_printf("flow_dma[$%x]: add cmd[%d] $%x->$%x len $%x\n", this, ncmd, src, dst, _len);
    ++ncmd;
    len += _len;
}

utcptx_dma_flow::inBrickPtr
utcptx_dma_flow::dma_task_compose::add(void* _dst
        , const utcptx_dma_flow::dma_source::brick_navigator& src
        , utcptx_dma_flow::inBrickPtr ofs, size_t size
        )
{
    assert(check_link(cmd_limit));
    assert(!is_null(ofs));
    char* dst = (char*) _dst;
    FLOW_printf("flow_dma[$%x]: add brick $%x->$%x limit $%x\n", this, src.data, dst, size);
    if (ncmd < cmd_limit)
    while ( size >= src.ptr_align ){
        if (is_null(ofs))
            break;
        if (ncmd >= cmd_limit)
            break;
        size_t piece_len = src.len_at(ofs);
        if (piece_len <= size)
            ;
        else {
            piece_len = src.align(size);
        }
        add(dst, src.data_at(ofs), piece_len);
        dst += piece_len;
        size -= piece_len;
        ofs = src.succ(ofs, piece_len);
    }
    return ofs;
}

void utcptx_dma_flow::dma_task_compose::clear(){
    MDMA_stopReq(req_coherent());
    len     = 0;
    ncmd    = 0;
}

unsigned
utcptx_dma_flow::post_assign(utcptx_dma_flow::dma_bank* bank, unsigned brick_ofs)
{
    dma_source::SoftBrick*      brick = &post_brick;
    dma_source::brick_navigator b(brick->data, src);
    dma_task_compose&           dmatask = bank->dmatask;

    UTCP_printf("utcp prepare: bank($%x) brick $%x at $%x\n", bank, brick->data, brick_ofs);

    bank->brick_sample  = post_sample;
    bank->brick_ofs     = brick_ofs;
    bank->slots_send    = 0;
    dmatask.clear();
    dma_bank::slot_t* ds = bank->slot;
    for (unsigned s = 0; s < slot_limit ; s++, ds++) //, brick_ofs < brick->size
    {
        if (brick_ofs < brick->size){
            buf_t* p    = ds->p;
            if (p->next != 0)
                buf_dechain(p);
            buf_reset_header(p, TCP_SEG_RESERVE);
            ds->src     = post_pos;
            assert2(bank->seg_limit == p->tot_len,
                    "buf($%x) assign len %d at $%x[$%x] \n", p, p->tot_len, brick->data, ds->src);
            FLOW_printf("utcp prepare:assign slot[%d] at $%x[$%x]\n", s, brick->data, ds->src);
            unsigned l = dmatask.len;
            post_pos = dmatask.add(p->payload, b, ds->src, bank->seg_limit);
            l = (dmatask.len - l);
            assert( l == bank->seg_limit);
            brick_ofs += l;
            bank->slots_send = (bank->slots_send << 1) | 1;
        }
        else
            ds->src     = brick_null;
    }
    UTCP_printf("utcp prepare:assigned slots $%x len $%x\n", bank->slots_send, dmatask.len);
    return dmatask.len;
}

bool utcptx_dma_flow::dma_bank::load_wait(){
    if(!dmatask.wait())
        return 0;
#ifdef _UTCP_DEBUG
    unsigned m = slots_send;
    for (unsigned si = 0 ;
            (si < slot_limit) && (m != 0) ;
            m = m >> 1, si++
        )
    {
        buf_t* p = slot[si].p;
        assert(p->len == seg_limit);
        void*   data = slot[si].src;
        assert2(memcmp(p->payload, data, sample_len) == 0
                , "dma copy[%d] $%x ->$%x failed"
                , si, slot[si].src , p->payload
                );
    }
#endif
    return 1;
}

buf_t* utcptx_dma_flow::dma_bank::bufs_chain(){
    assert(slot[0].p != 0);
    assert(slot[0].p->next == 0);
    slot_t* ds = slot+1;
    for (unsigned s = 1; (s < slot_limit) && !is_null(ds->src) ; s++, ds++)
    {
        assert(ds->p != 0);
        assert(slot[0].p != ds->p);
        assert(ds->p->next == 0);
        buf_chain(slot[0].p, ds->p);
    }
    return slot[0].p;
}

void utcptx_dma_flow::ack_waits_bank::assign(utcptx_dma_flow::dma_bank* bank)
{
    brick_sample    = bank->brick_sample;
    brick_data      = bank->brick->data;
    slots_unack     = bank->slots_send;
    //unsigned limit  = bank->brick->size;
    slot_t* ds = slot;
    dma_bank::slot_t* ss = bank->slot;
    for (unsigned s = 0; s < slot_limit ; s++, ds++, ss++){
        ds->pdata = ss->src;
        UTCP_printf("utcp: prepare ack: slot[%d] at $%x\n",s, ds->pdata);
    }
}
