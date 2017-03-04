#include <stream/stream-mt.h>

#if STREAM_HAVE_ACCEESS > 0

#ifdef __cplusplus
#define idx(i)
#define item(i)
#else
#define idx(i) [i] =
#define item(i) .i =
#endif

void mtstream_putchar(stream_t *u, short c){
    mt_stream_t* self = (mt_stream_t*)u;
    self->target->interface->putc(self->target, c);
}

unsigned short mtstream_getchar (stream_t *u){
    mt_stream_t* self = (mt_stream_t*)u;
    return self->target->interface->getc(self->target);
}

int mtstream_peekchar (stream_t *u){
    mt_stream_t* self = (mt_stream_t*)u;
    return self->target->interface->peekc(self->target);
}

void mtstream_fflush(stream_t *u){
    mt_stream_t* self = (mt_stream_t*)u;
    stream_interface_t* i = self->target->interface;
    if (i->flush != 0)
        i->flush(self->target);
}

bool_t mtstream_eof(stream_t *u){
    mt_stream_t* self = (mt_stream_t*)u;
    stream_interface_t* i = self->target->interface;
    if (i->eof != 0)
        return i->eof(self->target);
    else
        return 0;
}

void mtstream_close(stream_t *u){
    mt_stream_t* self = (mt_stream_t*)u;
    stream_interface_t* i = self->target->interface;
    if (i->close != 0) {
        mutex_lock(&self->access.tx);
        mutex_lock(&self->access.rx);
        i->close(self->target);
        mutex_unlock(&self->access.rx);
        mutex_unlock(&self->access.tx);
    }
}

struct _mutex_t * mtstream_receiver(stream_t *u){
        mt_stream_t* self = (mt_stream_t*)u;
        stream_interface_t* i = self->target->interface;
        if (i->receiver != 0)
            return i->receiver(self->target);
        else
            return 0;
}

bool_t  mtstream_accessrx(stream_t *u, bool_t onoff){
    mt_stream_t* self = (mt_stream_t*)u;
    if (onoff)
        mutex_lock(&self->access.rx);
    else
        mutex_unlock(&self->access.rx);
    return 1;
}

bool_t  mtstream_accesstx(stream_t *u, bool_t onoff){
    mt_stream_t* self = (mt_stream_t*)u;
    if (onoff)
        mutex_lock(&self->access.tx);
    else
        mutex_unlock(&self->access.tx);
    return 1;
}

static stream_interface_t mtstream_proxy_interface = {
    item(putc) (void (*) (stream_t*, short))    mtstream_putchar,
    item(getc) (unsigned short (*) (stream_t*)) mtstream_getchar,
    item(peekc) (int (*) (stream_t*))           mtstream_peekchar,
    item(flush) (void (*) (stream_t*))          mtstream_fflush,
    item(eof)   (bool_t (*) (stream_t*))        mtstream_eof,
    item(close) (void (*) (stream_t*))          mtstream_close,
    item(receiver) (mutex_t *(*) (stream_t*))   mtstream_receiver,
    //* позволяют потребовать монопольного захвата потока
    item(access_rx) (bool_t  (*)(stream_t *u, bool_t onoff)) mtstream_accessrx,
    item(access_tx) (bool_t  (*)(stream_t *u, bool_t onoff)) mtstream_accesstx,
};

void mtstream_init (mt_stream_t *u, stream_t* target){
    u->interface = &mtstream_proxy_interface;
    u->target = target;
    //mutex_init(u->access.rx);
    //mutex_init(u->access.tx);
}

#endif // STREAM_HAVE_ACCEESS
