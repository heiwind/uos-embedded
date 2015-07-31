#include <runtime/lib.h>
#include "cyc_buf.h"

void cyc_buf_init (cyc_buf_t *cb, uint8_t *buffer, unsigned size)
{
    cb->read = cb->write = cb->data = buffer;
    cb->size = size;
}

void cyc_buf_reset (cyc_buf_t *cb)
{
    cb->read = cb->write = cb->data;
}

unsigned cyc_buf_avail_read (cyc_buf_t *cb)
{
    return (cb->write - cb->read + cb->size) % cb->size;
}

unsigned cyc_buf_avail_write (cyc_buf_t *cb)
{
    return (cb->read - cb->write + cb->size - 1) % cb->size;
}

unsigned cyc_buf_read_to_end (cyc_buf_t *cb)
{
    return (cb->data + cb->size - cb->read);
}

unsigned cyc_buf_write_to_end (cyc_buf_t *cb)
{
    return (cb->data + cb->size - cb->write);
}

void cyc_buf_advance_read (cyc_buf_t *cb, unsigned size)
{
    unsigned to_end = cyc_buf_read_to_end(cb);
    if (size < to_end)
        cb->read += size;
    else cb->read = cb->data + (size - to_end);
}

void cyc_buf_advance_write (cyc_buf_t *cb, unsigned size)
{
    unsigned to_end = cyc_buf_write_to_end(cb);
    if (size < to_end)
        cb->write += size;
    else cb->write = cb->data + (size - to_end);
}

uint8_t *cyc_buf_cur_read (cyc_buf_t *cb)
{
    return cb->read;
}

uint8_t *cyc_buf_cur_write (cyc_buf_t *cb)
{
    return cb->write;
}

