#ifndef __DEBUG_H__
#define __DEBUG_H__

void logp_(const char *file, int line, const char *func, const char *fmt, ...);

#define logp(fmt, ...) \
	{ logp_(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); }

#endif
