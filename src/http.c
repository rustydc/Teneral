#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "http.h"

char *get_line(char *str, int length, int *length_used) {
	int i = 0;

	for (i = 0; i != length - 1; i++) {
		if (str[i] == '\0') {
			// TODO
			*length_used = 0;
			return NULL;
		}

		if (i < length - 1 && str[i] == '\r' && str[i + 1] == '\n') {
			// Up through str[i-1] is the line.
			// Make the destination one bigger to hold a NULL.
			char *retval = malloc(i + 1);
			memcpy(retval, str, i);
			retval[i] = '\0';
			*length_used = i + 2;  // Consume the CRLF.
			return retval;
		}
	}

	*length_used = 0;
	return NULL;
}

request_t *parse_request(char *str, int length, int *length_used) {
	int used, end;
	char *rest, *line;
	int num_headers = 0;
	header_t *headers[100];  // Allow 100 per request for now?

	line = get_line(str, length, &used);

	if (line) {
		rest = line;

		// Get METHOD
		end = index(rest, ' ') - rest;
		if (end < 0) {
			// index was probably NULL.
			*length_used = 0;
			return NULL;
		}
		char *method = malloc(end + 1);
		memcpy(method, rest, end);
		method[end] = '\0';
		rest = rest + end + 1;

		// Get URI
		end = index(rest, ' ') - rest;
		if (end < 0) {
			// index was probably NULL.
			free(method);
			*length_used = 0;
			return NULL;
		}
		char *uri = malloc(end + 1);
		memcpy(uri, rest, end);
		uri[end] = '\0';
		rest = rest + end + 1;

		// Get VERSION
		end = strlen(rest);
		char *version = malloc(end + 1);
		memcpy(version, rest, end);
		version[end] = '\0';
		rest = rest + end + 2;  // CRLF

		free(line);
		int used_tot = used;
		line = get_line(str + used, length - used, &used);
		while (line != NULL && strlen(line) > 0) {
			end = index(line, ':') - line;
			// Get header name
			char *header_name = malloc(end + 1);
			memcpy(header_name, line, end);
			header_name[end] = '\0';

			// Get header value
			rest = line + end + 2;  // ': '
			end = strlen(rest);
			char *header_value = malloc(end + 1);
			memcpy(header_value, rest, end);
			header_value[end] = '\0';

			free(line);
			used_tot += used;
			if (used_tot >= length) {
				*length_used = 0;
				return NULL;
			}
			line =
			    get_line(str + used_tot, length - used_tot, &used);

			headers[num_headers] = malloc(sizeof(header_t));
			headers[num_headers]->name = header_name;
			headers[num_headers]->value = header_value;
			num_headers++;
		}

		used_tot += used;  // The last two bytes?

		if (line == NULL) {
			// Didn't get a blank line at the end of the header.
			// TODO: Free the header strings
			int i;
			for (i = 0; i != num_headers; i++) {
				free(headers[i]->name);
				free(headers[i]->value);
				free(headers[i]);
			}
			free(uri);
			free(method);
			free(version);
			*length_used = 0;
			return NULL;
		}

		free(line);

		headers_t *headers2 = malloc(sizeof(headers_t));
		headers2->count = num_headers;
		headers2->headers = malloc(sizeof(header_t *) * num_headers);

		int i;
		for (i = 0; i != num_headers; i++) {
			headers2->headers[i] = headers[i];
		}

		request_t *retval = malloc(sizeof(request_t));
		retval->method = method;
		retval->uri = uri;
		retval->version = version;
		retval->headers = headers2;

		*length_used = used_tot;
		return retval;
	} else {
		// No complete line.
		*length_used = 0;
		return NULL;
	}
}

char *request_get_header(request_t *req, char *name) {
	int i = 0;
	for (i = 0; i != req->headers->count; i++) {
		if (!strcmp(req->headers->headers[i]->name, name)) {
			return req->headers->headers[i]->value;
		}
	}

	// Not found.
	return NULL;
}

void headers_free(headers_t *headers) {
	int i;
	for (i = 0; i != headers->count; i++) {
		free(headers->headers[i]->name);
		free(headers->headers[i]->value);
		free(headers->headers[i]);
	}
	free(headers->headers);
	free(headers);
}

void request_delete(request_t *req) {
	free(req->method);
	free(req->uri);
	free(req->version);
	headers_free(req->headers);
	free(req);
}

// Potentially return multiple:
request_t *parse_requests(
    char *str, int length, int *length_used, int *num_found);

void serve_file(socket_t *socket, char *filename) {
	char buf[4096];
	FILE *f = fopen(filename, "r");
	fseek(f, 0L, SEEK_END);
	int size = ftell(f);
	rewind(f);

	char hdr1[] = "HTTP/1.1 200 OK\r\n";
	socket_write(socket, hdr1, strlen(hdr1));

	char len_str[10];
	sprintf(len_str, "Content-Length: %d\r\n\r\n", size);
	socket_write(socket, len_str, strlen(len_str));

	int n = fread(buf, 1, 4096, f);
	while (n > 0) {
		socket_write(socket, buf, n);
		n = fread(buf, 1, 4096, f);
	}
	fclose(f);
}
