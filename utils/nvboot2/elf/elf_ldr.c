/* Copyright (c) Microsoft Corporation. All rights reserved. */
/*
 * MMLite loader for ARM, ELF image format.
 */

#define OPTIMIZE        /* enable Base.h optimizations, since we're the "base" */

#include <mmlite.h>
#include <mmhal.h>
#include <machdep.h> /* FlushCache */
#include <base/loader.h>
#include <base/cons.h>
#include <base/debugger.h>
#include <diagnostics.h>

#define ALLOCATE(_h_,_s_) (_h_)->v->Alloc(_h_,HEAP_DEBUG_NAME("elf_ldr"),_s_,0)
#define FREE(_h_,_p_) (_h_)->v->Free(_h_,0,_p_)

/* Types are defined in ELF specification */
typedef UINT32 Elf32_Addr, Elf32_Off, Elf32_Word;
typedef INT32 Elf32_Sword;
typedef UINT16 Elf32_Half;

/*
 * This included file should provide the following functions
 * (or replacements thereof) and define the type of FILE_HANDLE.
 */
#include "loader_io.c"

#if 0 /* protos are: */
extern FILE_HANDLE _Open( const _TCHAR * FileName, UINT Mode);
extern INT _Close( FILE_HANDLE File);
extern INT _Read( FILE_HANDLE File, BYTE *Buffer, UINT nBytes);
extern UINT _Seek( FILE_HANDLE File, UINT Position);
extern INT _Stat( FILE_HANDLE File);
#endif

typedef char *NAME;

/*
 * Describe structure of ELF object file format.
 */

/* Size of file identifier in ELF header */
#define EI_NIDENT       16

/* Identification indexes for e_ident field in ELF header */
#define EI_MAG0     0         /* file ID byte 0 */
#define EI_MAG1     1         /* file ID byte 1 */
#define EI_MAG2     2         /* file ID byte 2 */
#define EI_MAG3     3         /* file ID byte 3 */
#define EI_CLASS    4         /* file class (capacity) */
#define EI_DATA     5         /* data encoding */
#define EI_VERSION  6         /* file version */
#define EI_PAD      7         /* start of padding bytes */
#define EI_NIDENT  16         /* size of e_ident[] */

/* "Magic number" in e_ident field of ELF header */
#define ELFMAG0     '\x7f'    /* e_ident[0] */
#define ELFMAG1     'E'       /* e_ident[1] */
#define ELFMAG2     'L'       /* e_ident[2] */
#define ELFMAG3     'F'       /* e_ident[3] */

/* File class (or capacity) in e_ident[4] of ELF header */
#define ELFCLASSNONE  0       /* invalid class */
#define ELFCLASS32    1       /* 32-bit processor */
#define ELFCLASS64    2       /* 64-bit processor */

/* Data encoding in e_ident[5] of ELF header */
#define ELFDATANONE   0       /* invalid data encoding */
#define ELFDATA2LSB   1       /* little-endian format */
#define ELFDATA2MSB   2       /* big-endian format */

#if (BYTE_ORDER==BIG_ENDIAN)
#define MYBYTEORDER ELFDATA2MSB
#else
#define MYBYTEORDER ELFDATA2LSB
#endif

/* Object file type in e_type field of ELF header */
#define ET_NONE     0         /* no file type */
#define ET_REL      1         /* relocatble file */
#define ET_EXEC     2         /* executable file */
#define ET_DYN      3         /* shared object file */
#define ET_CORE     4         /* core file */
#define ET_LOPROC   0xff00    /* processor-specific */
#define ET_HIPROC   0xffff    /* processor-specific */

/* Required architecture in e_machine field of ELF header */
#define EM_NONE     0         /* no machine */
#define EM_M32      1         /* AT&T WE 32100 */
#define EM_SPARC    2         /* SPARC */
#define EM_386      3         /* Intel 80386 */
#define EM_68K      4         /* Motorola 68000 */
#define EM_88K      5         /* Motorola 88000 */
#define EM_860      7         /* Intel 80860 */
#define EM_MIPS     8         /* MIPS RS3000 */
#define EM_ARM      40        /* Advanced RISC Machines ARM */

/* Object file version in e_version field of ELF header */
#define EV_NONE    0          /* invalid version */
#define EV_CURRENT 1          /* current version */


/* Processor-specific values in e_flags field of ELF header */
#define EF_MIPS_NOREORDER   0x00000001  /* .noreorder directive in source  */
#define EF_MIPS_PIC         0x00000002  /* position independent code       */
#define EF_MIPS_CPIC        0x00000004  /* 'standard' PIC calling sequence */
#define EF_MIPS_ABI2        0x00000020  /* new ABI on Irix6                */
#define EF_MIPS_ARCH_1      0x00000000  /* -mips1 instruction set          */
#define EF_MIPS_ARCH_2      0x10000000  /* -mips2 instruction set          */
#define EF_MIPS_ARCH_3      0x20000000  /* -mips3 instruction set          */
#define EF_MIPS_ARCH_4      0x30000000  /* -mips4 instruction set          */

#define EF_ARM_RELEXEC      0x00000001
#define EF_ARM_HASENTRY     0x00000002
#define EF_ARM_INTERWORK    0x00000004
#define EF_ARM_APCS_26      0x00000008
#define EF_ARM_APCS_FLOAT   0x00000010
#define EF_ARM_PIC          0x00000020
#define EF_ARM_ALIGN8       0x00000040
#define EF_ARM_NEW_ABI      0x00000080
#define EF_ARM_OLD_ABI      0x00000100

