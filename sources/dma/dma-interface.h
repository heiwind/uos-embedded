/*
 * DMA interface.
 *
 * 2015 Dmitry Podkhvatilin <vatilin@gmail.com>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 *
 * As a special exception to the GPL, permission is granted for additional
 * uses of the text contained in this file.  See the accompanying file
 * "COPY-UOS.txt" for details.
 */
#ifndef __DMA_INTERFACE_H__
#define __DMA_INTERFACE_H__

//
// Коды результата
//

// Успешно
#define DMA_ERR_OK               0
// Возвращается, если запрошен неподдерживаемый режим
#define DMA_ERR_NOT_SUPP        -1
// Возвращается в случае, если запрошенное количество битов в слове превышает 
// возможности драйвера.
#define SPI_ERR_BAD_BITS        -1
// Возвращается в случае, если запрошена неподдерживаемая драйвером
// скорость передачи (битовая частота).
#define SPI_ERR_BAD_FREQ        -2
// Возвращается, если запрошен не существующий номер линии выбора устройства
// ("чип-селекта").
#define SPI_ERR_BAD_CS          -3
// Возращается в случае невозможности передачи сообщения из-за его большой
// длины (недостаточный размер буфера в драйвере).
#define SPI_ERR_SMALL_BUF       -4
// Возвращается, если указан недопустимый номер порта (контроллера) SPI.
#define SPI_ERR_BAD_PORT        -5


//
// Установки режима драйвера (для поля mode структуры spi_message_t)
//

// Полярность линии синхроимпульсов. Если флаг не задан, то в режиме ожидания
// линия переводится в низкий уровень; если задан, то в высокий.
#define SPI_MODE_CPOL           (1 << 0)
// Выбор фронта синхроимпульса, по которому осуществляется выборка данных.
// Если CPOL не задан, то при заданном CPHA - по заднему, при незаданном - по
// переднему. Если CPOL задан, то при заданном CPHA - по переднему, при
// незаданном - по заднему.
#define SPI_MODE_CPHA           (1 << 1)
// Выбор активного уровня линии выбора устройства. Если флаг не задан, то
// активный низкий, иначе активный высокий.
#define SPI_MODE_CS_HIGH        (1 << 2)
// Выбор порядка следования бит на линиях данных. Если флаг не задан, то
// первый передаётся старший бит, иначе младший бит.
#define SPI_MODE_LSB_FIRST      (1 << 3)
// Поведение линии выбора устройства после передачи сообщения. Если флаг не
// задан, то линия переходит в неактивное состояние автоматически. Иначе
// остаётся активным до передачи сообщения, в поле mode которого будет
// отсутствовать этот флаг.
#define SPI_MODE_CS_HOLD        (1 << 4)
// Макрос для извлечения флагов режима из поля mode структуры spi_message_t.
// Флагами режима считаются все вышеперечисленные флаги.
#define SPI_MODE_GET_MODE(x)    ((x) & 0xFF)
// Номер линии выборки устройства. Контроллер SPI, работающий в режиме 
// "Master" может иметь несколько линий выборки устройства, к каждой из
// которых может быть подключено одно устройство. Номер линии выборки
// задаётся в поле mode структуры spi_message_t с помощью CS_NUM.
#define SPI_MODE_CS_NUM(x)      ((x) << 8)
// Макрос для извлечения номера линии выборки устройства из поля 
// mode структуры spi_message_t
#define SPI_MODE_GET_CS_NUM(x)  (((x) >> 8) & 0xFF)
// Количество бит в одном слове.
#define SPI_MODE_NB_BITS(x)     ((x) << 16)
// Макрос для извлечения количества бит в слове из поля mode spi_message_t.
#define SPI_MODE_GET_NB_BITS(x) (((x) >> 16) & 0xFF)


typedef struct _dmaif_t dmaif_t;
typedef struct _dma_transmission_t dma_transmission_t;


typedef (* dma_event_handler)(dmaif_t *dma, int transmission_n);
//
// Базовый тип интерфейса DMA
//
struct _dmaif_t
{
    // Мьютекс для синхронизации
    mutex_t     lock;
    
    int (* add_transmission)(dmaif_t *dma, dma_transmission_t *trans, 
        unsigned flags);
        
    int (* set_handler)(dmaif_t *dma, dma_event_handler handler, 
        unsigned event_type);
        
    int (* start)(dmaif_t *dma);
    int (* stop)(dmaif_t *dma);
    int (* is_running)(dmaif_t *dma);
    int (* wait)(dmaif_t *dma);
    int (* pause)(dmaif_t *dma);
    int (* resume)(dmaif_t *dma);
};

struct _dma_transmission_t
{
    unsigned long src_addr;
    unsigned long dst_addr;
    long src_inc;
    long dst_inc;
    unsigned nb_words;
};

//
// Функции-обёрки для удобства вызова функции драйвера.
//
static inline __attribute__((always_inline)) 
int dma_init_task(dmaif_t *dma, dma_task_t *task, unsigned nb_tasks, 
        unsigned flags)
{
    return dma->init_task(dma, task, nb_tasks, flags);
}

#endif
