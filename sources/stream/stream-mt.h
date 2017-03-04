#ifndef __STREAM_MT_H_
#define __STREAM_MT_H_ 1
/*
 * Stream-MultiTread protected
 *
 *  Created on: 10.03.2016
 *      Author: a_lityagin
 *  utf8 ru
 *
 *  при конкурентном доступе к потоку, будут перемешиваться строки выводимые разными
 *  потоками. чтобы обеспечить их целостность надо вводить блокировку доступа потока
 *  этим занимается mt_stream_t
 */

// requested mutex_t
#include <kernel/uos.h>
#include <stream/stream.h>

#if STREAM_HAVE_ACCEESS > 0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    mutex_t rx;
    mutex_t tx;
} stream_io_access;

/** mt_stream_t - обертка-прокси над stream_t предоставляющая мутехи
 *  строкового доступа access_rx/tx.
 *  нужен чтобы обеспечить целостность строк выводимых print/scan функциями.
 *  см. uos-conf.h:STREAM_HAVE_ACCEESS
 * */
typedef struct {
        stream_interface_t *interface;
        stream_io_access    access;
        stream_t*           target;
} mt_stream_t;

void mtstream_init (mt_stream_t *u, stream_t* target);



#ifdef __cplusplus
}
#endif

#endif //STREAM_HAVE_ACCEESS > 0

#endif /* __STREAM_H_ */
