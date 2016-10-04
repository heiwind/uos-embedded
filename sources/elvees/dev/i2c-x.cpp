/* ============================================================================
 * Copyright (c) 2014 Lityagin Alexander
 *
 ** @file i2c-port.h
 *
 *  @brief сюда выгружены изменения CSL от Пойгина
 *
 *  Path: \(CSLPATH)\ inc

 * ============================================================================
 */

#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>

#include <iic/miic.h>
#include <elvees/i2c.h>
#include "i2c-x.h"
//#include <multicore/mcsl_i2c.h>
//#include <multicore/mcsl_system.h>

#ifdef UOS_I2C_HAVE_TO
#include <timer/timer.h>

#if (USER_TIMERS > 0) && ((UOS_I2C_HAVE_TO & UOS_I2C_TO_ETIMER) != 0)
#include <timer/etimer.h>
#include <timer/etimer_threads.h>
#define UOS_I2C_TO      UOS_I2C_HAVE_TO
#elif (USER_TIMERS > 0)
#define UOS_I2C_TO      (UOS_I2C_HAVE_TO & ~UOS_I2C_TO_ETIMER)
#elif (TIMER_TIMEOUTS > 0)
#define UOS_I2C_TO      (UOS_I2C_HAVE_TO & ~(UOS_I2C_TO_ETIMER|UOS_I2C_TO_UTIMER))
#else
#define UOS_I2C_TO      (UOS_I2C_HAVE_TO & ~(UOS_I2C_TO_ETIMER|UOS_I2C_TO_UTIMER|UOS_I2C_TO_TIMEOUT))
#endif//#if USER_TIMERS > 0
#endif//UOS_I2C_HAVE_TO

//#include <trace.h>
//#include <tracelog.h>
#if (0)
#define I2C_puts(s)     debug_puts(s)
#define I2C_putchar(c)  debug_putchar(0, c)
#define I2C_printf(...) debug_printf(__VA_ARGS__)
#else
#define I2C_puts(s)
#define I2C_putchar(c)
#define I2C_printf(...)

//* это пробы-сигналы тестовых пинов, для визуализации обмена на осцилографе
#define trace_probe_i2c_start_on()
#define trace_probe_i2c_start_off()
#define trace_probe_i2c_work_on()
#define trace_probe_i2c_work_off()
#define trace_probe_irq_on()
#define trace_probe_irq_off()
#endif



#ifndef NULL
#define NULL ((void*)0)
#endif

enum i2c_io_state{
      i2cio_IDLE
    , i2cio_SLA
    , i2cio_ADDR
    , i2cio_XSLA
    , i2cio_xRSLA
    , i2cio_XRECV
    , i2cio_xTSLA
    , i2cio_XSEND
    , i2cio_BREAK
};

#if (UOS_I2C_TO > 0)
struct I2C_timeout_ctx{
    timer_t*        clock;
    clock_time_t    start;
#   if ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
    MUTEX_GROUP(2)  mg;
#   endif
#   if ((UOS_I2C_TO & UOS_I2C_TO_UTIMER) != 0)
    user_timer_t*   ut;
#   endif
#   if ((UOS_I2C_TO & UOS_I2C_TO_TIMEOUT) != 0)
    timeout_t       to;
#   endif
#   if ((UOS_I2C_TO & UOS_I2C_TO_ETIMER) != 0)
    etimer          ev;
#   endif
};
#endif

struct i2c_descriptor : public I2C_basic 
{
#if (UOS_I2C_TO > 0)
    I2C_timeout_ctx     TO;
#endif

    unsigned char   file_sla;

    unsigned char   sla;

    //автомат I2C_xfer
    i2c_io_state    state;
    i2c_io_option   mode;

    const unsigned char* adata;
    unsigned        alen;
    unsigned char*       xdata;
    unsigned        xlen;

    enum ioDModes{
      i2co_IDLE       = 0
    , i2co_XMIT       = 2
    , i2co_DRECV      = i2co_XRECV | i2co_XMIT
    , i2co_DSEND      = i2co_XSEND | i2co_XMIT
    };

    //локальный автомат пересылки
    unsigned char*  data;
    unsigned        dlen;
    i2c_io_option   dmode;

    void Xfer_init(i2c_io_option mode, const void* from, unsigned len){
        dmode = i2c_io_option(mode | i2co_XMIT);
        data  = (unsigned char*)from;
        dlen = len;
    }

