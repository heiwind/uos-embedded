#ifndef __MIL1553_MILANDR_H__
#define __MIL1553_MILANDR_H__

#include <mil1553/mil1553-interface.h>
#include <mem/mem-queue.h>

#define STATUS_ITEMS_SIZE	32

// Реализация не потоко-защищённая

typedef struct _milandr_mil1553_t
{
    mil1553if_t         milif;

    unsigned            mode;
    unsigned            addr_self; // только для RT
    MIL_STD_1553B_t     *reg;
    int                 irq;
    mutex_t             tim_lock;
    TIMER_t             *tim_reg;
    int                 tim_irq;
    mem_pool_t          *pool;
    mem_queue_t         rxq;
    mil_slot_t          *cyclogram;
    mil_slot_t          *cur_slot;
    unsigned            nb_slots;
    unsigned            period_ms;
    mil_slot_desc_t     urgent_desc;
    uint16_t            urgent_data[MIL_SUBADDR_WORDS_COUNT];
    int                 is_running;
    
    uint16_t rx_buf[MIL_DATA_LENGTH];	// только для RT
    uint16_t tx_buf[MIL_DATA_LENGTH];   // только для RT

    // Статистика
    unsigned            nb_lost;
    unsigned            nb_errors;
} milandr_mil1553_t;


typedef struct _status_item_t {
	volatile uint32_t status;
	volatile uint16_t command_word_1;
	volatile uint16_t msg;
	volatile uint32_t time_stamp;
	volatile uint32_t done;
} status_item_t;

extern status_item_t status_array[STATUS_ITEMS_SIZE];
extern int read_idx;

// nb_rxq_msg и timer могут быть равными 0. nb_rxq_msg == 0 означает, что не нужно использовать приёмную очередь.
// Если nb_rxq_msg, то параметр pool не используется.
// timer может быть равен 0, если драйвер используется только в режиме ОУ, либо если в режиме КШ циклограмма должна
// всегда выполняться с минимальным периодом.
void milandr_mil1553_init(milandr_mil1553_t *mil, int port, mem_pool_t *pool, unsigned nb_rxq_msg, TIMER_t *timer);

void milandr_mil1553_init_pins(int port);

// временно
void mil_std_1553_bc_handler(milandr_mil1553_t *mil, const unsigned short status, const unsigned short comWrd1, const unsigned short msg);
// временно
void mil_std_1553_rt_handler(milandr_mil1553_t *mil, const unsigned short status, const unsigned short comWrd1, const unsigned short msg);
#endif
