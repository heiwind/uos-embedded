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

#define RT_DEBUG 			0
#define BC_DEBUG 			0

#define LEFT_LED	4
#define RIGHT_LED	2
#define TOP_LED		1

#define TOGLE(n)	((ARM_GPIOB->DATA & (n))?(ARM_GPIOB->DATA &= ~(n)):(ARM_GPIOB->DATA |= (n)))

static void copy_to_rxq(milandr_mil1553_t *mil, mil_slot_desc_t slot)
{
    if (mem_queue_is_full(&mil->rxq)) {
        mil->nb_lost++;
    } else {
        unsigned wrc = (slot.words_count == 0 ? 32 : slot.words_count);
        uint16_t *que_elem = mem_alloc_dirty(mil->pool, 2*wrc + 8);
        if (!que_elem) {
            mil->nb_lost++;
            return;
        }
        mem_queue_put(&mil->rxq, que_elem);
        // Номер слота всегда 0
        *que_elem = 0;
        *(que_elem + 1) = 0;
        // Копируем дескриптор слота
        memcpy(que_elem + 2, &slot, 4);
        // Копируем данные слота
        arm_reg_t *preg = &mil->reg->DATA[slot.subaddr * MIL_SUBADDR_WORDS_COUNT];


#if BC_DEBUG
        uint16_t *que_elem_debug = que_elem + 4; // Область данных
        int wordscount = wrc;
#endif

        que_elem += 4;  // Область данных
        while (wrc) {
            *que_elem++ = *preg++;
            wrc--;
        }

#if BC_DEBUG

        int i;
        debug_printf("\n");
        debug_printf("wc=%d\n", wordscount);
        for (i=0;i<wordscount;i++) {
        	debug_printf("%04x\n", *que_elem_debug++);
        }
        debug_printf("\n");
#endif

    }
}

static void copy_to_rt_rxq(milandr_mil1553_t *mil, uint8_t subaddr, uint8_t wordscount)
{
    if (mem_queue_is_full(&mil->rt_rxq)) {
        mil->nb_lost++;
    } else {
        uint16_t *que_elem = mem_alloc_dirty(mil->pool, 2*wordscount + 4);
        if (!que_elem) {
            mil->nb_lost++;
            return;
        }
        mem_queue_put(&mil->rt_rxq, que_elem);
        *que_elem = wordscount;
        *(que_elem + 1) = subaddr;
        // Копируем данные слота
        arm_reg_t *preg = &mil->reg->DATA[subaddr * MIL_SUBADDR_WORDS_COUNT];
        que_elem += 2;  // Область данных

#if RT_DEBUG
        uint16_t *que_elem_debug = que_elem;
        int wrc = wordscount;
        debug_printf("wc=%d\n", wordscount);
#endif

        while (wordscount) {
            *que_elem++ = *preg++;
            wordscount--;
        }

#if RT_DEBUG
        int i;
        debug_printf("\n");
        for (i=0;i<wrc;i++) {
        	debug_printf("%04x\n", *que_elem_debug++);
        }
        debug_printf("\n");
#endif
    }
}

static void start_slot(milandr_mil1553_t *mil, mil_slot_desc_t slot, uint16_t *pdata)
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
            unsigned wrdc = (slot.words_count == 0 ? 32 : slot.words_count);
#if BC_DEBUG
        uint16_t *que_elem_debug = pdata;
        int wordscount = wrdc;
#endif
            while (wrdc) {
                *preg++ = *pdata++;
                wrdc--;
            }
#if BC_DEBUG
        int i;
        debug_printf("\n");
        debug_printf("startslot wc=%d\n", wordscount);
        for (i=0;i<wordscount;i++) {
        	debug_printf("(%02d) %04x\n", i, *que_elem_debug++);
        }
        debug_printf("\n");
