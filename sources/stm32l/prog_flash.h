#ifndef __STM32L_PROG_FLASH_H__
#define __STM32L_PROG_FLASH_H__

#include <flash/flash-interface.h>

struct _stm32l_prog_flash_t
{
    flashif_t       flashif;
    
    uint32_t		start_addr;
};
typedef struct _stm32l_prog_flash_t stm32l_prog_flash_t;

void stm32l_prog_flash_init(stm32l_prog_flash_t *m, uint32_t start_addr, unsigned size);

#endif // __STM32L_PROG_FLASH_H__
