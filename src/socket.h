#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <ev.h>

typedef struct socket_s {
	int fd;

	char *rbuf;
	int rbuf_size;
	int rbuf_len;

	char *wbuf;
	int wbuf_size;
	int wbuf_len;
	int wbuf_watching;

	struct socket_s *wait_for;

	int broken;

	ev_io *read_watcher;
	void (*process_cb)(struct socket_s *socket);

	void *data;  // Generic callback data
} socket_t;

socket_t *socket_new(
    int fd, void (*process_cb)(socket_t *socket), void *data,
    socket_t *wait_for);
void socket_write(socket_t *socket, char *data, int len);
void socket_consume(socket_t *socket, int len);
void socket_delete(socket_t *socket);

#endif
