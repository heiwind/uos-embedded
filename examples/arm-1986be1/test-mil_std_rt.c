#include <runtime/lib.h>
#include <kernel/uos.h>
#include <stream/stream.h>
#include <kernel/internal.h>
#include <milandr/mil1553-interface.h>
#include <milandr/mil-std-1553_setup.h>
#include <milandr/mil-std-1553_rt.h>

// Номер контроллера MIL-STD (всегда 0)
#define MY_MIL_STD_PORT 0

// Собственный адрес ОУ
#define MIL_STD_SELF 2

// Длина приемной очереди
#define MIL_RXQ_LEN	100

ARRAY (test_milstd_rt_stack, 1000);
mem_pool_t pool;

static milandr_mil1553_t mil;
static uint16_t slot_data_tx[32];
static uint16_t slot_data_rx[32];
static void test_milstd_rt_main();


void uos_init (void)
{
    debug_printf("rt_init\n");

    milandr_mil1553_init(&mil, 0, &pool, MIL_RXQ_LEN, ARM_TIMER1);
    milandr_mil1553_init_pins(0);

    task_create(test_milstd_rt_main, 0, "test_milstd_rt_main", 1, test_milstd_rt_stack, sizeof(test_milstd_rt_stack));
}

// example
static void test_milstd_rt_main()
{

	mil1553_set_mode(&mil.milif, MIL_MODE_RT);
	mil.addr_self = MIL_STD_SELF;
    mil1553_start(&mil.milif);

    // Подадрес для получения данных от КШ (КШ-ОУ)
    const unsigned char bc_subaddr_from = 8;
    // Подадрес для передачи данных в КШ (ОУ-КШ)
    const unsigned char bc_subaddr_to = 7;

    int i;
    int counter = 0;

    for (;;)
    {
    	slot_data_tx[0] = counter++;
    	mil_rt_send_16(&mil.milif, bc_subaddr_to, slot_data_tx, 1);

    	int wc = MIL_SUBADDR_WORDS_COUNT/2;
    	mil_rt_receive_16(&mil.milif, bc_subaddr_from, slot_data_rx, &wc);
    	debug_printf("wc = %d data=", wc);
        for (i=0;i<wc;i++) {
        	debug_printf(" %04x", slot_data_rx[i]);
        }
        mdelay(500);
    }
}