    bool isXfer() const {return (dmode & i2co_XMIT) != 0;};
    void Xfer_stop() {dmode = i2c_io_option(dmode & ~i2co_XMIT);};
};

i2c_descriptor    I2C_port;
i2cHandle I2C_handle(int No){
    i2cHandle res = {&I2C_port};
    return res;
}

#if defined(ELVEES) //_NVCOM01

bool_t trx (miic_t *c, small_uint_t sla
        , void *tb, small_uint_t ts
        , void *rb, small_uint_t rs
        );

void i2c_reset(i2c_descriptor& io);
bool_t I2C_ISR_stop(i2c_descriptor& io);

i2cHandle I2C_init_master(/*I2C_REGS_TYPE*/ void* io, uint32_t rate, unsigned long TOms)
{
    if (I2C_port.lock.master != NULL)
        return I2C_handle(0);
    I2C_port.transaction = trx;
    I2C_port.rate        = rate;
    I2C_port.TOinterval     = TOms*USER_TIMER_MS;
    //etimer_init(&I2C_port.TO);
    i2c_reset(I2C_port);
#if (UOS_I2C_TO > 0)
    memset(&I2C_port.TO, 0, sizeof(I2C_port.TO));
#   if ((UOS_I2C_TO & UOS_I2C_TO_TIMEOUT) != 0)
    timeout_clear(&I2C_port.TO.to);
#   endif
#   if ((UOS_I2C_TO & UOS_I2C_TO_ETIMER) != 0)
    etimer_init(&I2C_port.TO.ev);
#   endif
#endif

    //i2c_descriptor& self = *((i2c_descriptor*)io);
    //mutex_attach_irq (&self.irq_lock, IRQ_EVT_I2C, &(I2C_ISR), io);
    return I2C_handle(0);
}

I2C_Status I2C_close_master(i2cHandle h){
    i2c_descriptor& self = *((i2c_descriptor*)h.port);
    if (I2C_port.lock.master != NULL)
        return I2C_ERROR;
#if ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
    mutex_group_unlisten (&self.TO.mg.g);
#endif
#   if ((UOS_I2C_TO & UOS_I2C_TO_ETIMER) != 0)
    etimer_stop(&self.TO.ev);
#endif
#   if ((UOS_I2C_TO & UOS_I2C_TO_UTIMER) != 0)
    user_timer_stop(self.TO.ut);
#endif
#   if ((UOS_I2C_TO & UOS_I2C_TO_TIMEOUT) != 0)
    timeout_stop(&self.TO.to);
#endif
    return I2C_OK;
}

#if (UOS_I2C_TO != 0)
//* назначает сигнал поллинга таймаута
void I2C_assign_timer(i2cHandle h, unsigned long TOms, timer_t* clock){
#   if ((UOS_I2C_TO & UOS_I2C_TO_ETIMER) != 0)
    if (etimer_system_online()){
        i2c_descriptor* self = (i2c_descriptor*)h.port;
        self->TO.clock = clock;
        self->TOinterval = TOms;
        etimer_init(&self->TO.ev);
        etimer_assign_mutex(&self->TO.ev, &self->irq_lock, h.port);
        I2C_puts("i2c:etimer use\n");
    }
    else
#endif
#if ((UOS_I2C_TO & UOS_I2C_TO_TIMEOUT) != 0)
    {
    I2C_assign_timeout(h, TOms, clock);
    I2C_puts("i2c:timeouts use\n");
    }
#elif ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
    I2C_assign_signal(h, TOms, clock, &clock->lock);
#else
    assert(etimer_system_online());
#endif
}

#if ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
//* назначает сигнал поллинга таймаута
void I2C_assign_signal(i2cHandle h, unsigned long TOms, timer_t* clock, mutex_t* signal){
    i2c_descriptor& self = *((i2c_descriptor*)h);
    self.TO.clock = clock;
    self.TOinterval = TOms;
    mutex_group_init (self.TO.mg.data , sizeof(self.TO.mg.data));
    mutex_group_add (&self.TO.mg.g, &self.irq_lock);
    mutex_group_add (&self.TO.mg.g, signal);
    mutex_group_listen (&self.TO.mg.g);
}
#endif

