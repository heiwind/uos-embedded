#ifndef NVBOOT2_HW_H
#define NVBOOT2_HW_H



#define FCPU            (KHZ*1000UL)
#define FXMEM           (MPORT_KHZ*1000UL)

#define T_NS(N, FKHZ)   ((N)*1000000ULL/FKHZ)
#define N_ofTns(Tns, FKHZ) ( (Tns*FKHZ + 999999ULL)/1000000ULL)

#define TCPU_NS(N)      T_NS((N), KHZ)
#define TMPORT_NS(N)    T_NS((N), MPORT_KHZ)
#define NCPU_NS(Tns)    N_ofTns((Tns), KHZ)
#define NMPORT_NS(Tns)  N_ofTns((Tns), MPORT_KHZ)

#define XMEM_WS(TCYR)   ( NMPORT_NS(TCYR)-2)



#ifdef TST4153

//* 1645РУ4Б
//#define XRAM_TCYR_NS         35
//* 1645РУ4А
#define XRAM_TCYR_NS    30
#define XRAM_BANK_ORG   0
#define XRAM_BANK_SIZE  (2<<20)
#define XRAM_TYPE       0
#define XRAM_WS         XMEM_WS(XRAM_TCYR_NS)

//* 1636РР2А/Б
//* время выборки до адреса
#define XROM_TAA_NS      55
#define XROM_TCYR_NS     65
#define XROM_PAGE_SIZE   (64<<10)
#define XROM_BANK_SIZE   (ROM_PAGE_SIZE*32)
#define XROM_WS          XMEM_WS(XROM_TAA_NS)

#define ROM_PAGE_SIZE   XROM_PAGE_SIZE
#define ROM_BANK_SIZE   XROM_BANK_SIZE
#define ROM_WS          XROM_WS

#else

#define SRAM_BANK_SIZE  (64<<20)
#define SRAM_WS         0
#define SRAM_TYPE       MC_CSCON_T

#define ROM_PAGE_SIZE   (256<<10)
#define ROM_BANK_SIZE   (ROM_PAGE_SIZE*256)
#define ROM_WS          9

#endif



#endif //TST4153_HW_H