/* File header for ELF object file format */
typedef struct {
    UINT8 e_ident[EI_NIDENT];  /* identify file as object file */
    Elf32_Half    e_type;        /* file type */
    Elf32_Half    e_machine;     /* required architecture */
    Elf32_Word    e_version;     /* object file version */
    Elf32_Addr    e_entry;       /* entry point address */
    Elf32_Off     e_phoff;       /* program header table's file offset */
    Elf32_Off     e_shoff;       /* section header table's file offset */
    Elf32_Word    e_flags;       /* processor-specific flags */
    Elf32_Half    e_ehsize;      /* ELF header's size in bytes */
    Elf32_Half    e_phentsize;   /* size of entry in program header table */
    Elf32_Half    e_phnum;       /* no. of entries in program header table */
    Elf32_Half    e_shentsize;   /* size of entry in section header table */
    Elf32_Half    e_shnum;       /* no. of entries in section header table */
    Elf32_Half    e_shstrndx;    /* section hdr index of string table */
} Elf32_Ehdr;

/* Special section indexes (reserved values) */
#define SHN_UNDEF          0     /* undefined section index */
#define SHN_LORESERVE 0xff00     /* LB (lower bound) of ELF reserved indexes */
#define SHN_LOPROC    0xff00     /* LB of processor-specific semantics */
#define SHN_HIPROC    0xff1f     /* UB of processor-specific semantics */
#define SHN_ABS       0xfff1     /* absolute (not relocatable) section */
#define SHN_COMMON    0xfff2     /* C external variables section */
#define SHN_HIRESERVE 0xffff     /* UB (upper bound) of ELF reserved indexes */

/* Section's semantics in sh_type field of ELF section header */
#define SHT_NULL            0    /* no section associated with header */
#define SHT_PROGBITS        1    /* program-defined data */
#define SHT_SYMTAB          2    /* link editing & dynamic linking symbols */
#define SHT_STRTAB          3    /* string table */
#define SHT_RELA            4    /* minimal set of dynamic linking symbols */
#define SHT_HASH            5    /* relocation entries with explicit addends */
#define SHT_DYNAMIC         6    /* symbol hash table (dynamic linking) */
#define SHT_NOTE            7    /* dynamic linking info */
#define SHT_NOBITS          8    /* file-marking information */
#define SHT_REL             9    /* relocation entries without explicit addends */
#define SHT_SHLIB          10    /* reserved */
#define SHT_DYNSYM         11    /* dynamic linking symbol table */
#define SHT_LOPROC 0x70000000    /* LB for processor-specific dynamics */
#define SHT_HIPROC 0x7fffffff    /* UB for processor-specific dynamics */
#define SHT_LOUSER 0x80000000    /* LB for application-specific dynamics */
#define SHT_HIUSER 0x8fffffff    /* UB for application-specific dynamics */

/* Section's attribute flags in sh_flags field of ELF section header */
#define SHF_WRITE             0x1    /* data writable during execution */
#define SHF_ALLOC             0x2    /* occupies memory during execution */
#define SHF_EXECINSTR         0x4    /* executable machine instructions */
#define SHF_MASKPROC   0xf0000000    /* mask for processor-specific semantics */

/* Section header for ELF object file format */
typedef struct {
    Elf32_Word    sh_name;       /* section name (string table index) */
    Elf32_Word    sh_type;       /* section's contents and semantics */
    Elf32_Word    sh_flags;      /* section attribute flag bits */
    Elf32_Addr    sh_addr;       /* address of section's first byte */
    Elf32_Off     sh_offset;     /* file offset to 1st byte in section */
    Elf32_Word    sh_size;       /* section's size in bytes */
    Elf32_Word    sh_link;       /* section header table index link */
    Elf32_Word    sh_info;       /* additional section information */
    Elf32_Word    sh_addralign;  /* address alignment constraints */
    Elf32_Word    sh_entsize;    /* gives size if entry size is fixed */
} Elf32_Shdr;

/* Undefined symbol in symbol (string) table */
#define STN_UNDEF   0

/* Manipulating symbol's type and binding attributes in st_info field */
#define ELF32_ST_BIND(i)     ((i)>>4)
#define ELF32_ST_TYPE(i)     ((i)&0xf)
#define ELF32_ST_INFO(b,t)   (((b)<<4)+((t)&0xf))

/* Symbol binding for ELF32_ST_BIND macro above */
#define STB_LOCAL    0    /* local symbols (scope = source module) */
#define STB_GLOBAL   1    /* global symbols (visible to all) */
#define STB_WEAK     3    /* weak symbols (multiply-defined globals) */
#define STB_LOPROC  13    /* LB for processor-specific semantics */
#define STB_HIPROC  15    /* UB for processor-specific semantics */

/* Symbol binding for ELF32_ST_TYPE macro above */
#define STT_NOTYPE   0    /* symbol's type is unspecified */
#define STT_OBJECT   1    /* symbol for data object (variable, array, etc.) */
#define STT_FUNC     2    /* symbol for function or other executable code */
#define STT_SECTION  3    /* symbol for section */
#define STT_FILE     4    /* name of source file corresp. to this obj. file */
#define STT_LOPROC  13    /* LB for processor-specific semantics */
#define STT_HIPROC  15    /* UB for processor-specific semantics */

/* Symbol table entry in ELF object file */
typedef struct {
    Elf32_Word    st_name;       /* index into symbol string table */
    Elf32_Addr    st_value;      /* value of associated symbol */
    Elf32_Word    st_size;       /* size of object */
    UINT8 st_info;       /* symbol's type and binding attributes */
    UINT8 st_other;      /* =0 (no defined meaning) */
    Elf32_Half    st_shndx;      /* relevant section header table index */
} Elf32_Sym;

/* Manipulating relocation entry's type and binding attributes in r_info field */
#define ELF32_R_SYM(i)     ((i)>>8)
#define ELF32_R_TYPE(i)     ((UINT8)(i))
#define ELF32_R_INFO(b,t)   (((b)<<8)+((UINT8)(t)))

/* Or-ed in to relocation type */
#define R_DLL_TAG   0x80
#define IsaDllReloc(_t_)  (((_t_)&0xe0)==R_DLL_TAG)
#define MaskOffDllTag(_t_) ((_t_)&(R_DLL_TAG-1))
#define AddDllTag(_t_)    ((_t_)|R_DLL_TAG)

