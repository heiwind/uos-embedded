#include <runtime/lib.h>
//#include <runtime/string.h>
#include <stdint.h>
#include <hwcfg.h>

#ifndef NVBOOT_SILENT_MODE
#define boot_puts(x) debug_puts(x)
#define boot_printf(...) debug_printf(__VA_ARGS__)
#else
#define    boot_puts(x)
#define    boot_printf(...)
#endif

extern unsigned char _etext[];
extern unsigned __data_start, _edata;
extern unsigned char _stext[];

static const unsigned flash_org   = 0xBC000000u;
static const unsigned starter_org = 0xBFC00000u;

#define MAGIC(a,b,c,d) ((((unsigned long)(d))<<24)\
                        |(((unsigned long)(c))<<16) \
                        |(((unsigned long)(b))<<8) \
                        | ((unsigned long)(a)) )
#define MAGIC_ELF   MAGIC( 0x7f, 'E', 'L', 'F')
#define MAGIC_IMGC  MAGIC( 'I', 'M', 'G', 'C')
#define MAGIC_IMGR  MAGIC( 'I', 'M', 'G', 'R')
#define MAGIC_IMGr  MAGIC( 'I', 'M', 'G', 'r')

static const unsigned char image_mark[] = "IMAG";

void* seek_img(void* from);
void* load_elf(void*from);
void* load_bin(void*from, void* target);

__attribute__((noreturn))
int main (void)
{
	boot_puts("\n\nNVBOOT2\n");
	/* Configure 16 Mbyte of external Flash memory at nCS3. */
	MC_CSCON3 = MC_CSCON_WS (9);		/* Wait states  */

	unsigned char *p = _etext;  /* Flash image pointer */
	if ((unsigned)_stext != starter_org){
	        p = (unsigned char *)(starter_org) + (_etext - _stext);
	}
	p += (&_edata - &__data_start) << 2;
	
	p = seek_img(p);

	if (p == 0){
	    boot_printf ("Image not found!\n");
	    while(1);
	}

	uint32_t mark = p[0] | (p[1] << 8) | (p[2]<<16) | (p[3]<<24);

    void* target = 0;
    switch (mark){
        case MAGIC_IMGC: target = (void*)0x98000000;break;
        case MAGIC_IMGR: target = (void*)0x80000000;break; //cached xram
        case MAGIC_IMGr: target = (void*)0xa0000000;break; //nocached xram
        case MAGIC_ELF : target = (void*)0; break;
        default:
            p = 0;
    }

    if (p == 0){
            boot_printf ("Image target uncknown\n");
            //debug_printf ("%08x %08x %08x %08x\n", *(p+0), *(p+1), *(p+2), *(p+3));
            //debug_printf ("%08x %08x %08x %08x\n", *(p+4), *(p+5), *(p+6), *(p+7));
            while(1);
    }
    void* go = 0;
    if (target == 0)
        go = load_elf(p);
    else
        go = load_bin(p+4, target);

    if (go == 0){
            boot_printf ("Image failed to load\n");
            //debug_printf ("%08x %08x %08x %08x\n", *(p+0), *(p+1), *(p+2), *(p+3));
            //debug_printf ("%08x %08x %08x %08x\n", *(p+4), *(p+5), *(p+6), *(p+7));
            while(1);
    }
	
    boot_printf ("Image start to $%x\n", (unsigned)go);
	typedef void (*uos_entry) (void) __attribute__((noreturn));
	uos_entry entry = (uos_entry) (go);
#ifdef MIPS32
    unsigned status = mips_read_c0_register (C0_STATUS);
    //*мипс образ запущу с битом Reseta чтобы он имел возможность отличить старт от
    // исключения TLBrefill
    mips_write_c0_register(C0_STATUS, status | ST_ERL );
#endif
	entry();	

	while(1);
}

bool_t check_mark(uint32_t x){
    if (( ( x ^ MAGIC_IMGC ) & 0xffffff) == 0)
        return 1;
    if (x == MAGIC_ELF)
        return 1;
    return 0;
}

