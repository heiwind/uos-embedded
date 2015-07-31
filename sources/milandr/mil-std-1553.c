// ID: SPO-UOS-milandr-mil-std-1553_bc.c VER: 1.0.0
//
// История изменений:
//
// 1.0.0	Начальная версия
//
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include "mil-std-1553.h"

static void copy_to_rxq(milandr_mil1553_t *mil, mil_slot_desc_t slot)
{
    if (mem_queue_is_full(&mil->rxq)) {
        mil->nb_lost++;
    } else {
        unsigned wrc = slot.words_count;
        uint16_t *que_elem = mem_alloc_dirty(mil->pool, wrc + 8);
        if (!que_elem) {
            mil->nb_lost++;
            return;
        }
        mem_queue_put(&mil->rxq, que_elem);
        // Копируем номер слота
        memcpy(que_elem, &mil->cur_slot, 4);
        // Копируем дескриптор слота
        memcpy(que_elem + 2, &slot, 4);
        // Копируем данные слота
        arm_reg_t *preg = &mil->reg->DATA[slot.subaddr * MIL_SUBADDR_WORDS_COUNT];
        que_elem += 4;  // Область данных
        while (wrc) {
            *que_elem++ = *preg++;
            wrc--;
        }
    }
}

static void start_next_slot(milandr_mil1553_t *mil, mil_slot_desc_t slot, uint16_t *pdata)
{
    if (slot.transmit_mode == MIL_SLOT_BC_RT) {
        // Режим передачи КШ-ОУ
        mil->reg->CommandWord1 =
                // Количество слов выдаваемых данных
                MIL_STD_COMWORD_WORDSCNT_CODE(slot.words_count) |
                // Подадрес приёмника
                MIL_STD_COMWORD_SUBADDR_MODE(slot.subaddr) |
                // Адрес приёмника
                MIL_STD_COMWORD_ADDR(slot.addr);

        arm_reg_t *preg = &mil->reg->DATA[slot.subaddr * MIL_SUBADDR_WORDS_COUNT];
        if (pdata) {
            unsigned wrdc = slot.words_count;
            while (wrdc) {
                *preg++ = *pdata++;
                wrdc--;
            }
        }
    } else if (slot.transmit_mode == MIL_SLOT_RT_BC) {
        // Режим передачи ОУ-КШ
        mil->reg->CommandWord1 =
                // Количество слов принимаемых данных
                MIL_STD_COMWORD_WORDSCNT_CODE(slot.words_count) |
                // Подадрес источника
                MIL_STD_COMWORD_SUBADDR_MODE(slot.subaddr_src) |
                // Направление передачи: ОУ-КШ
                MIL_STD_COMWORD_RT_BC |
                // Адрес источника
                MIL_STD_COMWORD_ADDR(slot.addr_src);
    } else {
        // Режим передачи ОУ-ОУ
        mil->reg->CommandWord1 =
                // Количество слов выдаваемых данных
                MIL_STD_COMWORD_WORDSCNT_CODE(slot.words_count) |
                // Подадрес источника
                MIL_STD_COMWORD_SUBADDR_MODE(slot.subaddr_src) |
                // Адрес источника
                MIL_STD_COMWORD_ADDR(slot.addr_src);
        mil->reg->CommandWord2 =
                // Количество слов принимаемых данных
                MIL_STD_COMWORD_WORDSCNT_CODE(slot.words_count) |
                // Подадрес приёмника
                MIL_STD_COMWORD_SUBADDR_MODE(slot.subaddr) |
                // Направление передачи: ОУ-ОУ
                MIL_STD_COMWORD_RT_BC |
                // Адрес приёмника
                MIL_STD_COMWORD_ADDR(slot.addr);
    }

    // Инициировать передачу команды в канал в режиме КШ
    mil->reg->CONTROL |= MIL_STD_CONTROL_BCSTART;
}

