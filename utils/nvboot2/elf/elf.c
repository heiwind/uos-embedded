#include <runtime/lib.h>
#include "elf.h"
#include "elf-object.h"
#include <mips/elf-relocs.h>
#include <stdint.h>

#ifndef NVBOOT_SILENT_MODE
#define elf_puts(x) debug_puts(x)
#define elf_printf(...) debug_printf(__VA_ARGS__)
#else
#define    elf_puts(x)
#define    elf_printf(...)
#endif


void elf_section_show(const elf_sh *eshp
                    , const char* prefix
                    , const char* nametab
                    );

void* elf_process(const elf_process_handle *task)
{
    elf_header *ehp;
	//struct elf_ph *ephp;
	elf_sh *eshp;
	int i;
	uint32_t* ptr;
	//char *shstrndx_offset;
	void* pElf;
	pElf = task->object;

    ehp=(elf_header *)pElf;
	if (!ehp)
		return (0);

	if (ehp->e_magic != ELF_MAGIC)
	{
		elf_puts("Wrong magic.\n");
		return (0);
	}

	if (ehp->e_type != ET_EXEC)
    {
        elf_puts("Not exec file.\n");
        return 0;
    }
	if (ehp->e_machine != EM_MIPS)
    {
		elf_puts("Not MIPS.\n");
        return 0;
    }
	if (ehp->e_ehsize < sizeof(elf_header))
    {
		elf_puts("Invalid ELF header size.\n");
        return 0;
    }
	if (ehp->e_phentsize < sizeof(elf_ph))
    {
		elf_puts("Invalid program header size.\n");
        return 0;
    }

	if (ehp->e_phoff)
		;//ephp = (struct elf_ph *)((char *)ehp + ehp->e_phoff);
	else {
		elf_puts("No ELF program header\n");
		return (0);
	}

    eshp = elf_sheader(ehp);
	if (eshp == 0) {
	    elf_puts("No ELF section header\n");
		return (0);
	}

	elf_printf("entry point: $%#x\n", ehp->e_entry);

	
    const char* nametab = elf_secnames_table(ehp);
	/*
	 * Load sections to memory.
	 */
	//shstrndx_offset = (char *)((char *)ehp + eshp[ehp->e_shstrndx].sh_offset);
	for (i = 0; i < ehp->e_shnum; i++, eshp++)
	{
		if (eshp->sh_addr)
		{
            ptr=(uint32_t *)eshp->sh_addr;
			if (eshp->sh_type == SHT_NOBITS)
			{
	            elf_section_show(eshp, "fill: ", nametab);
			    if (0 == task->onfill(ptr, 0, eshp->sh_size)){
			        elf_puts("...failed\n");
			        return 0;
			    }
			}
			else
            if (eshp->sh_type == SHT_PROGBITS)
			{
                elf_section_show(eshp, "copy: ", nametab);
	            if (0 == task->oncopy(ptr
	                                  , (unsigned long *)((unsigned long)ehp+eshp->sh_offset)
	                                  , eshp->sh_size
	                                  )
	               )
	            {
                    elf_puts("...failed\n");
                    return 0;
	            }
			}
            else
            if (eshp->sh_type == SHT_REL){
                if (elf_relocate(ehp, eshp))
                    elf_section_show(eshp, "ok reloc: ", nametab);
                else {
                    elf_section_show(eshp, " can`t reloc: ", nametab);
                    return 0;
                }
            }
            else {
                unsigned t = eshp->sh_type;
                const unsigned allowed_types = (1<<SHT_NULL)
                                    | (1<<SHT_NOTE)
                                    | (1<<SHT_HASH)
                                    ;
                if (t < 31)
                if (((1L << t) & allowed_types) == 0) {
                    elf_section_show(eshp, "unsupported!:", nametab);
                    return 0;
                }
                elf_section_show(eshp, "skip:", nametab);
            }
		}
	}

    elf_puts("ELF load finished\n");
	return (void*)ehp->e_entry;
}

void elf_section_show(const elf_sh *eshp
                    , const char* prefix
                    , const char* nametab
                    )
{
    const char* name = nametab;
    if (name != 0)
        name += eshp->sh_name;
    elf_printf("%s %s type $%x at $%#x [$%#x]\n"
            , prefix, name, eshp->sh_type
            , eshp->sh_addr, eshp->sh_size
            );
}

