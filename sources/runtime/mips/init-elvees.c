/*
 * Startup initialization and exception handlers for MIPS microcontrollers.
 *
 * Copyright (C) 2008-2010 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 *
 * As a special exception to the GPL, permission is granted for additional
 * uses of the text contained in this file.  See the accompanying file
 * "COPY-UOS.txt" for details.
 */
#include <runtime/lib.h>
#include <uos-conf-platform.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void _etext();
extern unsigned __data_start, _edata, _end, _estack[];
extern int main ();

void _init_ (void);
void _irq_handler_ ();
void _exception_handler_ (unsigned int context[]);
void _pagefault_handler_ (unsigned int context[]);

#ifdef __cplusplus
}
#endif

/*
 * Initialize the system configuration, cache, intermal SRAM,
 * and set up the stack. Then call main().
 * _init_ is called from startup.S.
 * Attribute "naked" skips function prologue.
 */
#ifndef DEBUG_UARTBAUD
#define DEBUG_UARTBAUD 115200
#endif

#if defined(ELVEES_INIT_SDRAM) && (UOS_XRAM_BANK_SIZE > 0)

#   ifndef UOS_XRAM_BANK_SIZE
#   define UOS_XRAM_BANK_SIZE  (64UL<<20)
/* Base address */
#   define UOS_XRAM_BANK_ORG   0
/* Sync memory */
#   define UOS_XRAM_TYPE       MC_CSCON_T
#   define UOS_XRAM_WS         0

#       ifndef REFRESH_RATE_NS
#           ifndef XRAM_REFRESH_PERIOD_MS
#           define XRAM_REFRESH_PERIOD_MS 64
#           endif
#       define REFRESH_RATE_NS ((XRAM_REFRESH_PERIOD_MS*1000000ul)/8192)
#       endif

#       ifndef UOS_SDREFRESH_MODE
#       define UOS_SDREFRESH_MODE  (MC_SDRCON_PS_512 /* Page size 512 */\
                | MC_SDRCON_CL_3 /* CAS latency 3 cycles */\
                | MC_SDRCON_RFR (REFRESH_RATE_NS, MPORT_KHZ)   /* Refresh period */\
                )
#       endif

#       ifndef UOS_SDTIMING_MODE
#       define UOS_SDTIMING_MODE (MC_SDRTMR_TWR(2)      /* Write recovery delay */\
            | MC_SDRTMR_TRP(2)      /* Минимальный период Precharge */\
            | MC_SDRTMR_TRCD(2)     /* Между Active и Read/Write */\
            | MC_SDRTMR_TRAS(5)     /* Между * Active и Precharge */\
            | MC_SDRTMR_TRFC(15)     /* Интервал между Refresh */\
            )
#       endif
#    endif //!UOS_XRAM_BANK_SIZE

#define SCON0_MASK (~(((unsigned long)UOS_XRAM_BANK_SIZE)-1))

#if defined(ELVEES_MC24) || defined(ELVEES_MC0226)
inline void _init_sdram(void){
    /* Configure 128 Mbytes of external 64-bit SDRAM memory at nCS0. */
    MC_CSCON0 = MC_CSCON_E |        /* Enable nCS0 */
        MC_CSCON_WS (0) |       /* Wait states  */
        MC_CSCON_T |            /* Sync memory */
        MC_CSCON_W64 |          /* 64-bit data width */
        MC_CSCON_CSBA (0x00000000) |    /* Base address */
        MC_CSCON_CSMASK (0xF8000000);   /* Address mask */

    MC_SDRCON = MC_SDRCON_INIT |                /* Initialize SDRAM */
        MC_SDRCON_BL_PAGE |             /* Bursh full page */
        MC_SDRCON_RFR (64000000/8192, MPORT_KHZ) |  /* Refresh period */
        MC_SDRCON_PS_512;               /* Page size 512 */
    udelay (2);
}
#else //defined(ELVEES_MC24) || defined(ELVEES_MC0226)
inline void _init_sdram(void)
{
    /* Configure 64 Mbytes of external 32-bit SDRAM memory at nCS0. */
    MC_CSCON0 = MC_CSCON_E                /* Enable nCS0 */
              | UOS_XRAM_TYPE
              | MC_CSCON_CSBA (UOS_XRAM_BANK_ORG)
              | MC_CSCON_CSMASK (SCON0_MASK)
              | MC_CSCON_WS (UOS_XRAM_WS)
              ;   /* Address mask */

#   if UOS_XRAM_TYPE == MC_CSCON_T
    MC_SDRCON = UOS_SDREFRESH_MODE;

    MC_SDRTMR = UOS_SDTIMING_MODE;
    MC_SDRCSR = 1;              /* Initialize SDRAM */
    udelay (2);
#   endif
}
#endif //else defined(ELVEES_MC24) || defined(ELVEES_MC0226)