static bool_t mil_handler(void *arg)
{
    milandr_mil1553_t *mil = arg;

    const unsigned int status = mil->reg->STATUS;

    if (status & MIL_STD_STATUS_ERR) {
        mil->nb_errors++;

    } else if (status & MIL_STD_STATUS_VALMESS) {
        if (mil->urgent_desc.reserve) {
            // Была передача вне очереди
            if (mil->pool && mil->urgent_desc.transmit_mode == MIL_SLOT_RT_BC)
                copy_to_rxq(mil, mil->urgent_desc);
        } else {
            mil_slot_desc_t slot = mil->cur_slot->desc;
            if (mil->pool && slot.transmit_mode == MIL_SLOT_RT_BC)
                copy_to_rxq(mil, slot);
        }
    }

    if (mil->urgent_desc.reserve) // Если была передача вне очереди, то сбрасываем дескриптор,
        mil->urgent_desc.raw = 0; // чтобы не начать её повторно
    else {
        mil->cur_slot = mil->cur_slot->next;
        if (mil->cur_slot == 0)
            mil->cur_slot = mil->cycle;
    }

    if (mil->urgent_desc.raw != 0) {    // Есть требование на выдачу вне очереди
        start_next_slot(mil, mil->urgent_desc, mil->urgent_data);
        mil->urgent_desc.reserve = 1;   // Признак того, что идёт передача вне очереди
    } else if (mil->cur_slot != mil->cycle || mil->tim_reg == 0 || mil->period_ms == 0) {
        start_next_slot(mil, mil->cur_slot->desc, mil->cur_slot->data);
    }

    arch_intr_allow(mil->irq);

    return 0;
}

static bool_t timer_handler(void *arg)
{
    milandr_mil1553_t *mil = arg;

    if (mil->urgent_desc.raw != 0) {
        start_next_slot(mil, mil->urgent_desc, mil->urgent_data);
        mil->urgent_desc.reserve = 1;           // Признак того, что идёт передача вне очереди
    } else if (mil->cur_slot == mil->cycle) {
        start_next_slot(mil, mil->cur_slot->desc, mil->cur_slot->data);
    }

    mil->tim_reg->TIM_STATUS = ARM_TIM_CNT_ZERO_EVENT;

    arch_intr_allow(mil->tim_irq);

    return 0;
}

static int mil_lock(mil1553if_t *_mil)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;
    mutex_lock(&_mil->mutex);
    mutex_lock(&mil->tim_lock);
    return MIL_ERR_OK;
}

static int mil_unlock(mil1553if_t *_mil)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;
    mutex_unlock(&mil->tim_lock);
    mutex_unlock(&_mil->mutex);
    return MIL_ERR_OK;
}

static int mil_set_mode(mil1553if_t *_mil, unsigned mode)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;
    // На ходу можно переключаться только между основным и резервным каналом
    // в режиме КШ.
    if (_mil->is_started(_mil)) {
        if (mil->mode & (MIL_MODE_BC_MAIN | MIL_MODE_BC_RSRV)) {
            mil_lock(_mil);
            if (mode & MIL_MODE_BC_MAIN) {
                // Отключить резервный, включить основной канал.
                mil->reg->CONTROL = (mil->reg->CONTROL & ~MIL_STD_CONTROL_TRB) | MIL_STD_CONTROL_TRA;
            } else if (mode & MIL_MODE_BC_RSRV) {
                // Отключить основной, включить резервный канал.
                mil->reg->CONTROL = (mil->reg->CONTROL & ~MIL_STD_CONTROL_TRA) | MIL_STD_CONTROL_TRB;
            } else {
                return MIL_ERR_BAD_ARG;
            }
            mil->mode = mode;
            mil_unlock(_mil);
            return MIL_ERR_OK;
        } else {
            return MIL_ERR_NOT_PERM;
        }
    } else {
        mil->mode = mode;
    }
        
    mil->mode = mode;
    return MIL_ERR_OK;
}

static int mil_start(mil1553if_t *_mil)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    if (_mil->is_started(_mil))
        return MIL_ERR_OK;
        
    unsigned control = MIL_STD_CONTROL_DIV(KHZ/1000);
        
    if (mil->mode & (MIL_MODE_BC_MAIN | MIL_MODE_BC_RSRV)) {
        control |= MIL_STD_CONTROL_MODE(MIL_STD_MODE_BC);
        if (mil->mode & MIL_MODE_BC_MAIN)
            control |= MIL_STD_CONTROL_TRA;
        else
            control |= MIL_STD_CONTROL_TRB;
            
        if (mil->tim_reg) {
            mil->tim_reg->TIM_ARR = mil->period_ms * KHZ;
            mil->tim_reg->TIM_IE = ARM_TIM_CNT_ARR_EVENT_IE;
            mil->tim_reg->TIM_CNTRL = ARM_TIM_CNT_EN;
        }
        mil->reg->INTEN = MIL_STD_INTEN_VALMESSIE | MIL_STD_INTEN_ERRIE;
        mil->reg->CONTROL = control;

        mil->cur_slot = mil->cycle;
        if (mil->urgent_desc.raw != 0) {
            mil->urgent_desc.reserve = 1;
            start_next_slot(mil, mil->cur_slot->desc, mil->cur_slot->data);
        } else {
            start_next_slot(mil, mil->urgent_desc, mil->urgent_data);
        }
    } else if (mil->mode & MIL_MODE_RT) {
        control |= MIL_STD_CONTROL_MODE(MIL_STD_MODE_RT);
        control |= MIL_STD_CONTROL_ADDR(mil->mode & MIL_MODE_RT_ADDR_MASK) | MIL_STD_CONTROL_TRA | MIL_STD_CONTROL_TRB;
        mil->reg->StatusWord1 = MIL_STD_STATUS_ADDR_OU(mil->mode & MIL_MODE_RT_ADDR_MASK);
        mil->reg->INTEN = MIL_STD_INTEN_RFLAGNIE | MIL_STD_INTEN_VALMESSIE | MIL_STD_INTEN_ERRIE;
        mil->reg->CONTROL = control;
    } else {
        return MIL_ERR_NOT_SUPP;
    }

    // debug
    ARM_GPIOB->DATA = 4;
    
    mil->is_running = 1;
    return MIL_ERR_OK;
}

