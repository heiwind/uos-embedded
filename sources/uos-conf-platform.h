/*
 * uos-conf-platform.h
 *  Created on: 11.12.2015
 *      Author: a_lityagin <alexraynepe196@gmail.com>
 *                         <alexraynepe196@hotbox.ru>
 *                         <alexraynepe196@mail.ru>
 *                         <alexrainpe196@hotbox.ru>
 *
 *  *\~russian UTF8
 *  это дефолтовый конфигурационный файл уОС. здесь сведены настройки модулей
 *  свзаные с платформой/железом.
 *  для сборки своей оси, скопируйте этот файл в папку своего проекта и
 *      переопределите настройки.
 */
#ifndef UOS_CONF_PLATFORM_H
#define UOS_CONF_PLATFORM_H



#define FCPU            (KHZ*1000UL)
#define FXMEM           (MPORT_KHZ*1000UL)

#define T_NS(N, FKHZ)   ((N)*1000000ULL/FKHZ)
#define N_ofTns(Tns, FKHZ) ( (Tns*FKHZ + 999999ULL)/1000000ULL)

#define TCPU_NS(N)      T_NS((N), KHZ)
#define TMPORT_NS(N)    T_NS((N), MPORT_KHZ)
#define NCPU_NS(Tns)    N_ofTns((Tns), KHZ)
#define NMPORT_NS(Tns)  N_ofTns((Tns), MPORT_KHZ)

#define XMEM_WS(TCYR)   ( NMPORT_NS(TCYR)-3)

#ifdef TST4153

#define XRAM_TCYR_NS    35
#define UOS_XRAM_BANK_ORG    0
#define UOS_XSRAM_BANK_SIZE  (1<<20)
#define UOS_XSRAM_TYPE       0
#define UOS_XSRAM_WS         XMEM_WS(XRAM_TCYR_NS)

#define XROM_TCYR_NS     65
#define UOS_XROM_WS          XMEM_WS(XROM_TCYR_NS)

#else
//NVCOM02TEM

//ету настройку важно учесть в линкере - это описание секции sdram
#define UOS_XRAM_BANK_ORG   0
#define UOS_XRAM_BANK_SIZE  (64<<20)
#define UOS_XRAM_WS         0
#define UOS_XRAM_TYPE       MC_CSCON_T
            /*refresh period 64ms*/
#define XRAM_REFRESH_PERIOD_MS 64
#define REFRESH_RATE_NS ((XRAM_REFRESH_PERIOD_MS*1000000ul)/8192)
#define UOS_SDREFRESH_MODE (MC_SDRCON_PS_512 /* Page size 512 */\
    | MC_SDRCON_CL_3 /* CAS latency 3 cycles */\
    | MC_SDRCON_RFR (REFRESH_RATE_NS, MPORT_KHZ)   /* Refresh period */\
    )
#define UOS_SDTIMING_MODE (MC_SDRTMR_TWR(2)      /* Write recovery delay */\
        | MC_SDRTMR_TRP(2)      /* Минимальный период Precharge */\
        | MC_SDRTMR_TRCD(2)     /* Между Active и Read/Write */\
        | MC_SDRTMR_TRAS(5)     /* Между * Active и Precharge */\
        | MC_SDRTMR_TRFC(15)     /* Интервал между Refresh */\
        )

#define UOS_XROM_WS          9


#endif



#endif //UOS_CONF_PLATFORM_H
