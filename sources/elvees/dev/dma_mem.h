#ifndef _ELVEES_DMAMEM_
#define _ELVEES_DMAMEM_

/* ============================================================================
 * Copyright (c) 2015 Lityagin Alexander
 *
 * ============================================================================
 */


/** @file dma_mem.h
 *
 *  @brief 
 *
 */

/** @defgroup ELVEES_DMA_MEM_API DMA_MEM
 *
 * @section Introduction
 *
 * @subsection xxx Overview
 *
 *  @subsection References
 *  1892VM10YA USERS GUIDE.pdf
 */

#include <kernel/uos.h>
#include <runtime/sys/uosc.h>
#include <stdint.h>

#include <elvees/dev/dma_mem-io.h>

#if UOS_DMA_MEM_IO_H_ > 0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    MC_DMA_MEM_CH_REGS_TYPE*   io;
    mutex_t                 access;
    mutex_t                 isr;
    list_t                  chain;
} MDMA_Descriptor;

typedef MDMA_Descriptor* MDMA_handle;

typedef struct __attribute__((packed,aligned(8))) {
    list_t              item;
    mutex_t             signal;
    MDMA_handle         dma;
    MC_DMA_MemCh_Settings   cmd[1];
} MDMA_request;

//* anonimous type should be only static, cause it cant be referenced anywhere else
//    static
#define DMAMEM_REQ_CHAIN(n) \
    union __attribute__((packed,aligned(8))) {\
        ARRAY (data, sizeof(MDMA_request) + (n-1) * sizeof(MC_DMA_MemCh_Settings));\
        MDMA_request r;\
        struct { \
            MDMA_request      r;\
            MC_DMA_MemCh_Settings  tail[n-1];\
        } field;\
    }

MDMA_Descriptor* MDMA_clearDescriptor(MDMA_Descriptor* target) __cpp_decls;
MDMA_request*    MDMA_clearRequest(MDMA_request* src) __cpp_decls;

enum MDMA_chanel_id{
      DMA_mem_chAny   = -1
    , DMA_mem_ch0   = 0
    , DMA_mem_ch1   = 1
    , DMA_mem_ch2   = 2
    , DMA_mem_ch3   = 3
    , DMA_mem_TOTAL
};
MDMA_handle MDMA_requestChanel(MDMA_handle h, enum MDMA_chanel_id chanel) __cpp_decls;
/*
 * \return true - если при закрытии были открытые запросы и они были сброшены 
 * */
bool_t MDMA_close(MDMA_handle h) __cpp_decls;

INLINE
bool_t MDMA_pending(MDMA_handle h){
    return !list_is_empty(&h->chain);
}

/*
 * \return true - если началась или идет передача  
 * */
bool_t MDMA_start(MDMA_handle h) __cpp_decls;

/*
 * \return true - если была прервана текущая передача
 * \return false - если канал простаивал  
 * */
bool_t MDMA_stop(MDMA_handle h)__cpp_decls;

/*
 * \return true - если была прервана текущая передача
 * \return false - если канал простаивал  
 * */
INLINE
bool_t MDMA_pause(MDMA_handle h){
    MC_DMA_MEM_CH_REGS_TYPE*   io = h->io;
    if (io->RUN.bits.RUN == MC_DMA_MEM_CH_CSR_RUN_STOP) 
        return false;
    io->RUN.bits.RUN   = MC_DMA_MEM_CH_CSR_RUN_STOP;
    return true;
}

INLINE
void MDMA_resume(MDMA_handle h){
    MC_DMA_MEM_CH_REGS_TYPE*   io = h->io;
    io->RUN.bits.RUN   = MC_DMA_MEM_CH_CSR_RUN_GO;
}

INLINE
int MDMA_wait(MDMA_handle h){
    mutex_wait(&(h->isr));
    return true;
}



//* \return - некешируемый адрес х, действительно для FixedMode
INLINE
void* MDMA_coherent(void* x){
    return (void*)( mips_virtual_addr_to_physical((size_t)x) | MC_KSEG_ORG(MC_KSYNC) );
}

INLINE
dma_phyadr MDMA_phyaddr(void* x){
    return mips_virtual_addr_to_physical((size_t)x);
}


bool_t MDMA_startReq(MDMA_handle h, MDMA_request* r) __cpp_decls;

/*
 * \return false - если запрос не активен(не поставлен на выполнения)  
 * */
bool_t MDMA_stopReq(MDMA_request* r) __cpp_decls;

/*
 * \return false - если запрос не активен(не поставлен на выполнения)  
 * */
bool_t MDMA_waitReq(MDMA_request* r) __cpp_decls;

/*
 * \return false - если запрос не активен(не поставлен на выполнения)  
 * */
bool_t MDMA_waitReqTimed(MDMA_request* r, unsigned TOus) __cpp_decls;



