/*******************************************************************************
 * elvees nvcom dma_mem_ch_regs.h
 * ru - utf8
 *
 *  Created on: 5.10.2016
 *      Author: alexraynepe196@gmail.com
 *
 *******************************************************************************
 *  дополнение и исправление к базовой CSL Multicore
 *	Содержит следующие макросы для доступа к регистрам DMA_MEM (n=0...3):
 * DMA_MEM_CHn
 * DMA_MEM_CH (n)
 *
 * Пример использования:
 * 	tmp = DMA_MEM_CH0.CSR.data;		//Чтение регистра CSR канала 0 как 32 битное слово
 * 	tmp = DMA_MEM_CH0.CSR.bits.WN;	//Чтение значения поля WN регистра CSR канала 0
 * 	DMA_MEM_CH0.Y.bits.OY = tmp;		//Запись значения в поле OY регистра Y канала 0
 ******************************************************************************/
#ifndef UOS_DMA_MEM_IO_H_
#define UOS_DMA_MEM_IO_H_ 1

#include <runtime/arch.h>
#include <stdint.h>
#include <runtime/sys/uosc.h>

#ifdef ELVEES
#ifdef ELVEES_NVCOM02T
//#include <multicore/nvcom02t.h>
#include <multicore/nvcom02t/nvcom02_mem_map.h>
#include <multicore/nvcom02t/nvcom02_dma_mem_ch_regs.h>
#elif defined(ELVEES_NVCOM02)
#include <multicore/nvcom02/nvcom02_dma_mem_ch_regs.h>
#else
#undef  UOS_DMA_MEM_IO_H_
#define UOS_DMA_MEM_IO_H_ 0
#endif
#else
#error "uncknown processor! ELVEES processors need."
#endif


#if UOS_DMA_MEM_IO_H_ > 0

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// 	Описание регистров DMA_MEM
//==============================================================================

//==================================================
//	CSR
//==================================================

#define MC_DMA_MEM_CH_CSR_DIR_MASK    1
#define MC_DMA_MEM_CH_CSR_DIR_SHIFT   1
#define MC_DMA_MEM_CH_CSR_WN_MASK     0xf
#define MC_DMA_MEM_CH_CSR_WN_SHIFT    2
#define MC_DMA_MEM_CH_CSR_EN64_MASK   1
#define MC_DMA_MEM_CH_CSR_EN64_SHIFT  6
#define MC_DMA_MEM_CH_CSR_DSPGO_MASK  1
#define MC_DMA_MEM_CH_CSR_DSPGO_SHIFT 7
#define MC_DMA_MEM_CH_CSR_MODE_MASK   1
#define MC_DMA_MEM_CH_CSR_MODE_SHIFT  8
#define MC_DMA_MEM_CH_CSR_IR12D_MASK  1
#define MC_DMA_MEM_CH_CSR_IR12D_SHIFT 9
#define MC_DMA_MEM_CH_CSR_DMAR_MASK     1
#define MC_DMA_MEM_CH_CSR_DMAR_SHIFT    10
#define MC_DMA_MEM_CH_CSR_FLYBY_MASK    1
#define MC_DMA_MEM_CH_CSR_FLYBY_SHIFT   11
#define MC_DMA_MEM_CH_CSR_CHEN_MASK     1
#define MC_DMA_MEM_CH_CSR_CHEN_SHIFT    12
#define MC_DMA_MEM_CH_CSR_IM_MASK       1
#define MC_DMA_MEM_CH_CSR_IM_SHIFT      13
#define MC_DMA_MEM_CH_CSR_END_MASK      1
#define MC_DMA_MEM_CH_CSR_END_SHIFT     14
#define MC_DMA_MEM_CH_CSR_DONE_MASK     1
#define MC_DMA_MEM_CH_CSR_DONE_SHIFT    15
#define MC_DMA_MEM_CH_CSR_WCX_MASK    0xffff
#define MC_DMA_MEM_CH_CSR_WCX_SHIFT   16

enum {
      MC_DMA_MEM_CH_CSR_RUN_STOP    = 0
    , MC_DMA_MEM_CH_CSR_RUN_GO      = 1
    , MC_DMA_MEM_CH_CSR_DIR_IR01    = 0
    , MC_DMA_MEM_CH_CSR_DIR_IR10    = 1
    , MC_DMA_MEM_CH_CSR_WIDTH_32    = 0
    , MC_DMA_MEM_CH_CSR_WIDTH_64    = 1
    , MC_DMA_MEM_CH_CSR_MODE_PLAINADR    = 0
    , MC_DMA_MEM_CH_CSR_MODE_REVERSEADR  = 1
    , MC_DMA_MEM_CH_CSR_IR12D_1D    = 0
    , MC_DMA_MEM_CH_CSR_IR12D_2D    = 1
    , MC_DMA_MEM_CH_CSR_DMAR_DISABLE  = 0
    , MC_DMA_MEM_CH_CSR_DMAR_ENABLE   = 1
    , MC_DMA_MEM_CH_CSR_FLYBY_DISABLE  = 0
    , MC_DMA_MEM_CH_CSR_FLYBY_ENABLE   = 1
    , MC_DMA_MEM_CH_CSR_CHEN_DISABLE  = 0
    , MC_DMA_MEM_CH_CSR_CHEN_ENABLE   = 1
    , MC_DMA_MEM_CH_CSR_IM_DISABLE  = 0
    , MC_DMA_MEM_CH_CSR_IM_ENABLE   = 1
    , MC_DMA_MEM_CH_CSR_END_CLEAR   = 0
    , MC_DMA_MEM_CH_CSR_END_SET     = 1
    , MC_DMA_MEM_CH_CSR_DONE_CLEAR  = 0
    , MC_DMA_MEM_CH_CSR_DONE_SET    = 1
};

