/*
 * Проверка светодиодов на отладочной плате
 */
#include <runtime/lib.h>

#define LED_ON(n) (GPIOB->BSRR = GPIO_SET(6 + n))
#define LED_OFF(n) (GPIOB->BSRR = GPIO_RESET(6 + n))

void init_leds ()
{
    RCC->AHBENR |= RCC_GPIOBEN;
    GPIOB->MODER = GPIO_OUT(7) | GPIO_OUT(6);
}

int main (void)
{
	init_leds ();

	int i = 0;	
	for (;;) {
		LED_OFF(i);
		i = (i + 1) % 2;
		LED_ON(i);
		mdelay (500);
	}
}