/* Relocation entry structures (two types) in ELF object file */
typedef struct {
    Elf32_Addr    r_offset;      /* address offset from section base */
    Elf32_Word    r_info;        /* symbol table index and type */
} Elf32_Rel;

typedef struct {
    Elf32_Addr    r_offset;      /* address offset from section base */
    Elf32_Word    r_info;        /* symbol table index and type */
    Elf32_Sword   r_addend;      /* constant addend to relocatable field */
} Elf32_Rela;

/* Segment type values in p_type field of ELF program header */
#define PT_NULL             0    /* ignore entry */
#define PT_LOAD             1    /* loadable segment */
#define PT_DYNAMIC          2    /* dynamic linking info */
#define PT_INTERP           3    /* interpreter pathname */
#define PT_NOTE             4    /* location and size of auxiliary info */
#define PT_SHLIB            6    /* reserved (not used) */
#define PT_PHDR             7    /* location & size of program header table */
#define PT_LOPROC  0x70000000    /* LB processor specific semantics */
#define PT_HIPROC  0x7fffffff    /* UB processor specific semantics */

#define PT_MIPS_REGINFO 0x70000000 /* register usage information */

#define PTF_IMPORT          1
#define PTF_EXPORT          2

/* Program header in ELF object file */
typedef struct {
    Elf32_Word    p_type;       /* kind of segment */
    Elf32_Off     p_offset;     /* file offset of segment */
    Elf32_Addr    p_vaddr;      /* virtual address of segment */
    Elf32_Addr    p_paddr;      /* physical address of segment */
    Elf32_Word    p_filesz;     /* file image size */
    Elf32_Word    p_memsz;      /* memory image size */
    Elf32_Word    p_flags;      /* flags for segment */
    Elf32_Word    p_align;      /* segment alignment */
} Elf32_Phdr;

/* Dynamic array tag values in d_tag field of ELF dynamic structure */
#define DT_NULL             0    /* ignore d_un */
#define DT_NEEDED           1    /* marks end of dynamic array */
#define DT_PLTRELSZ         2    /* total size of relation entries */
#define DT_PLTGOT           3    /* procedure linkage/global offset tbl */
#define DT_HASH             4    /* symbol hash table */
#define DT_STRTAB           5    /* address of string table address */
#define DT_SYMTAB           6    /* address of symbol table */
#define DT_RELA             7    /* address of relocation table */
#define DT_RELASZ           8    /* total size of DT_RELA reloc table */
#define DT_RELAENT          9    /* size of DT_RELA relocation entry */
#define DT_STRSZ           10    /* size of string table */
#define DT_SYMENT          11    /* size of symbol table entry */
#define DT_INIT            12    /* address of initialization function */
#define DT_FINI            13    /* address of termination function */
#define DT_SONAME          14    /* string tbl offset of sh. obj. name */
#define DT_RPATH           15    /* string tbl offset of search path */
#define DT_SYMBOLIC        16    /* start search at shared object */
#define DT_REL             17    /* like DT_RELA, but with addends */
#define DT_RELSZ           18    /* size of DT_REL relocation table */
#define DT_RELENT          19    /* size of DT_REL relocation entry */
#define DT_PLTREL          20    /* size reloc entry from proc link table */
#define DT_DEBUG           21    /* used for debugging */
#define DT_TEXTREL         22    /* mods to nonwritable segment */
#define DT_JMPREL          23    /* procedure linkage table reloc entries */
#define DT_LOPROC  0X70000000    /* LB for processor-specific semantics */
#define DT_HIPROC  0X7FFFFFFF    /* UB for processor-specific semantics */


/* ARM-related relocation types (with GCC extensions)
 */
#define R_ARM_NONE              0x00
#define R_ARM_PC24              0x01
#define R_ARM_ABS32             0x02
#define R_ARM_REL32             0x03
#define R_ARM_PC13              0x04
#define R_ARM_ABS16             0x05
#define R_ARM_ABS12             0x06
#define R_ARM_THM_ABS5          0x07
#define R_ARM_ABS8              0x08
#define R_ARM_SBREL32           0x09
#define R_ARM_THM_PC22          0x0a
#define R_ARM_THM_PC8           0x0b
#define R_ARM_AMP_VCALL9        0x0c
#define R_ARM_SWI24             0x0d
#define R_ARM_THM_SWI8          0x0e
#define R_ARM_XPC25             0x0f
#define R_ARM_THM_XPC22         0x10
#define R_ARM_COPY              0x14       /* copy symbol at runtime */
#define R_ARM_GLOB_DAT          0x15       /* create GOT entry */
#define R_ARM_JUMP_SLOT         0x16       /* create PLT entry */
#define R_ARM_RELATIVE          0x17       /* adjust by program base */
#define R_ARM_GOTOFF            0x18       /* 32 bit offset to GOT */
#define R_ARM_GOTPC             0x19       /* 32 bit PC relative offset to GOT */
#define R_ARM_GOT32             0x1a       /* 32 bit GOT entry */
#define R_ARM_PLT32             0x1b       /* 32 bit PLT address */
#define R_ARM_GNU_VTENTRY       0x64
#define R_ARM_GNU_VTINHERIT     0x65
#define R_ARM_THM_PC11          0x66       /* cygnus extension to abi: thumb unconditional branch */
#define R_ARM_THM_PC9           0x67       /* cygnus extension to abi: thumb conditional branch */
#define R_ARM_RXPC25            0xf9
#define R_ARM_RSBREL32          0xfa
#define R_ARM_THM_RPC22         0xfb
#define R_ARM_RREL32            0xfc
#define R_ARM_RABS32            0xfd
#define R_ARM_RPC24             0xfe
#define R_ARM_RBASE             0xff

/* MIPS-related relocation types
 */
#define R_MIPS_16               0x01
#define R_MIPS_32               0x02
#define R_MIPS_REL32            0x03
#define R_MIPS_26               0x04
#define R_MIPS_HI16             0x05
#define R_MIPS_LO16             0x06
#define R_MIPS_GPREL16          0x07
#define R_MIPS_LITERAL          0x08
#define R_MIPS_GOT16            0x09
#define R_MIPS_PC16             0x0a
#define R_MIPS_CALL16           0x0b
#define R_MIPS_GPREL32          0x0c

