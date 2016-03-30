/*
 * UART interface implementation for STM32L microcontrollers.
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
#ifndef _STM32_UART_H_
#define _STM32_UART_H_

#include <timer/timeout.h>
#include <buf/ring.h>
#include <uart/uart-interface.h>

#ifndef UART_INBUFSZ
#define UART_INBUFSZ	8
#endif

struct _stm32l_uart_t {
    uartif_t            uartif;
    unsigned            port;
    timeout_t           timeout;
    int                 irq;
    
    uint8_t             ibuf[UART_INBUFSZ];
    ring_t              iring;
    unsigned            idiscarded;
    
    int                 tx_busy;

    USART_t*            reg;
};
typedef struct _stm32l_uart_t stm32l_uart_t;

/* uartif - UART interface, see uart-master-interface.h
 * port   - phisical port number : 0 means USART1, 1 means USART2
 * timer  - указатель на системный таймер
 */
void stm32l_uart_init(stm32l_uart_t* uart, unsigned port, timer_t* timer);

#endif /* _STM32_UART_H_ */
