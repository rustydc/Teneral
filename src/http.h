#ifndef __HTTP_H__
#define __HTTP_H__

#include "socket.h"

typedef struct header_s {
	char *name;
	char *value;
} header_t;

typedef struct headers_s {
	int count;
	header_t **headers;
} headers_t;

typedef struct request_s {
	char *method;
	char *uri;
	char *version;
	headers_t *headers;
} request_t;

typedef struct connection_s {
	int fd;
	int is_websocket;
	socket_t *socket;
} connection_t;

request_t *parse_request(char *str, int length, int *length_used);
// Potentially return multiple:
request_t *parse_requests(
    char *str, int length, int *length_used, int *num_found);

char *request_get_header(request_t *req, char *name);
void request_delete(request_t *req);

char *request_get_cookie(request_t *req);

void serve_file(socket_t *wbuf, char *filename);

#endif