void* seek_img(void* from){
    unsigned char* p = (unsigned char*)from;
    boot_printf("Seek IMAG at $%x .. \n", (unsigned)from);
    uint32_t mark = p[0] | (p[1] << 8) | (p[2]<<16) | (p[3]<<24);
    if ( ( ( mark ^ MAGIC_IMGC ) & 0xffffff) != 0){
        //seek image markers at sector boundary
        //offset of start vector point (starter image) in flash
        const unsigned start_ofs = starter_org & (ROM_BANK_SIZE-1);

        if (start_ofs != 0) {
            //scan flash under starter
            uint32_t*   start = (uint32_t*)flash_org;
            uint32_t*   stop  = (uint32_t*)(flash_org | start_ofs);
            uint32_t*   m;
            for (m = start; m < stop; m+=(ROM_PAGE_SIZE/4)){
                mark = *m;
                if (check_mark(mark))
                    return m;
            }
        }
        {
            unsigned from_org = (unsigned)from & (ROM_BANK_SIZE-1);
                     from_org = (from_org + (ROM_PAGE_SIZE-1)) & ~(ROM_PAGE_SIZE-1);
            uint32_t*   start = (uint32_t*) (flash_org | from_org );
            uint32_t*   stop  = (uint32_t*) (flash_org | ROM_BANK_SIZE);
            uint32_t*   m;
            for (m = start; m < stop; m+=(ROM_PAGE_SIZE/4)){
                mark = *m;
                if (check_mark(mark))
                    return m;
            }
            p = (void*)m;
        }
        boot_printf (" finished at $%x\n", (unsigned)p);
        p = 0;
        mark = 0;
    }
    else {
        boot_printf("have  mark $%x in solid image\n", mark);
    }
    return p;
}

#include <elf.h>
uint32_t* memwset(uint32_t * target, uint32_t x, unsigned size){
    uint32_t* p;
    int len = size;
    for(p = target; len > 0; len-=4)
        *p++ = x;
    return target;
}

uint32_t* memwcpy(uint32_t * target, const uint32_t* from, unsigned size){
    uint32_t* p;
    int len = size;
    for(p = target; len > 0; len-=4)
        *p++ = *from++;
    return target;
}

elf_process_handle elf_job = {
        0           //void*      object;
        , memwset    //elf_filler onfill;
        , memwcpy    //elf_copyer oncopy;
};

void* load_elf(void*from) {
    elf_job.object = from;
    boot_printf ("ELF Image at address %08x\n", (unsigned)from);
    return elf_process(&elf_job);
}

void* load_bin(void*from, void* target)
{

    unsigned char*p = (unsigned char*)from;
    /* Extracting size of application image which is written to img.bin as string */
    unsigned size = strtoul(p, &p, 10);
    //skip stop symbol - \n or \0
    p += 1;
    boot_printf ("Image at address %08x, size %d\n", p, size);
    //debug_printf ("%08x %08x %08x %08x\n", *(p+0), *(p+1), *(p+2), *(p+3));
    //debug_printf ("%08x %08x %08x %08x\n", *(p+4), *(p+5), *(p+6), *(p+7));

    if (size == 0)
        return 0;

    memcpy (target, p, size);
#ifndef NVBOOT_SILENT_MODE
    boot_printf ("Done copying, checking... ");
    if (memcmp (target, p, size) == 0)
        boot_printf ("OK\n");
    else
        debug_printf ("FAIL\n");

    //p = (unsigned *) DEST_ADDRESS;
    //debug_printf ("%08x %08x %08x %08x\n", *(p+0), *(p+1), *(p+2), *(p+3));
    //debug_printf ("%08x %08x %08x %08x\n", *(p+4), *(p+5), *(p+6), *(p+7));
#endif
    return target;
}

#if 0
extern void _etext();
extern unsigned __data_start, _edata, _end, _estack[];
extern int main ();

/*
 * Initialize the system configuration, cache, intermal SRAM,
 * and set up the stack. Then call main().
 * _init_ is called from startup.S.
 * Attribute "naked" skips function prologue.
 */
