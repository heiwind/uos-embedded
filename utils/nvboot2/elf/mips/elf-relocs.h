#if !defined(ELF_RELOCS_H)
#define ELF_RELOCS_H

#include <stdint.h>


/* Manipulating relocation entry's type and binding attributes in r_info field */
#define ELF32_R_SYM(i)     ((i)>>8)
#define ELF32_R_TYPE(i)     ((uint8_t)(i))
#define ELF32_R_INFO(b,t)   (((b)<<8)+((uint8_t)(t)))

/* MIPS-related relocation types
 */
typedef enum {
      R_MIPS_NONE             = 0
    , R_MIPS_16               = 0x01
    , R_MIPS_32               = 0x02
    , R_MIPS_REL32            = 0x03
    , R_MIPS_26               = 0x04
    , R_MIPS_HI16             = 0x05
    , R_MIPS_LO16             = 0x06
    , R_MIPS_GPREL16          = 0x07
    , R_MIPS_LITERAL          = 0x08
    , R_MIPS_GOT16            = 0x09
    , R_MIPS_PC16             = 0x0a
    , R_MIPS_CALL16           = 0x0b
    , R_MIPS_GPREL32          = 0x0c
} R_MIPS_type;

typedef struct {
    Elf32_Addr  offs;
    Elf32_Word  info;   //
    Elf32_Word  addend;
} elf_rel;
typedef elf_rel elf_rela;

//* \return 0  - ok
//* \return -1 - fails
int elf_relocate(const elf_header *ehp, const elf_sh *eshp);



#endif //ELF_RELOCS_H
