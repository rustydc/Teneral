#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>


#include <ev.h>

#include "writebuf.h"

/*typedef struct writebuf_s {
	int fd;
	
	char buf[8192];
	int buflen;
	
	struct ev_loop *loop;
	ev_io *watcher;
	
} writebuf_t;
*/

writebuf_t *new_writebuf(int fd, struct ev_loop *loop)
{
	writebuf_t *buf = malloc(sizeof(writebuf_t));
	
	buf->fd = fd;
	buf->len = 0;
	buf->loop = loop;
	buf->watching = 0;	

	ev_io_init ((ev_io *)buf, write_cb, buf->fd, EV_WRITE);

	return buf;
}

void write_cb(EV_P_ ev_io *w, int revents)
{
	writebuf_t *buf = (writebuf_t *) w;

	// Just in case, don't write zero length.
	if (buf->len == 0) {
		ev_io_stop(buf->loop, w);
		buf->watching = 0;
	}

	int len_sent = write(buf->fd, buf->buf, buf->len);
	if (len_sent < 0) {
		perror("write");
		return;
	}

	printf("%d: writebuf: Wrote %d bytes.\n", buf->fd, len_sent);

	bcopy(buf->buf + len_sent, buf->buf, buf->len - len_sent);
	buf->len -= len_sent;

	if (buf->len == 0) {
		ev_io_stop(buf->loop, w);
		buf->watching = 0;
	}
}

int buf_write(writebuf_t *buf, char *msg, int len)
{
	if (buf->len + len > 4096) {
		printf("%d: Buffer overflow.  Not writing.\n", buf->fd);
		return 1;
	}
	
	bcopy(msg, buf->buf + buf->len, len);
	buf->len += len;

	if (!buf->watching) {
		ev_io_start(buf->loop, (ev_io *)buf);
		buf->watching = 1;
	}
	return 0;
}

