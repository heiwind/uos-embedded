#if !defined(ELF_OBJECT_H)
#define ELF_OBJECT_H

#include <stdint.h>

/* Types are defined in ELF specification */
typedef uint32_t    Elf32_Addr, Elf32_Off, Elf32_Word;
typedef int32_t     Elf32_Sword;
typedef uint16_t    Elf32_Half;

typedef struct {
	uint32_t	e_magic;
	uint8_t	e_class;
	uint8_t	e_data;
	uint8_t	e_iversion;
	uint8_t	e_reserve[9];
	uint16_t	e_type;
	uint16_t	e_machine;
	uint32_t	e_version;
	uint32_t	e_entry;
	uint32_t	e_phoff;
	uint32_t	e_shoff;
	uint32_t	e_flags;
	uint16_t	e_ehsize;
	uint16_t	e_phentsize;
	uint16_t	e_phnum;
	uint16_t	e_shentsize;
	uint16_t	e_shnum;
	uint16_t	e_shstrndx; //section names strtable idx
} elf_header;

#define ELF_MAGIC 0x464c457f

#define ET_NONE 0
#define ET_REL	1
#define ET_EXEC 2
#define ET_DYN	3
#define ET_CORE	4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define EM_NONE		0
#define EM_M32		1
#define EM_SPARC	2
#define EM_386		3
#define EM_68K		4
#define EM_88K		5
#define EM_860		7
#define EM_MIPS		8

#define EV_NONE		0
#define EV_CURRENT	1

#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'

#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

typedef struct {
	uint32_t	p_type;
	uint32_t	p_offset;
	uint32_t	p_vaddr;
	uint32_t	p_paddr;
	uint32_t	p_filesz;
	uint32_t	p_memsz;
	uint32_t	p_flags;
	uint32_t	p_align;
} elf_ph;

#define PT_LOAD 1

typedef struct {
	uint32_t	sh_name;
	uint32_t	sh_type;
	uint32_t	sh_flags;
	uint32_t	sh_addr;
	uint32_t	sh_offset;
	uint32_t	sh_size;
	uint32_t	sh_link;
	uint32_t	sh_info;
	uint32_t	sh_addralign;
	uint32_t	sh_entsize;
} elf_sh;

#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00	/* ??? wrong ELF spec ??? */
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff

#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

/*
 * Symbol table entry.
 */
typedef struct {
	uint32_t	st_name;
	uint32_t	st_value;
	uint32_t	st_size;
	uint8_t	st_info;
	uint8_t	st_other;
	uint16_t	st_shndx;
} elf_ste;

#define ELF32_ST_BIND(info)		((info) >> 4)
#define ELF32_ST_TYPE(info)		((info) & 0xf)
#define ELF32_ST_INFO(bind, type)	(((bind) << 4) +((type) & 0xf))

#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_LOPROC	13
#define STB_HIPROC	15

#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_LOPROC	13
#define STT_HIPROC	15



static inline
void* elf_ptr(const elf_header* hp, Elf32_Addr offs){
    return (void*)((Elf32_Addr)hp + offs);
}

static inline
elf_sh *elf_sheader(const elf_header *hdr) {
    return (elf_sh*)( elf_ptr(hdr, hdr->e_shoff) );
}

static inline
elf_sh *elf_section(const elf_header *hdr, int idx) {
    return &elf_sheader(hdr)[idx];
}

//sections names table operations
static inline
char *elf_str_table(const elf_header *hdr, unsigned sec_idx) {
    if(sec_idx == SHN_UNDEF) return 0;
    return (char *)hdr + elf_section(hdr, sec_idx)->sh_offset;
}

static inline
char *elf_secnames_table(const elf_header *hdr) {
    return elf_str_table(hdr, hdr->e_shstrndx);
}

static inline
char *elf_lookup_secname(const elf_header *hdr, int offset) {
    char *strtab = elf_secnames_table(hdr);
    if(strtab == 0) return 0;
    return strtab + offset;
}



#endif /* ELF_OBJECT_H */
