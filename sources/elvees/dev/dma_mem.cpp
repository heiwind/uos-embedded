#include <runtime/lib.h>
#include <cstddef>
#include <elvees/dev/dma_mem.h>

#if UOS_DMA_MEM_IO_H_ > 0

#include <kernel/uos.h>
#include <kernel/internal.h>
#include <runtime/list.h>
#include <runtime/assert.h>
#include <timer/etimer_threads.h>



#define DEBUG_DMA
#define MCSL_MDMA_SAFE      1

#ifdef DEBUG_DMA
#ifdef RT_PRINTF
#define DMA_printf(...)    RT_PRINTF(__VA_ARGS__)
#define DMA_puts(...)      RT_PRINTF(__VA_ARGS__)
#else
#define DMA_printf(...)    debug_printf(__VA_ARGS__)
#define DMA_puts(s)        debug_puts(s)
#endif
#else
#define DMA_printf(...)
#define DMA_puts(...)
#endif

#ifndef UOS_STRICT_DMA_MEM
#define UOS_STRICT_DMA_MEM   (~0ul)
#endif



extern "C" {

bool_t MDMA_ch_ISR(void* arg) __cpp_decls;
}

/** 
 * по останову обработки цепочки, последний запрос цепочки (на котором произошелостанов)
 *  сигналится, т.о. для задания сигналящего запроса - он должен быть поставлен
 *  на исполнение без флага CHEN.
 *  после сигнала в запрос, из цепочки запросов выбирается следующий и с него
 *      продолжается обработка цепи.
 * */
CODE_ISR __NOTHROW
bool_t MDMA_ch_ISR(void* arg) __noexcept
{
    MDMA_Descriptor* self = ((MDMA_handle)arg);
    DMA_MEM_CH_CSR_REG status;
    status.data     = self->io->CSR.data;
    if (status.bits.DONE == 0){
        //текущее прерывание не завершение цепочки, пропущу его
        arch_intr_allow (self->isr.irq->irq);
        DMA_printf("dma[$%x]: not DONE\n", self);
        return 1;
    }

    if (!list_is_empty(&self->chain)){
        //найду текущий запрос и просигналю его, а запросы перед ним освобожу
        MDMA_request* item;
        item = (MDMA_request*)list_first(&self->chain);
        {
            item->cmd[0].csr.bits.DONE = MC_DMA_MEM_CH_CSR_DONE_SET;
            mutex_awake(&item->signal, item);
            //освобождаю все запросы выполненные до текущего включительно
            DMA_printf("dma[$%x]: unlink completed $%x\n", self, item);
            list_unlink(&item->item);
        }
    }
    if (!list_is_empty(&self->chain)){
        //есть еще запросы,запущу их
        MDMA_request* curent = (MDMA_request*)list_first(&self->chain);
        DMA_printf("dma[$%x]: go $%x\n", self, curent);
        dma_mem_run_cmd(self->io, curent->cmd);
    }
    arch_intr_allow (self->isr.irq->irq);
    // просигналю ожидающим на мутехе
    return 0;
}

MDMA_Descriptor* MDMA_clearDescriptor(MDMA_Descriptor* target){
    UOS_STRICT(DMA_MEM , assert( ((size_t)target & 7) == 0 ) );
    memsetw(target, 0, sizeof(MDMA_Descriptor));
    list_init(&target->chain);
    return target;
}

MDMA_request*    MDMA_clearRequest(MDMA_request* src){
    assert( ((size_t)src & 7) == 0 );
    memsetw(src, 0, sizeof(MDMA_request));
    memsetw(MDMA_coherent(src->cmd), 0, sizeof(src->cmd));
    list_init(&src->item);
    return src;
}


MDMA_handle MDMA_requestChanel(MDMA_handle h, enum MDMA_chanel_id chanel){
    if (h->isr.irq == NULL) {
        h->io = &(MC_DMA_MEM_CH(chanel));
        mutex_attach_irq(&(h->isr), MC_IRQ_EVT_DMAMEM_CHn(chanel), &(MDMA_ch_ISR), h );
        DMA_printf("dma[$%x]: attached\n", h);
    }
    //else
        //видимо хендл давно открыт и действует
    MC_SYSCLK_enable(MC_CLKEN_DMA_MEM);
    return h;
}

