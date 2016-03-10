#include <runtime/lib.h>
#include <stream/stream.h>

/*
 * Read a newline-terminated string from stream.
 */
unsigned char *
stream_gets (stream_t *stream, unsigned char *buf, int len)
{
	int c;
	unsigned char *s;

	s = buf;

#if STREAM_HAVE_ACCEESS > 0
    if (stream->interface->access_rx != 0)
        (stream->interface->access_rx(stream, 1));
#endif

    while (--len > 0) {
		if (feof (stream)) {
			if (s == buf)
				buf = 0;
			break;
		}
		c = getchar (stream);
//debug_printf ("c = `%c'\n", c);
		*s++ = c;
		if (c == /*'\n'*/ 13)
			break;
	}
	*s = '\0';
#if STREAM_HAVE_ACCEESS > 0
    if (stream->interface->access_rx != 0)
        (stream->interface->access_rx(stream, 0));
#endif
	return buf;
}
