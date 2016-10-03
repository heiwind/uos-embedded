#include <runtime/lib.h>
#include <stream/stream.h>

int stream_puts (stream_t *stream, const char *str)
{
	int length;
	unsigned char c;
	UOS_STRICT(STREAM_MEM, ) assert(uos_valid_memory_address(stream));
    UOS_STRICT(STREAM_MEM, ) assert(uos_valid_memory_address(stream->interface));
	void (*putc) (stream_t *u, short c) = stream->interface->putc;
    UOS_STRICT(STREAM_MEM, ) assert(uos_valid_code_address(putc));

#if STREAM_HAVE_ACCEESS > 0
	if (stream->interface->access_tx != 0)
	    (stream->interface->access_tx(stream, 1));
#endif

	for (length = 0; ; length++) {
		c = FETCH_BYTE (str);
		if (! c)
		    break;
		putc(to_stream(stream), c);
		//putchar (stream, c);
		++str;
	}

#if STREAM_HAVE_ACCEESS > 0
    if (stream->interface->access_tx != 0)
        (stream->interface->access_tx(stream, 0));
#endif
    return length;
}

int stream_write (stream_t *stream, const void *src, unsigned len)
{
    if (len <= 0)
        return 0;

    const char *str = (const char *)src;
    int length;
    unsigned char c;
    void (*putc) (stream_t *u, short c) = stream->interface->putc;

#if STREAM_HAVE_ACCEESS > 0
    if (stream->interface->access_tx != 0)
        (stream->interface->access_tx(stream, 1));
#endif

    for (length = 0; length < len; length++) {
        c = FETCH_BYTE (str++);
        putc(to_stream(stream), c);
        //putchar (stream, c);
    }

#if STREAM_HAVE_ACCEESS > 0
    if (stream->interface->access_tx != 0)
        (stream->interface->access_tx(stream, 0));
#endif
    return length;
}

