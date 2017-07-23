#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

void logp_(const char *file, int line, const char *func, const char *fmt, ...) {
	char buf[512];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);

	printf("%s:%d %s: %s", file, line, func, buf);
	fflush(stdout);
}
