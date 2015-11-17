#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "regexp9.h"

void
regexp_error(const char *s)
{
	char buf[132];

	strcpy(buf, "regexp_error: ");
	strcat(buf, s);
	strcat(buf, "\n");
	if (write(2, buf, strlen(buf)) < 0)
		/* ignore */;
	exit(1);
}
