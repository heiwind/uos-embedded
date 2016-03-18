/*
 * GPIO interface.
 *
 * Copyright (C) 2014-2015 Dmitry Podkhvatilin <vatilin@gmail.com>
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
#ifndef __GPIO_INTERFACE_H__
#define __GPIO_INTERFACE_H__


#define GPIO_EVENT_RISING_EDGE          (1 << 0)
#define GPIO_EVENT_FALLING_EDGE         (1 << 1)


typedef struct _gpioif_t gpioif_t;

typedef void (* gpio_handler_t) (gpioif_t *pin, void *arg);

//
// Базовый тип интерфейса GPIO
//
struct _gpioif_t
{
    // Мьютекс для синхронизации
    mutex_t         lock;
    
    gpio_handler_t  handler;
    void            *handler_arg;
    
    void (* to_input)(gpioif_t *pin);
    
    void (* to_output)(gpioif_t *pin);
    
    void (* set_value)(gpioif_t *pin, int v);
    
    int (* value)(gpioif_t *pin);
    
    void (* attach_interrupt)(gpioif_t *pin, unsigned event, 
        gpio_handler_t handler, void *arg);
    
    void (* detach_interrupt)(gpioif_t *pin);
};

#define to_gpioif(x)   ((gpioif_t*)&(x)->gpioif)

//
// Функции-обёрки для удобства вызова функций интерфейса.
//


static inline __attribute__((always_inline)) 
void gpio_to_input(gpioif_t *pin)
{
    pin->to_input(pin);
}

static inline __attribute__((always_inline)) 
void gpio_to_output(gpioif_t *pin)
{
    pin->to_output(pin);
}

static inline __attribute__((always_inline)) 
void gpio_set_val(gpioif_t *pin, int val)
{
    pin->set_value(pin, val);
}

static inline __attribute__((always_inline)) 
int gpio_val(gpioif_t *pin)
{
    return pin->value(pin);
}

static inline __attribute__((always_inline)) 
void gpio_attach_interrupt(gpioif_t *pin, int event, 
        gpio_handler_t handler, void *arg)
{
    pin->attach_interrupt(pin, event, handler, arg);
}

static inline __attribute__((always_inline)) 
void gpio_detach_interrupt(gpioif_t *pin)
{
    pin->detach_interrupt(pin);
}

static inline __attribute__((always_inline)) 
void gpio_wait_irq(gpioif_t *pin)
{
    mutex_wait(&pin->lock);
}


#endif
