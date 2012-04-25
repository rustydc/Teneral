#ifndef __WRITEBUF_H__
#define __WRITEBUF_H__

#include <ev.h>

typedef struct writebuf_s {
	ev_io watcher;

	int fd;

	struct ev_loop *loop;
	
	int watching;
	int len;
	char buf[8192];
} writebuf_t;

writebuf_t *new_writebuf(int fd, struct ev_loop *loop);
int buf_write(writebuf_t *buf, char *msg, int len);
void write_cb(EV_P_ ev_io *w, int revents);

#endif
