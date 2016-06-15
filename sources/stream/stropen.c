#include <runtime/lib.h>
#include <stream/stream.h>

static void
buf_putchar (stream_buf_t *u, short c)
{
	if (u->size > 1) {
		/* TODO: unicode to utf8 conversion. */
		*u->buf++ = c;
		--u->size;
	}
}

static unsigned short
buf_getchar (stream_buf_t *u)
{
	/* TODO: utf8 to unicode conversion. */
	unsigned char c = *u->buf;

	if (! c)
		return 0;
	++u->buf;
	return c;
}

static int
buf_peekchar (stream_buf_t *u)
{
	/* TODO: utf8 to unicode conversion. */
	unsigned char c = *u->buf;

	if (! c)
		return -1;
	return c;
}

static bool_t
buf_feof (stream_buf_t *u)
{
	return ! *u->buf;
}

#ifdef __cplusplus
#define idx(i)
#define item(i)
#else
#define idx(i) [i] = 
#define item(i) .i =
#endif


static stream_interface_t buf_interface = {
	item(putc) (void (*) (stream_t*, short)) buf_putchar,
	item(getc) (unsigned short (*) (stream_t*))	buf_getchar,
	item(peekc) (int (*) (stream_t*))   buf_peekchar,
	item(flush) (void (*) (stream_t*))  0,
	item(eof) (bool_t (*) (stream_t*))		buf_feof,
};

stream_t *
stropen (stream_buf_t *u, unsigned char *buf, int size)
{
	u->interface = &buf_interface;
	u->buf = buf;
	u->size = size;
	return (stream_t*) u;
}

void
strclose (stream_buf_t *u)
{
	if (u->size > 0) {
		*u->buf++ = 0;
		--u->size;
	}
}