enum {
      MC_DMA_MEM_CH_CSR_STOP     = 0
    , MC_DMA_MEM_CH_CSR_GO       = 1
    , MC_DMA_MEM_CH_CSR_IR01DIR  = 0
    , MC_DMA_MEM_CH_CSR_IR10DIR  = (1 << MC_DMA_MEM_CH_CSR_DIR_SHIFT)
    , MC_DMA_MEM_CH_CSR_32BIT    = 0
    , MC_DMA_MEM_CH_CSR_64BIT    = (1 << MC_DMA_MEM_CH_CSR_EN64_SHIFT)
    , MC_DMA_MEM_CH_CSR_PLAINADR    = 0
    , MC_DMA_MEM_CH_CSR_REVERSEADR  = (1<<MC_DMA_MEM_CH_CSR_MODE_SHIFT)
    , MC_DMA_MEM_CH_CSR_IR1_1D      = 0
    , MC_DMA_MEM_CH_CSR_IR1_2D      = (1 << MC_DMA_MEM_CH_CSR_IR12D_SHIFT)
    , MC_DMA_MEM_CH_CSR_CHAIN       = (1 << MC_DMA_MEM_CH_CSR_CHEN_SHIFT)
    , MC_DMA_MEM_CH_CSR_NOCHAIN     = 0
    , MC_DMA_MEM_CH_CSR_IRQ         = (1 << MC_DMA_MEM_CH_CSR_IM_SHIFT)
    , MC_DMA_MEM_CH_CSR_NOIRQ       = 0
};

//==================================================
//	OR
//==================================================
struct __attribute__((packed,aligned(4)))
MC_DMA_MEM_CH_OR_BITS { 	// bits  description
   int16_t  OR0	:16;  // Смещение (приращение) адреса для индексного регистра IR0
   int16_t  OR1	:16;  // Смещение (приращение) адреса для индексного регистра IR1
};
union MC_DMA_MEM_CH_OR_REG {
   uint32_t                     data;
   struct MC_DMA_MEM_CH_OR_BITS bits;
};
//==================================================
//	Y
//==================================================
struct __attribute__((packed,aligned(4)))
MC_DMA_MEM_CH_Y_BITS { 		// bits  description
   int16_t      OY  :16;    // Смещение (приращение) адреса памяти в 32-разрядных словах по направлению Y
   uint16_t     WCY :16;    // Число строк по Y направлению
};

union MC_DMA_MEM_CH_Y_REG {
   uint32_t                     data;
   struct MC_DMA_MEM_CH_Y_BITS  bits;
};
//==================================================
// DMA_MEM структура
//==================================================
struct _MC_DMA_MEM_CH_REGS_TYPE {
   union DMA_MEM_CH_CSR_REG			CSR;	// Регистр управления и состояния
   union DATA_REG_COMMON_UNION		CP;		// Регистр указателя цепочки
   union DATA_REG_COMMON_UNION		IR0;	// Регистр индекса "0"
   union DATA_REG_COMMON_UNION		IR1;	// Регистр индекса "1"
   union MC_DMA_MEM_CH_OR_REG       OR;		// Регистр смещений
   union MC_DMA_MEM_CH_Y_REG        Y;		// Регистр параметров направления Y при двухмерной адресации
   union DMA_MEM_CH_CSR_REG			RUN;	// На запись:Псевдорегистр управления состоянием бита RUN регистра CSR0
   											// На чтение: Регистр управления и состояния без сброса битов “END” и ”DONE”
};
typedef volatile struct _MC_DMA_MEM_CH_REGS_TYPE MC_DMA_MEM_CH_REGS_TYPE;
//==============================================================================

#define MC_DMA_MEM_CH(n)    (*(MC_DMA_MEM_CH_REGS_TYPE*)(&DMA_MEM_CH(n)))

// структура - блок параметров DMA
/*******************************
 63____________________________0
 {IR132,           IR032       };
 {{WCY16,ORY16},{ OR116,OR016 }};
 { CSR32,           CP32       }.
 ********************************/
typedef uint32_t dma_phyadr;
typedef struct __attribute__((packed,aligned(8)))
_MC_DMA_MemCh_Settings {
    // начальный адрес массива приемника
    uint32_t                ir0;
    // начальный адрес массива источника
    uint32_t                ir1;
    //смещение
    union DMA_MEM_CH_OR_REG x;
    // двумерная адресация, ее нет в данном примере, Y=0)
    union DMA_MEM_CH_Y_REG  y;
    // адрес _следующего_ блока параметров
    // struct _DMA_MemCh_Settings*
    dma_phyadr              chain; //Uint32
    // настройки канала (регистр CSR)
    union DMA_MEM_CH_CSR_REG csr;
} MC_DMA_MemCh_Settings;




INLINE
dma_phyadr dma_mem_run_cmd(MC_DMA_MEM_CH_REGS_TYPE* io, MC_DMA_MemCh_Settings* cmd){
    dma_phyadr phy_addr = mips_virtual_addr_to_physical((size_t)cmd);
    io->CP.data = phy_addr | MC_DMA_MEM_CH_CSR_RUN_GO;
    return phy_addr;
}

#ifdef __cplusplus
}
#endif

#endif //UOS_DMA_MEM_IO_H_ > 0

#endif /* UOS_NVCOM_DMA_MEM_CH_REGS_H_ */