#if ((UOS_I2C_HAVE_TO & UOS_I2C_TO_UTIMER) != 0)
//* назначает сигнал поллинга таймаута
void I2C_assign_utimer(i2cHandle h, unsigned long TOms, timer_t* clock, user_timer_t* signal){
    i2c_descriptor& self = *((i2c_descriptor*)h);
    self.TO.ut = signal;
    user_timer_stop(signal);
#   if ((UOS_I2C_HAVE_TO & UOS_I2C_TO_TIMEOUT) != 0)
    I2C_assign_timeout(h, TOms, NULL);
#   endif
    I2C_assign_signal(h, TOms, clock, &signal->lock);
}
#endif

#if ((UOS_I2C_HAVE_TO & UOS_I2C_TO_TIMEOUT) != 0)

void I2C_on_timeout(timeout_t *to, void *arg){
    I2C_putchar('"');
    i2c_descriptor& self = *((i2c_descriptor*)arg);
    self.state = i2cio_BREAK;
}

//* назначает сигнал поллинга таймаута
void I2C_assign_timeout(i2cHandle h, unsigned long TOms, timer_t* clock)
{
    i2c_descriptor& self = *((i2c_descriptor*)h);
    timeout_t* signal = &self.TO.to;
    timeout_stop(signal);
    timeout_set_autoreload(signal, tsLoadRemove);
    if (clock != NULL) {
        self.TO.clock = clock;
        self.TOinterval = TOms;
        timeout_set_mutex(signal, &self.irq_lock, h);
        timeout_set_handler(signal, I2C_on_timeout, h);
        self.TO.to.timer = clock;
#       if ((UOS_I2C_HAVE_TO & UOS_I2C_TO_UTIMER) != 0)
        self.TO.ut = NULL;
#       endif
    }
    else {
        timeout_stop(signal);
        self.TO.to.timer = clock;
    }
}
#endif



static unsigned i2c_to_polls = 0;

void I2C_TO_start(i2c_descriptor& self){
    self.TO.start   =  timer_milliseconds(self.TO.clock);
    i2c_to_polls = 0;

#   if ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
    if (self.TO.mg.g.num == 0)
#   endif
    //for non-group mutex, waiting do on locked mutex
    mutex_lock(&self.irq_lock);

#   if ((UOS_I2C_TO & UOS_I2C_TO_TIMEOUT) != 0)
    if (self.TO.to.timer != NULL) {
        timeout_set_value(&self.TO.to, self.TOinterval);
        timeout_start(&self.TO.to);
        return;
    }
#   endif

#   if ((UOS_I2C_TO & UOS_I2C_TO_UTIMER) != 0)
    if (self.TO.ut != NULL){
        if (list_is_empty(&(self.TO.ut->to.item)))
            I2C_putchar('!');
        user_timer_arm_nomt(self.TO.ut, self.TOinterval);
        //* usertimer assigned as signal source on mutexgroup
        //return;
    }
#   endif

#   if ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
    if (self.TO.mg.g.num > 0){
        arch_intr_allow (self.irq_lock.irq->irq);
        mutex_group_relisten(&self.TO.mg.g);
        arch_intr_allow (self.irq_lock.irq->irq);
        return;
    }
#   endif

#   if ((UOS_I2C_TO & UOS_I2C_TO_ETIMER) != 0)
    etimer_set(&self.TO.ev, self.TOinterval*USER_TIMER_MS);
#   endif
}

void I2C_TO_stop(i2c_descriptor& self){

#   if ((UOS_I2C_TO & UOS_I2C_TO_TIMEOUT) != 0)
    if (self.TO.to.timer != NULL) {
        timeout_break(&self.TO.to);
    }
#   endif

#   if ((UOS_I2C_TO & UOS_I2C_TO_UTIMER) != 0)
    if (self.TO.ut != NULL){
        user_timer_stop(self.TO.ut);
    }
#   endif

#   if ((UOS_I2C_TO & UOS_I2C_TO_ETIMER) != 0)
    etimer_stop(&self.TO.ev);
#   endif

    //for non-group mutex, waiting do on locked mutex
#   if ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
    if (self.TO.mg.g.num == 0)
#   endif
    mutex_unlock(&self.irq_lock);

    I2C_printf(" p:%d by %lu ms", i2c_to_polls
                , ( timer_milliseconds(self.TO.clock) - self.TO.start )
                );
}