#else //defined(ELVEES_INIT_SDRAM) && (UOS_XRAM_BANK_SIZE > 0)
inline void _init_sdram(void){};
#endif //defined(ELVEES_INIT_SDRAM) && (UOS_XRAM_BANK_SIZE > 0)



#ifndef EXTERNAL_SETUP
inline void _init_pll(void){

#ifdef ELVEES_NVCOM01
    /* Clock: enable only core. */
    MC_CLKEN = MC_CLKEN_CORE | MC_CLKEN_CPU | MC_CLKEN_CORE2;

    /* Clock multiply from CLKIN to KHZ. */
    MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ*2/ELVEES_CLKIN) |
           MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ*2/ELVEES_CLKIN);
#elif defined(ELVEES_NVCOM02)
    /* Clock: enable only core. */
    MC_CLKEN = MC_CLKEN_CORE | MC_CLKEN_CPU | MC_CLKEN_CORE2;

    /* Clock multiply from CLKIN to KHZ. */
    MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ/ELVEES_CLKIN) |
           MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ/ELVEES_CLKIN);
#elif defined(ELVEES_MC24) || defined(ELVEES_MC0226)
    /* Fixed mapping, clock multiply from CLKIN to KHZ. */
    MC_CSR = MC_CSR_CLK(KHZ/ELVEES_CLKIN) | MC_CSR_CLKEN;
#elif defined(ELVEES_MC24R2)
    /* Clock: enable only core. */
    MC_CLKEN = MC_CLKEN_CORE;

    /* Clock multiply from CLKIN to KHZ. */
    MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ/ELVEES_CLKIN) |
           MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ/ELVEES_CLKIN);
#elif defined(ELVEES_MCT02)
    /* Clock: enable only core. */
    MC_CLKEN = MC_CLKEN_CORE;

    /* Clock multiply from CLKIN to KHZ. */
    MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ/ELVEES_CLKIN) |
           MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ/ELVEES_CLKIN);
#elif defined(ELVEES_MCT03P)
    /* Clock: enable only core. */
    MC_CLKEN = MC_CLKEN_CORE | MC_CLKEN_CPU | MC_CLKEN_CORE2;

    /* Clock multiply from CLKIN to KHZ. */
    MC_CRPLL = MC_CRPLL_MPORT | MC_CRPLL_CLKSEL_CORE (KHZ*2/ELVEES_CLKIN) |
           MC_CRPLL_CORE | MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ*2/ELVEES_CLKIN);
#elif defined(ELVEES_MC0428)
    /* Clock: enable only core. */
    MC_CLKEN = MC_CLKEN_CORE;

    /* Clock multiply from CLKIN to KHZ. */
    MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ*2/ELVEES_CLKIN) |
           MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ*2/ELVEES_CLKIN);
#elif defined(ELVEES_MC30SF6)
    /* Clock: enable only core. */
    MC_CLKEN = MC_CLKEN_CORE | MC_CLKEN_CPU | MC_CLKEN_CORE2;

    /* Clock multiply from CLKIN to KHZ. */
    MC_CRPLL = MC_CRPLL_MPORT | MC_CRPLL_CLKSEL_CORE (KHZ*2/ELVEES_CLKIN) |
           MC_CRPLL_CORE | MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ*2/ELVEES_CLKIN);
#endif
}
#else //!EXTERNAL_SETUP
inline void _init_pll(void){};
#endif //EXTERNAL_SETUP



