#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "socket.h"
#include "tnrld.h"

#define INIT_BUF 32000

extern tnrld_t *tnrld;

static void rbuf_cb(struct ev_loop *loop, struct ev_io *w, int revents);

// TODO: Save the watcher, don't realloc every time.
static void write_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
	socket_t *socket = w->data;

	// Anything to write?  If not, stop this.
	if (socket->wbuf_len == 0 || socket->broken) {
		ev_io_stop(loop, w);
		socket->wbuf_watching = 0;
		free(w);
		return;
	}

	int len_sent = write(socket->fd, socket->wbuf, socket->wbuf_len);
	if (len_sent < 0) {
		socket->broken = 1;
		perror("write");
		return;
	}

	// Move the to-copy text down to the beginning of the array.
	bcopy(
	    socket->wbuf + len_sent, socket->wbuf, socket->wbuf_len - len_sent);
	socket->wbuf_len -= len_sent;

	// Check again if we're empty, and stop it if so.
	if (socket->wbuf_len == 0) {
		ev_io_stop(loop, w);
		socket->wbuf_watching = 0;
		free(w);
	}
}

// Create a new buffered reader/writer for this fd.
// (process_cb will be called after each read on the data so far.)
socket_t *socket_new(
    int fd, void (*process_cb)(socket_t *socket), void *data,
    socket_t *wait_for) {
	socket_t *socket = malloc(sizeof(socket_t));

	socket->fd = fd;

	socket->rbuf = malloc(INIT_BUF);
	socket->rbuf_size = INIT_BUF;
	socket->rbuf_len = 0;

	socket->wbuf = malloc(INIT_BUF);
	socket->wbuf_size = INIT_BUF;
	socket->wbuf_len = 0;
	socket->wbuf_watching = 0;

	socket->wait_for = wait_for;

	socket->broken = 0;

	ev_io *watcher = malloc(sizeof(ev_io));
	ev_io_init(watcher, rbuf_cb, fd, EV_READ);
	watcher->data = socket;

	socket->read_watcher = watcher;
	socket->process_cb = process_cb;

	socket->data = data;

	ev_io_start(tnrld->evloop, watcher);

	return socket;
}

static void rbuf_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
	unsigned char buf[4096];
	int len;

	socket_t *socket = w->data;

	// If there's a dependent write socket that's full:
	//   Stop this and return?
	//   Set a timeout to wait?
	//   Maybe just return for now.
	// TODO: UGLY HACK
	if (socket->wait_for != NULL) {
		if (socket->wait_for->wbuf_len > 31000) {
			logp(
			    "Dest looks full.  Returning. (%d)\n",
			    socket->wait_for->wbuf_len);
			sleep(1);
			return;
		}
	}

	// Read it all into rbuf.
	len = read(w->fd, buf, 4096);
	if (len == -1) {
		perror("read");
		ev_io_stop(loop, w);
		free(w);
		return;
	}

	// If len = 0, we're done.
	if (!len) {
		ev_io_stop(loop, w);
		free(w);
		return;
		// TODO: socket_delete?
	}

	if (socket->rbuf_len + len > socket->rbuf_size) {
		// TODO: resize
	}

	memcpy(socket->rbuf + socket->rbuf_len, buf, len);
	socket->rbuf_len += len;

	// Call the parser for this socket!
	(*(socket->process_cb))(socket);
}

void socket_write(socket_t *socket, char *data, int len) {
	if (socket->wbuf_len + len > socket->wbuf_size) {
		logp("Socket full!\n");
		// TODO: resize!
		return;
	}

	// Copy the data into wbuf.
	memcpy(socket->wbuf + socket->wbuf_len, data, len);
	socket->wbuf_len += len;

	// Activate a write cb if it isn't.
	if (!socket->wbuf_watching) {
		ev_io *watcher = malloc(sizeof(ev_io));
		ev_io_init(watcher, write_cb, socket->fd, EV_WRITE);
		watcher->data = socket;
		ev_io_start(tnrld->evloop, watcher);
		socket->wbuf_watching = 1;
	}
}

void socket_consume(socket_t *socket, int len) {
	// TODO:  fail if len > rbuf_len ?
	bcopy(socket->rbuf, socket->rbuf + len, socket->rbuf_len - len);
	socket->rbuf_len -= len;
}

void socket_delete(socket_t *socket);