inline
void DMAmm_on_drop_req(MDMA_request* item){
    DMA_printf("dma: signal on req\n", item);
    if (mutex_is_wait(&(item->signal)))
    mutex_signal(&(item->signal), item);
}

bool_t MDMA_close(MDMA_handle h){
    DMA_printf("dma[$%x]: close\n", h);
    MDMA_stop(h);
    MDMA_Descriptor& self = *h;
    if (list_is_empty(&self.chain))
        return false;
    mutex_lock(&self.isr);
    //освобожу запросы в очереди
    MDMA_request* item;
    MDMA_request* next;
    DMA_printf("dma[$%x]: clean unprocessed chain\n", h);
    list_safe_iterate(item, next, &self.chain){
        DMAmm_on_drop_req(item);
        list_unlink(&item->item);
    }
    mutex_dettach_irq(&self.isr);
    mutex_unlock_irq(&self.isr);
    return true;
}

bool_t MDMA_start(MDMA_handle h){
    MDMA_Descriptor& self = *h;
    if (self.io->RUN.bits.RUN == MC_DMA_MEM_CH_CSR_RUN_STOP) {
        if (!list_is_empty(&self.chain)){
            //есть еще запросы,запущу их
            MDMA_request* curent = (MDMA_request*)list_first(&self.chain);
            DMA_printf("dma[$%x]: start from $%x\n", h, curent);
            dma_mem_run_cmd(self.io, curent->cmd);
            return true;
        }
    }
    else {
        //канал уже пашет, запускать его не имеет смысла
        return true;
    }
    return false;
}

bool_t MDMA_stop(MDMA_handle h){
    MDMA_Descriptor& self = *h;
    DMA_printf("dma[$%x]: stop\n", h);
    if (self.io->RUN.bits.RUN == MC_DMA_MEM_CH_CSR_RUN_STOP) 
        return false;
    DMA_printf("dma[$%x]: break\n", h);
    self.io->RUN.bits.RUN   = MC_DMA_MEM_CH_CSR_RUN_STOP;
    self.io->CSR.bits.WCX   = 0;
    self.io->Y.bits.WCY     = 0;
    return true;
}

bool_t MDMA_startReq(MDMA_handle h, MDMA_request* r){
    MDMA_Descriptor* self = h;
    MC_DMA_MemCh_Settings* cmd = (MC_DMA_MemCh_Settings*)MDMA_coherent(r->cmd);
    if (cmd->chain == 0){
        cmd->csr.bits.CHEN = MC_DMA_MEM_CH_CSR_CHEN_DISABLE;
        cmd->csr.bits.IM   = MC_DMA_MEM_CH_CSR_IM_ENABLE;
    }
    cmd->csr.bits.DONE = MC_DMA_MEM_CH_CSR_DONE_CLEAR;
    mutex_lock(&(self->isr));
    bool need_start = list_is_empty(&self->chain);
    list_append(&self->chain, &r->item);
    r->dma = h;
    DMA_printf("dma[$%x]: start req $%x\n", h, r);
    if (need_start){
        dma_mem_run_cmd(self->io, cmd);
        DMA_printf("dma[$%x]: go req $%x\n", h, r);
    }
    mutex_unlock(&self->isr);
    return true;
}

bool_t MDMA_stopReq(MDMA_request* r){
    if (r->dma == NULL) return false;
    if (list_is_empty(&r->item)) return false;
    MDMA_Descriptor& self = *(r->dma);
    DMA_printf("dma[$%x]: stop req $%x\n", &self, r);
    mutex_lock(&self.isr);
    if (!list_is_empty(&r->item)){
        MDMA_handle h = r->dma;
        bool break_current = ( MDMA_phyaddr(list_first(&self.chain))
                             == MDMA_phyaddr(&(r->item)) );
        if (break_current){
            DMA_printf("dma[$%x]: break current req\n", &self);
            MDMA_stop(h);
        }
        list_unlink(&r->item);
        if (break_current)
            MDMA_start(h);
    }
    mutex_unlock(&self.isr);
    DMAmm_on_drop_req(r);
    return true;
}

