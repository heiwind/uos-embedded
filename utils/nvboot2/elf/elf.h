#if !defined(ELF_H)
#define ELF_H

#include <stdint.h>



/*
 * Load ELF file to memory.
 * \return  entry point on sucess, zero otherwise.
 */
typedef uint32_t* (*elf_filler)(uint32_t * target, uint32_t x, unsigned len);
typedef uint32_t* (*elf_copyer)(uint32_t * target, const uint32_t* from, unsigned len);

typedef struct {
    void*      object;
    elf_filler onfill;
    elf_copyer oncopy;
} elf_process_handle;

void* elf_process(const elf_process_handle *task);



#endif //ELF_H
