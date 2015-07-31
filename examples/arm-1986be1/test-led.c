/*
 * Проверка светодиодов на отладочной плате
 */
#include <runtime/lib.h>

void init_leds ()
{
	ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_GPIOB;
	
	ARM_GPIOB->FUNC = 0;
	ARM_GPIOB->DATA = 0;
	ARM_GPIOB->OE = 7;
	ARM_GPIOB->ANALOG = 7;
	ARM_GPIOB->PWR = 0x3F;	
}

int main (void)
{
	init_leds ();
	
	for (;;) {
	    ARM_GPIOB->DATA = 0;
		mdelay (500);
	    ARM_GPIOB->DATA = 1;
		mdelay (500);		
	}	
}