bool_t dmamm_wait_until(void* arg){
    MDMA_request* r = (MDMA_request*)arg;
    volatile MC_DMA_MemCh_Settings * cmd = (MC_DMA_MemCh_Settings*)MDMA_coherent(r->cmd);
    if (cmd->csr.bits.DONE == MC_DMA_MEM_CH_CSR_DONE_SET)
        return 1;
    if (list_is_empty(&r->item))
        return 1;
    return 0;
}

bool_t MDMA_waitReq(MDMA_request* r){
    if (r->dma == NULL) {
        DMA_puts("dma: inactive\n");
        return false;
    }

    volatile MC_DMA_MemCh_Settings * cmd = (MC_DMA_MemCh_Settings*)MDMA_coherent(r->cmd);
    if (cmd->csr.bits.DONE == MC_DMA_MEM_CH_CSR_DONE_SET)
        return true;
    if (list_is_empty(&r->item))
        return (cmd->csr.bits.DONE == MC_DMA_MEM_CH_CSR_DONE_SET);

    DMA_printf("dma: wait req $%x\n", r);
    mutex_wait_until(&r->signal, dmamm_wait_until, r);

    if (cmd->csr.bits.DONE == MC_DMA_MEM_CH_CSR_DONE_SET)
        //alredy completed
        return true;
    if (list_is_empty(&r->item)) {
        DMA_puts("dma: nothing wait\n");
        return false;
    }
    return false;
}

bool_t MDMA_waitReqTimed(MDMA_request* r, unsigned TOus){
    if (r->dma == NULL) return false;
    MC_DMA_MemCh_Settings* cmd = (MC_DMA_MemCh_Settings*)MDMA_coherent(r->cmd);
    if (cmd[0].csr.bits.DONE == MC_DMA_MEM_CH_CSR_DONE_SET)
        //alredy completed
        return true;
    if (list_is_empty(&r->item)) return false;
    return mutex_etimedwait(&r->signal, TOus);
}


//* \~russian - соединяет в цепь соманды ДМА
void MDMA_cmdChain_link(MC_DMA_MemCh_Settings* cmd, int tail_len)
{
    cmd = (MC_DMA_MemCh_Settings*)MDMA_coherent(cmd);
    dma_phyadr dma_next;
    for(dma_next = MDMA_phyaddr(cmd+1);
        tail_len > 0;
        tail_len--, cmd++, dma_next+=sizeof(*cmd) )
    {
        cmd->chain = dma_next;
    }
    cmd->chain = 0;
}

//* \~russian - настраивает флаги цепи соманд с активацие прерывания на последней
void MDMA_cmdChain_activateAtLast(MC_DMA_MemCh_Settings* cmd, int tail_len){
    cmd = (MC_DMA_MemCh_Settings*)MDMA_coherent(cmd);
    for(;tail_len > 0;tail_len--, cmd++)
    {
        cmd->csr.bits.CHEN  = MC_DMA_MEM_CH_CSR_CHEN_ENABLE;
        cmd->csr.bits.IM    = MC_DMA_MEM_CH_CSR_IM_DISABLE;
        UOS_STRICT( DMA_MEM, ) assert2(cmd->chain == MDMA_phyaddr(cmd+1)
                , "mcsl:memdma:Chain_activateAtLast broken cmd $%x by tail %d\n"
                , cmd, tail_len
                );
    }
    cmd->csr.bits.CHEN  = MC_DMA_MEM_CH_CSR_CHEN_DISABLE;
    cmd->csr.bits.IM    = MC_DMA_MEM_CH_CSR_IM_ENABLE;
}



#endif //UOS_DMA_MEM_IO_H_ > 0