#endif
        }
    } else if (slot.transmit_mode == MIL_SLOT_RT_BC) {
        // Режим передачи ОУ-КШ
        mil->reg->CommandWord1 =
                // Количество слов принимаемых данных
                MIL_STD_COMWORD_WORDSCNT_CODE(slot.words_count) |
                // Подадрес источника
                MIL_STD_COMWORD_SUBADDR_MODE(slot.subaddr) |
                // Направление передачи: ОУ-КШ
                MIL_STD_COMWORD_RT_BC |
                // Адрес источника
                MIL_STD_COMWORD_ADDR(slot.addr);
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

void mil_std_1553_rt_handler(milandr_mil1553_t *mil, const unsigned short status, const unsigned short comWrd1, const unsigned short msg)
{
	if (status & MIL_STD_STATUS_ERR) {
	    mil->nb_errors++;
	    return;
	}

    // Подадрес из командного слова 1
    const unsigned char subaddr = (comWrd1 & MIL_STD_COMWORD_SUBADDR_MODE_MASK) >> MIL_STD_COMWORD_SUBADDR_MODE_SHIFT;

    // Код команды
    const unsigned int cmdCode = (comWrd1 & MIL_STD_COMWORD_WORDSCNT_CODE_MASK) >> MIL_STD_COMWORD_WORDSCNT_CODE_SHIFT;

    // Количество слов данных в сообщении
    const unsigned char wordsCount = (cmdCode == 0 ? 32 : cmdCode);

    unsigned short answerWord = MIL_STD_STATUS_ADDR_OU(mil->addr_self);

    mil1553_lock(&mil->milif);

    switch (msg)
    {
    // приём данных от КШ (КШ-ОУ), формат сообщения 7
    case MSG_DATARECV__BC_RT__GROUP:
        answerWord |= MIL_STD_STATUS_GROUP_COMMAND_RECEIVED;
    // приём данных от КШ (КШ-ОУ), формат сообщения 1
    case MSG_DATARECV__BC_RT__SINGLE:
        // Получено достоверное слово из канала
        if ((status & MIL_STD_STATUS_RFLAGN) != 0) {
            // Установить ответное слово
            mil->reg->StatusWord1 = answerWord;
        }
        if ((status & MIL_STD_STATUS_VALMESS) != 0) {
            copy_to_rt_rxq(mil, subaddr, wordsCount);
        }

#if RT_DEBUG
        if ((status & MIL_STD_STATUS_RFLAGN) != 0) {
            int i;
            const int index = MIL_STD_SUBADDR_WORD_INDEX(subaddr);
            debug_printf("MSG_DATARECV__BC_RT__SINGLE n=%u\n", wordsCount);
//            for (i=0; i<wordsCount; ++i)
//                debug_printf(" %x", mil->rx_buf[index + i]);
//            debug_printf("\n");
        }
#endif
        break;
    // приём данных от ОУ (ОУ-ОУ), формат сообщения 8
    case MSG_DATARECV__RT_RT__GROUP:
        answerWord |= MIL_STD_STATUS_GROUP_COMMAND_RECEIVED;
    // приём данных от ОУ (ОУ-ОУ), формат сообщения 3
    case MSG_DATARECV__RT_RT__SINGLE:
        // Получено достоверное слово из канала
        if ((status & MIL_STD_STATUS_RFLAGN) != 0)
        {
            // Установить ответное слово
            mil->reg->StatusWord1 = answerWord;
#if RT_DEBUG
            int i;
            int index = MIL_STD_SUBADDR_WORD_INDEX(subaddr);
            debug_printf("STATUS(F3,8 in)=%x", status);
            //            for (i=0; i<wordsCount; ++i)
            //                debug_printf(" %x", mil->rx_buf[index + i]);
            //            debug_printf("\n");

#endif

        }
        if ((status & MIL_STD_STATUS_VALMESS) != 0) {
            copy_to_rt_rxq(mil, subaddr, wordsCount);
        }
        break;
    // передача данных в КШ (ОУ-КШ), формат сообщения 2
    case MSG_DATASEND__RT_BC__SINGLE:
        // Получено достоверное слово из канала
        if ((status & MIL_STD_STATUS_RFLAGN) != 0)
        {
            // Установить ответное слово
            mil->reg->StatusWord1 = answerWord;

//            int index = MIL_STD_SUBADDR_WORD_INDEX(subaddr);

#if RT_DEBUG
            int i;
            debug_printf("STATUS(F2)=%x, dataToSend =", status);
            for (i=0; i<wordsCount; ++i)
                debug_printf(" %x", mil->tx_buf[index + i]);
            debug_printf("\n");
#endif

//			for (i=0; i<wordsCount; ++i)
//				mil->reg->DATA[index + i] = mil->tx_buf[index + i];

        }
        break;
    // передача данных в ОУ (ОУ-ОУ), формат сообщения 8
    case MSG_DATASEND__RT_RT__GROUP:
        answerWord |= MIL_STD_STATUS_GROUP_COMMAND_RECEIVED;
    // передача данных в ОУ (ОУ-ОУ), формат сообщения 3
    case MSG_DATASEND__RT_RT__SINGLE:
        // Получено достоверное слово из канала
        if ((status & MIL_STD_STATUS_RFLAGN) != 0)
        {
            // Установить ответное слово
            mil->reg->StatusWord1 = answerWord;

#if RT_DEBUG
            debug_printf("STATUS(F3,8 out)=%x\n", status);
#endif

            // Подадрес из командного слова 2
 //           const unsigned char subaddr2 = (mil->reg->CommandWord2 & MIL_STD_COMWORD_SUBADDR_MODE_MASK) >> MIL_STD_COMWORD_SUBADDR_MODE_SHIFT;

            // Количество слов данных в сообщении
//            int wcField2 = (mil->reg->CommandWord2 & MIL_STD_COMWORD_WORDSCNT_CODE_MASK) >> MIL_STD_COMWORD_WORDSCNT_CODE_SHIFT;
//            const unsigned char wordsCount2 = wcField2 == 0 ? 32 : wcField2;

//            int i;
//            int index = MIL_STD_SUBADDR_WORD_INDEX(subaddr2);

//            for (i=0; i<wordsCount2; ++i)
//            	mil->reg->DATA[index + i] = mil->tx_buf[index + i];
        }
        break;
    // команда управления 0-15 от КШ без слов данных, формат сообщения 9
    case MSG_CMD_WITH_NODATA__0_0xf__GROUP:
        answerWord |= MIL_STD_STATUS_GROUP_COMMAND_RECEIVED;
    // команда управления 0-15 от КШ без слов данных, формат сообщения 4
    case MSG_CMD_WITH_NODATA__0_0xf__SINGLE:
        // Получено достоверное слово из канала
        if ((status & MIL_STD_STATUS_RFLAGN) != 0)
        {
            // неподдерживаемые команды (резерв)
            if (cmdCode >= 9 && cmdCode <= 0xf)
            {
                answerWord |= MIL_STD_STATUS_MSG_ERR;

                // Установить ответное слово
                mil->reg->StatusWord1 = answerWord;
            }
            else
            {
                // Установить ответное слово
                mil->reg->StatusWord1 = answerWord;

#if RT_DEBUG
                debug_printf("STATUS(F4,9)=%x, com=%x\n", status, cmdCode);
#endif

                // Команда "блокировать передатчик"
                if (cmdCode == CMD_LockSender)
                {
                    if ((status & MIL_STD_STATUS_RCVA) != 0)
                    {
                        mil->reg->CONTROL &= ~MIL_STD_CONTROL_TRA;
                    }
                    else if ((status & MIL_STD_STATUS_RCVB) != 0)
                        mil->reg->CONTROL &= ~MIL_STD_CONTROL_TRB;
                }
                // Команда "разблокировать передатчик"
                else if (cmdCode == CMD_UnlockSender)
                {
                    if ((status & MIL_STD_STATUS_RCVA) != 0)
                    {
                        mil->reg->CONTROL |= MIL_STD_CONTROL_TRA;
                    }
                    else if ((status & MIL_STD_STATUS_RCVB) != 0)
                        mil->reg->CONTROL |= MIL_STD_CONTROL_TRB;
                }
                // Команда "установить ОУ в исходное состояние"
                else if (cmdCode == CMD_SetRtInitialState)
                {

                }
            }
        }
        break;
    // команда управления 16-31 от КШ, ожидается слово данных (не групповая), формат сообщения 5
    case MSG_CMD_WITH_NODATA__0x10_0x1f__DATAWORD_EXPECTED__SINGLE:
        // Получено достоверное слово из канала
        if ((status & MIL_STD_STATUS_RFLAGN) != 0)
        {
            // неподдерживаемые команды (резерв)
            if (cmdCode >= 0x16)
            {
                answerWord |= MIL_STD_STATUS_MSG_ERR;

                // Установить ответное слово
                mil->reg->StatusWord1 = answerWord;
            }
            else
            {
                // Установить ответное слово
                mil->reg->StatusWord1 = answerWord;

#if RT_DEBUG
                debug_printf("STATUS(F5)=%x\n", status);
#endif

                // (!) to be implemented
            }
        }
        break;
    // команда управления 16-31 от КШ со словом данных, формат сообщения 10
    case MSG_CMD_WITH_DATA__0x10_0x1f__GROUP:
        answerWord |= MIL_STD_STATUS_GROUP_COMMAND_RECEIVED;
    // команда управления 16-31 от КШ со словом данных, формат сообщения 6
    case MSG_CMD_WITH_DATA__0x10_0x1f__SINGLE:
        // Получено достоверное слово из канала
        if ((status & MIL_STD_STATUS_RFLAGN) != 0)
        {
            // неподдерживаемые команды (резерв)
            if (cmdCode >= 0x16)
            {
                answerWord |= MIL_STD_STATUS_MSG_ERR;

                // Установить ответное слово
                mil->reg->StatusWord1 = answerWord;
            }
            else
            {
                // Установить ответное слово
                mil->reg->StatusWord1 = answerWord;

#if RT_DEBUG
                debug_printf("STATUS(F6,10)=%x\n", status);
#endif

                // Команда "синхронизация (со словом данных)"
                if (cmdCode == CMD_SynchronizeWithDataWord)
                {
                    // Заменить собственный адрес ОУ значением, которое получено в команде от КШ.
                    const unsigned short newCtrl = (mil->reg->CONTROL & ~MIL_STD_CONTROL_ADDR_MASK) | ((mil->reg->ModeData & 0x1F)<<MIL_STD_CONTROL_ADDR_SHIFT);
                    mil->reg->CONTROL = newCtrl;
                }
            }
        }
        break;
    } // switch (msg)
    mil1553_unlock(&mil->milif);
} // static void mil_std_1553_rt_handler(milandr_mil1553_t *mil, const unsigned short status, const unsigned short comWrd1, const unsigned short msg)

void mil_std_1553_bc_handler(milandr_mil1553_t *mil, const unsigned short status, const unsigned short comWrd1, const unsigned short msg)
{
	mil1553_lock(&mil->milif);
    if (status & MIL_STD_STATUS_VALMESS) {
        if (mil->urgent_desc.reserve) {
            // Была передача вне очереди
            if (mil->pool && mil->urgent_desc.transmit_mode == MIL_SLOT_RT_BC)
                copy_to_rxq(mil, mil->urgent_desc);
        } else {
            mil_slot_desc_t slot = mil->cur_slot->desc;
            if (mil->pool && slot.transmit_mode == MIL_SLOT_RT_BC)
                copy_to_rxq(mil, slot);
        }
    } else if (status & MIL_STD_STATUS_ERR) {
	    mil->nb_errors++;
	}


	if (mil->urgent_desc.reserve) // Если была передача вне очереди, то сбрасываем дескриптор,
		mil->urgent_desc.raw = 0; // чтобы не начать её повторно
	else {
		if (mil->cur_slot != 0) {
			mil->cur_slot = mil->cur_slot->next;
		}
		if (mil->cur_slot == 0)
			mil->cur_slot = mil->cyclogram;
	}

	volatile unsigned int d = KHZ/1000; // ~ 10мкс
	while(d--);

	if (mil->urgent_desc.raw != 0) {    // Есть требование на выдачу вне очереди
		start_slot(mil, mil->urgent_desc, mil->urgent_data);
		mil->urgent_desc.reserve = 1;   // Признак того, что идёт передача вне очереди
	} else if (mil->cur_slot != mil->cyclogram || mil->tim_reg == 0 || mil->period_ms == 0) {
		// если таймер не задан, или его период равен нулю циклограмма начинается с начала
		if (mil->cur_slot != 0) {
			start_slot(mil, mil->cur_slot->desc, mil->cur_slot->data);
		}
	}

    mil1553_unlock(&mil->milif);

}

static bool_t timer_handler(void *arg)
{
    milandr_mil1553_t *mil = arg;

    if (mil->urgent_desc.raw != 0) {
        mil->urgent_desc.reserve = 1;           // Признак того, что идёт передача вне очереди
        start_slot(mil, mil->urgent_desc, mil->urgent_data);
    } else if (mil->cur_slot == mil->cyclogram) {
    	if (mil->cur_slot != 0) {
    		start_slot(mil, mil->cur_slot->desc, mil->cur_slot->data);
    	}
    }

    mil->tim_reg->TIM_STATUS = ARM_TIM_CNT_ZERO_EVENT;

    arch_intr_allow(mil->tim_irq);

    return 1;
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
        if ((mil->mode == MIL_MODE_BC_MAIN) || (mil->mode == MIL_MODE_BC_RSRV)) {
            mil_lock(_mil);
            if (mode == MIL_MODE_BC_MAIN) {
                // Отключить резервный, включить основной канал.
                mil->reg->CONTROL = (mil->reg->CONTROL & ~MIL_STD_CONTROL_TRB) | MIL_STD_CONTROL_TRA;
            } else if (mode == MIL_MODE_BC_RSRV) {
                // Отключить основной, включить резервный канал.
                mil->reg->CONTROL = (mil->reg->CONTROL & ~MIL_STD_CONTROL_TRA) | MIL_STD_CONTROL_TRB;
            } else {
            	mil_unlock(_mil);
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

    return MIL_ERR_OK;
}

static int mil_start(mil1553if_t *_mil)
{
    milandr_mil1553_t *mil = (milandr_mil1553_t *)_mil;

    if (_mil->is_started(_mil))
        return MIL_ERR_OK;
        
    unsigned control = MIL_STD_CONTROL_DIV((KHZ/4)/1000); // на блок подается MAIN_CLK/4 (128/4=32)
        
    if ((mil->mode == MIL_MODE_BC_MAIN) || (mil->mode == MIL_MODE_BC_RSRV)) {
        control |= MIL_STD_CONTROL_MODE(MIL_STD_MODE_BC);
        if (mil->mode == MIL_MODE_BC_MAIN)
            control |= MIL_STD_CONTROL_TRA;
        else
            control |= MIL_STD_CONTROL_TRB;
            
        if (mil->tim_reg) {
        	if (mil->period_ms==0) {
        		mil->tim_reg->TIM_ARR = 0;
        	} else {
        		mil->tim_reg->TIM_ARR = mil->period_ms * KHZ-1;
        	}
            mil->tim_reg->TIM_IE = ARM_TIM_CNT_ARR_EVENT_IE;
            mil->tim_reg->TIM_CNTRL = ARM_TIM_CNT_EN;
        }
        mil->reg->INTEN = MIL_STD_INTEN_VALMESSIE | MIL_STD_INTEN_ERRIE;
        mil->reg->CONTROL = control;

        mil->cur_slot = mil->cyclogram;

        if (mil->urgent_desc.raw != 0) {
            mil->urgent_desc.reserve = 1;           // Признак того, что идёт передача вне очереди
            start_slot(mil, mil->urgent_desc, mil->urgent_data);
        } else {
        	if (mil->cur_slot != 0) {
        		start_slot(mil, mil->cur_slot->desc, mil->cur_slot->data);
        	}
        }
    } else if (mil->mode == MIL_MODE_RT) {
        control |= MIL_STD_CONTROL_MODE(MIL_STD_MODE_RT);
        control |= MIL_STD_CONTROL_ADDR(mil->addr_self) | MIL_STD_CONTROL_TRA | MIL_STD_CONTROL_TRB;
        mil->reg->StatusWord1 = MIL_STD_STATUS_ADDR_OU(mil->addr_self);
        mil->reg->INTEN = MIL_STD_INTEN_RFLAGNIE | MIL_STD_INTEN_VALMESSIE | MIL_STD_INTEN_ERRIE;
        mil->reg->CONTROL = control;
    } else {
        return MIL_ERR_NOT_SUPP;
    }

    // debug
    // ARM_GPIOB->DATA = 4; // левый
    
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
    
    while (!mem_queue_is_empty(&mil->rxq)) {
    	void *que_elem = 0;
    	mem_queue_get(&mil->rxq, &que_elem);
    	mem_free(que_elem);
    }

    while (!mem_queue_is_empty (&mil->rt_rxq)) {
        	void *que_elem = 0;
        	mem_queue_get(&mil->rt_rxq, &que_elem);
        	mem_free(que_elem);
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

    mil->cyclogram = head;
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
    int wc = (descr.words_count == 0 ? 32 : descr.words_count);
    memcpy(mil->urgent_data, data, 2*wc);
    mil_unlock(_mil);
    return MIL_ERR_OK;
}

int read_idx, write_idx;
status_item_t status_array[STATUS_ITEMS_SIZE];
uint32_t nb_missing;
static bool_t status_handler(void *arg)
{
	MIL_STD_1553B_t     *reg = (MIL_STD_1553B_t *)arg;

	if (status_array[write_idx].done) {
		nb_missing++;
	} else {
		status_array[write_idx].status = reg->STATUS;
		status_array[write_idx].command_word_1 = reg->CommandWord1;
		status_array[write_idx].msg = reg->MSG;
		status_array[write_idx].time_stamp = ARM_TIMER2->TIM_CNT;
		status_array[write_idx].done = 1;
		write_idx = (write_idx+1>=STATUS_ITEMS_SIZE?0:write_idx+1);
	}

	if (reg == ARM_MIL_STD_1553B1) {
		arch_intr_allow(MIL_STD_1553B1_IRQn);
	} else {
		arch_intr_allow(MIL_STD_1553B2_IRQn);
	}

    return 1;
}

static mutex_t status_lock; // формальный mutex, не должен запрещатся

void milandr_mil1553_init(milandr_mil1553_t *_mil, int port, mem_pool_t *pool, unsigned nb_rxq_msg, TIMER_t *timer)
{
	extern uint32_t mask_intr_disabled;
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
    ARM_RSTCLK->ETH_CLOCK |= ARM_ETH_CLOCK_MAN_BRG(2) | ARM_ETH_CLOCK_MAN_EN; // частота делится на 4

    mil->pool = pool;
    if (nb_rxq_msg) {
        mem_queue_init(&mil->rxq, pool, nb_rxq_msg);
        mem_queue_init(&mil->rt_rxq, pool, nb_rxq_msg);
    }

    
    mil->tim_reg = timer;
    if (timer == ARM_TIMER1) {
        mil->tim_irq = TIMER1_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_TIMER1;
        ARM_RSTCLK->TIM_CLOCK |= ARM_TIM_CLOCK_BRG1(0) | ARM_TIM_CLOCK_EN1;
    } else if (timer == ARM_TIMER2) {
        mil->tim_irq = TIMER2_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_TIMER2;
        ARM_RSTCLK->TIM_CLOCK |= ARM_TIM_CLOCK_BRG2(0) | ARM_TIM_CLOCK_EN2;
    } else if (timer == ARM_TIMER3) {
        mil->tim_irq = TIMER3_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_TIMER3;
        ARM_RSTCLK->TIM_CLOCK |= ARM_TIM_CLOCK_BRG3(0) | ARM_TIM_CLOCK_EN3;
    } else if (timer == ARM_TIMER4) {
        mil->tim_irq = TIMER4_IRQn;
        ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_TIMER4;
        ARM_RSTCLK->UART_CLOCK |= ARM_UART_CLOCK_TIM4_BRG(0) | ARM_UART_CLOCK_TIM4_EN;
    }
    
    mil->cyclogram = 0;
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


    // IRQ 1 (MIL_STD_1553B1_IRQn)  никогда не запрещается
	mask_intr_disabled = ~(1<<MIL_STD_1553B1_IRQn);
    ARM_NVIC_IPR(MIL_STD_1553B1_IRQn/4) = 0x40400040;
    mutex_attach_irq(&status_lock, MIL_STD_1553B1_IRQn, status_handler, ARM_MIL_STD_1553B1);


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

