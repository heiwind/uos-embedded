/*
 * Precise microsecond delay using SysTick timer.
 *
 * Copyright (C) 2010 Serge Vakulenko, <serge@vak.ru>
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

#define USE_OLDTICKS	0

#ifdef ARM_1986BE1
#if (ARM_SYS_TIMER==1)
#	define TIMER_IRQ	TIMER1_IRQn
#	define SYS_TIMER	ARM_TIMER1
#	define PER_CLOCK_EN	ARM_PER_CLOCK_TIMER1
#	define TIM_CLK_EN	ARM_TIM_CLOCK_EN1
#elif (ARM_SYS_TIMER==2)
#	define TIMER_IRQ	TIMER2_IRQn
#	define SYS_TIMER	ARM_TIMER2
#	define PER_CLOCK_EN	ARM_PER_CLOCK_TIMER2
#	define TIM_CLK_EN	ARM_TIM_CLOCK_EN2
#elif (ARM_SYS_TIMER==3)
#	define TIMER_IRQ	TIMER3_IRQn
#	define SYS_TIMER	ARM_TIMER3
#	define PER_CLOCK_EN	ARM_PER_CLOCK_TIMER3
#	define TIM_CLK_EN	ARM_TIM_CLOCK_EN3
#elif (ARM_SYS_TIMER==4)
#	define TIMER_IRQ	TIMER4_IRQn
#	define SYS_TIMER	ARM_TIMER4
#	define PER_CLOCK_EN	ARM_PER_CLOCK_TIMER4
#	define TIM_CLK_EN	ARM_UART_CLOCK_TIM4_EN
#else
#	warning "ARM_SYS_TIMER is not defined in CFLAGS (target.cfg). Using TIMER1 for system timer."
#	define TIMER_IRQ	TIMER1_IRQn
#	define SYS_TIMER	ARM_TIMER1
#	define PER_CLOCK_EN	ARM_PER_CLOCK_TIMER1
#	define TIM_CLK_EN	ARM_TIM_CLOCK_EN1
#endif
#endif


extern volatile uint32_t __timer_ticks_uos;

void udelay (unsigned usec)
{
	if (! usec)
		return;

#ifdef ARM_1986BE1
	if (! (ARM_RSTCLK->PER_CLOCK & PER_CLOCK_EN)) {
		ARM_RSTCLK->PER_CLOCK |= PER_CLOCK_EN;
#if (ARM_SYS_TIMER==4)
		ARM_RSTCLK->UART_CLOCK |= TIM_CLK_EN;
#else
		ARM_RSTCLK->TIM_CLOCK |= TIM_CLK_EN;
#endif
		SYS_TIMER->TIM_ARR = 0xFFFFFFFF;
		SYS_TIMER->TIM_CNT = 0;
		SYS_TIMER->TIM_PSG = 0;
		SYS_TIMER->TIM_IE = 0;
		SYS_TIMER->TIM_CNTRL = ARM_TIM_CNT_EN;
	}
	unsigned arr = SYS_TIMER->TIM_ARR;

	if (SYS_TIMER->TIM_STATUS & ARM_TIM_CNT_ZERO_EVENT) {
		SYS_TIMER->TIM_STATUS &= ~ARM_TIM_CNT_ZERO_EVENT;
	}

	unsigned now = SYS_TIMER->TIM_CNT;

	if (SYS_TIMER->TIM_STATUS & ARM_TIM_CNT_ZERO_EVENT) {
		SYS_TIMER->TIM_STATUS &= ~ARM_TIM_CNT_ZERO_EVENT;
		now = SYS_TIMER->TIM_CNT;
	}

	unsigned final = now + (usec-1)* (KHZ / 1000);
	
	//debug_printf("timer %X %X %X %X\n", arr, now, final, SYS_TIMER->TIM_STATUS);

	for (;;) {
		if (SYS_TIMER->TIM_STATUS & ARM_TIM_CNT_ZERO_EVENT) {
			SYS_TIMER->TIM_STATUS &= ~ARM_TIM_CNT_ZERO_EVENT;
			final -= arr; // если arr == 0xFFFFFFFF то + 1 даст ошибку (0)
			if (arr != 0xFFFFFFFF)
				final--;
		}

		/* This comparison is valid only when using a signed type. */
		now = SYS_TIMER->TIM_CNT;
		if ((int) (final - now) < 0)
			break;
	}
#else

#if USE_OLDTICKS
	unsigned ctrl = ARM_SYSTICK->CTRL;
	if (! (ctrl & ARM_SYSTICK_CTRL_ENABLE)) {
		/* Start timer using HCLK clock, no interrupts. */
		ARM_SYSTICK->LOAD = 0xFFFFFF;
		ARM_SYSTICK->CTRL = ARM_SYSTICK_CTRL_HCLK |
			ARM_SYSTICK_CTRL_ENABLE;
	}
#else
	uint32_t ctrl;
	uint32_t prev_ticks;
	uint32_t ticks = __timer_ticks_uos;

	prev_ticks = ticks;

	if (!ticks) {
		ctrl = ARM_SYSTICK->CTRL;
		if (! (ctrl & ARM_SYSTICK_CTRL_ENABLE)) {
			/* Start timer using HCLK clock, no interrupts. */
			ARM_SYSTICK->LOAD = 0xFFFFFF;
			ARM_SYSTICK->CTRL = ARM_SYSTICK_CTRL_HCLK |
				ARM_SYSTICK_CTRL_ENABLE;
		}
	}
#endif

	unsigned load = ARM_SYSTICK->LOAD & 0xFFFFFF;
	unsigned now = ARM_SYSTICK->VAL & 0xFFFFFF;
#ifdef SETUP_HCLK_HSI
	unsigned final = now - usec * 8 + 42;
#else
	unsigned final = now - usec * (KHZ / 1000) + 42;
#endif
	for (;;) {

#if USE_OLDTICKS
		ctrl = ARM_SYSTICK->CTRL;
		if (ctrl & ARM_SYSTICK_CTRL_COUNTFLAG) {
			final += load;
		}
#else
		if (!ticks) {
			ctrl = ARM_SYSTICK->CTRL;
			if (ctrl & ARM_SYSTICK_CTRL_COUNTFLAG) {
				final += load;
			}
		} else {
			if (prev_ticks != ticks) {
				uint32_t delta;
				if (ticks < prev_ticks) {
					delta = ticks - prev_ticks + 1;
				} else {
					delta = ticks - prev_ticks;
				}
				final += (load + 1) * delta;
			}
			prev_ticks = ticks;
			ticks = __timer_ticks_uos;
		}
#endif
		/* This comparison is valid only when using a signed type. */
		now = ARM_SYSTICK->VAL & 0xFFFFFF;
		if ((int) (now - final) < 0)
			break;
	}
#endif
}