INLINE
bool_t I2C_is_timeout(void* arg){
    i2c_descriptor& self = *((i2c_descriptor*)arg);
    i2c_to_polls++;
    I2C_putchar('`');

#   if ((UOS_I2C_TO & UOS_I2C_TO_TIMEOUT) != 0)
    if (self.TO.to.timer != NULL) {
        return timeout_expired(&self.TO.to);
    }
#   endif

#   if ((UOS_I2C_TO & UOS_I2C_TO_UTIMER) != 0)
    if (self.TO.ut != NULL)
        return user_timer_expired(self.TO.ut);
#   endif

#   if ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
    if (self.TO.mg.g.num > 0){
        return timer_passed(self.TO.clock, self.TO.start, self.TOinterval);
    }
#   endif

#   if ((UOS_I2C_TO & UOS_I2C_TO_ETIMER) != 0)
    return etimer_expired(&self.TO.ev);
#   endif
    return 1;
}

I2C_Status I2C_wait(i2c_descriptor& self){
    bool_t ok = 0;
    I2C_Status res = I2C_TIMEOUT;

#if ((UOS_I2C_TO & UOS_I2C_TO_MUTEX) != 0)
    if (self.TO.mg.g.num > 0){
        mutex_t*  lock;
        void *    msg;
        while (!I2C_is_timeout(&self)) {
            mutex_group_wait(&self.TO.mg.g, &lock, &msg);
            ok = (lock == &self.irq_lock);
            if (ok){
                I2C_putchar('~');
                res = I2C_OK;
                break;
            }
        }
    }
    else
#endif

    {
      ok = mutex_wait_until(&self.irq_lock, I2C_is_timeout, &self);
      if(ok) {
          I2C_putchar('~');
      if (!I2C_is_timeout( &self)) {
          res = I2C_OK;
      }
      }
    }

    I2C_TO_stop(self);

    return res;
}

#else  //#if (UOS_I2C_TO != 0)

I2C_Status I2C_wait(i2c_descriptor& self){
    mutex_wait (&self.irq_lock);
    return I2C_OK;
}

#endif //#if (UOS_I2C_TO != 0)



bool_t I2C_ISR(void * data);

/** операции и2ц с выделеным адресом. адресная часть - передается как нормальные данные
 * */
/** тоже самое что и miic_transfer(port, SLA, addr, alen, dara, dlen)
 * \sa miic_transfer
 */
I2C_Status I2C_xfer(i2cHandle port, unsigned char SLA
          , const void* addr, unsigned alen
          , void* data, unsigned dlen
          , i2c_io_option mode
         )
{

    I2C_Status res = I2C_OK;
    i2c_descriptor& io = I2C_port;
    mutex_lock(&io.lock);
    while (io.state != i2cio_IDLE) {
         mutex_wait(&io.lock);
    }
    if (io.state == i2cio_IDLE){
        I2C_putchar('<');
        //mutex_lock_irq (&io.irq_lock, IRQ_EVT_I2C, &(I2C_ISR), (void*)&io);
        mutex_attach_irq (&io.irq_lock, MC_IRQ_EVT_I2C, &(I2C_ISR), (void*)&io);
        trace_probe_i2c_start_on();
        io.adata    = (const unsigned char*)addr;
        io.alen     = alen;
        io.xdata    = (unsigned char*)data;
        io.xlen     = dlen;
        io.mode     = mode;
        //io.dmode    = i2co_IDLE;
        io.Xfer_stop();

        SLA = SLA <<1;
        if (alen > 0)
            //отсылаем адрес
            SLA &= ~1;
        else if ((mode & i2cio_XSEND) != 0)
            //только отсылаем data
            SLA &= ~1;
        else
            //только читаем data
            SLA |= 1;
        io.sla      = SLA;
        io.state    = i2cio_SLA;

        I2C_printf("-%02x", SLA);
        I2C_TO_start(io);
        MC_I2C_TXR  = SLA;
        MC_I2C_CR = MC_I2C_SND | MC_I2C_STA;
        trace_probe_i2c_start_off();

        I2C_putchar('[');
        trace_probe_i2c_work_on();
        res = I2C_wait (io);
        trace_probe_i2c_work_off();
        I2C_putchar(']');

        if (res != I2C_TIMEOUT) {
        if ( ((MC_I2C_SR & MC_I2C_AL) == 0) 
                && (io.dlen == 0)
            )
            res = I2C_OK;
        else
            res = I2C_ERROR;
        }

        if (io.state == i2cio_IDLE);
        else{
            //сохраню длину отосланого у надлежащем месте
            I2C_putchar('#');
            switch (io.state){
                case i2cio_SLA:
                    res = I2C_NOSLA;
                    break;

                case i2cio_xRSLA:
                case i2cio_xTSLA:
                    io.alen = io.dlen;
                    break;
    
                case i2cio_XSEND:
                case i2cio_XRECV:
                    io.xlen = io.dlen;
                    break;
                default:;
            }
            io.state = i2cio_IDLE;
            I2C_ISR_stop(io);
        }
        io.irq_lock.irq->lock = 0;
    }
    else
        res = I2C_BUSY;
    //mutex_unlock_irq (&io.irq_lock);
    I2C_putchar('>');
    mutex_signal(&io.lock, (void*)&io);
    mutex_unlock(&io.lock);
    return res;
}

