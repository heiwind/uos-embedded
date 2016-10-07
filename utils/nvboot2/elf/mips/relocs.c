#include <elf-object.h>
#include "elf-relocs.h"

int elf_relocate(const elf_header *ehp, const elf_sh *eshp){
    elf_rel* rel = (elf_rel*)elf_ptr(ehp, eshp->sh_offset);
    elf_rel* rellimit = (elf_rel*)((char*)rel + eshp->sh_size);
    for (; rel < rellimit; rel++){
        R_MIPS_type RelaType = ELF32_R_TYPE(rel->info);
        if (RelaType == R_MIPS_NONE)
            continue;
        return -1;
    }
    return 0;
}

