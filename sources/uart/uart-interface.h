/*
 * UART interface.
 *
 * Copyright (C) 2016 Dmitry Podkhvatilin <vatilin@gmail.com>,
 *                    Anton Smirnov <asmirnov@smartiko.ru>
 *
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
#ifndef __UART_INTERFACE_H__
#define __UART_INTERFACE_H__

//
//        Интерфейс драйвера контроллера UART
//
//    Данный заголовочный файл предназначен для унификации интерфейса драйверов
// контроллеров UART различных производителей. Унификация интерфейса нужна для
// возможности написания ПЕРЕНОСИМЫХ драйверов контроллеров более высокого
// уровня, использующих UART в качестве технологической линии связи (например,
// драйверов передатчиков по RS-485 и т.д.).
//    Этот интерфейс отличается от традиционного в uOS, представленного
// заголовочным файлом uart/uart.h, тем что он ориентирован на передачу и приём
// бинарных данных в виде пакетов заданной длины, тогда как второй предназначен,
// прежде всего, для работы с символьной информацией.

//
// Коды результата
//

// Успешно
#define UART_ERR_OK              0
// Возвращается в случае, если запрошена неподдерживаемая драйвером
// скорость передачи (битовая частота).
#define UART_ERR_BAD_FREQ        -1
// Возращается в случае невозможности передачи/приёма сообщения из-за его
// большой длины (недостаточный размер буфера в драйвере).
#define UART_ERR_SMALL_BUF       -2
// Возвращается, если указан недопустимый номер порта (контроллера) UART.
#define UART_ERR_BAD_PORT        -3
// Возвращается, если запрошен неподдерживаемый режим.
#define UART_ERR_MODE_NOT_SUPP   -4

//
// Установки режима драйвера
//
#define UART_BITS(n)            (n)         // число бит в слове
#define UART_PE_E               (1 << 4)    // контроль чётности: нечётность
#define UART_PE_O               (1 << 5)    // контроль чётности: чётность
#define UART_STOP_05            (1 << 8)    // 0,5 стоп-бит
#define UART_STOP_1             0           // по умолчанию 1 стоп-бит
#define UART_STOP_15            (1 << 9)    // 1,5 стоп-бит
#define UART_STOP_2             (1 << 10)   // 2 стоп-бит

#define UART_BITS_MASK          0xF         // маска для поля с числом бит                                            
#define UART_PARITY_MASK        (0xF << 4)  // маска для полей контроля чётности
#define UART_STOP_MASK          (0xF << 8)  // маска для кол-ва стоп-бит


typedef struct _uartif_t uartif_t;

//
// Базовый тип интерфейса UART
//
struct _uartif_t
{
    // Мьютекс для синхронизации
    mutex_t     lock;

    // Требование к выравниванию данных (к адресам tx_data и rx_data),
    // сообщаемое драйвером UART использующему его коду.
    // Должны соблюдаться равенства:
    // (unsigned) tx_data & data_align == 0
    // (unsigned) rx_data & data_align == 0
    // Иначе не гарантируются правильные приём и передача данных.
    unsigned data_align;

    // В данное поле должен быть записан указатель на аппаратно-зависимую
    // функцию, позволяющую установить параметры драйвера с помощью 
    // битовых полей UART_*, указанных в параметре params.
    int (* set_param)(uartif_t *uart, unsigned params);

    // В данное поле должен быть записан указатель на аппаратно-зависимую
    // функцию, позволяющую установить скорость передачи в байтах/с.
    int (* set_speed)(uartif_t *uart, unsigned bytes_per_sec);

    // В данное поле должен быть записан указатель на аппаратно-зависимую
    // функцию, выполняющую передачу по интерфейсу UART сообщения из буфера
    // data размера size. Функция возвращает реальное количество переданных
    // байт или код ошибки (отрицательное число).
    int (* tx)(uartif_t *uart, void *data, int size);

    // В данное поле должен быть записан указатель на аппаратно-зависимую
    // функцию, выполняющую приём по интерфейсу UART сообщения в буфер
    // data; size - размер буфера. Если wait_full_msg == 1, то функция
    // должна принять сообщение размера size и только потом вернуть управление
    // вызывавшей функции; иначе функция должна сразу же вернуть имеющееся 
    // (ненулевое) количество байт. timeout - время ожидания в миллисекундах.
    // Функция возвращает реальное количество принятых байт или код ошибки
    // (отрицательное число).
    int (* rx)(uartif_t *uart, void *data, int size,
        int wait_full_msg, unsigned timeout_msec);
};

#define to_uartif(x)   ((uartif_t*)&(x)->uartif)

static inline __attribute__((always_inline)) 
int uart_set_param(uartif_t *uart, unsigned params)
{
    return uart->set_param(uart, params);
}

static inline __attribute__((always_inline)) 
int uart_set_speed(uartif_t *uart, unsigned bytes_per_sec)
{
    return uart->set_speed(uart, bytes_per_sec);
}

static inline __attribute__((always_inline)) 
int uart_tx(uartif_t *uart, void *data, int size)
{
    return uart->tx(uart, data, size);
}

static inline __attribute__((always_inline)) 
int uart_rx(uartif_t *uart, void *data, int size, int wait_full_msg, 
    unsigned timeout_msec)
{
    return uart->rx(uart, data, size, wait_full_msg, timeout_msec);
}

#endif /* __UART_INTERFACE_H__ */
