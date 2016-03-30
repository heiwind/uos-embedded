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
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <timer/timer.h>
#include <timer/timeout.h>
#include "uart.h"

const unsigned TIMEOUT_SIGNAL = 0xFF;
const unsigned USART_CR1_INIT = USART_UE | USART_TE | USART_RE | 
    USART_TCIE | USART_RXNEIE;

static int stm32l_uart_set_param (uartif_t *uart, unsigned params)
{
    stm32l_uart_t* stmu = (stm32l_uart_t*) uart;
    USART_t *uregs = stmu->reg;

    mutex_lock(&uart->lock);

    // проверка правильности входных данных
    if ( ((params & UART_BITS_MASK) != 8) && ((params & UART_BITS_MASK) != 9) ) {
        //debug_printf ("Запрошенное количество бит не поддерживается!\n");
        mutex_unlock(&uart->lock);
        return UART_ERR_MODE_NOT_SUPP;
    }
    if ( ((params & 0xF0) >> 4) > 2 ) {
        //debug_printf ("Неправильно указана чётность!\n");
        mutex_unlock(&uart->lock);
        return UART_ERR_MODE_NOT_SUPP;
    }
    if ( ((params >> 8) != 0) && ((params >> 8) != 1) &&
           ((params >> 8) != 2) && ((params >> 8) != 4) )  {
        //debug_printf ("Неправильно указано количество стоп-бит!\n");
        mutex_unlock(&uart->lock);
        return UART_ERR_MODE_NOT_SUPP;
    }

    uregs->CR1 &= ~USART_UE;
    
    switch (params & UART_BITS_MASK) {
    case 8:
        uregs->CR1 &= ~USART_M;
        break;
    case 9:
        uregs->CR1 |= USART_M;
        break;
    }
    
    switch (params & UART_PARITY_MASK) {
    case 0:
        uregs->CR1 &= ~USART_PCE;
        break;
    case 1:
        uregs->CR1 |= USART_PCE;
        uregs->CR1 &= ~USART_PS;
        break;
    case 2:
        uregs->CR1 |= USART_PCE;
        uregs->CR1 |= USART_PS;
        break;
    }
    
    uregs->CR2 &= ~USART_STOP_MASK;
    switch (params & UART_STOP_MASK) {
    case UART_STOP_1:
        uregs->CR2 |= USART_STOP_1;
        break;
    case UART_STOP_05:
        uregs->CR2 |= USART_STOP_05;
        break;
    case UART_STOP_2:
        uregs->CR2 |= USART_STOP_2;
        break;
    case UART_STOP_15:
        uregs->CR2 |= USART_STOP_15;
        break;
    }
    
    uregs->CR1 |= USART_UE;

    mutex_unlock(&uart->lock);

    return UART_ERR_OK;
} //set_param

static int stm32l_uart_set_speed (uartif_t *uart, unsigned bytes_per_sec)
{
    stm32l_uart_t* stmu = (stm32l_uart_t*) uart;
    USART_t *uregs = stmu->reg;
    
    mutex_lock(&uart->lock);

    uregs->CR1 &= ~USART_UE;
    uregs->BRR = KHZ * 1000 / bytes_per_sec;
    uregs->CR1 |= USART_UE;
    
    mutex_unlock(&uart->lock);

    return UART_ERR_OK;
} //set_speed

static int stm32l_uart_tx(uartif_t *uart, void *data, int size)
{
    stm32l_uart_t* stmu = (stm32l_uart_t*) uart;
    uint8_t *pu8 = data;
    int words_txed = 0;
    USART_t *uregs = stmu->reg;
    
    if (size <= 0)
        return 0;

    mutex_lock(&uart->lock);
    
    uregs->CR1 &= ~USART_RE;

    do {
        stmu->tx_busy = 1;
        uregs->DR = pu8[words_txed++];

        while (stmu->tx_busy)
            mutex_wait(&uart->lock);
    } while (words_txed < size);
    
    uregs->CR1 |= USART_RE;

    mutex_unlock(&uart->lock);
    
    return words_txed;
} //stm32l_uart_tx

