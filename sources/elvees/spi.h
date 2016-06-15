#ifndef __SPI_MASTER_ELVEES_H__
#define __SPI_MASTER_ELVEES_H__

#include <kernel/uos.h>
#include <spi/spi-master-interface.h>
#include <stdint.h> 

#ifndef SPI_DMA_BUFSZ
#define SPI_DMA_BUFSZ   4096
#endif

#define SPI_MOSI_OUT        (1 << 8)
#define SPI_MISO_OUT        (1 << 9)
#define SPI_SS0_OUT         (1 << 10)
#define SPI_SS1_OUT         (1 << 11)
#define SPI_TSCK_OUT        (1 << 12)
#define SPI_RSCK_OUT        (1 << 13)

#define SPI_SS0_ACTIVE_HIGH	(1 << 14)
#define SPI_SS1_ACTIVE_HIGH	(1 << 15)



#ifdef __cplusplus
extern "C" {
#endif

struct _elvees_spim_t {
    spimif_t        spimif;
    mutex_t         irq_lock;
    unsigned        port;
    unsigned        last_freq;
    unsigned        last_bits;
    unsigned        last_mode;
#ifndef SPI_NO_DMA
    uint8_t         dma_txbuf[SPI_DMA_BUFSZ] __attribute__((aligned(8)));
    uint8_t         dma_rxbuf[SPI_DMA_BUFSZ] __attribute__((aligned(8)));
#endif
};
typedef struct _elvees_spim_t elvees_spim_t;

int spim_init(elvees_spim_t *spi, unsigned port, unsigned io_mode);



#ifdef __cplusplus
}
#endif

#endif
