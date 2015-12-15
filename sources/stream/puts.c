#include <runtime/lib.h>
#include <stream/stream.h>

int stream_puts (stream_t *stream, const char *str)
{
	int length;
	unsigned char c;
	void (*putc) (stream_t *u, short c) = stream->interface->putc;

	for (length = 0; ; length++) {
		c = FETCH_BYTE (str);
		if (! c)
			return length;
		putc(to_stream(stream), c);
		//putchar (stream, c);
		++str;
	}
}