//#define UOS_START_MODE_BINARY   0
//#define UOS_START_MODE_LOADED   1
#if UOS_START_MODE == 1
#define DONT_COPY_DATA_SEGS
#endif

void __attribute ((noreturn))
#ifdef ELVEES_INIT_SDRAM
//!!! этот код должен лежать в памяти доступной по вектору сброса=прерываня, ибо положить в СДРАМ до ее настрйки не представляется нормальным
CODE_ISR
#endif
_init_ (void)
{
	unsigned *dest, *limit;
	unsigned int divisor;

	_init_sdram();

	/* Clear CAUSE register. Use special irq vector. */
	mips_write_c0_register (C0_CAUSE, CA_IV);

	/* Initialize STATUS register: CP0 usable, ROM vectors used,
	 * internal interrupts enabled, master interrupt disable. */
	mips_write_c0_register (C0_STATUS, ST_CU0
#ifndef MIPS_NOBEV
		| ST_BEV
#endif
#ifdef ARCH_HAVE_FPU
		| ST_CU1
#endif
#ifdef ELVEES_MC24
		| ST_IM_MCU
#endif
#ifdef ELVEES_MC0226
		| ST_IM_MCU
#endif
#ifdef ELVEES_MC24R2
		| ST_IM_QSTR0 | ST_IM_QSTR1 | ST_IM_QSTR2 | ST_IM_QSTR3
#endif
#ifdef ELVEES_NVCOM01
		| ST_IM_QSTR0 | ST_IM_QSTR1 | ST_IM_QSTR2
#endif
#ifdef ELVEES_NVCOM02
		/* TODO */
		| ST_IM_QSTR0 | ST_IM_QSTR1 | ST_IM_QSTR2
#endif
#ifdef ELVEES_MCT02
		| ST_IM_QSTR0 | ST_IM_QSTR1 | ST_IM_QSTR2 | ST_IM_QSTR3 | ST_IM_QSTR4
#endif
#ifdef ELVEES_MCT03P
		| ST_IM_QSTR0 | ST_IM_QSTR1 | ST_IM_QSTR2 | ST_IM_QSTR3 | ST_IM_QSTR4
#endif
#ifdef ELVEES_MC0428
		| ST_IM_QSTR0 | ST_IM_QSTR1 | ST_IM_QSTR2 | ST_IM_QSTR3
#endif
#ifdef ELVEES_MC30SF6
		| ST_IM_QSTR0 | ST_IM_QSTR1 | ST_IM_QSTR2 | ST_IM_QSTR3
#endif
		);

    /*
     * Setup all essential system registers.
     */
	{
	unsigned long csr = MC_CSR;

#if defined (ENABLE_ICACHE) || defined (ENABLE_DCACHE)
	/* Enable cache for kseg0 segment. */
	mips_write_c0_register (C0_CONFIG, 3);
#   if defined (ENABLE_ICACHE)
	csr |= MC_CSR_FLUSH_I;
#   endif
#   if defined (ENABLE_DCACHE)
	csr |= MC_CSR_FLUSH_D;
#   endif
#else
	/* Disable cache for kseg0 segment. */
	mips_write_c0_register (C0_CONFIG, 2);
#   ifndef EXTERNAL_SETUP
#       ifndef ENABLE_ICACHE
	    csr &= ~MC_CSR_FLUSH_I;
#       endif
#       ifndef ENABLE_DCACHE
	    csr &= ~MC_CSR_FLUSH_D;
#       endif
#   endif
#endif
	MC_CSR = csr;
	}

#ifdef ENABLE_ICACHE
	/* Jump to cached kseg0 segment. */
	asm volatile (
		"la	$k0, 1f \n"
		"jr	$k0 \n"
	"1:");
#endif

#ifdef ARCH_HAVE_FPU
	/* Clear all FPU registers. */
	mips_write_fpu_control (C1_FCSR, 0);
	mips_write_fpu_register (0, 0);
	mips_write_fpu_register (1, 0);
	mips_write_fpu_register (2, 0);
	mips_write_fpu_register (3, 0);
	mips_write_fpu_register (4, 0);
	mips_write_fpu_register (5, 0);
	mips_write_fpu_register (6, 0);
	mips_write_fpu_register (7, 0);
	mips_write_fpu_register (8, 0);
	mips_write_fpu_register (9, 0);
	mips_write_fpu_register (10, 0);
	mips_write_fpu_register (11, 0);
	mips_write_fpu_register (12, 0);
	mips_write_fpu_register (13, 0);
	mips_write_fpu_register (14, 0);
	mips_write_fpu_register (15, 0);
	mips_write_fpu_register (16, 0);
	mips_write_fpu_register (17, 0);
	mips_write_fpu_register (18, 0);
	mips_write_fpu_register (19, 0);
	mips_write_fpu_register (20, 0);
	mips_write_fpu_register (21, 0);
	mips_write_fpu_register (22, 0);
	mips_write_fpu_register (23, 0);
	mips_write_fpu_register (24, 0);
	mips_write_fpu_register (25, 0);
	mips_write_fpu_register (26, 0);
	mips_write_fpu_register (27, 0);
	mips_write_fpu_register (28, 0);
	mips_write_fpu_register (29, 0);
	mips_write_fpu_register (30, 0);
	mips_write_fpu_register (31, 0);
#endif

	_init_pll();

	/*
     * Setup all essential system registers.
     */

#ifdef ELVEES_NVCOM01
	{
#   ifndef EXTERNAL_SETUP
	unsigned long csr;
    /* Fixed mapping. */
    csr = MC_CSR_FM;
#   else
    /* Fixed mapping. */
    unsigned long csr = MC_CSR;
    csr |= MC_CSR_FM;
#   endif

#   ifdef ELVEES_VECT_CRAM
    csr |= MC_CSR_TR;
#   elif !defined(EXTERNAL_SETUP)
    csr &= ~MC_CSR_TR;
#   endif
    MC_CSR = csr;
	}

    MC_MASKR0 = 0;
    MC_MASKR1 = 0;
    MC_MASKR2 = 0;
#endif //ELVEES_NVCOM01

#ifdef ELVEES_NVCOM02
	/* Fixed mapping. */
	MC_CSR |= MC_CSR_FM;

#ifdef ELVEES_VECT_CRAM
	MC_CSR |= MC_CSR_TR;
#endif

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
#endif

#if defined(ELVEES_MC24) || defined(ELVEES_MC0226)
	/* Fixed mapping, clock multiply from CLKIN to KHZ. */
	MC_CSR |= MC_CSR_FM;
	MC_MASKR = 0;
#endif

#ifdef ELVEES_MC24R2
	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
	MC_MASKR3 = 0;
#endif

#ifdef ELVEES_MCT02
	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
	MC_MASKR3 = 0;
	MC_MASKR4 = 0;
#endif

#ifdef ELVEES_MCT03P
	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

#ifdef ELVEES_VECT_CRAM
	MC_CSR |= MC_CSR_TR;
#endif

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
	MC_MASKR3 = 0;
	MC_MASKR4 = 0;
#endif

#ifdef ELVEES_MC0428
	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

#ifdef ELVEES_VECT_CRAM
	MC_CSR |= MC_CSR_TR;
#endif

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
	MC_MASKR3 = 0;
#endif

#ifdef ELVEES_MC30SF6
	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

#ifdef ELVEES_VECT_CRAM
	MC_CSR |= MC_CSR_TR;
#endif

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
	MC_MASKR3 = 0;
#endif

	MC_ITCSR = 0;
#ifdef MC_ITCSR1
	MC_ITCSR1 = 0;
#endif
#ifdef MC_RTCSR
	MC_RTCSR = 0;
#endif
	MC_WTCSR = 0;
#ifdef MC_CSR_LPCH
	MC_CSR_LPCH(0) = 0;
	MC_CSR_LPCH(1) = 0;
	MC_CSR_LPCH(2) = 0;
	MC_CSR_LPCH(3) = 0;
#endif
#ifdef MC_CSR_MEMCH
	MC_CSR_MEMCH(0) = 0;
	MC_CSR_MEMCH(1) = 0;
	MC_CSR_MEMCH(2) = 0;
	MC_CSR_MEMCH(3) = 0;
#endif
#ifdef MC_HAVE_SPORT
	MC_STCTL(0) = 0;
	MC_STCTL(1) = 0;
	MC_SRCTL(0) = 0;
	MC_SRCTL(1) = 0;
#endif
#ifdef MC_HAVE_LPORT
	MC_LCSR(0) = 0;
	MC_LCSR(1) = 0;
	MC_LCSR(2) = 0;
	MC_LCSR(3) = 0;
	MC_LDIR(0) = 0;
	MC_LDIR(1) = 0;
	MC_LDIR(2) = 0;
	MC_LDIR(3) = 0;
#endif

#if 0
	/* Disable all external memory except nCS3.
	 * Set to default values. */
	MC_CSCON0 = MC_CSCON_WS (15);
	MC_CSCON1 = MC_CSCON_WS (15);
	MC_CSCON2 = MC_CSCON_WS (15);
#ifdef BOOT_FLASH_8BIT
	MC_CSCON3 = MC_CSCON_WS (15) | MC_CSCON3_BYTE;
#else
	MC_CSCON3 = MC_CSCON_WS (15);
#endif
	MC_CSCON4 = MC_CSCON_WS (15);
	MC_SDRCON = 0;
#endif
	
	/*
	 * Setup UART registers.
	 * Compute the divisor for 115.2 kbaud.
	 */
	divisor = MC_DL_BAUD (KHZ * 1000, DEBUG_UARTBAUD);

	MC_LCR = MC_LCR_8BITS | MC_LCR_DLAB;
	MC_DLM = divisor >> 8;
	MC_DLL = divisor;
	MC_LCR = MC_LCR_8BITS;
	MC_SCLR = 0;
	MC_SPR = 0;
	MC_IER = 0;
	MC_MSR = 0;
	MC_MCR = MC_MCR_DTR | MC_MCR_RTS | MC_MCR_OUT2;
	MC_FCR = MC_FCR_RCV_RST | MC_FCR_XMT_RST | MC_FCR_ENABLE;

	/* Clear pending status, data and irq. */
	(void) MC_LSR;
	(void) MC_MSR;
	(void) MC_RBR;
	(void) MC_IIR;

#ifndef DONT_COPY_DATA_SEGS
	unsigned *src;
	/* Copy the .data image from flash to ram.
	 * Linker places it at the end of .text segment. */
	src = (unsigned*) &_etext;
	dest = &__data_start;
	limit = &_edata;
	while (dest < limit)
		*dest++ = *src++;
#endif

	/* Initialize .bss segment by zeroes. */
	dest = &_edata;
	limit = &_end;
	while (dest < limit)
		*dest++ = 0;
		
	for (;;)
		main ();
}

