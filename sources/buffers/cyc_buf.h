#ifndef _CYC_BUF_H_
#define _CYC_BUF_H_

typedef struct _cyc_buf_t
{
    unsigned    size;
    uint8_t     *data;
    uint8_t     *read;
    uint8_t     *write;
} cyc_buf_t;

void cyc_buf_init (cyc_buf_t *cb, uint8_t *buffer, unsigned size);
void cyc_buf_reset (cyc_buf_t *cb);
unsigned cyc_buf_avail_read (cyc_buf_t *cb);
unsigned cyc_buf_avail_write (cyc_buf_t *cb);
unsigned cyc_buf_read_to_end (cyc_buf_t *cb);
unsigned cyc_buf_write_to_end (cyc_buf_t *cb);
void cyc_buf_advance_read (cyc_buf_t *cb, unsigned size);
void cyc_buf_advance_write (cyc_buf_t *cb, unsigned size);
uint8_t *cyc_buf_cur_read (cyc_buf_t *cb);
uint8_t *cyc_buf_cur_write (cyc_buf_t *cb);

#endif
