#ifndef __TNRLD_H__
#define __TNRLD_H__

#include <ev.h>

#include "http.h"
#include "ilist.h"

typedef struct tnrld_s {
	int listen_port;
	int listen_fd;
	struct ev_loop *evloop;
} tnrld_t;

tnrld_t *new_tnrld(int listen_port);
void delete_tnrld(tnrld_t *tnrld);

void tnrld_run(tnrld_t *tnrld);  // Init, start server, start the loop.

#endif