bool_t __attribute__((weak))
uos_valid_memory_address (void *ptr)
{
	unsigned address = (unsigned) ptr;

	/* Internal SRAM. */
	if (address >= (unsigned) &__data_start &&
	    address < (unsigned) _estack)
		return 1;

#if defined (PIC32MX)
	/* Boot flash. */
	if (address >= 0xbfc00000 && address < 0xbfc03000)
		return 1;

	/* Program flash. */
	if (address >= 0x9d000000 && address < 0x9d080000)
		return 1;
#endif /* PIC32MX */

#if defined (ELVEES)
#ifdef BOOT_SRAM_SIZE
	/* Boot SRAM. */
	if (address >= 0xbfc00000 && address < 0xbfc00000+BOOT_SRAM_SIZE)
		return 1;
#endif /* BOOT_SRAM_SIZE */
#endif /* ELVEES */
	return 0;
}

void __attribute__ ((weak))
CODE_ISR
_irq_handler_ ()
{
	/* This is needed when no kernel is present. */
}

#if defined (ELVEES)
DEBUG_NORETURN
static void dump_of_death (unsigned int context[])
{
	debug_printf ("                t0 = %8x   s0 = %8x   t8 = %8x   lo = %8x\n",
		context [CONTEXT_R8], context [CONTEXT_R16],
		context [CONTEXT_R24], context [CONTEXT_LO]);
	debug_printf ("at = %8x   t1 = %8x   s1 = %8x   t9 = %8x   hi = %8x\n",
		context [CONTEXT_R1], context [CONTEXT_R9], context [CONTEXT_R17],
		context [CONTEXT_R25], context [CONTEXT_HI]);
	debug_printf ("v0 = %8x   t2 = %8x   s2 = %8x               status = %8x\n",
		context [CONTEXT_R2], context [CONTEXT_R10],
		context [CONTEXT_R18], context [CONTEXT_STATUS]);
	debug_printf ("v1 = %8x   t3 = %8x   s3 = %8x                  epc = %8x\n",
		context [CONTEXT_R3], context [CONTEXT_R11],
		context [CONTEXT_R19], context [CONTEXT_PC]);
	debug_printf ("a0 = %8x   t4 = %8x   s4 = %8x   gp = %8x\n",
		context [CONTEXT_R4], context [CONTEXT_R12],
		context [CONTEXT_R20], context [CONTEXT_GP]);
	debug_printf ("a1 = %8x   t5 = %8x   s5 = %8x   sp = %8x\n",
		context [CONTEXT_R5], context [CONTEXT_R13],
		context [CONTEXT_R21], context + CONTEXT_WORDS);
	debug_printf ("a2 = %8x   t6 = %8x   s6 = %8x   fp = %8x\n",
		context [CONTEXT_R6], context [CONTEXT_R14],
		context [CONTEXT_R22], context [CONTEXT_FP]);
	debug_printf ("a3 = %8x   t7 = %8x   s7 = %8x   ra = %8x\n",
		context [CONTEXT_R7], context [CONTEXT_R15],
		context [CONTEXT_R23], context [CONTEXT_RA]);

	debug_printf ("\nHalt...\n\n");
	//asm volatile ("1: j 1b; nop");
	uos_halt(1);
    while(1);
}