#define BIT23       0x00800000               /* 1 << 23 */

/* Dynamic structure in ELF object file */
typedef struct {
    Elf32_Sword     d_tag;  /* how to interpret d_un */
    union {
        Elf32_Word    d_val;   /* integer value */
        Elf32_Addr    d_ptr;   /* virtual address */
    } d_un;
} Elf32_Dyn;

/* Our extensions
 */
#define ENL 16
typedef struct {
    UINT8 ih_name[ENL];         /* name of the DLL we IMPORT from */
    UINT8 ih_uuid[ENL];         /* UUID of the CInterface         */
} Elf32_Ihdr;

typedef union {
    Elf32_Ihdr Ihdr;
    struct {
        Elf32_Addr *pRtl;
        Elf32_Off nRtl;
    } Rhdr;
} IMPORTS, *PIMPORTS;


/* Utilities
 */
static UINT32 SwapWord(UINT32 w)
{
   return (((w & 0xff) << 24) |
           ((w & 0xff00) << 8) |
           ((w & 0xff0000) >> 8) |
           ((w & 0xff000000) >> 24));
}

static UINT16 SwapHalf(UINT16 h)
{
   return (((h & 0xff) << 8) |
           ((h & 0xff00) >> 8));
}

#define GetWord(_swap_,_w_)  ((_swap_) ? SwapWord(_w_) : (_w_))
#define GetHalf(_swap_,_h_)  ((_swap_) ? SwapHalf(_h_) : (_h_))
#define SetWord(_swap_,_a_,_v_) _a_ = ((_swap_) ? SwapWord(_v_) : (_v_))
#define SetHalf(_swap_,_a_,_v_) _a_ = ((_swap_) ? SwapHalf(_v_) : (_v_))

/* Relocate a single storage location in the ELF movable executable.
 */
#if defined(arm)
#define MYMACHINE EM_ARM
#define RelocateEntry RelocateArmEntry


/*
 * Compute the size of an in-memory ARM ELF image.
 * [Used in case the simulator loaded the image, not us]
 */
SCODE SizeofArmImage(PTR pImage, INT *pSize, THREAD_FUNCTION **ppEntry )
{
    Elf32_Ehdr *pElf = (Elf32_Ehdr *)pImage;
    char *s;
    BOOL MustSwap;

    s = (char *)pElf->e_ident;
    if (*s != ELFMAG0 || memcmp(s + 1, "ELF", 3)) {
        DBGME(3,printf("Error -- Not a valid ELF file!\n"));
        return E_INVALID_PARAMETER;
    }

    MustSwap = (pElf->e_ident[EI_DATA] != MYBYTEORDER);

#if defined(MMLITE_VERSION)
    /* Is the version compatible with our build ?
     */
    if (GetWord(MustSwap,pElf->e_version) > MMLITE_VERSION) {
        DBGME(3,printf("Error -- Incompatible version!\n"));
        return E_INCOMPATIBLE_VERSION;
    }
#endif

    /* Compute how much memory we need.
     */
    if (pSize) {
        *pSize = ((INT)GetWord(MustSwap,pElf->e_shoff)) +
                   (INT)(GetHalf(MustSwap,pElf->e_shnum) *
                         GetHalf(MustSwap,pElf->e_shentsize));
    }

    /* Compute entry-point address.
     */
    if (ppEntry) {
        *ppEntry = (THREAD_FUNCTION *)GetWord(MustSwap,pElf->e_entry);
    }

    return NO_ERROR;
}

BOOL RelocateArmEntry(BOOL MustSwap, int RelaType,
                      Elf32_Off Delta, Elf32_Addr A, char * P)
{
    DBGME(0,printf("RelocArmEntry: %x %x %x %x [%x]\n",
                 RelaType, Delta, A, (ADDRESS) P, ((UINT32*)P)[0]));

    switch (RelaType) {
    case R_ARM_RABS32:
    case R_ARM_ABS32:
        /* example:  .long foo+0xNNN
         */
        A = GetWord(MustSwap,*(UINT32*)P) + Delta;
        SetWord(MustSwap,*(UINT32*)P,A);
        break;

    case R_ARM_PC24:
        /* example:  bl foo
         */
#if 0
        /* This is PC-relative so it needs no runtime relocation
         */
        A = A - PC; /* make PC-relative */
        A = (A - 8) >> 2;
        A = (A & 0x00ffffff) | (GetWord(MustSwap,*(UINT32 *)P) & 0xff000000);
        SetWord(MustSwap,*(UINT32 *)P,A);
#endif
        break;

    case R_ARM_RBASE:
        /* Nothing, just says where the start is
         */
        break;

    default:
        return FALSE;   /* error */
    }

    return TRUE;   /* okay */
}

#endif

#if defined(mips)
#define MYMACHINE EM_MIPS
#define RelocateEntry RelocateMipsEntry

static
BOOL RelocateMipsEntry(BOOL MustSwap, int RelaType,
                       Elf32_Off Delta, Elf32_Addr A, char * P)
{
    switch (RelaType) {
    case R_MIPS_PC16:
        /* example:  bgtz   a0,foo
         */
        return TRUE;

    case R_MIPS_16:
        /* example:  .short foo
         */
        SetHalf(MustSwap,*(UINT16 *)P,(UINT16)A);
        break;

    case R_MIPS_32:
        /* example:  .long foo
         */
        SetWord(MustSwap,*(UINT32 *)P,A);
        break;

    case R_MIPS_26:
        /* example:  jal foo
         */
        A = (A /*- 4*/) >> 2;
        A = (A & 0x03ffffff) | (GetWord(MustSwap,*(UINT32 *)P) & 0xfc000000);
        SetWord(MustSwap,*(UINT32 *)P,A);
        break;

    case R_MIPS_GOT16: /* not really but.. */
    case R_MIPS_HI16:
        /* example:  lui a0,foo
         */
        /* beware of sign extension */
        if (A & 0x8000)
            A = (A >> 16) + 1;
        else
            A = (A >> 16);
        /* fall through... */

    case R_MIPS_LO16:
        /* example:  addiu a0,a0,foo
         */
            P += 2;
        if (MustSwap) {
//            P += 2;

            A = SwapHalf((UINT16)A);
        }
        *(UINT16 *)P = (UINT16)A;
        break;

    default:
        return FALSE;   /* error */
    }

    return TRUE;   /* okay */
}
#endif