bool_t
trx (miic_t *c, small_uint_t sla, void *tb, small_uint_t ts,
    void *rb, small_uint_t rs)
{
    return (I2C_xfer(I2C_handle(0), sla, tb, ts, rb, rs, i2co_XSEND) == I2C_OK)? 1: 0; //&I2C_port
}


CODE_ISR bool_t I2C_ISR_xfer(i2c_descriptor& io);
CODE_ISR bool_t I2C_OnEvent(i2c_io_state    state);

//bool_t I2C_ISR_xfer(i2c_descriptor& io);

CODE_ISR 
bool_t I2C_ISR(void * data){
    i2c_descriptor& io = I2C_port;

    if (io.state == i2cio_BREAK){
        return 0;
    }

    trace_probe_irq_on();

    unsigned char flags = MC_I2C_SR;
    MC_I2C_CR = MC_I2C_IACK;
    bool_t ok = 1;

    if (flags & MC_I2C_TIP){
        I2C_putchar('&');
        //wait transfer finish, so just propagate event
        trace_probe_irq_off();
        arch_intr_allow (io.irq_lock.irq->irq);
        return 0;
    }

    if (flags & MC_I2C_AL){
        //conflict!
        ok = 0;
    }

    //протокол сам отпустил шину, потому не считаем этот стоп прерыванием обмена
    if ((io.dmode & MC_I2C_STO) == 0)
    if ((flags & MC_I2C_BUSY) == 0){
        // bus cant be accessed or lost
        //io.dlen = 0;
        ok = 0;
    }
    if (!ok){
        trace_probe_irq_off();
        I2C_printf("$%02x", flags);
        return 0;
    }

    bool_t res = 0;
    if (io.isXfer()){
        res = I2C_ISR_xfer(io);
        if (!res)
            io.Xfer_stop();
    }
    if (!res)
        res = I2C_OnEvent(io.state);

    trace_probe_irq_off();
    if (res)
        arch_intr_allow (io.irq_lock.irq->irq);
    I2C_putchar('('+res);
    return res;
}

