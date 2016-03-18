#include <runtime/lib.h>
#include <kernel/uos.h>
#include <stm32l/gpio.h>
#include <kernel/internal.h>

static void stm32l_to_input(gpioif_t *pin)
{
    stm32l_gpio_t *stm_pin = (stm32l_gpio_t *)pin;
    
    mutex_lock(&pin->lock);
    
    stm_pin->reg->MODER &= ~GPIO_MODE_MASK(stm_pin->pin_n);
    
    mutex_unlock(&pin->lock);
}
    
static void stm32l_to_output(gpioif_t *pin)
{
    stm32l_gpio_t *stm_pin = (stm32l_gpio_t *)pin;
    
    mutex_lock(&pin->lock);
    
    stm_pin->reg->MODER = (stm_pin->reg->MODER & 
        ~GPIO_MODE_MASK(stm_pin->pin_n)) | GPIO_OUT(stm_pin->pin_n);
        
    mutex_unlock(&pin->lock);
}
    
static void stm32l_set_value(gpioif_t *pin, int v)
{
    stm32l_gpio_t *stm_pin = (stm32l_gpio_t *)pin;
    
    if (v)
        stm_pin->reg->BSRR = GPIO_SET(stm_pin->pin_n);
    else
        stm_pin->reg->BSRR = GPIO_RESET(stm_pin->pin_n);
}
    
static int stm32l_value(gpioif_t *pin)
{
    stm32l_gpio_t *stm_pin = (stm32l_gpio_t *)pin;
    
    return (stm_pin->reg->IDR >> stm_pin->pin_n) & 1;
}

static bool_t stm32l_gpio_irq_handler(void *arg)
{
    gpioif_t *pin = (gpioif_t *)arg;
    stm32l_gpio_t *stm_pin = (stm32l_gpio_t *)arg;
    
    if (pin->handler)
        pin->handler(pin, pin->handler_arg);
    
    EXTI->PR = EXTI_GPIO(stm_pin->pin_n);
    
    arch_intr_allow(stm_pin->irq);
    
    return 0;
}
    
static void stm32l_attach_interrupt(gpioif_t *pin, unsigned event, 
        gpio_handler_t handler, void *arg)
{
    stm32l_gpio_t *stm_pin = (stm32l_gpio_t *)pin;
    arm_reg_t *exticr;
    
    pin->handler = handler;
    pin->handler_arg = arg;
    
    RCC->APB2ENR |= RCC_SYSCFGEN;
    
    switch (stm_pin->pin_n) {
    case 0:
        stm_pin->irq = IRQ_EXTI0;
        exticr = &SYSCFG->EXTICR1;
        break;
    case 1:
        stm_pin->irq = IRQ_EXTI1;
        exticr = &SYSCFG->EXTICR1;
        break;
    case 2:
        stm_pin->irq = IRQ_EXTI2;
        exticr = &SYSCFG->EXTICR1;
        break;
    case 3:
        stm_pin->irq = IRQ_EXTI3;
        exticr = &SYSCFG->EXTICR1;
        break;
    case 4:
        stm_pin->irq = IRQ_EXTI4;
        exticr = &SYSCFG->EXTICR2;
        break;
    case 5:
        stm_pin->irq = IRQ_EXTI9_5;
        exticr = &SYSCFG->EXTICR2;
        break;
    case 6:
        stm_pin->irq = IRQ_EXTI9_5;
        exticr = &SYSCFG->EXTICR2;
        break;
    case 7:
        stm_pin->irq = IRQ_EXTI9_5;
        exticr = &SYSCFG->EXTICR2;
        break;
    case 8:
        stm_pin->irq = IRQ_EXTI9_5;
        exticr = &SYSCFG->EXTICR3;
        break;
    case 9:
        stm_pin->irq = IRQ_EXTI9_5;
        exticr = &SYSCFG->EXTICR3;
        break;
    case 10:
        stm_pin->irq = IRQ_EXTI15_10;
        exticr = &SYSCFG->EXTICR3;
        break;
    case 11:
        stm_pin->irq = IRQ_EXTI15_10;
        exticr = &SYSCFG->EXTICR3;
        break;
    case 12:
        stm_pin->irq = IRQ_EXTI15_10;
        exticr = &SYSCFG->EXTICR4;
        break;
    case 13:
        stm_pin->irq = IRQ_EXTI15_10;
        exticr = &SYSCFG->EXTICR4;
        break;
    case 14:
        stm_pin->irq = IRQ_EXTI15_10;
        exticr = &SYSCFG->EXTICR4;
        break;
    case 15:
        stm_pin->irq = IRQ_EXTI15_10;
        exticr = &SYSCFG->EXTICR4;
        break;
    default:
        return;
    }

    if (stm_pin->reg == GPIOA)
        *exticr = (*exticr & ~SYSCFG_PMASK(stm_pin->pin_n)) | SYSCFG_PA(stm_pin->pin_n);
    else if (stm_pin->reg == GPIOB)
        *exticr = (*exticr & ~SYSCFG_PMASK(stm_pin->pin_n)) | SYSCFG_PB(stm_pin->pin_n);
    else if (stm_pin->reg == GPIOC)
        *exticr = (*exticr & ~SYSCFG_PMASK(stm_pin->pin_n)) | SYSCFG_PC(stm_pin->pin_n);
    else if (stm_pin->reg == GPIOD)
        *exticr = (*exticr & ~SYSCFG_PMASK(stm_pin->pin_n)) | SYSCFG_PD(stm_pin->pin_n);
    else if (stm_pin->reg == GPIOE)
        *exticr = (*exticr & ~SYSCFG_PMASK(stm_pin->pin_n)) | SYSCFG_PE(stm_pin->pin_n);
    else if (stm_pin->reg == GPIOF)
        *exticr = (*exticr & ~SYSCFG_PMASK(stm_pin->pin_n)) | SYSCFG_PF(stm_pin->pin_n);
    else if (stm_pin->reg == GPIOG)
        *exticr = (*exticr & ~SYSCFG_PMASK(stm_pin->pin_n)) | SYSCFG_PG(stm_pin->pin_n);
    else if (stm_pin->reg == GPIOH)
        *exticr = (*exticr & ~SYSCFG_PMASK(stm_pin->pin_n)) | SYSCFG_PH(stm_pin->pin_n);
    else
        return;
        
    EXTI->IMR |= EXTI_GPIO(stm_pin->pin_n);
    if (event & GPIO_EVENT_RISING_EDGE)
        EXTI->RTSR |= EXTI_GPIO(stm_pin->pin_n);
    if (event & GPIO_EVENT_FALLING_EDGE)
        EXTI->FTSR |= EXTI_GPIO(stm_pin->pin_n);
        
    mutex_attach_irq(&pin->lock, stm_pin->irq, stm32l_gpio_irq_handler, pin);
}

