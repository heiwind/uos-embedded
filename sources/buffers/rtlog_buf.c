#include <runtime/lib.h>
#include <kernel/internal.h>
#include <stdarg.h>
#include "rtlog_buf.h"


#ifdef DEBUG_RTLOG
#define RTLOG_printf(...) debug_printf(__VA_ARGS__)
#else
#define RTLOG_printf(...)
#endif

//*\arg size - размер store в байтах. буфера выделяется как массив rtlog_node
//*            размером в степень2
void rtlog_init    ( rtlog* u, void* store, size_t size)
{
    assert(size > 0);
    size_t count = size / sizeof(rtlog_node);
    //rount to nearest power2
    count = (1u<<31) >> __builtin_clz(count) ;
    ring_uindex_init(&u->idx, count);
    u->store = (rtlog_node*)store;
    u->stamp = 0;
}

//* это эмуляторы аналогичных функций печати. количество сохраняемых аргументов 
//*     не более RTLOG_ARGS_LIMIT
int rtlog_printf  ( rtlog* u, unsigned nargs, ...){
    va_list args;
    int err;
    va_start (args, nargs);
    const char *fmt = va_arg(args, const char *);
    err = rtlog_vprintf(u, nargs, fmt, args);
    va_end (args);
    return err;

    
    if (ring_uindex_free(&u->idx) <= 0)
        return -1;

}

int rtlog_vprintf ( rtlog* u, unsigned nargs, const char *fmt, va_list args){
    unsigned slot;
    arch_state_t x;
    arch_intr_disable (&x);
    if (ring_uindex_full(&u->idx))
        // drop first message if full
        ring_uindex_get(&u->idx);
    slot = u->idx.write;
    rtlog_node* n = u->store + slot;
    n->stamp = u->stamp;
    ++(u->stamp);
    n->msg = fmt;
    int i;
    for (i = 0; i < nargs; i++)
        n->args[i] = va_arg(args, unsigned long);
    for (i = nargs; i < RTLOG_ARGS_LIMIT; i++)
        n->args[i] = 0;
    ring_uindex_put(&u->idx);
    arch_intr_restore (x);
    RTLOG_printf("rtlog: printf %s to %d[stamp%x] %d args : %x, %x, %x, %x, %x, %x\n"
                , fmt
                , slot, n->stamp
                , nargs
                , n->args[0], n->args[1], n->args[2], n->args[3]
                , n->args[4], n->args[5]
                );
    return 0;
}

int rtlog_puts( rtlog* u, const char *str){
    return rtlog_vprintf(u, 0, str, 0);
}

//* печатает records_count последних записей журнала в dst
void rtlog_dump_last( rtlog* u, stream_t *dst, unsigned records_count)
{
    unsigned slot;
    rtlog_node n;
    unsigned last_stamp = u->stamp;
    while (records_count > 0){
        arch_state_t x;
        arch_intr_disable (&x);

        unsigned avail = ring_uindex_avail(&u->idx);
        if (records_count > avail)
            records_count = avail;
        if (records_count > 0) {
            slot = u->idx.write;
            slot = (slot - records_count) & u->idx.mask;
            memcpy(&n, u->store+slot, sizeof(n));
        }
        arch_intr_restore (x);
        if (records_count == 0)
            break;
        --records_count;

        RTLOG_printf("rtdump: at %d[stamp%x] %s"
                    , slot, n.stamp
                    , n.msg
                    );

        if ( (last_stamp+1) != n.stamp)
            stream_printf(dst, ".... droped %d messages ....\n"
                          , (n.stamp - 1 - last_stamp) 
                          );
        last_stamp = n.stamp;

        stream_printf(dst, n.msg
                    , n.args[0], n.args[1], n.args[2], n.args[3]
                    , n.args[4], n.args[5]
                    );
    }
}
