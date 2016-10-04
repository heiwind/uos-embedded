/* ============================================================================
 * Copyright (c) 2015 Lityagin Alexander
 * ru utf8
 * здесь расширеная версия драйвера Elvees I2C с:
 *  * - более умным обработчиком прерываний. обработка на отдельный байт ведется вызываемой
 *      процедурой, что не требует перключения контекста, и облегчает жизнь процессору.
 *  * - поддержкой таймаутов. таймауты позволяют задать пределы ожидания устройств и шины
 *      и получить соотвествующий результат аварии
 *  * - поддержка расширеного интерфейса: I2C_read/write позволяют задавать длину адресной части
 *      , стиль отделения адреса от данных - в одном фрейме или с рестартом фрейма.
 *  * - поддержка linux-подобного интерфейса на устройсве i2cHandle read/write/close/ioctl
 *  * - расширеная поддержка АПИ С++
 * ============================================================================
 */

#ifndef _DEV_I2C_EXTENDED_
#define _DEV_I2C_EXTENDED_

#include <runtime/lib.h>
#include <kernel/uos.h>
#include <stdint.h>
#include <iic/miic.h>

#define UOS_I2C_TO_TIMER    1

#if (USER_TIMERS > 0) || (TIMER_TIMEOUTS > 0)
#define UOS_I2C_TO_ETIMER   2
#else
#define UOS_I2C_TO_ETIMER   0
#endif

//* ! user_timer_t устарело, вместо них используйте timeout_t
#if (USER_TIMERS > 0)
#define UOS_I2C_TO_UTIMER   4
#else
#define UOS_I2C_TO_UTIMER   0
#endif

#if (TIMER_TIMEOUTS > 0)
//* i2c использует собственный таймаут который надо настроить I2C_assign_timeout
//* и   или I2C_assign_timer
#define UOS_I2C_TO_TIMEOUT  8
#else
#define UOS_I2C_TO_TIMEOUT  0
#endif

//* таймаут может быть реализован с мутексом, на котором можно реализовать такое ожидание
//* тут задан перечень стилей таймаутов предоставляющих такой мутекс
#define UOS_I2C_TO_MUTEX    (UOS_I2C_TO_TIMER | UOS_I2C_TO_UTIMER)

//* задает стиль реализуемых таймаутов.
#define UOS_I2C_HAVE_TO     (UOS_I2C_TO_ETIMER)



#if (UOS_I2C_HAVE_TO > 0)
#include <timer/timer.h>
#endif
#if (UOS_I2C_HAVE_TO & UOS_I2C_TO_TIMEOUT)
#include <timer/timeout.h>
#endif



#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    MIIC_T;
    uint32_t        rate;
    unsigned long   TOinterval;
} I2C_basic;

typedef struct {I2C_basic* port;} i2cHandle;

i2cHandle I2C_handle(int No);

typedef enum {
      I2C_OK       = 0x00
    , I2C_ERROR    = 0x01
    , I2C_BUSY     = 0x02
    , I2C_TIMEOUT  = 0x03
    , I2C_NOSLA    = 0x04   //SLA not acknowledges
    , I2C_ERRCRC   = 0x05   //use for aside api
} I2C_Status;

i2cHandle I2C_init_master(/*I2C_REGS_TYPE*/ void* io, uint32_t rate_khz, unsigned long TOms);
I2C_Status I2C_close_master(i2cHandle h);

#if (UOS_I2C_HAVE_TO != 0)
//* назначает сигнал поллинга таймаута
void I2C_assign_timer(i2cHandle h, unsigned long TOms, timer_t* clock);
#endif

#if ((UOS_I2C_HAVE_TO & UOS_I2C_TO_MUTEX) != 0)
//* назначает сигнал поллинга таймаута
void I2C_assign_signal(i2cHandle h, unsigned long TOms, timer_t* clock, mutex_t* signal);
#endif

#if ((UOS_I2C_HAVE_TO & UOS_I2C_TO_UTIMER) != 0)
//* назначает сигнал поллинга таймаута
void I2C_assign_utimer(i2cHandle h, unsigned long TOms, timer_t* clock, user_timer_t* signal);
#endif

#if (UOS_I2C_HAVE_TO & UOS_I2C_TO_TIMEOUT)
void I2C_assign_timeout(i2cHandle h, unsigned long TOms, timer_t* clock);
#endif



typedef enum {
      i2co_XRECV      = 0
    , i2co_XSEND      = 1
    //!< XRECV делает без рестарта (отменяет рестарт фрейма) после передачи адреса.
    , i2co_OneFrame   = 4
    //!< XSEND - отсылает фрейм данных после рестрата (форсирует рестарт фрейма) после адресной части
    , i2co_XFrame     = 8
    //! подавляет освобождение шины после транзакции. обычный запрос освобождает шину после передачи данных.
    //, i2co_LockBus    = 0x10
} i2c_io_option;

I2C_Status I2C_xfer(i2cHandle port, unsigned char SLA
          , const void* addr, unsigned alen
          , void* data, unsigned dlen
          , i2c_io_option mode
          );

/** операции и2ц с выделеным адресом. адресная часть - передается как нормальные данные
 * */
/** тоже самое что и miic_transfer(port, SLA, addr, alen, dara, dlen)
 * \sa miic_transfer
 */
INLINE 
I2C_Status I2C_read(i2cHandle port, unsigned char SLA
          , const void* addr, unsigned alen
          , void* data, unsigned dlen
          )
{
    return I2C_xfer(port, SLA, addr, alen, data, dlen, i2co_XRECV);
}

INLINE 
I2C_Status I2C_write(i2cHandle port, unsigned char SLA
          , const void* addr, unsigned alen
          , void* data, unsigned dlen
          )
{
    return I2C_xfer(port, SLA, addr, alen, data, dlen, i2co_XSEND);
}

#ifdef __cplusplus
}

namespace i2c {

    struct i2c_file_t : public i2cHandle {
        typedef i2cHandle inherited;
        typedef i2cHandle base;

        int as_fid() {return (intptr_t)port;};
        int assign(int fid) {
            port = (I2C_basic*)(intptr_t)fid;
            return fid;
        };
        i2cHandle& assign(const i2cHandle& h) {
            port = h.port;
            return *this;
        };

        bool operator==(const int b){
            return (intptr_t)port == b;
        };
        bool operator!=(const int b){
            return (intptr_t)port != b;
        };
        bool operator!=(const i2cHandle& b){
            return port != b.port;
        };
        bool operator==(const i2cHandle& b){
            return port == b.port;
        };

        i2c_file_t& operator=(int a){
            assign(a);
            return *this;
        };
        i2c_file_t& operator=(const i2cHandle& a){
            assign(a);
            return *this;
        };
    };

/** это врап линуховых файловых операций на И2Цпорту.
 * они используют собственный SLA и не пересекаются с I2C_write/read
 * */
int read(i2cHandle port, void* data, int len);
int write(i2cHandle port, const void* data, int len);

enum I2C_ioctl_code{
    I2C_SLAVE
};
int ioctl(i2cHandle port, I2C_ioctl_code code, int data);
inline
int close(i2cHandle port){return 0;};

}; // namespace i2c

using namespace i2c;

#endif //__cplusplus

#endif    //_CSL_I2C_H_

