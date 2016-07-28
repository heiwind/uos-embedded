#ifndef __M25PXX_H__
#define __M25PXX_H__

#include <flash/flash-interface.h>
#include <spi/spi-master-interface.h>
#include <stdint.h> 

#ifdef __cplusplus
extern "C" {
#endif


#ifndef FLASH_M25_VALIDATE
// this is turn on flash jedec manufacter id validation on connect
// TODO valid jedec codes hardcoded with m25_connect code!
#define FLASH_M25_VALIDATE 0
#endif

struct _m25pxx_t
{
    flashif_t       flashif;
    spimif_t       *spi;
    spi_message_t   msg;
    uint8_t         databuf[4];
};
typedef struct _m25pxx_t m25pxx_t;

void m25pxx_init(m25pxx_t *m, spimif_t *s, unsigned freq, unsigned mode);



#ifdef __cplusplus
}
#endif

#endif