CODE_ISR 
bool_t I2C_OnEvent(i2c_io_state    state){
    I2C_putchar(char('0'+unsigned(state)) );
    i2c_descriptor& io = I2C_port;
    unsigned char flags = MC_I2C_SR;
    switch (state){
    case i2cio_SLA:
        MC_I2C_CR = MC_I2C_IACK;
        if (flags & (MC_I2C_AL | MC_I2C_RXACK)){
            //conflict!
            I2C_printf("^%02x", flags);
            return 0;
        }

        if ( (((unsigned)(io.adata)) != 0) && (io.alen != 0) ) {
            io.Xfer_init(i2co_XSEND, io.adata, io.alen);
            io.state = i2cio_ADDR;
            if (I2C_ISR_xfer(io))
                return 1;
        }

    case i2cio_ADDR:
        if ( (((unsigned)(io.xdata)) == 0) || (io.xlen == 0) ) {
            io.state = i2cio_IDLE;
            return 0;
        }

    case i2cio_XSLA:
        if ((io.mode & i2co_XSEND) != 0){
            io.dmode = (i2c_io_option)(i2co_XSEND| MC_I2C_STO);
            io.state = i2cio_xTSLA;
            if ((io.mode & i2co_XFrame) != 0){
                //MC_I2C_CR   = MC_I2C_STO;
                I2C_printf("\\%02x", io.sla);
                MC_I2C_TXR  = io.sla & ~1;
                MC_I2C_CR = MC_I2C_SND | MC_I2C_STA;
                return 1;
            }
        }
        else{
            io.dmode = (i2c_io_option)(i2co_XRECV | MC_I2C_STO);
            io.state = i2cio_xRSLA;
            if ((io.mode & i2co_OneFrame) == 0){
                //MC_I2C_CR   = MC_I2C_STO;
                I2C_printf("/%02x", io.sla);
                MC_I2C_TXR  = io.sla |1;
                MC_I2C_CR = MC_I2C_SND | MC_I2C_STA;
                return 1;
            }
        }
        return I2C_OnEvent(io.state);
        break;

    case i2cio_xRSLA:
        io.Xfer_init(io.dmode, io.xdata, io.xlen);
        io.state = i2cio_XRECV;
        io.dlen--;
        if (io.dlen > 0)
            MC_I2C_CR = MC_I2C_RCV;
        else
            MC_I2C_CR = MC_I2C_RCV | MC_I2C_NACK | ((char)io.dmode & MC_I2C_STO);
        break;

    case i2cio_xTSLA:
        io.Xfer_init(io.dmode, io.xdata, io.xlen);
        io.state = i2cio_XSEND;
        return I2C_ISR_xfer(io);
        break;

    case i2cio_XRECV:
    case i2cio_XSEND:
        io.state = i2cio_IDLE;
        if (flags & MC_I2C_BUSY)
        if ( (char)io.dmode & MC_I2C_STO )
            MC_I2C_CR = MC_I2C_STO;
        return 0;
        return 0;
        break;

    case i2cio_IDLE:
    default:
        return 0;
    };

    return 1;
}

CODE_ISR 
bool_t I2C_ISR_xfer(i2c_descriptor& io){
    if ((io.dmode & i2co_XSEND) != 0){
        //I2C_putchar('*');
        if (io.dlen <= 0)
            return 0;
        if (MC_I2C_SR & MC_I2C_RXACK)
            return 0;
        unsigned tmp = *io.data++;
        I2C_printf("*%02x", tmp);
        MC_I2C_TXR = tmp;
        io.dlen--;
        if (io.dlen > 0)
            MC_I2C_CR = MC_I2C_SND;
        else
            MC_I2C_CR = MC_I2C_SND | ((char)io.dmode & MC_I2C_STO);
    }
    else{
        //I2C_putchar('+');
        unsigned tmp = MC_I2C_RXR;
        *io.data++ = tmp;
        I2C_printf("+%02x", tmp);
        if (io.dlen <= 0){
            return 0;
        }
        io.dlen--;
        if (io.dlen > 0)
            MC_I2C_CR = MC_I2C_RCV;
        else
            MC_I2C_CR = MC_I2C_RCV | MC_I2C_NACK | ((char)io.dmode & MC_I2C_STO);
    }
    return 1;
}

inline
bool_t I2C_ISR_stop(i2c_descriptor& io)
{
    io.Xfer_stop();
    io.state = i2cio_IDLE;
    MC_I2C_CR = MC_I2C_STO;
    return 1;
}

inline
void i2c_reset(i2c_descriptor& io)
{
    MC_I2C_CTR = MC_I2C_PRST;
    MC_I2C_CTR = MC_I2C_EN | MC_I2C_IEN;
    MC_I2C_PRER = KHZ / (5 * io.rate) - 1;
}

/** это врап линуховых файловых операций на И2Цпорту.
 * они используют собственный SLA и не пересекаются с I2C_write/read
 * */
int read(i2cHandle port, void* data, int len){
    i2c_descriptor& io = I2C_port;
    I2C_Status res = I2C_xfer(port, io.file_sla, NULL, 0, data, len, i2co_XRECV);
    if (res == I2C_OK)
        return len;
    else if (res == I2C_NOSLA)
        return 0;
    else
        return len - io.xlen;
}

int write(i2cHandle port, const void* data, int len){
    i2c_descriptor& io = I2C_port;
    I2C_Status res = I2C_xfer(port, io.file_sla, NULL, 0, (void*)data, len, i2co_XSEND);
    if (res == I2C_OK)
        return len;
    else if (res == I2C_NOSLA)
        return 0;
    else
        return len - io.xlen;
}

int ioctl(i2cHandle port, I2C_ioctl_code code, int data){
    i2c_descriptor& io = I2C_port;
    io.file_sla = data;
    return 0;
}


#else
#   warning "i2c not implements on uncknown CHIP!!!"
#endif


