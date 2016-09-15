/*
 * Проверка светодиодов на отладочной плате
 */
#include <runtime/lib.h>

#define LED_ON(n) (GPIOD->BSRR = GPIO_SET(12 + n))
#define LED_OFF(n) (GPIOD->BSRR = GPIO_RESET(12 + n))

#define NB_ROWS 3
#define NB_COLUMNS 4
#define NB_KEYS (NB_ROWS * NB_COLUMNS)
#define NB_DEBOUNCE_CYCLES 5

#define KEY_INPUT_MASK ((1 << NB_ROWS) - 1)

enum {
    KEY_1_PRESSED,
    KEY_2_PRESSED,
    KEY_3_PRESSED,
    KEY_4_PRESSED,
    KEY_5_PRESSED,
    KEY_6_PRESSED,
    KEY_7_PRESSED,
    KEY_8_PRESSED,
    KEY_9_PRESSED,
    KEY_A_PRESSED,
    KEY_0_PRESSED,
    KEY_B_PRESSED,
    KEY_1_RELEASED,
    KEY_2_RELEASED,
    KEY_3_RELEASED,
    KEY_4_RELEASED,
    KEY_5_RELEASED,
    KEY_6_RELEASED,
    KEY_7_RELEASED,
    KEY_8_RELEASED,
    KEY_9_RELEASED,
    KEY_A_RELEASED,
    KEY_0_RELEASED,
    KEY_B_RELEASED,
    NB_OF_EVENTS
};

typedef void (*event_handler) (void);
event_handler handlers[NB_OF_EVENTS];

void init_gpio ()
{
    RCC->AHB1ENR |= RCC_GPIOAEN | RCC_GPIOBEN | RCC_GPIOCEN | 
        RCC_GPIODEN | RCC_GPIOEEN;
    
    // Настройка ножек для чтения клавиш
    GPIOD->MODER |= GPIO_OUT(8) | GPIO_OUT(9) | GPIO_OUT(10) | GPIO_OUT(11);
    GPIOD->OTYPER |= GPIO_OD(8) | GPIO_OD(9) | GPIO_OD(10) | GPIO_OD(11);
    GPIOD->PUPDR = GPIO_PULL_UP(8) | GPIO_PULL_UP(9) |
        GPIO_PULL_UP(10) | GPIO_PULL_UP(11);
        
    GPIOB->MODER = GPIO_IN(12) | GPIO_IN(13) | GPIO_IN(14);
    GPIOB->PUPDR = GPIO_PULL_UP(12) | GPIO_PULL_UP(13) | GPIO_PULL_UP(14);
    
    // Настройка управляющих выходов
    GPIOA->MODER |= GPIO_OUT(0) | GPIO_OUT(1) | GPIO_OUT(3) | GPIO_OUT(8);
    GPIOA->OTYPER |= GPIO_PP(0) | GPIO_PP(1) | GPIO_PP(3) | GPIO_PP(8);
    GPIOA->ODR = 0;

    GPIOB->MODER |= GPIO_OUT(0) | GPIO_OUT(1) | GPIO_OUT(4) | GPIO_OUT(5) | 
        GPIO_OUT(6) | GPIO_OUT(7) | GPIO_OUT(8) | GPIO_OUT(9) | GPIO_OUT(11) | 
        GPIO_OUT(15);
    GPIOB->OTYPER |= GPIO_PP(0) | GPIO_PP(1) | GPIO_PP(4) | GPIO_PP(5) |
        GPIO_PP(6) | GPIO_PP(7) | GPIO_PP(8) | GPIO_PP(9) | GPIO_PP(11) | 
        GPIO_PP(15);
    GPIOB->ODR = 0;

    GPIOC->MODER |= GPIO_OUT(1) | GPIO_OUT(2) | GPIO_OUT(6) | GPIO_OUT(7) | 
        GPIO_OUT(8) | GPIO_OUT(9) | GPIO_OUT(12) | GPIO_OUT(13);
    GPIOC->OTYPER |= GPIO_PP(1) | GPIO_PP(2) | GPIO_PP(6) | GPIO_PP(7) |
        GPIO_PP(8) | GPIO_PP(9) | GPIO_PP(12) | GPIO_PP(13);
    GPIOC->ODR = 0;

    GPIOD->MODER |= GPIO_OUT(0) | GPIO_OUT(6) | GPIO_OUT(7);
    GPIOD->OTYPER |= GPIO_PP(0) | GPIO_PP(6) | GPIO_PP(7);
    GPIOD->ODR = 0;
    
    GPIOE->MODER |= GPIO_OUT(2) | GPIO_OUT(4) | GPIO_OUT(7) | GPIO_OUT(8) | 
        GPIO_OUT(9) | GPIO_OUT(10) | GPIO_OUT(11) | GPIO_OUT(12) |
        GPIO_OUT(13) | GPIO_OUT(14) | GPIO_OUT(15);
    GPIOE->OTYPER |= GPIO_PP(2) | GPIO_PP(4) | GPIO_PP(7) | GPIO_PP(8) |
        GPIO_PP(9) | GPIO_PP(10) | GPIO_PP(11) | GPIO_PP(12) |
        GPIO_PP(13) | GPIO_PP(14) | GPIO_PP(15);
    GPIOE->ODR = 0;
 
    
    // LEDs
    GPIOD->MODER |= GPIO_OUT(12) | GPIO_OUT(13) | GPIO_OUT(14) | GPIO_OUT(15);
}

