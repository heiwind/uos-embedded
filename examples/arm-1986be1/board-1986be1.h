/*
 * Definitions of input signals and output actions for Milandr 1986BE1T board.
 */
 #include <runtime/lib.h>
#define BUTTON_RIGHT	5	/* pins of PORTE */
#define BUTTON_LEFT	10
#define BUTTON_DOWN	15
#define BUTTON_UP	8
#define BUTTON_SEL	9
#define BUTTON_BACK	11

#define MASK_RIGHT	(1 << BUTTON_RIGHT)
#define MASK_LEFT	(1 << BUTTON_LEFT)
#define MASK_DOWN	(1 << BUTTON_DOWN)
#define MASK_UP		(1 << BUTTON_UP)
#define MASK_SEL	(1 << BUTTON_SEL)
#define MASK_BACK	(1 << BUTTON_BACK)

#define ALL_BUTTONS	(MASK_RIGHT | MASK_LEFT | MASK_DOWN | MASK_UP | MASK_SEL | MASK_BACK)

inline int joystick_left (void)
{
	return (~ARM_GPIOE->DATA & ALL_BUTTONS) == MASK_LEFT;
}

inline int joystick_down (void)
{
        return (~ARM_GPIOE->DATA & ALL_BUTTONS) == MASK_DOWN;
}

inline int joystick_up (void)
{
        return (~ARM_GPIOE->DATA & ALL_BUTTONS) == MASK_UP;
}

inline int joystick_right (void)
{
        return (~ARM_GPIOE->DATA & ALL_BUTTONS) == MASK_RIGHT;
}

inline int joystick_select (void)
{
        return (~ARM_GPIOE->DATA & ALL_BUTTONS) == MASK_SEL;
}

inline void buttons_init (void)
{
	/* Enable clock for PORTC. */
	ARM_RSTCLK->PER_CLOCK |= ARM_PER_CLOCK_GPIOE;

	/* Функция по умолчанию для кнопок. */
	unsigned func = ARM_GPIOE->FUNC;
	func &= ~( ARM_FUNC_MASK(BUTTON_RIGHT) | ARM_FUNC_MASK(BUTTON_LEFT)
                 | ARM_FUNC_MASK(BUTTON_DOWN)  | ARM_FUNC_MASK(BUTTON_UP)
                 | ARM_FUNC_MASK(BUTTON_SEL)   | ARM_FUNC_MASK(BUTTON_BACK) );
	ARM_GPIOE->FUNC = func;

	/* Цифровые выводы. */
	ARM_GPIOE->ANALOG |= ALL_BUTTONS;

	/* Обычный фронт. */
	ARM_GPIOE->PWR &= ~( ARM_PWR_MASK(BUTTON_RIGHT) | ARM_PWR_MASK(BUTTON_LEFT)
                           | ARM_PWR_MASK(BUTTON_DOWN)  | ARM_PWR_MASK(BUTTON_UP)
                           | ARM_PWR_MASK(BUTTON_SEL)   | ARM_PWR_MASK(BUTTON_BACK));
}