static int mil_stop(mil1553if_t *_mil)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    mil->reg->INTEN = 0;
    mil->reg->CONTROL = MIL_STD_CONTROL_MR;
    if (mil->tim_reg) {
        mil->tim_reg->TIM_IE = 0;
        mil->tim_reg->TIM_CNTRL = 0;
    }
    
    mil->is_running = 0;
    return MIL_ERR_OK;
}

static int mil_is_started(mil1553if_t *_mil)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    return mil->is_running;
}

static int mil_read_subaddr(mil1553if_t *_mil, int subaddr, void *data, int nb_words)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    uint16_t *pdata = data;
    arm_reg_t *preg = &mil->reg->DATA[subaddr * MIL_SUBADDR_WORDS_COUNT];
    int i;
    
    mil_lock(_mil);
    for (i = 0; i < nb_words; ++i)
        *pdata++ = *preg++;
    mil_unlock(_mil);

    return MIL_ERR_OK;
}

static int mil_write_subaddr(mil1553if_t *_mil, int subaddr, void *data, int nb_words)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    uint16_t *pdata = data;
    arm_reg_t *preg = &mil->reg->DATA[subaddr * MIL_SUBADDR_WORDS_COUNT];
    int i;
    
    mil_lock(_mil);
    for (i = 0; i < nb_words; ++i)
        *preg++ = *pdata++;
    mil_unlock(_mil);

    return MIL_ERR_OK;
}

static int mil_bc_set_cyclogram(mil1553if_t *_mil, mil_slot_t *head, unsigned nb_slots)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    if (_mil->is_started(_mil))
        return MIL_ERR_NOT_PERM;

    mil->cycle = head;
    mil->nb_slots = nb_slots;
    return MIL_ERR_OK;
}

static int mil_bc_set_period(mil1553if_t *_mil, unsigned period_ms)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    if (_mil->is_started(_mil))
        return MIL_ERR_NOT_PERM;

    mil->period_ms = period_ms;

    return MIL_ERR_OK;
}


static int mil_bc_urgent_send(mil1553if_t *_mil, mil_slot_desc_t descr, void *data)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    mil_lock(_mil);
    mil->urgent_desc = descr;
    mil->urgent_desc.reserve = 0;
    mil->urgent_data = data;
    mil_unlock(_mil);
    return MIL_ERR_OK;
}