inline void lamp_01_white(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(15);
    else    GPIOB->BSRR = GPIO_RESET(15);
}

inline void lamp_01_blue(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(11);
    else    GPIOB->BSRR = GPIO_RESET(11);
}

inline void lamp_02_white(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(13);
    else    GPIOE->BSRR = GPIO_RESET(13);
}

inline void lamp_02_blue(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(15);
    else    GPIOE->BSRR = GPIO_RESET(15);
}

inline void lamp_03_white(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(14);
    else    GPIOE->BSRR = GPIO_RESET(14);
}

inline void lamp_03_blue(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(12);
    else    GPIOE->BSRR = GPIO_RESET(12);
}

inline void lamp_04_white(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(11);
    else    GPIOE->BSRR = GPIO_RESET(11);
}

inline void lamp_04_blue(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(10);
    else    GPIOE->BSRR = GPIO_RESET(10);
}

inline void lamp_05_white(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(9);
    else    GPIOE->BSRR = GPIO_RESET(9);
}

inline void lamp_05_blue(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(8);
    else    GPIOE->BSRR = GPIO_RESET(8);
}

inline void lamp_06_white(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(0);
    else    GPIOB->BSRR = GPIO_RESET(0);
}

inline void lamp_06_blue(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(7);
    else    GPIOE->BSRR = GPIO_RESET(7);
}

inline void lamp_07_white(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(1);
    else    GPIOB->BSRR = GPIO_RESET(1);
}

inline void lamp_07_blue(int on)
{
    if (on) GPIOA->BSRR = GPIO_SET(3);
    else    GPIOA->BSRR = GPIO_RESET(3);
}

inline void lamp_08_white(int on)
{
    if (on) GPIOA->BSRR = GPIO_SET(0);
    else    GPIOA->BSRR = GPIO_RESET(0);
}

inline void lamp_08_blue(int on)
{
    if (on) GPIOC->BSRR = GPIO_SET(1);
    else    GPIOC->BSRR = GPIO_RESET(1);
}

inline void lamp_09_white(int on)
{
    if (on) GPIOA->BSRR = GPIO_SET(1);
    else    GPIOA->BSRR = GPIO_RESET(1);
}

inline void lamp_09_blue(int on)
{
    if (on) GPIOC->BSRR = GPIO_SET(2);
    else    GPIOC->BSRR = GPIO_RESET(2);
}

inline void lamp_10_white(int on)
{
    if (on) GPIOC->BSRR = GPIO_SET(6);
    else    GPIOC->BSRR = GPIO_RESET(6);
}

inline void lamp_10_blue(int on)
{
    if (on) GPIOA->BSRR = GPIO_SET(8);
    else    GPIOA->BSRR = GPIO_RESET(8);
}

inline void lamp_11_white(int on)
{
    if (on) GPIOC->BSRR = GPIO_SET(7);
    else    GPIOC->BSRR = GPIO_RESET(7);
}

inline void lamp_11_blue(int on)
{
    if (on) GPIOD->BSRR = GPIO_SET(0);
    else    GPIOD->BSRR = GPIO_RESET(0);
}