static void stm32l_detach_interrupt(gpioif_t *pin)
{
    mutex_lock(&pin->lock);
    mutex_unlock_irq(&pin->lock);
}

void stm32l_gpio_init(stm32l_gpio_t *gpio, unsigned port, unsigned pin_n, 
        unsigned flags)
{
    gpio->pin_n  = pin_n;
    
    switch (port) {
    case GPIO_PORT_A:
        RCC->AHBENR |= RCC_GPIOAEN;
        gpio->reg = GPIOA;
        break;
    case GPIO_PORT_B:
        RCC->AHBENR |= RCC_GPIOBEN;    
        gpio->reg = GPIOB;
        break;
    case GPIO_PORT_C:
        RCC->AHBENR |= RCC_GPIOCEN;
        gpio->reg = GPIOC;
        break;
    case GPIO_PORT_D:
        RCC->AHBENR |= RCC_GPIODEN;
        gpio->reg = GPIOD;
        break;
    case GPIO_PORT_E:
        RCC->AHBENR |= RCC_GPIOEEN;
        gpio->reg = GPIOE;
        break;
    case GPIO_PORT_F:
        RCC->AHBENR |= RCC_GPIOFEN;
        gpio->reg = GPIOF;
        break;
    case GPIO_PORT_G:
        RCC->AHBENR |= RCC_GPIOGEN;
        gpio->reg = GPIOG;
        break;
    case GPIO_PORT_H:
        RCC->AHBENR |= RCC_GPIOHEN;
        gpio->reg = GPIOH;
        break;
    default:
        gpio->reg = 0;
    }
    
    if (flags & GPIO_FLAGS_OUTPUT)
        gpio->reg->MODER = (gpio->reg->MODER & ~GPIO_MODE_MASK(pin_n)) | GPIO_OUT(pin_n);
    else
        gpio->reg->MODER &= ~GPIO_MODE_MASK(pin_n);
        
    if (flags & GPIO_FLAGS_PULL_UP)
        gpio->reg->PUPDR |= GPIO_PULL_UP(pin_n);
    else
        gpio->reg->PUPDR &= ~GPIO_PULL_UP(pin_n);
        
    if (flags & GPIO_FLAGS_PULL_DOWN)
        gpio->reg->PUPDR |= GPIO_PULL_DOWN(pin_n);
    else
        gpio->reg->PUPDR &= ~GPIO_PULL_DOWN(pin_n);
        
    if (flags & GPIO_FLAGS_OPEN_DRAIN)
        gpio->reg->OTYPER |= GPIO_OD(pin_n);
    else
        gpio->reg->OTYPER &= ~GPIO_OD(pin_n);
        
    gpioif_t *gpioif = &gpio->gpioif;
    
    gpioif->to_input = stm32l_to_input;
    gpioif->to_output = stm32l_to_output;
    gpioif->set_value = stm32l_set_value;
    gpioif->value = stm32l_value;
    gpioif->attach_interrupt = stm32l_attach_interrupt;
    gpioif->detach_interrupt = stm32l_detach_interrupt;
}
