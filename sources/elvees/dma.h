#ifndef __DMA_ELVEES_H__
#define __DMA_ELVEES_H__

#include <dma/dma-interface.h>

#ifndef SPI_DMA_BUFSZ
#define SPI_DMA_BUFSZ   4096
#endif

#define SPI_MOSI_OUT        (1 << 8)
#define SPI_MISO_OUT        (1 << 9)
#define SPI_SS0_OUT         (1 << 10)
#define SPI_SS1_OUT         (1 << 11)
#define SPI_TSCK_OUT        (1 << 12)
#define SPI_RSCK_OUT        (1 << 13)

struct _elvees_mem_dma_t {
    dmaif_t         dmaif;
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
typedef struct _elvees_mem_dma_t elvees_mem_dma_t;

int elvees_mem_dma_init(elvees_mem_dma_t *dma, unsigned dma_chan);

#endif