DEBUG_NORETURN
void _exception_handler_ (unsigned int context[])
{
	unsigned int cause, badvaddr, config;
	unsigned int errEPC;
	const char *code = 0;

	badvaddr = mips_read_c0_register (C0_BADVADDR);
    config = mips_read_c0_register (C0_CONFIG);
    cause = mips_read_c0_register (C0_CAUSE);
    errEPC  = mips_read_c0_register (C0_ERROREPC);

	debug_printf ("\n\n*** 0x%08x: exception ", context [CONTEXT_PC]);

	switch (cause >> 2 & 31) {
	case 0:	code = "Interrupt"; break;
	case 1: code = "TLB Modification"; break;
	case 2: code = "TLB Load"; break;
	case 3: code = "TLB Save"; break;
	case 4: code = "Address Load"; break;
	case 5: code = "Address Save"; break;
	case 8: code = "System"; break;
	case 9: code = "BBreakpoint"; break;
	case 10: code = "Reserved Instruction"; break;
	case 11: code = "Coprocessor Unavailable"; break;
	case 12: code = "Integer Overflow"; break;
	case 13: code = "Trap"; break;
	case 15: code = "FPU"; break;
	case 24: code = "MCheck"; break;
	}
	if (code)
		debug_printf ("'%s'\n", code);
	else
		debug_printf ("%d\n", cause >> 2 & 31);

	debug_printf ("*** cause=0x%08x, badvaddr=0x%08x, config=0x%08x, errEPC=%p\n",
		cause, badvaddr, config, errEPC);

	dump_of_death (context);
}

DEBUG_NORETURN
void _pagefault_handler_ (unsigned int context[])
{
	unsigned int cause, badvaddr, config;

	debug_printf ("\n\n*** 0x%08x: page fault\n", context [CONTEXT_PC]);

	cause = mips_read_c0_register (C0_CAUSE);
	badvaddr = mips_read_c0_register (C0_BADVADDR);
	config = mips_read_c0_register (C0_CONFIG);
	debug_printf ("*** cause=0x%08x, badvaddr=0x%08x, config=0x%08x\n",
		cause, badvaddr, config);

	dump_of_death (context);
}
#endif /* ELVEES */