void milandr_mil1553_init(milandr_mil1553_t *_mil, int port, mem_pool_t *pool, unsigned nb_rxq_msg, TIMER_t *timer)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    if (port == 0) {
        mil->reg = ARM_MIL_STD_1553B1;
        mil->irq = MIL_STD_1553B1_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_MIL_STD_1553B1;
    } else {
        mil->reg = ARM_MIL_STD_1553B2;
        mil->irq = MIL_STD_1553B2_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_MIL_STD_1553B2;
    }

    // Разрешение тактовой частоты на контроллер ГОСТ Р52070-2003
    ARM_RSTCLK->ETH_CLOCK |= ARM_ETH_CLOCK_MAN_EN;

    mil->pool = pool;
    if (nb_rxq_msg)
        mem_queue_init(&mil->rxq, pool, nb_rxq_msg);
    
    mil->tim_reg = timer;
    if (timer == ARM_TIMER1) {
        mil->tim_irq = TIMER1_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_TIMER1;
        ARM_RSTCLK->TIM_CLOCK = ARM_TIM_CLOCK_BRG1(0) | ARM_TIM_CLOCK_EN1;
    } else if (timer == ARM_TIMER2) {
        mil->tim_irq = TIMER2_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_TIMER2;
        ARM_RSTCLK->TIM_CLOCK = ARM_TIM_CLOCK_BRG2(0) | ARM_TIM_CLOCK_EN2;
    } else if (timer == ARM_TIMER3) {
        mil->tim_irq = TIMER3_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_TIMER3;
        ARM_RSTCLK->TIM_CLOCK = ARM_TIM_CLOCK_BRG3(0) | ARM_TIM_CLOCK_EN3;
    } else if (timer == ARM_TIMER4) {
        mil->tim_irq = TIMER4_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_TIMER4;
        ARM_RSTCLK->UART_CLOCK = ARM_UART_CLOCK_TIM4_BRG(0) | ARM_UART_CLOCK_TIM4_EN;
    }
    
    mil->cycle = 0;
    mil->cur_slot = 0;
    mil->is_running = 0;
    mil->mode = MIL_MODE_UNDEFINED;
    mil->period_ms = 0;
    
    mil->milif.set_mode = mil_set_mode;
    mil->milif.start = mil_start;
    mil->milif.stop = mil_stop;
    mil->milif.is_started = mil_is_started;
    mil->milif.read_subaddr = mil_read_subaddr;
    mil->milif.write_subaddr = mil_write_subaddr;
    mil->milif.lock = mil_lock;
    mil->milif.unlock = mil_unlock;
    mil->milif.bc_set_cyclogram = mil_bc_set_cyclogram;
    mil->milif.bc_set_period = mil_bc_set_period;
    mil->milif.bc_urgent_send = mil_bc_urgent_send;

    mutex_attach_irq(&mil->milif.mutex, mil->irq, mil_handler, mil);
    if (timer) {
        timer->TIM_CNTRL = 0;
        timer->TIM_CNT = 0;
        timer->TIM_PSG = 0;

        mutex_attach_irq(&mil->tim_lock, mil->tim_irq, timer_handler, mil);
    }
}

void milandr_mil1553_init_pins(int port)
{
    if ((port & 1) == 0)
    {
        milandr_init_pin(ARM_GPIOC, PORT_C, 13, FUNC_MAIN);  // PRMA+
        milandr_init_pin(ARM_GPIOC, PORT_C, 14, FUNC_MAIN);  // PRMA-
        milandr_init_pin(ARM_GPIOD, PORT_D,  1, FUNC_MAIN);  // PRDA+
        milandr_init_pin(ARM_GPIOD, PORT_D,  2, FUNC_MAIN);  // PRDA-
        milandr_init_pin(ARM_GPIOD, PORT_D,  5, FUNC_MAIN);  // PRD_PRM_A

        milandr_init_pin(ARM_GPIOC, PORT_C, 15, FUNC_MAIN);  // PRMB+
        milandr_init_pin(ARM_GPIOD, PORT_D,  0, FUNC_MAIN);  // PRMB-
        milandr_init_pin(ARM_GPIOD, PORT_D,  3, FUNC_MAIN);  // PRDB+
        milandr_init_pin(ARM_GPIOD, PORT_D,  4, FUNC_MAIN);  // PRDB-
        milandr_init_pin(ARM_GPIOD, PORT_D,  6, FUNC_MAIN);  // PRD_PRM_B
    }
    else
    {
        milandr_init_pin(ARM_GPIOF, PORT_F,  3, FUNC_MAIN);  // PRMC+
        milandr_init_pin(ARM_GPIOF, PORT_F,  4, FUNC_MAIN);  // PRMC-
        milandr_init_pin(ARM_GPIOF, PORT_F,  7, FUNC_MAIN);  // PRDC+
        milandr_init_pin(ARM_GPIOF, PORT_F,  8, FUNC_MAIN);  // PRDC-
        milandr_init_pin(ARM_GPIOF, PORT_F, 11, FUNC_MAIN);  // PRD_PRM_C

        milandr_init_pin(ARM_GPIOF, PORT_F,  5, FUNC_MAIN);  // PRMD+
        milandr_init_pin(ARM_GPIOF, PORT_F,  6, FUNC_MAIN);  // PRMD-
        milandr_init_pin(ARM_GPIOF, PORT_F,  9, FUNC_MAIN);  // PRDD+
        milandr_init_pin(ARM_GPIOF, PORT_F, 10, FUNC_MAIN);  // PRDD-
        milandr_init_pin(ARM_GPIOF, PORT_F, 12, FUNC_MAIN);  // PRD_PRM_D
    }
}

