/*
 * Testing task switching.
 */
#include "runtime/lib.h"
#include "kernel/uos.h"
#include <kernel/internal.h>
#include <timer/timer.h>
#include <timer/etimer.h>
#include <timer/etimer_threads.h>
#include <mem/mem.h>

#include <stdint.h>

ARRAY (task_space, 0x800);	/* Memory for task stack */
ARRAY (task2_space, 0x800);  /* Memory for task stack */
ARRAY (task3_space, 0x800);  /* Memory for task stack */
ARRAY (taskloose_space, 0x800);  /* Memory for task stack */
unsigned count_init;		/* Time when uos_init() started */

timer_t  uos_timer;
mem_pool_t pool;

//* на нем тестирую таймаут ожидания мутекса с потерей захвата мутеха
//*     после получения сигнала
mutex_t     timeout_loose_room;

#define ETICK_US    1000

void dump_tasks(list_t* l);

int print_hook = 0;

void task (void *arg)
{
    unsigned timeout = (unsigned)arg;
	int cycle = 0;
	const char* name = task_current->name; 
	unsigned count_task;        /* Time when task() started */
	unsigned count_now;

    for(;;){
        count_task = mips_read_c0_register (C0_COUNT);
        debug_printf("task %S started at %d with timeout %d \n"
                , name
                , count_task - count_init
                , timeout
                );
    
        count_task = mips_read_c0_register (C0_COUNT);
        timer_delay(&uos_timer, 100);
        count_now = mips_read_c0_register (C0_COUNT);
        debug_printf("task %S delayed 100ms at %d \n", name, count_now - count_task);
        count_task = count_now;
    
        for (cycle = 0; cycle < 10; cycle++) {
            count_task = mips_read_c0_register (C0_COUNT);
            etimer_usleep(timeout);
            count_now = mips_read_c0_register (C0_COUNT);
            debug_printf("task %S delayed %d by %d (%+d us) \n", name, cycle
                    , count_now - count_task
                    , (count_now - count_task)/(KHZ/1000) - timeout
                    );
            count_task = count_now;
        }
        break;
    }
    task_exit(0);
}

void task_etm (void *arg)
{
    unsigned timeout = (unsigned)arg;
    etimer   et;
    mutex_t  m;
    int cycle = 0;
    const char* name = task_current->name; 
    unsigned count_task;        /* Time when task() started */
    unsigned count_now;

    etimer_init(&et);
    etimer_assign_mutex(&et, &m, 0);

    for(;;){
        count_task = mips_read_c0_register (C0_COUNT);
        debug_printf("task %S started at %d with timeout %d \n"
                , name
                , count_task - count_init
                , timeout
                );
    
        count_task = mips_read_c0_register (C0_COUNT);
        timer_delay(&uos_timer, 100);
        count_now = mips_read_c0_register (C0_COUNT);
        debug_printf("task %S delayed 100ms at %d \n", name, count_now - count_task);
        count_task = count_now;
    
        count_task = mips_read_c0_register (C0_COUNT);
        etimer_set(&et, timeout);
        for (cycle = 0; cycle < 10; cycle++) {
            mutex_wait(&m);
            count_now = mips_read_c0_register (C0_COUNT);

            debug_printf("task %S delayed %d by %d (%+d us) \n", name, cycle
                    , count_now - count_task
                    , (count_now - count_task)/(KHZ/1000) - timeout
                    );

            count_task = mips_read_c0_register (C0_COUNT);
            //etimer_reset(&et);
            etimer_restart(&et);
        }
        break;
    }

    debug_printf("task %S start loose_room\n", name);
    mutex_lock(&timeout_loose_room);
    debug_printf("task %S have loose_room\n", name);
    mutex_signal(&timeout_loose_room, 0);
    debug_printf("task %S signaled loose_room\n", name);
    etimer_usleep(4000*ETIMER_MS);
    debug_printf("task %S done\n", name);
    task_exit(0);
}

void task_loose (void *arg)
{
    etimer   et;
    mutex_t*  m = &timeout_loose_room;
    const char* name = task_current->name;

    debug_printf("task %S start with etimer $%p\n", name, &et);
    mutex_lock(m);
    etimer_init(&et);
    //etimer_assign_mutex(&et, &m, 0);
    etimer_assign_task(&et, task_current);
    etimer_set(&et, 2000*ETIMER_MS);
    debug_printf("task %S wait loose_room ...\n", name);
    bool_t ok = etimer_mutex_wait(m, &et);
    debug_printf("task %S wait loose_room done %d\n", name, ok);
    if (ok)
        mutex_unlock(m);
    task_exit(0);
}

void uos_init (void)
{

	/* Configure 16 Mbyte of external Flash memory at nCS3. */
	MC_CSCON3 = MC_CSCON_WS (4);		/* Wait states  */

    count_init = mips_read_c0_register (C0_COUNT);

    /* Выделяем место для динамической памяти */
    extern unsigned __bss_end[];
#ifdef ELVEES_DATA_SDRAM
    /* Динамическая память в SDRAM */
    if (((unsigned) __bss_end & 0xF0000000) == 0x80000000)
        mem_init (&pool, (unsigned) __bss_end, 0x82000000);
    else
        mem_init (&pool, (unsigned) __bss_end, 0xa2000000);
#else
    /* Динамическая память в CRAM */
    extern unsigned _estack[];
    mem_init (&pool, (unsigned) __bss_end, (unsigned) _estack - 256);
#endif

    assert (task_current == task_idle);
    mutex_init(&timeout_loose_room);

    timer_init_us (&uos_timer, KHZ, ETICK_US);
    etimer_system_init(&uos_timer);

    task_create (task_etm, (void*)(ETICK_US*20)       , "task_m"  , 1, task3_space, sizeof (task3_space));
    task_create (task, (void*)(ETICK_US*25+300)       , "task2"   , 1, task2_space, sizeof (task2_space));
    task_create (task, (void*)(ETICK_US*18-100)       , "task1"   , 1, task_space, sizeof (task_space));
    task_create (task_loose, (void*)(0)               , "taskloose", 1,taskloose_space, sizeof (taskloose_space));

}

///*
// * Call user initialization routine uos_init(),
// * then create the idle task, and run the OS.
// * The idle task uses the default system stack.
// 

unsigned tasks[2];

int t = 0;
void uos_on_timer_hook(timer_t *t)
{
    static int time_test_cnt = 0;
    static unsigned last_time = 0;
    unsigned time_now = mips_read_c0_register (C0_COUNT);
    if (time_test_cnt < 10)
    {
        debug_printf("%d", time_now - last_time);
        last_time = time_now;
        time_test_cnt++;
    }
    else
    if (print_hook != 0){
        t++;
        print_hook++;
        //debug_putchar(0, '#');
    }
    else if (task_need_schedule != 0)
        debug_putchar(0, ';');
    else
        debug_putchar(0, ',');
    if (0)//(task_current == task_idle)//(time_test_cnt < 10)
    {
        //debug_printf("%d", time_now - last_time);
        debug_printf("%d", time_now);
        last_time = time_now;
        time_test_cnt++;
    }
}

void dump_tasks(list_t* l){
    task_t* t;
    list_iterate (t, l) {
        task_print(&debug, t);
    }
}

void uos_on_task_switch(task_t *t)
{
    debug_printf("$%s\n", t->name);
}
