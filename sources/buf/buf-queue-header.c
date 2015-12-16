#include <runtime/lib.h>
#include <buf/buf.h>
#include <buf/buf-queue-header.h>

buf_t *
buf_queueh_get (buf_queue_header_t *q)
{
	buf_t *p;

    if (q->count == 0) {
        /*debug_printf ("buf_queue_get: returned 0\n");*/
        return 0;
    }

    buf_t **queue = buf_queueh_base(q);

	assert (q->tail >= queue);
	assert (q->tail < queue + q->size);
	assert (*q->tail != 0);

	/* Get the first packet from queue. */
	p = *q->tail;

	/* Advance head pointer. */
	if (--q->tail < queue)
		q->tail += q->size;
	--q->count;

	/*debug_printf ("buf_queue_get: returned 0x%04x\n", p);*/
	return p;
}

void
buf_queueh_put (buf_queue_header_t *q, buf_t *p)
{
	buf_t **head;

	/*debug_printf ("buf_queue_put: p = 0x%04x, count = %d, head = 0x%04x\n", p, q->count, q->head);*/

	/* Must be called ONLY when queue is not full. */
	assert (q->count < q->size);

    buf_t **queue = buf_queueh_base(q);

	/* Compute the last place in the queue. */
	head = q->tail - q->count;
	if (head < queue)
		head += q->size;

	/* Put the packet in. */
    assert (p != 0);
	*head = p;
	++q->count;
	/*debug_printf ("    on return count = %d, head = 0x%04x\n", q->count, q->head);*/
}

void
buf_queueh_clean (buf_queue_header_t *q)
{
    buf_t **queue = buf_queueh_base(q);
	for (; q->count > 0; --q->count) {
		buf_free (*q->tail);

		if (--q->tail < queue)
			q->tail += q->size;
	}
}

void
buf_queueh_init (buf_queue_header_t *q, int bytes)
{
	q->tail = buf_queueh_base(q);
	q->size = bytes / sizeof (buf_t*);
	q->count = 0;
}
