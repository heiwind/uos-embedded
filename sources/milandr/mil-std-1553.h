#ifndef __MIL1553_MILANDR_H__
#define __MIL1553_MILANDR_H__

#include <mil1553/mil1553-interface.h>
#include <mem/mem-queue.h>

// Реализация не потоко-защищённая

struct _milandr_mil1553_t
{
    mil1553if_t         milif;

    unsigned            mode;
    MIL_STD_1553B_t     *reg;
    int                 irq;
    mutex_t             tim_lock;
    TIMER_t             *tim_reg;
    int                 tim_irq;
    mem_pool_t          *pool;
    mem_queue_t         rxq;
    mil_slot_t          *cycle;
    mil_slot_t          *cur_slot;
    unsigned            nb_slots;
    unsigned            period_ms;
    mil_slot_desc_t     urgent_desc;
    uint16_t            *urgent_data;
    int                 is_running;
    
    // Статистика
    unsigned            nb_errors;
    unsigned            nb_lost;
};
typedef struct _milandr_mil1553_t milandr_mil1553_t;

// nb_rxq_msg и timer могут быть равными 0. nb_rxq_msg == 0 означает, что не нужно использовать приёмную очередь.
// Если nb_rxq_msg, то параметр pool не используется.
// timer может быть равен 0, если драйвер используется только в режиме ОУ, либо если в режиме КШ циклограмма должна
// всегда выполняться с минимальным периодом.
void milandr_mil1553_init(milandr_mil1553_t *mil, int port, mem_pool_t *pool, unsigned nb_rxq_msg, TIMER_t *timer);

void milandr_mil1553_init_pins(int port);

#endif
