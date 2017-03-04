/*
 * buf queue header.h
 *
 * utf8 ru_RU
 *
 *  Created on: 21.10.2015
 *      Author: a_lityagin
 * \~russian
 *  этот тип задает заговловок очереди буфферов перед массивом buf_t*
 *  его надо заявлять в структурах непосредственно перед массивом, 
 *  чтобы он мог корректно с ним связаться 
 * */
#ifndef __BUF_QUEUE_HEADER_H_
#define	__BUF_QUEUE_HEADER_H_ 1

#include <buf/buf.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef struct _buf_queue_header_t buf_queue_header_t;

struct _buf_queue_header_t {
#if (INT_SIZE == 2)
    unsigned char count;
    unsigned char size;
#elif (INT_SIZE == 4)
    unsigned short count;
    unsigned short size;
#else
    small_uint_t count;
    small_uint_t size;
#endif
	struct _buf_t **tail;
};

void buf_queueh_init (buf_queue_header_t *q, int bytes);
void buf_queueh_put (buf_queue_header_t *q, buf_t *b);
buf_t *buf_queueh_get (buf_queue_header_t *q);

static inline __attribute__((always_inline)) bool_t buf_queueh_is_full (buf_queue_header_t *q)
{
	return (q->count == q->size);
}

static inline __attribute__((always_inline)) bool_t buf_queueh_is_empty (buf_queue_header_t *q)
{
	return (q->count == 0);
}

static inline
__attribute__((always_inline))
buf_t **buf_queueh_base(buf_queue_header_t *q){
    return (buf_t **)((unsigned char*)q + sizeof(*q));
}

static inline 
__attribute__((always_inline)) 
buf_t **buf_queueh_first (buf_queue_header_t *q)
{
	struct _buf_t **head;
	buf_t **queue = buf_queueh_base(q);

	head = q->tail - q->count;
	if (head < queue)
		head += q->size;
	return head;
}

static inline 
__attribute__((always_inline)) 
buf_t **buf_queueh_last (buf_queue_header_t *q)
{
	return q->tail;
}

static inline 
__attribute__((always_inline)) 
buf_t **buf_queueh_next (buf_queue_header_t *q, struct _buf_t **ptr)
{
    buf_t **queue = buf_queueh_base(q);
	if (--ptr < queue)
		ptr += q->size;
	return ptr;
}

void buf_queueh_clean (buf_queue_header_t *q);


#ifdef __cplusplus
}
#endif

#endif /* !__BUF_QUEUE_H_ */