inline void lamp_12_white(int on)
{
    if (on) GPIOC->BSRR = GPIO_SET(8);
    else    GPIOC->BSRR = GPIO_RESET(8);
}

inline void lamp_12_blue(int on)
{
    if (on) GPIOC->BSRR = GPIO_SET(12);
    else    GPIOC->BSRR = GPIO_RESET(12);
}

inline void lamp_13_white(int on)
{
    if (on) GPIOC->BSRR = GPIO_SET(9);
    else    GPIOC->BSRR = GPIO_RESET(9);
}

inline void lamp_13_blue(int on)
{
    if (on) GPIOD->BSRR = GPIO_SET(6);
    else    GPIOD->BSRR = GPIO_RESET(6);
}

inline void lamp_14_white(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(4);
    else    GPIOB->BSRR = GPIO_RESET(4);
}

inline void lamp_14_blue(int on)
{
    if (on) GPIOD->BSRR = GPIO_SET(7);
    else    GPIOD->BSRR = GPIO_RESET(7);
}

inline void lamp_15_white(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(5);
    else    GPIOB->BSRR = GPIO_RESET(5);
}

inline void lamp_15_blue(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(9);
    else    GPIOB->BSRR = GPIO_RESET(9);
}

inline void lamp_16_white(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(6);
    else    GPIOB->BSRR = GPIO_RESET(6);
}

inline void lamp_16_blue(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(2);
    else    GPIOE->BSRR = GPIO_RESET(2);
}

inline void lamp_17_white(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(7);
    else    GPIOB->BSRR = GPIO_RESET(7);
}

inline void lamp_17_blue(int on)
{
    if (on) GPIOE->BSRR = GPIO_SET(4);
    else    GPIOE->BSRR = GPIO_RESET(4);
}

inline void lamp_18_white(int on)
{
    if (on) GPIOB->BSRR = GPIO_SET(8);
    else    GPIOB->BSRR = GPIO_RESET(8);
}

inline void lamp_18_blue(int on)
{
    if (on) GPIOC->BSRR = GPIO_SET(13);
    else    GPIOC->BSRR = GPIO_RESET(13);
}


unsigned key_state[NB_DEBOUNCE_CYCLES];
unsigned debounced_keys = 0;

void key_1_pressed()
{
    // Все горят
    lamp_01_white(1);
    lamp_02_white(1);
    lamp_03_white(1);
    lamp_04_white(1);
    lamp_05_white(1);
    lamp_06_white(1);
    lamp_07_white(1);
    lamp_08_white(1);
    lamp_09_white(1);
    lamp_10_white(1);
    lamp_11_white(1);
    lamp_12_white(1);
    lamp_13_white(1);
    lamp_14_white(1);
    lamp_15_white(1);
    lamp_16_white(1);
    lamp_17_white(1);
}

void key_2_pressed()
{
    // Прихожая
    lamp_01_white(0);
    lamp_02_white(0);
    lamp_03_white(0);
    lamp_04_white(0);
    lamp_05_white(0);
    lamp_06_white(0);
    lamp_07_white(0);
    lamp_08_white(0);
    lamp_09_white(0);
    lamp_10_white(0);
    lamp_11_white(0);
    lamp_12_white(0);
    lamp_13_white(0);
    lamp_14_white(0);
    lamp_15_white(1);
    lamp_16_white(1);
    lamp_17_white(1);
}

void key_3_pressed()
{
    // Гостиная
    lamp_01_white(1);
    lamp_02_white(1);
    lamp_03_white(1);
    lamp_04_white(1);
    lamp_05_white(1);
    lamp_06_white(1);
    lamp_07_white(1);
    lamp_08_white(1);
    lamp_09_white(1);
    lamp_10_white(1);
    lamp_11_white(1);
    lamp_12_white(1);
    lamp_13_white(1);
    lamp_14_white(1);
    lamp_15_white(1);
    lamp_16_white(0);
    lamp_17_white(0);
}

void key_4_pressed()
{
    // Глажка
    lamp_01_white(0);
    lamp_02_white(0);
    lamp_03_white(0);
    lamp_04_white(0);
    lamp_05_white(1);
    lamp_06_white(1);
    lamp_07_white(0);
    lamp_08_white(0);
    lamp_09_white(1);
    lamp_10_white(0);
    lamp_11_white(0);
    lamp_12_white(0);
    lamp_13_white(0);
    lamp_14_white(0);
    lamp_15_white(0);
    lamp_16_white(0);
    lamp_17_white(0);
}

