#ifndef __GPIO_STML32_H__
#define __GPIO_STML32_H__

#include <gpio/gpio-interface.h>

struct _stm32l_gpio_t {
    gpioif_t            gpioif;
    
    GPIO_t              *reg;
    unsigned            pin_n;
    int                 irq;
};
typedef struct _stm32l_gpio_t stm32l_gpio_t;

enum
{
    GPIO_PORT_A,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E,
    GPIO_PORT_F,
    GPIO_PORT_G,
    GPIO_PORT_H
};


#define GPIO_FLAGS_INPUT              0
#define GPIO_FLAGS_OUTPUT             (1 << 0)
#define GPIO_FLAGS_PULL_UP            (1 << 1)
#define GPIO_FLAGS_PULL_DOWN          (1 << 2)
#define GPIO_FLAGS_OPEN_DRAIN         (1 << 3)

void stm32l_gpio_init(stm32l_gpio_t *gpio, unsigned port, unsigned pin, unsigned flags);

#endif