INLINE
void MDMA_cmd_assign(MC_DMA_MemCh_Settings* cmd
            , void* dst, void* src
            , int cx
            , int ddx, int sdx
            )
{
    cmd->ir0 = MDMA_phyaddr(src);
    cmd->ir1 = MDMA_phyaddr(dst);
    cmd->x.bits.OR0 = sdx;
    cmd->x.bits.OR1 = ddx;
    cmd->y.data = 0;
    cmd->csr.data = MC_DMA_MEM_CH_CSR_GO
                  | MC_DMA_MEM_CH_CSR_IR01DIR
                  | MC_DMA_MEM_CH_CSR_32BIT
                  | MC_DMA_MEM_CH_CSR_PLAINADR
                  | MC_DMA_MEM_CH_CSR_IR1_1D
                  | MC_DMA_MEM_CH_CSR_NOCHAIN
                  | MC_DMA_MEM_CH_CSR_IRQ
                  ;
    cmd->csr.bits.WCX = cx-1;
}

INLINE
void MDMA_cmd_assign_dst2d(MC_DMA_MemCh_Settings* cmd
            , void* dst, void* src
            , int cx
            , int ddx, int sdx
            , int cy , int ddy
            )
{
    cmd->ir0 = MDMA_phyaddr(src);
    cmd->ir1 = MDMA_phyaddr(dst);
    cmd->x.bits.OR0 = sdx;
    cmd->x.bits.OR1 = ddx;
    cmd->y.bits.OY  = ddy - (ddx*(cx-1));
    cmd->y.bits.WCY = cy-1;
    cmd->csr.data = MC_DMA_MEM_CH_CSR_GO
                  | MC_DMA_MEM_CH_CSR_IR01DIR
                  | MC_DMA_MEM_CH_CSR_32BIT
                  | MC_DMA_MEM_CH_CSR_PLAINADR
                  | MC_DMA_MEM_CH_CSR_IR1_2D
                  | MC_DMA_MEM_CH_CSR_NOCHAIN
                  | MC_DMA_MEM_CH_CSR_IRQ
                  ;
    cmd->csr.bits.WCX = cx-1;
}

INLINE
void MDMA_cmd_assign_src2d(MC_DMA_MemCh_Settings* cmd
            , void* dst, void* src
            , int cx
            , int ddx, int sdx
            , int cy , int sdy
            )
{
    cmd->ir0 = MDMA_phyaddr(dst);
    cmd->ir1 = MDMA_phyaddr(src);
    cmd->x.bits.OR1 = sdx;
    cmd->x.bits.OR0 = ddx;
    // вычислю инкремент позиции из абсолютных размерностей массива
    cmd->y.bits.OY  = sdy - (sdx*(cx-1));
    cmd->y.bits.WCY = cy-1;
    cmd->csr.data = MC_DMA_MEM_CH_CSR_GO
                  | MC_DMA_MEM_CH_CSR_IR10DIR
                  | MC_DMA_MEM_CH_CSR_32BIT
                  | MC_DMA_MEM_CH_CSR_PLAINADR
                  | MC_DMA_MEM_CH_CSR_IR1_2D
                  | MC_DMA_MEM_CH_CSR_NOCHAIN
                  | MC_DMA_MEM_CH_CSR_IRQ
                  ;
    cmd->csr.bits.WCX = cx-1;
}

INLINE
bool_t MDMA_req_assign(MDMA_request* r
            , void* dst, void* src
            , int cx
            , int ddx, int sdx
            )
{
    MDMA_cmd_assign(r->cmd, dst, src, cx, ddx, sdx);
    return true;
}

INLINE
bool_t MDMA_req_assign_dst2d(MDMA_request* r
            , void* dst, void* src
            , int cx
            , int ddx, int sdx
            , int cy , int ddy
            )
{
    MDMA_cmd_assign_dst2d(r->cmd, dst, src, cx, ddx, sdx, cy,  ddy);
    return true;
}

INLINE
bool_t MDMA_req_assign_src2d(MDMA_request* r
            , void* dst, void* src
            , int cx
            , int ddx, int sdx
            , int cy , int sdy
            )
{
    MDMA_cmd_assign_src2d(r->cmd, dst, src, cx, ddx, sdx, cy, sdy);
    return true;
}



//* \~russian - соединяет в цепь соманды ДМА.
//* \arg cmd      - массив команд, 1й элемент - головная команда, за которой
//*                 следует присоединяемый хвост
//* \arg tail_len - длина хвоста присоединяемого к cmd
void MDMA_cmdChain_link(MC_DMA_MemCh_Settings* cmd, int tail_len)   __cpp_decls;
//* \~russian - настраивает флаги цепи соманд с активацие прерывания на последней
void MDMA_cmdChain_activateAtLast(MC_DMA_MemCh_Settings* cmd, int tail_len)   __cpp_decls;




#ifdef __cplusplus
}
#endif

#endif //UOS_DMA_MEM_IO_H_ > 0


#endif    //_ELVEES_DMAMEM_