extern PIBASERTL pTheBaseRtl;

extern SCODE MCT ModuleGetVtable(
             IModule *pThis,
             ADDRESS **pExportData,
             UINT *pExportSize);

static BOOL LookupExportSection(char *Name, UINT *pnExports, Elf32_Addr **ppExports)
{
    PIMODULE pModule = LookupModule(Name,0/*version check*/);
    SCODE sc;
    if (pModule == NULL)
        return FALSE;
    sc = ModuleGetVtable(pModule,ppExports,pnExports);
    return (sc == NO_ERROR) ? TRUE : FALSE;
}
#define RegisterExportSection(Name,nExports,pExports) TRUE

/* Make sure all DLLs named in the import list are loaded.
 * Replace import name-list with rtl-list.
 * Returns FALSE Iff it cannot find a DLL.
 */
static BOOL LookupImports(PIMPORTS pImports, UINT nImports)
{
    UINT i, nExports;
    Elf32_Addr *pExports;

    for (i = 0; i < nImports; i++, pImports++) {
      if (!LookupExportSection((char *)pImports->Ihdr.ih_name,&nExports,&pExports)) {
        DBGME(3,printf("NOT found DLL %s !\n", pImports->Ihdr.ih_name));
        return FALSE;
      }
      pImports->Rhdr.pRtl = pExports;
      pImports->Rhdr.nRtl = nExports;
    }
    return TRUE;
}

/* Adjust the entries in the export section by the given DELTA
 */
static BOOL AdjustExports(BOOL MustSwap,
                   Elf32_Addr *pExports,
                   UINT nExports,
                   Elf32_Off Delta,
                   char *SectionName)
{
    UINT i;
    Elf32_Addr A;

    for (i = 0; i < nExports; i++) {
        A = GetWord(MustSwap,pExports[i]);
        if (A != (Elf32_Addr)~0) {
            DBGME(1,printf(" Ord[%d]@%x %x -> %x\n", i,
                         (ADDRESS) pExports, A, A + Delta));
            A += Delta;
            pExports[i] = GetWord(MustSwap,A);
        }
    }
    return RegisterExportSection(SectionName, nExports, pExports);
}

/* Perform all relocations specified in the ELF file pointed to by PELF.
 * Loadable sections have been loaded in memory at PBITS.
 * Those sections will execute at address DESTINATION.
 * This file should be a movable executable with program headers,
 * relocation tables, and DYNSYM simplified dynamic symbols.
 * We make some simplifying assumtions on the file layout.
 */