void key_5_pressed()
{
    // Подсветка по периметру гостиной
    lamp_01_white(1);
    lamp_02_white(1);
    lamp_03_white(1);
    lamp_04_white(1);
    lamp_05_white(0);
    lamp_06_white(1);
    lamp_07_white(1);
    lamp_08_white(0);
    lamp_09_white(1);
    lamp_10_white(1);
    lamp_11_white(0);
    lamp_12_white(1);
    lamp_13_white(1);
    lamp_14_white(1);
    lamp_15_white(1);
    lamp_16_white(0);
    lamp_17_white(0);
}

void key_6_pressed()
{
    // Подсветка центра гостиной
    lamp_01_white(0);
    lamp_02_white(0);
    lamp_03_white(0);
    lamp_04_white(0);
    lamp_05_white(1);
    lamp_06_white(0);
    lamp_07_white(0);
    lamp_08_white(0);
    lamp_09_white(0);
    lamp_10_white(0);
    lamp_11_white(1);
    lamp_12_white(0);
    lamp_13_white(0);
    lamp_14_white(0);
    lamp_15_white(0);
    lamp_16_white(0);
    lamp_17_white(0);
}

void key_7_pressed()
{
    // Шахматка 1 в гостиной
    lamp_01_white(0);
    lamp_02_white(1);
    lamp_03_white(0);
    lamp_04_white(1);
    lamp_05_white(0);
    lamp_06_white(1);
    lamp_07_white(0);
    lamp_08_white(1);
    lamp_09_white(0);
    lamp_10_white(1);
    lamp_11_white(0);
    lamp_12_white(1);
    lamp_13_white(0);
    lamp_14_white(1);
    lamp_15_white(0);
    lamp_16_white(0);
    lamp_17_white(0);
}

void key_8_pressed()
{
    // Подсветка середины гостиной
    lamp_01_white(0);
    lamp_02_white(1);
    lamp_03_white(0);
    lamp_04_white(0);
    lamp_05_white(1);
    lamp_06_white(0);
    lamp_07_white(0);
    lamp_08_white(1);
    lamp_09_white(0);
    lamp_10_white(0);
    lamp_11_white(1);
    lamp_12_white(0);
    lamp_13_white(0);
    lamp_14_white(1);
    lamp_15_white(0);
    lamp_16_white(0);
    lamp_17_white(0);
}

void key_9_pressed()
{
    // Шахматка 2 в гостиной
    lamp_01_white(1);
    lamp_02_white(0);
    lamp_03_white(1);
    lamp_04_white(0);
    lamp_05_white(1);
    lamp_06_white(0);
    lamp_07_white(1);
    lamp_08_white(0);
    lamp_09_white(1);
    lamp_10_white(0);
    lamp_11_white(1);
    lamp_12_white(0);
    lamp_13_white(1);
    lamp_14_white(0);
    lamp_15_white(1);
    lamp_16_white(0);
    lamp_17_white(0);
}

void key_0_pressed()
{
    // Выключить белый свет
    lamp_01_white(0);
    lamp_02_white(0);
    lamp_03_white(0);
    lamp_04_white(0);
    lamp_05_white(0);
    lamp_06_white(0);
    lamp_07_white(0);
    lamp_08_white(0);
    lamp_09_white(0);
    lamp_10_white(0);
    lamp_11_white(0);
    lamp_12_white(0);
    lamp_13_white(0);
    lamp_14_white(0);
    lamp_15_white(0);
    lamp_16_white(0);
    lamp_17_white(0);
    lamp_18_white(0);
}

void key_ast_pressed()
{
    // Голубая подсветка
    static int on = 0;
    
    on = !on;
    
    lamp_01_blue(on);
    lamp_02_blue(on);
    lamp_03_blue(on);
    lamp_04_blue(on);
    lamp_05_blue(on);
    lamp_06_blue(on);
    lamp_07_blue(on);
    lamp_08_blue(on);
    lamp_09_blue(on);
    lamp_10_blue(on);
    lamp_11_blue(on);
    lamp_12_blue(on);
    lamp_13_blue(on);
    lamp_14_blue(on);
    lamp_15_blue(on);
    lamp_16_blue(on);
    lamp_17_blue(on);
}