static int stm32l_uart_rx (uartif_t *uart, void *data, int size,
    int wait_full_msg, unsigned timeout_msec)
{
    unsigned words_rcvd = 0;
    unsigned words_avail;
    unsigned words_to_read;
    stm32l_uart_t* stmu = (stm32l_uart_t*) uart;
    uint8_t *pu8;

    if (size <= 0)
        return 0;

    mutex_lock(&uart->lock);
    
    words_rcvd = ring_avail_read(&stmu->iring);
    if (words_rcvd >= size) {
        ring_read(&stmu->iring, data, size);
        mutex_unlock(&uart->lock);
        return size;
    }
    else {
        if ((words_rcvd > 0) && !wait_full_msg) {
            ring_read(&stmu->iring, data, words_rcvd);
            mutex_unlock(&uart->lock);
            return words_rcvd;
        }
    }
    
    pu8 = (uint8_t *)data + words_rcvd;

    if (timeout_msec > 0) {
        timeout_set_value (&stmu->timeout, timeout_msec);
        timeout_set_signal (&stmu->timeout, (void *) TIMEOUT_SIGNAL);
        timeout_start (&stmu->timeout);
    }

    for (;;)  {
        if (mutex_wait (&uart->lock) == (void *) TIMEOUT_SIGNAL) {
            //debug_printf ("Сработал таймаут!\n", size);
            break;                      // выход по таймауту
        }
        else {
            words_avail = ring_avail_read(&stmu->iring);
            if (words_avail) {
                words_to_read = (words_avail < (size - words_rcvd)) ?
                    words_avail : size - words_rcvd;
                ring_read(&stmu->iring, pu8, words_to_read);
                words_rcvd += words_to_read;
                if ((words_rcvd >= size) || !wait_full_msg) {
                    break;
                }
                else {
                    pu8 += words_to_read;
                }
            }
        }
    }
    timeout_stop (&stmu->timeout);

    mutex_unlock(&uart->lock);
    return words_rcvd;
} //stm32l_uart_rx


static bool_t uart_handler(void *arg)
{
    stm32l_uart_t *stmu = arg;
    uint32_t sr = stmu->reg->SR;
    uint8_t rcvd = stmu->reg->DR;
    
//debug_printf("uart_handler, sr = %02X, symbol '%c' (%d)\n", sr, rcvd, rcvd);
    
    if (sr & USART_ORE) {
        stmu->idiscarded++;
    }
    
    if (sr & USART_RXNE) {
        if (ring_avail_write(&stmu->iring) > 0)
            ring_write(&stmu->iring, &rcvd, 1);
        else
            stmu->idiscarded++;
    }
    
    if (sr & USART_TC) {
        stmu->tx_busy = 0;
        stmu->reg->SR &= ~USART_TC;
    }
    
    arch_intr_allow(stmu->irq);
    
    return 0;
} //uart_handler

void stm32l_uart_init(stm32l_uart_t* stm_uart, unsigned port, timer_t* timer) 
{
    assert(port <= 1);
    
    stm_uart->port = port;
    stm_uart->uartif.set_param       = stm32l_uart_set_param;
    stm_uart->uartif.set_speed       = stm32l_uart_set_speed;
    stm_uart->uartif.tx              = stm32l_uart_tx;
    stm_uart->uartif.rx              = stm32l_uart_rx;
    stm_uart->uartif.data_align = 1;
    
    ring_init(&stm_uart->iring, stm_uart->ibuf, sizeof(stm_uart->ibuf));

    if (port == 0) {
        RCC->APB2ENR |= RCC_USART1EN;
        RCC->APB2LPENR |= RCC_USART1LPEN;
        stm_uart->reg = USART1;
        stm_uart->irq = IRQ_USART1;
        USART1->SR = 0;
        USART1->CR1 = USART_CR1_INIT;
    }
    else {
        RCC->APB1ENR |= RCC_USART2EN;
        RCC->APB1LPENR |= RCC_USART2LPEN;
        stm_uart->reg = USART2;
        stm_uart->irq = IRQ_USART2;
        USART2->SR = 0;
        USART2->CR1 = USART_CR1_INIT;
        
    }
    
    mutex_attach_irq(&stm_uart->uartif.lock, stm_uart->irq, uart_handler, stm_uart);
    timeout_init (&stm_uart->timeout, timer, &stm_uart->uartif.lock);
} //stm32l_uart_init