static BOOL DoElfRelocations(FILE_HANDLE f,
                      BOOL MustSwap,
                      Elf32_Ehdr *pElf,
                      BYTE *pBits,
                      Elf32_Addr Destination,
                      Elf32_Addr **pVtable,
                      Elf32_Off *pVtableCount)
{
    int i;
    BOOL sc, IsRel = FALSE;
    UINT32 numPhdrs;
    Elf32_Word RelaSize, RelaEntSize, ImportSize, ExportSize;
    Elf32_Addr linkBase, delta;
    Elf32_Off offset, ImportOffset, ExportOffset;
    Elf32_Phdr *pPhdr, *pPhdrBase;
    Elf32_Dyn *pDyn;
    Elf32_Rela *pRelaBase, *pRela, *pRelaEnd;
    IMPORTS *pImp;
    Elf32_Addr *pExp;
    Elf32_Word w;
    PIHEAP pIHeap = CurrentHeap();


    /*compilerbrain*/
    offset = ImportOffset = 0;

    /* Find base address to which ELF file was linked.  This is the lowest p_vaddr
     * value for a PT_LOAD segment.  Search the program headers for this value.
     */
    pPhdrBase = (Elf32_Phdr *)(((char *)pElf) + GetWord(MustSwap,pElf->e_phoff));
    numPhdrs = GetHalf(MustSwap,pElf->e_phnum);
    linkBase = (Elf32_Addr)~0;
    pRela = NULL;
    RelaSize = 0;
    RelaEntSize = 0;
    ImportSize = 0;
    ExportSize = 0;
    for (i = 0, pPhdr = pPhdrBase; i < numPhdrs; ++i, ++pPhdr) {
        switch (GetWord(MustSwap,pPhdr->p_type)) {
        case PT_LOAD:
            delta = GetWord(MustSwap,pPhdr->p_vaddr);
            if (delta < linkBase) {
                linkBase = delta;
            }
            break;
        case PT_DYNAMIC:
            pDyn = (Elf32_Dyn *)(((char *)pElf) + GetWord(MustSwap,pPhdr->p_offset));
            w = GetWord(MustSwap,pDyn->d_tag);
            while (w != DT_NULL) {
                switch (w) {
                case DT_RELA:
                    offset = GetWord(MustSwap,pDyn->d_un.d_ptr);
                    break;
                case DT_RELASZ:
                    RelaSize = GetWord(MustSwap,pDyn->d_un.d_val);
                    break;
                case DT_RELAENT:
                    RelaEntSize = GetWord(MustSwap,pDyn->d_un.d_val);
                    break;

                case DT_REL:
                    IsRel = TRUE;
                    offset = GetWord(MustSwap,pDyn->d_un.d_ptr) +
                             GetWord(MustSwap,pPhdr->p_offset);
                    break;
                case DT_RELSZ:
                    RelaSize = GetWord(MustSwap,pDyn->d_un.d_val);
                    break;
                case DT_RELENT:
                    RelaEntSize = GetWord(MustSwap,pDyn->d_un.d_val);
                    break;

                default:
                    break;
                }
                ++pDyn;
                w = GetWord(MustSwap,pDyn->d_tag);
            }
            break;
        case PT_SHLIB:
            w = GetWord(MustSwap,pPhdr->p_flags);
            if (w == PTF_IMPORT) {
                ImportSize = GetWord(MustSwap,pPhdr->p_filesz);
                ImportOffset = GetWord(MustSwap,pPhdr->p_offset);
            } else if (w == PTF_EXPORT) {
                ExportSize = GetWord(MustSwap,pPhdr->p_filesz);
                ExportOffset = GetWord(MustSwap,pPhdr->p_offset);
            }

            break;
        default:
            break;
        }
    }

    /* Calculate difference between base address of actual memory image
     * and base address to which file was originally linked.
     */
    delta = Destination - linkBase;

    /* If there is a difference we must have some valid relocations
     * Same if we depend on some DLL.
     * Exports could be faster but..
     * Otherwise we are done.
     */
    if ((delta == 0) && (ImportSize == 0) && (ExportSize == 0))
        return TRUE;
    if ((RelaSize == 0) || (RelaEntSize == 0)) {
        DBGME(3,printf("Error -- No valid relocations!  Can only load at %x\n",
                     linkBase));
        return FALSE;
    }

    /* Patch up entry-point address in ELF header.
     */
    SetWord(MustSwap,pElf->e_entry,
            GetWord(MustSwap,pElf->e_entry) + delta);

    /* Allocate and read in the RELA section
     * The import, if any, follows immediately.
     */
    if (ImportSize &&
        (ImportOffset != (offset + RelaSize))) {
        return FALSE;
    }
    /* Borrow this local var for a while
     */
    RelaEntSize = RelaSize + ImportSize;
    pRela = (Elf32_Rela *)ALLOCATE(pIHeap,RelaEntSize);
    if (pRela == NULL) {
        return FALSE;
    }
    pRelaBase = pRela;
    _Seek(f, offset);
    i = _Read(f, (PBYTE)pRela, RelaEntSize);
    if (RelaEntSize != i) {
        goto Failure;
    }
    pImp = (PIMPORTS)(((char *)pRela) + RelaSize);
    /* The export is at the file's start
     */
    pExp = (Elf32_Addr *)pBits;

    /* Lookup all imports
     */
    if (ImportSize && !LookupImports(pImp, ImportSize / sizeof *pImp)) {
        goto Failure;
    }

    /* Adjust export table
     * Ugly-eeee: Name should be in the header !!!
     */
    if (ExportSize) {
        if (!AdjustExports(MustSwap,
                           pExp,
                           ExportSize / sizeof *pExp,
                           delta,
                           (char *)pElf->e_ident))
        {
            goto Failure;
        }
        *pVtable = pExp;
        *pVtableCount = ExportSize / sizeof *pExp;
    }

    /* Perform the relocations.
     */
    pRelaEnd = (Elf32_Rela *)(((char *)pRela) + RelaSize);
    RelaEntSize = (IsRel) ? sizeof(Elf32_Rel) : sizeof(Elf32_Rela);

    /* Perform all relocations specified in table.
     */
    for ( ; pRela < pRelaEnd;) {
        UINT t = GetWord(MustSwap,pRela->r_info);
        Elf32_Addr A;
        char *P;
        UINT DllIx, Ordinal;

        /* r_offset is a file offset as usual.
         * r_addend is from linkBase
         */
        offset = GetWord(MustSwap,pRela->r_offset);
        A = ((Elf32_Sword)GetWord(MustSwap,pRela->r_addend))+delta;
        P = (char *)pBits+offset;

        /* Check for DLLs
         */
        DllIx = ELF32_R_SYM(t);
        t = ELF32_R_TYPE(t);
        if (IsaDllReloc(t)) {

            Ordinal = GetWord(MustSwap,pRela->r_addend);
            if ((pImp[DllIx].Rhdr.pRtl == NULL) ||
                (Ordinal >= pImp[DllIx].Rhdr.nRtl)) {
                DBGME(3,printf(" Bad DLLRel, ordinal %d DllIx %d pImp %x\n",
                             Ordinal, DllIx, (UINT) pImp[DllIx].Rhdr.pRtl));
                goto Failure;
            }
            A = pImp[DllIx].Rhdr.pRtl[Ordinal];
            A = GetWord(MustSwap,A);
            DBGME(0,printf("DLLrel[%d] %d %d -> %x\n", t, DllIx, Ordinal, A));
            t = MaskOffDllTag(t);
        }
        sc = RelocateEntry(MustSwap, t, delta, A, P);
        if (!sc) {
            goto Failure;
        }
        pRela = (Elf32_Rela *)(((char *)pRela) + RelaEntSize);
    }
    sc = TRUE;   /* okay */
 Return:
    if (pRelaBase)
        FREE(pIHeap,pRelaBase);
    return sc;
 Failure:
    sc = FALSE;
    goto Return;
}


/*
 * Load an ELF-formatted program image into memory.
 * Code can be compiled to run off-target.
 */