void key_num_pressed()
{
    // Пока ничего
}


int bright2delay(int bright)
{
    switch (bright) {
    case 0:
        return 0;
    case 1:
        return 16;
    case 2:
        return 36;
    case 3:
        return 64;
    case 4:
        return 100;
    default:
        return 1000;
    }
}

int main (void)
{
	init_gpio ();
	


/*
int j = 0;
int brightness = 0;
int up = 1;
while (1) {
    if (brightness >= 5) {
        GPIOC->BSRR = (1 << 6);
        udelay(1000);
    } else {
        if (j & 1) {
            GPIOC->BSRR = (1 << 6) << 16;
            udelay(1000 - bright2delay(brightness));
        } else {
            GPIOC->BSRR = (1 << 6);
            udelay(bright2delay(brightness));
        }
    }
    j++;
    
    if (debug_peekchar() > 0) {
        debug_getchar();
        if (up) {
            if (++brightness >= 5)
                up = 0;
        } else {
            if (--brightness == 0)
                up = 1;
        }
        debug_printf("brightness = %d\n", brightness);
    }
}
*/

    debug_printf("Test keyboard\n");

	memset(key_state, 0xFF, sizeof(key_state));
	
    int col;
    int prev_col;
    int debounce_cycle = 0;
    unsigned debounced_ones;
    unsigned debounced_zeroes;
    int i;
    
    handlers[KEY_1_PRESSED] = key_1_pressed;
    handlers[KEY_2_PRESSED] = key_2_pressed;
    handlers[KEY_3_PRESSED] = key_3_pressed;
    handlers[KEY_4_PRESSED] = key_4_pressed;
    handlers[KEY_5_PRESSED] = key_5_pressed;
    handlers[KEY_6_PRESSED] = key_6_pressed;
    handlers[KEY_7_PRESSED] = key_7_pressed;
    handlers[KEY_8_PRESSED] = key_8_pressed;
    handlers[KEY_9_PRESSED] = key_9_pressed;
    handlers[KEY_0_PRESSED] = key_0_pressed;
    handlers[KEY_A_PRESSED] = key_ast_pressed;
    handlers[KEY_B_PRESSED] = key_num_pressed;
    
    GPIOD->BSRR = GPIO_SET(8) | GPIO_SET(9) | GPIO_SET(10) | GPIO_SET(11);
    
	for (;;) {
	    for (prev_col = 0; prev_col < NB_COLUMNS; ++prev_col) {
	        col = (prev_col + 1) % NB_COLUMNS;
    	    GPIOD->BSRR = GPIO_SET(prev_col + 8) | GPIO_RESET(col + 8);
    	    mdelay(2);
    	    key_state[debounce_cycle] = (key_state[debounce_cycle] &
    	        ~(KEY_INPUT_MASK << (col * NB_ROWS))) | 
    	        (((GPIOB->IDR >> 12) & KEY_INPUT_MASK) << (col * NB_ROWS));
	    }
	    debounce_cycle++;
	    if (debounce_cycle == NB_DEBOUNCE_CYCLES)
	        debounce_cycle = 0;
	    debounced_ones = ~key_state[0];
	    debounced_zeroes = key_state[0];
	    for (i = 1; i < NB_DEBOUNCE_CYCLES; ++i) {
    	    debounced_ones |= ~key_state[i];
    	    debounced_zeroes &= key_state[i];
    	}
        
        for (i = 0; i < NB_KEYS; ++i) {
            if (debounced_keys & (1 << i)) {
                if (debounced_zeroes & (1 << i)) {
                    debounced_keys &= ~(1 << i);
                    debug_printf("KEY %d released\n", i+1);
                    if (handlers[KEY_1_RELEASED + i])
                        handlers[KEY_1_RELEASED + i]();
                }
            } else {
                if (debounced_ones & (1 << i)) {
                    debounced_keys |= (1 << i);
                    debug_printf("KEY %d pressed, keys %X\n", i+1, debounced_keys);
                    if (handlers[KEY_1_PRESSED + i])
                        handlers[KEY_1_PRESSED + i]();
                }
            }
        }
	}
}
