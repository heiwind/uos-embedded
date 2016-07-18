// ID: SPO-UOS-milandr-mil-std-1553_rt.c VER: 1.0.0
//
// История изменений:
//
// 1.0.0	Начальная версия
//
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <milandr/mil-std-1553_setup.h>
#include <milandr/mil-std-1553_rt.h>

#if 0
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
#endif
// Хендлер не вызывается
#if 0
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
            mil->nb_words += wordsCount;
            //copy_to_rt_rxq(mil, subaddr, wordsCount);
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
            mil->nb_words += wordsCount;
            //copy_to_rt_rxq(mil, subaddr, wordsCount);
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

#endif