void __attribute ((noreturn))_init_ (void)
{
	unsigned *dest, *limit;
	
#ifdef ELVEES
	unsigned int divisor;

	/* Clear CAUSE register. Use special irq vector. */
	mips_write_c0_register (C0_CAUSE, CA_IV);

	/* Initialize STATUS register: CP0 usable, ROM vectors used,
	 * internal interrupts enabled, master interrupt disable. */
	mips_write_c0_register (C0_STATUS, ST_BEV | ST_CU0
#ifdef ARCH_HAVE_FPU
		| ST_CU1
#endif
#ifdef ELVEES_MC24
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
		);

#if defined (ENABLE_ICACHE) || defined (ENABLE_DCACHE)
	/* Enable cache for kseg0 segment. */
	mips_write_c0_register (C0_CONFIG, 3);
	MC_CSR |= MC_CSR_FLUSH_I | MC_CSR_FLUSH_D;
#else
	/* Disable cache for kseg0 segment. */
	mips_write_c0_register (C0_CONFIG, 2);
#endif
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

#ifndef EXTERNAL_SETUP
	/*
	 * Setup all essential system registers.
	 */
#ifdef ELVEES_NVCOM01
	/* Clock: enable only core. */
	MC_CLKEN = MC_CLKEN_CORE | MC_CLKEN_CPU | MC_CLKEN_CORE2;

	/* Clock multiply from CLKIN to KHZ. */
	MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ*2/ELVEES_CLKIN) |
		   MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ*2/ELVEES_CLKIN);

	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
#endif

#ifdef ELVEES_NVCOM02
	/* Clock: enable only core. */
	MC_CLKEN = MC_CLKEN_CORE | MC_CLKEN_CPU | MC_CLKEN_CORE2;

	/* Clock multiply from CLKIN to KHZ. */
	MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ/ELVEES_CLKIN) |
		   MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ/ELVEES_CLKIN);

	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
#endif

#ifdef ELVEES_MC24
	/* Fixed mapping, clock multiply from CLKIN to KHZ. */
	MC_CSR = MC_CSR_FM | MC_CSR_CLK(KHZ/ELVEES_CLKIN) | MC_CSR_CLKEN;
	MC_MASKR = 0;
#endif

#ifdef ELVEES_MC24R2
	/* Clock: enable only core. */
	MC_CLKEN = MC_CLKEN_CORE;

	/* Clock multiply from CLKIN to KHZ. */
	MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ/ELVEES_CLKIN) |
		   MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ/ELVEES_CLKIN);

	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
	MC_MASKR3 = 0;
#endif

#ifdef ELVEES_MCT02
	/* Clock: enable only core. */
	MC_CLKEN = MC_CLKEN_CORE;

	/* Clock multiply from CLKIN to KHZ. */
	MC_CRPLL = MC_CRPLL_CLKSEL_CORE (KHZ/ELVEES_CLKIN) |
		   MC_CRPLL_CLKSEL_MPORT (MPORT_KHZ/ELVEES_CLKIN);

	/* Fixed mapping. */
	MC_CSR = MC_CSR_FM;

	MC_MASKR0 = 0;
	MC_MASKR1 = 0;
	MC_MASKR2 = 0;
	MC_MASKR3 = 0;
	MC_MASKR4 = 0;
#endif

	MC_ITCSR = 0;
#ifdef MC_ITCSR1
	MC_ITCSR1 = 0;
#endif
#ifdef MC_RTCSR
	MC_RTCSR = 0;
#endif
	MC_WTCSR = 0;
#ifdef MC_HAVE_SWIC
	MC_SWIC_RX_DESC_CSR(0) = 0;
	MC_SWIC_RX_DESC_CSR(1) = 0;
	MC_SWIC_RX_DATA_CSR(0) = 0;
	MC_SWIC_RX_DATA_CSR(1) = 0;
	MC_SWIC_TX_DESC_CSR(0) = 0;
	MC_SWIC_TX_DESC_CSR(1) = 0;
	MC_SWIC_TX_DATA_CSR(0) = 0;
	MC_SWIC_TX_DATA_CSR(1) = 0;
#endif
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

#endif // EXTERNAL_SETUP
	
	/*
	 * Setup UART registers.
	 * Compute the divisor for 115.2 kbaud.
	 */
	divisor = MC_DL_BAUD (KHZ * 1000, 115200);

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
	
#endif /* ELVEES */

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
_irq_handler_ ()
{
	/* This is needed when no kernel is present. */
}

#if defined (ELVEES)
static void dump_of_death (unsigned int context[])
{
	asm volatile ("1: j 1b; nop");
}

void _exception_handler_ (unsigned int context[])
{
	dump_of_death (context);
}

void _pagefault_handler_ (unsigned int context[])
{
	dump_of_death (context);
}
#endif /* ELVEES */
#endif