SCODE LdrLoadImage(PINAMESPACE pNameSpace,
                const _TCHAR * pImage,
                const _TCHAR * pArgs,
                UINT32 Flags,
                IModule **ppIModule)
{
    SCODE sc = E_INVALID_PARAMETER; /* most likely */
    FILE_HANDLE f;
    UINT i, MemorySize, FileSize, FileStart;
    Elf32_Word t;
    Elf32_Half n;
    MODULEENTRY EntryPoint;
    PIMODULE pMod;
    PIPROCESS pPrc;
    Elf32_Ehdr *pElf = NULL;
    Elf32_Phdr *pPhdr;
    char *s;
    BYTE *pMem = NULL;
    PIHEAP pIHeap = CurrentHeap();
    BOOL MustSwap;
    Elf32_Addr *Vtable;
    Elf32_Off VtableCount;

    /* Check the file exists, see how big it is
     */
    f = _Open( pImage, 0x1 );
    if (f == INVALID_FILE_HANDLE) {
        DBGME(3,printf("File %s not found\n",pImage));
        return E_FILE_NOT_FOUND;
    }

    FileSize = _Stat( f );
    if (FileSize < sizeof(Elf32_Ehdr)) {
        DBGME(3,printf("File too small %d\n",FileSize));
        goto Exit;
    }

    /* Guesstimate that we can get all headers at once
     * PMDLL needs 52(ELF)+5*32(PHDR)+32(DYN) == 244
     * ADS   needs 52(ELF)+2*32(PHDR)+88(DYN) == 204
     */
    MemorySize = 248; //0x1000;

    pMem = (BYTE *)ALLOCATE(pIHeap,MemorySize);
    if (pMem == NULL) {
        DBGME(3,printf("Outtamem\n"));
        sc = E_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    /* Read in the ELF header plus..
     */
    MemorySize = _Read( f, pMem, MemorySize);
    if (MemorySize < sizeof(Elf32_Ehdr)) {
        DBGME(3,printf("File too small %d\n",MemorySize));
        goto Exit;
    }

    /* Is this a valid ELF file?
     */
    pElf = (Elf32_Ehdr *)pMem;
    MustSwap = (pElf->e_ident[EI_DATA] != MYBYTEORDER);
    s = (char *)pElf->e_ident;
    if ((*s != ELFMAG0) ||
        (memcmp(s + 1, "ELF", 3) != 0) ||
        (GetHalf(MustSwap,pElf->e_machine) != MYMACHINE)) {
        DBGME(3,printf("Error -- Not a valid ELF file!\n"));
        goto Exit;
    }

#if defined(MMLITE_VERSION)
    /* Is the version compatible with our build ?
     */
    if (GetWord(MustSwap,pElf->e_version) > MMLITE_VERSION) {
        DBGME(3,printf("Error -- Incompatible version!\n"));
        sc = E_INCOMPATIBLE_VERSION;
        goto Exit;
    }
#endif

    /* Determine size of ELF header plus all program headers.
     * If they are all at the start and contiguous we have them already
     * [this is true if the image was produced by GLD+ the PMDLL tool]
     */
    i = GetHalf(MustSwap,pElf->e_phnum) * GetHalf(MustSwap,pElf->e_phentsize);
    FileSize = (INT)GetWord(MustSwap,pElf->e_phoff) + i;

    /* If we need to read more then do it all over again
     */
    if (FileSize > MemorySize) {

#if defined(ADS) && 1
        /* This compiler puts the PGM headers ~at the end of the file.
         * But they might fit in what we allocated above, check ifso.
         * NB: We only account for part of the PT_DYNAMIC stuff here,
         *     the RELs will be read in separately.
         */
#define PT_DYNAMIC_RECORDS 11
#define PT_DYNAMIC_SIZE (PT_DYNAMIC_RECORDS * sizeof(Elf32_Dyn))
        t = i + PT_DYNAMIC_SIZE + sizeof(Elf32_Ehdr);
        if (t <= MemorySize) {

            /* Read in the PGM headers from wherever they are
             */
            FileStart = GetWord(MustSwap,pElf->e_phoff);
            _Seek(f, FileStart);
            pPhdr = (Elf32_Phdr *)(pMem + sizeof(Elf32_Ehdr));

            t = _Read( f, (BYTE*)pPhdr , i);
            if (t != i) {
                DBGME(3,printf("Read2 fail %d @%d\n",i,FileStart));
                goto Exit;
            }

            /* Fix the offset in the Elf header
             */
            pElf->e_phoff = GetWord(MustSwap,sizeof(Elf32_Ehdr));

            /* Find the PT_DYNAMIC one, if any (yes there will be one)
             */
            n = GetHalf(MustSwap,pElf->e_phnum);
            for (i = 0; i < n; ++i) {

                /* This one ?
                 */
                t = GetWord(MustSwap,pPhdr[i].p_type);
                if (t == PT_DYNAMIC) {

                    /* Read in the first 10 DYN records
                     */
                    Elf32_Dyn *pDyn;
                    pDyn = (Elf32_Dyn *)(pPhdr + n);
                    FileStart = GetWord(MustSwap,pPhdr[i].p_offset);
                    _Seek(f, FileStart);

                    t = _Read( f, (BYTE*)pDyn, PT_DYNAMIC_SIZE);
                    if (t != PT_DYNAMIC_SIZE) {
                        DBGME(3,printf("Read3 fail %d @%d\n",PT_DYNAMIC_SIZE,
                                       FileStart));
                        goto Exit;
                    }

                    /* Fix the PGM offset
                     */
                    t = ((PBYTE)pDyn) - pMem;
                    pPhdr[i].p_offset = GetWord(MustSwap,t);


                    /* The DT_REL record indicates the offset from the start
                     * of the DYN records [illegally ??]. Must fix it.
                     */
                    t = FileStart - t;       /* correction to REL record  */

                    /* See if we got one
                     */
                    for (i = 0; i < PT_DYNAMIC_RECORDS; i++) {
                        if (GetWord(MustSwap,pDyn[i].d_tag) == DT_REL) {

                            /* Apply the offset correction
                             */
                            t += GetWord(MustSwap,pDyn[i].d_un.d_ptr);
                            pDyn[i].d_un.d_ptr = GetWord(MustSwap,t);

                            /* Done
                             */
                            break;
                        }
                    }

                    /* And we are done
                     */
                    break;
                }
            }

        }
        else
#endif
        {
            /* Done with ELF header.
             */
            FREE(pIHeap, pMem);

            /* Allocate memory for ELF header plus program headers.
             */
            pMem = (BYTE *)ALLOCATE(pIHeap, FileSize);
            if (pMem == NULL) {
                DBGME(3,printf("Outtamem\n"));
                sc = E_NOT_ENOUGH_MEMORY;
                goto Exit;
            }

            _Seek(f, 0);        /* as if nothing happened */

            /* Try again, if cant read something amiss
             */
            MemorySize = _Read( f, pMem, FileSize);
            if (MemorySize != FileSize) {
                DBGME(3,printf("File too small %d\n",MemorySize));
                goto Exit;
            }
        }
    }

    pElf = (Elf32_Ehdr *)pMem;

    /* Look at the program headers to see how much stuff we load
     */
    pPhdr = (Elf32_Phdr *)(((char *)pElf) + GetWord(MustSwap,pElf->e_phoff));
    n = GetHalf(MustSwap,pElf->e_phnum);

    if (n == 0) {
        /* No pgm headers ?  */
        DBGME(3,printf("Error -- no pgm headers\n"));
        goto Exit;
    }

    FileSize = MemorySize = 0;
    FileStart = ~0;
    DBGME(1,printf("elf_ldr(%s)\n", pImage));
    for (i = 0; i < n; ++i) {
        t = GetWord(MustSwap,pPhdr[i].p_type);
        DBGME(1,printf("PgmHeader %d: ",i));
        DBGME(1,printf(" %x %x %x %x %x %x %x %x\n",
               t,
               GetWord(MustSwap,pPhdr[i].p_offset),
               GetWord(MustSwap,pPhdr[i].p_vaddr),
               GetWord(MustSwap,pPhdr[i].p_paddr),
               GetWord(MustSwap,pPhdr[i].p_filesz),
               GetWord(MustSwap,pPhdr[i].p_memsz),
               GetWord(MustSwap,pPhdr[i].p_flags),
               GetWord(MustSwap,pPhdr[i].p_align) ));
        if (t == PT_LOAD) {
            FileSize   += GetWord(MustSwap,pPhdr[i].p_filesz);
            MemorySize += GetWord(MustSwap,pPhdr[i].p_memsz);
            t = GetWord(MustSwap,pPhdr[i].p_offset);
            if (t < FileStart)
                FileStart = t;
        }
    }
    DBGME(1,printf("Fsz=%x Msz=%x Fs=%x\n", FileSize, MemorySize, FileStart));

    /* Allocate memory for (contiguous!) program segments.
     */
    pMem = (BYTE *)ALLOCATE(pIHeap, MemorySize);
    if (pMem == NULL) {
        DBGME(3,printf("Outtamem\n"));
        sc = E_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    /* Position file pointer
     */
    _Seek(f, FileStart);

    if (FileSize != _Read( f, pMem, FileSize)) {
        DBGME(3,printf("File too small %d\n",FileSize));
        goto Exit;
    }

    /* Zero bss
     */
    if (MemorySize-FileSize) {
        DBGME(0,printf(" Zeroing BSS @%x  %d bytes\n",
               (UINT) pMem+FileSize, MemorySize-FileSize));
        memset(pMem+FileSize, 0, MemorySize-FileSize);
    }

    /* Ugly-eeee: export name hackywhacky.
     */
    memcpy(pElf->e_ident,pImage,EI_NIDENT);

    /* Perform the relocations if needed.
     * Also handles import/export issues.
     */
    Vtable = NULL;
    VtableCount = 0;
#define Destination ((Elf32_Addr)pMem)
    if (!DoElfRelocations(f,
                          MustSwap,
                          pElf,
                          pMem,
                          Destination,
                          &Vtable,
                          &VtableCount
                          )) {
        DBGME(3,printf("Failed to relocate\n"));
        goto Exit;
    }

    /* Done with file handle
     */
    _Close(f);
    f = INVALID_FILE_HANDLE;

    /* Get Entry point
     */
    EntryPoint = (MODULEENTRY)(GetWord(MustSwap,pElf->e_entry));
    DBGME(2,printf("Image %hs is x%x bytes at x%x [%x], entry = %x\n",
           pImage, MemorySize, Destination, (UINT)pMem, (UINT)EntryPoint));

    /* Flush cache
     */
    FlushCache();

    /* Create a descriptor for this module we just loaded.
     */
    sc = ModuleCreate(pImage,
                      pArgs,
                      pMem,
                      MemorySize,
                      EntryPoint,
                      Vtable,
                      VtableCount,
                      0, 0, 0, 0,
                      Flags,
                      &pMod);

    DBGME(1,printf("elf_ldr(%s) Flags=%08x\n", pImage, Flags));
    /* Hand caller one ref iff ppIModule is valid pointer.
     */
    if (ppIModule) {
        pMod->v->AddRef(pMod);
        *ppIModule = pMod;
    }

    if (Flags & LOAD_IMAGE_CALL_ENTRY_POINT) {
        if (Flags & LOAD_IMAGE_CREATE_THREAD) {

            /* Find an IProcess descriptor for the module. */
            (void) pMod->v->QueryInterface(pMod,
                                           &IID_IProcess,
                                           (void**) &pPrc);

            /* Create a new thread to invoke image's entry point. */
            sc = pPrc->v->CreateThread(pPrc, Flags,
                                       (THREAD_FUNCTION) EntryPoint,
                                       pTheBaseRtl,
                                       0,/*default stacksize*/
                                       NULL,
                                       NULL);

            /* Release our reference (new thread still holds one). */
            (void) pPrc->v->Release(pPrc);
        } else {
            /* Call the entry point directly. */
            (EntryPoint)( pTheBaseRtl );
        }
    }
    pMod->v->Release(pMod);

    sc = NO_ERROR;
    pMem = NULL;

Exit:
    /* Done with ELF header and program headers.
     */
    if (pElf)
        FREE(pIHeap,pElf);

    /* Rid of pgm memory if failure
     */
    if (pMem && (pMem != (PBYTE)pElf))
        FREE(pIHeap,pMem);

    /* Close files
     */
    if (f != INVALID_FILE_HANDLE)
        _Close(f);
    DBGME(2,printf("elf_ldr loaded(%s)\n", pImage, Flags));
    return sc;
}
