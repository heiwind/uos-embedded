#ifndef __BUF_H_
#define	__BUF_H_ 1

#ifdef __cplusplus
extern "C" {
#endif



struct _mem_pool_t;

typedef struct _buf_t buf_t;

struct _buf_t {
	/* Pointer to the next buffer in chain. */
	buf_t		*next;

	/* Beginning of actual payload in the data region. */
	unsigned char	*payload;

	/* Total length of buffer + additionally chained buffers. */
	unsigned short	tot_len;

	/* Length of this buffer. */
	unsigned short	len;

	/* Data region is allocated here. */
	/* unsigned char data [...]; */
};

/*
 * Allocate a buf of the requested size, plus the reserved space
 * for protocol headers. Buffer memory for buf is allocated as one
 * large chunk.
 */
buf_t *buf_alloc (struct _mem_pool_t *m, unsigned short size,
	unsigned short header_space) __cpp_decls;

/*
 * Deallocate the buffer. If the buf is a chain all bufs in the
 * chain are deallocated. Return the number of deallocated segments.
 */
small_int_t buf_free (buf_t *p) __cpp_decls;

/*
 * Shrink the buf to the size given by the size parameter.
 */
void buf_truncate (buf_t *p, unsigned short size) __cpp_decls;
//* same as buf_truncate, but leave buffer memory allocation as is
void buf_truncate_soft (buf_t *p, unsigned short size) __cpp_decls;
//* truncates memory blocks on buffers to actual buffer size,
//*    free empty buffers from chain
void buf_pack(buf_t *p) __cpp_decls;

/*
 * Try to move the p->payload pointer header_size number of bytes
 * upward within the buf. The return value is non-zero if it
 * fails. If so, an additional buf should be allocated for the header
 * and it should be chained to the front.
 */
bool_t buf_add_header (buf_t *p, short header_size) __cpp_decls;

/*
 * reset p->payload pointer to header_size offset from allocated mem buffer
 * */
bool_t buf_reset_header (buf_t *p, short header_size) __cpp_decls;

/*
 * Chain buf t on the end of buf h. Pbuf h will have it's tot_len
 * field adjusted accordingly. Pbuf t should no be used any more after
 * a call to this function, since buf t is now a part of buf h.
 */
void buf_chain (buf_t *h, buf_t *t) __cpp_decls;

/*
 * Pick off the first buf from the buf chain p. Returns the tail of
 * the buf chain or NULL if the buf p was not chained.
 */
buf_t *buf_dechain (buf_t *p) __cpp_decls;

/*
 * Convert a buffer to a single continuous memory chunk.
 */
buf_t *buf_make_continuous (buf_t *h);

/*
 * Make a copy of buffer.
 */
buf_t *buf_copy (buf_t *h) __cpp_decls;

/* copy  len bytes from buffer src to single memory chunk dst
 *      takes into account if src have less data then len
 * \return - copyed data size 
 * */
unsigned buf_copy_continous(void* dst, buf_t *src, unsigned len) __cpp_decls;

/*
 * Count number of bufs in a chain.
 */
small_int_t buf_chain_len (buf_t *p) __cpp_decls;

/*
 * Print the buffer contents using debug_printf.
 */
void buf_print (buf_t *p) __cpp_decls;
void buf_print_data (unsigned char *data, int len) __cpp_decls;
void buf_print_ethernet (buf_t *p) __cpp_decls;
void buf_print_ip (buf_t *p) __cpp_decls;
void buf_print_tcp (buf_t *p) __cpp_decls;

/*
 * Compute the inet-compatible checksum of the buffer data.
 */
unsigned short buf_chksum (buf_t *p, unsigned short sum) __cpp_decls;

/*
 * Compute the 32-bit checksum (baset on VAK's rot13) of the buffer data.
 */
unsigned long buf_chksum32 (buf_t *p, unsigned long sum) __cpp_decls;



#ifdef __cplusplus
}
#endif

#endif /* !__BUF_H_ */
