/*  Teneral's main daemon.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <signal.h>

// From package 'libev-dev'
#include <ev.h>

#include "tnrld.h"
#include "ilist.h"
#include "http.h"
#include "websocket.h"
#include "messages.h"
#include "processes.h"

tnrld_t *tnrld;

struct ev_loop *loop;

ilist_t *connections;
ilist_t *processes;

typedef struct sock_pid_s {
	socket_t *socket;
	process_t *pid;
} sock_pid_t;

void child_cb(EV_P_ ev_child *w, int revents);

// Get data from a child process, write it out to a websocket.
void process_child_out_cb(socket_t *socket)
{
	sock_pid_t *sp = socket->data;

	char *str = json_pid_out(sp->pid->pid, "stdout", socket->rbuf, socket->rbuf_len);

	frame_t *frame = malloc(sizeof(frame_t));
	frame->fin = 1;
	frame->len = strlen(str);
	frame->opcode = WS_OP_BIN;
	frame->payload = str;

	int fr_len;
	char *fr_str = write_frame(frame, &fr_len);

	socket_write(sp->socket, fr_str, fr_len);

	socket_consume(socket, socket->rbuf_len);

	free(frame);
	free(fr_str);
}

// Handle input from an HTTP socket.
void process_http_cb(socket_t *socket)
{
	int used;
	
	connection_t *ptr = ilist_fetch(connections, socket->fd);
	if (ptr == NULL) {
		printf("%d: Tried to get from list, but got NULL.\n", socket->fd);
		return;
	}

	if (ptr->is_websocket) {
		// Try to decode a frame.
		frame_t *f = read_frame(socket->rbuf, socket->rbuf_len, &used);
		if (f == NULL) {
			return;
		}

		// TODO: Consume instead.
		bcopy(socket->rbuf, socket->rbuf + used, socket->rbuf_len - used);
		socket->rbuf_len -= used;
		
		char *p = malloc(f->len + 1);
		strncpy(p, f->payload, f->len);
		p[f->len] = 0;
		
		if (f->opcode == 0x8) {
			// Close the websocket.
			frame_t *f = new_frame(1, 0, WS_OP_CLOSE, "");
			int fr_len;
			char *fr_str = write_frame(f, &fr_len);
			free(f->payload);
			free(f);
			free(p);
			socket_write(socket, fr_str, fr_len);
			free(fr_str);
			return;
			// TODO: Remove and free this connection.
			// TODO: Maybe kill their processes?
		}

		cmd_t *cmd = parse_msg(p);
		if (cmd != NULL && cmd->type == CMD_EXECUTE) {
			process_t *p = execute_cmd((cmd_execute_t *)cmd);

			ilist_insert(processes, p);


			sock_pid_t *sp = malloc(sizeof(sock_pid_t));
			sp->socket = socket;
			sp->pid = p;

			// Write new_process message?
			char format[] = 
			    "{\"newProcess\" : {"
			        "\"pid\" : %d, \"command\" : \"%s\","
			        "\"startTime\" : %ld,"
			        "\"requestId\" : %d}}";
			char str[1000];

			struct timeval tp;
			gettimeofday(&tp, NULL);
			long int time = tp.tv_sec * 1000 + tp.tv_usec;
			
			sprintf(str, format, p->pid, ((cmd_execute_t*)cmd)->command_str, time, ((cmd_execute_t*)cmd)->request_id);

			frame_t *frame = malloc(sizeof(frame_t));
			frame->fin = 1;
			frame->len = strlen(str);
			frame->opcode = WS_OP_BIN;
			frame->payload = str;

			int fr_len;
			char *fr_str = write_frame(frame, &fr_len);
			free(frame);

			socket_write(socket, fr_str, fr_len);
			free(fr_str);

			p->output =
				socket_new(p->out, &process_child_out_cb, sp, socket);

			ev_child *child_watcher = (struct ev_child*) malloc (sizeof(struct ev_child));
			ev_child_init (child_watcher, child_cb, p->pid, 1);
			child_watcher->data = sp;
			ev_child_start(loop, child_watcher);
		}
	
		free_msg(cmd);
		free(f->payload);
		free(f);
		free(p);
		return;
	}
	

	request_t *req = parse_request(socket->rbuf, socket->rbuf_len, &used);
	if (req) {
		printf("%d: New Request: %s %s\n", socket->fd, req->method, req->uri);

		// Take it out of the read buffer.
		// TODO: Consume
		bcopy(socket->rbuf, socket->rbuf + used, socket->rbuf_len - used);
		socket->rbuf_len -= used;

		// Got a request!


		int upgraded = try_upgrade(req, ptr);
		if (upgraded) {
			printf("%d: Upgraded to WebSocket!\n", socket->fd);
		} else if (!strcmp(req->uri, "/")) {
			serve_file(socket, "static/index.html");
		} else if (!strcmp(req->uri, "/tnrl.js")) {
			serve_file(socket, "static/tnrl.js");
		} else if (!strcmp(req->uri, "/ansi_up.js")) {
			serve_file(socket, "static/ansi_up.js");
		} else if (!strcmp(req->uri, "/tnrl.css")) {
			serve_file(socket, "static/tnrl.css");
		} else if (!strcmp(req->uri, "/favicon.ico")) {
			serve_file(socket, "static/favicon.ico");
		} else {
			// Unknown URL?
			char *str = "HTTP/1.1 404 Not Found\r\n\r\n";
			socket_write(socket, str, strlen(str));
		}

		request_delete(req);
		
	}
}

// Accept a new a connection, set up the callbacks.
static void accept_cb(EV_P_ ev_io *w, int revents)
{
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);
	
	int fd = accept(w->fd, (struct sockaddr *) &their_addr, &addr_size);

	if (fd < 0) {
		printf("Accept failed from %d...  revents = %d\n", w->fd, revents);
		perror("accept_cb");
		return;
	}

	printf ("%d: New Connection: %d!\n", w->fd, fd);	
	
	connection_t *con = malloc (sizeof(connection_t));
	con->fd = fd;
	con->is_websocket = 0;
	con->socket = socket_new(fd, &process_http_cb, NULL, NULL);
	ilist_insert(connections, con);
}

// Report that a child has exited
void child_cb(EV_P_ ev_child *w, int revents)
{
	printf("Process %d exited with status %x\n", w->rpid, w->rstatus);
	char str[100];
	char format[] = 
	    "{\"processTerminated\" : "
	    "    {\"pid\" : %d, \"returnCode\" : %d, \"bySignal\" : %d, \"endTime\" : %ld }}";

	struct timeval tp;
	gettimeofday(&tp, NULL);
	long int time = (long int) tp.tv_sec * 1000 + tp.tv_usec;

	sprintf(str, format, w->rpid, WEXITSTATUS(w->rstatus), WSTOPSIG(w->rstatus), time);
	sock_pid_t *sp = w->data;
	socket_t *s = sp->socket;

	ev_child_stop (EV_A_ w);

	frame_t *frame = malloc(sizeof(frame_t));
	frame->fin = 1;
	frame->opcode = WS_OP_BIN;
	frame->len = strlen(str);
	frame->payload = str;
	
	int fr_len;
	char *fr_str = write_frame(frame, &fr_len);
	socket_write(s, fr_str, fr_len);
	free(frame);
	free(fr_str);

	ilist_remove(processes, w->rpid);

	// TODO: Can't free here, output still coming.
	//free(sp->pid);
}

// Start up the webserver.
int start_webserver(char *port) {
	struct addrinfo hints, *res;
	int sockfd;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, port, &hints, &res);
	
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	int optval;
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	int val = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (val < 0) {
		perror("bind");
		freeaddrinfo(res);
		return -1;
	}

	val = listen(sockfd, 5);
	if (val < 0) {
		perror("listen");
		freeaddrinfo(res);
		return -1;
	}
	printf("%d: Listening on %s.\n", sockfd, port);

	freeaddrinfo(res);

	return sockfd;
}

// Create a new instance of tnrld.
tnrld_t *new_tnrld(int port) 
{
	tnrld_t *tnrld = malloc(sizeof(tnrld_t));
	tnrld->listen_port = port;
	return tnrld;
}

void int_handler(int sig)
{
	close(tnrld->listen_fd);
	printf("Socket closed.\n");
	exit(0);
}

int main(int argc, char **argv)
{
	int webfd;
	
	tnrld = new_tnrld(10002);

	connections = new_ilist(500);
	processes = new_ilist(500);

	loop = EV_DEFAULT;

	// Start tnrld on port 10002.
	webfd = start_webserver("10002");
	if (webfd < 0) {
		exit(1);
	}

	tnrld->listen_fd = webfd;
	tnrld->evloop = loop;

	// Signal handler to close cleanly.
	signal(SIGINT, int_handler);

	// Signal handler so child pipes end safely.
	signal(SIGPIPE, SIG_IGN);

	// Set up the main event loop.
	ev_io http_watcher;
	ev_io_init (&http_watcher, accept_cb, webfd, EV_READ);
	ev_io_start(loop, &http_watcher);
	ev_set_io_collect_interval (loop, 0.001);
	ev_loop(loop, 0);

	printf("Server shut down.\n");
	exit(0);
}
