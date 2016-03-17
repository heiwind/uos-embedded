#include <runtime/lib.h>
#include <stream/stream.h>

int stream_puts (stream_t *stream, const char *str)
{
	int length;
	unsigned char c;
	void (*putc) (stream_t *u, short c) = stream->interface->putc;

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
