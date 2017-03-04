#include <runtime/lib.h>
#include <stream/stream.h>

int
snprintf (unsigned char *buf, int size, const char *fmt, ...)
{
	stream_buf_t u;
	va_list	args;
	int err;

	stropen (&u, buf, size);
	va_start (args, fmt);
	err = vprintf ((stream_t*)&u.interface, fmt, args);
	va_end (args);
	strclose (&u);
	return err;
}
