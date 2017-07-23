#include <arpa/inet.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// From 'libssl-dev'
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include "http.h"
#include "websocket.h"

int try_upgrade(request_t *req, connection_t *con) {
	// Does it contain "Upgrade: websocket"?
	// TODO: Also needs "Connection: Upgrade"
	// TODO: 'websocket' match is case insensitive.
	char *upgrade = request_get_header(req, "Upgrade");
	if (upgrade == NULL || strcmp(upgrade, "websocket")) {
		// Not an upgrade.
		return 0;
	}

	char *version = request_get_header(req, "Sec-WebSocket-Version");
	if (strcmp(version, "8") && strcmp(version, "13")) {
		printf(
		    "Wrong WebSocket version!  %s, expecting 8 or 13.\n",
		    version);
		return 0;
	}

	char iv[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	char *key = request_get_header(req, "Sec-WebSocket-Key");

	char concat[80];  // TODO: exact length?
	strcpy(concat, key);
	strcat(concat, iv);
	char *hash =
	    (char *)SHA1((unsigned char *)concat, strlen(concat), NULL);

	BIO *b64 = BIO_new(BIO_f_base64());
	BIO *bmem = BIO_new(BIO_s_mem());
	BIO_push(b64, bmem);
	BIO_write(b64, hash, 20);
	if (BIO_flush(b64) != 1) perror("BIO_flush");
	BUF_MEM *bptr;
	BIO_get_mem_ptr(b64, &bptr);

	char *hash_enc = malloc(bptr->length);
	memcpy(hash_enc, bptr->data, bptr->length);
	hash_enc[bptr->length - 1] = 0;

	char msg_f[] =
	    "HTTP/1.1 101 Switching Protocols\r\n"
	    "Connection: Upgrade\r\n"
	    "Upgrade: websocket\r\n"
	    "Sec-WebSocket-Accept: %s\r\n\r\n";

	// hash_enc is 28 bytes long, replacing '%s'...
	char *msg = malloc(strlen(msg_f) + (28 - 1));
	sprintf(msg, msg_f, hash_enc);
	free(hash_enc);

	socket_write(con->socket, msg, strlen(msg));
	free(msg);

	BIO_free_all(b64);

	con->is_websocket = 1;

	return 1;
}

frame_t *new_frame(char fin, int len, int opcode, char *payload) {
	frame_t *frame = malloc(sizeof(frame_t));
	frame->fin = 1;
	frame->len = len;
	frame->opcode = WS_OP_BIN;
	frame->payload = payload;
	return frame;
}

char *write_frame(frame_t *frame, int *length) {
	char *data;

	if (frame->len < 126) {
		*length = 2 + frame->len;
		data = malloc(*length);
		data[0] = (frame->opcode & 15);
		if (frame->fin) {
			data[0] = data[0] | 128;
		}
		data[1] = (char)frame->len;
		memcpy(data + 2, frame->payload, frame->len);
	} else if (frame->len < pow(2, 16)) {
		*length = 4 + frame->len;
		data = malloc(*length);
		data[0] = (frame->opcode & 15);
		if (frame->fin) {
			data[0] = data[0] | 128;
		}
		data[1] = (char)126;
		((short *)data)[1] = htons((short)frame->len);
		memcpy(data + 4, frame->payload, frame->len);
	} else {
		printf("Frame length too long.\n");
		return NULL;
	}
	return data;
}

frame_t *read_frame(char *data, int length, int *used) {
	unsigned int fin, opcode, do_mask, payload_len;
	char mask_val[4];
	int mask_start = 2;

	if (length < 8) {
		return NULL;
	}

	fin = data[0] & 128;
	opcode = data[0] & 15;
	do_mask = data[1] & 128;
	payload_len = data[1] & 127;

	switch (payload_len) {
		case 126:
			// Two byte length. [3] and [4].
			// Network order.
			mask_start = 4;
			memcpy(&payload_len, data + 2, 2);
			payload_len = ntohs(payload_len);
			break;
		case 127:
			mask_start = 10;
			// ntohl is only 32 bit, need 64.
			printf("Long payload length not yet implemented!\n");
			return NULL;
			break;
	}

	if (payload_len > 2 + length) {
		// Incomplete.
		return NULL;
	}

	frame_t *frame = malloc(sizeof(frame_t));
	frame->fin = fin;
	frame->opcode = opcode;
	frame->len = payload_len;

	if (do_mask) {
		memcpy(mask_val, data + mask_start, 4);
		frame->payload =
		    mask(mask_val, payload_len, data + mask_start + 4);
	} else {
		frame->payload = malloc(payload_len);
		memcpy(frame->payload, data + mask_start, payload_len);
	}

	*used = mask_start + (do_mask ? 4 : 0) + payload_len;

	return frame;
}

char *mask(char *mask, int length, char *data) {
	char *retval = malloc(length);
	int i;
	for (i = 0; i != length; i++) {
		retval[i] = data[i] ^ mask[i % 4];
	}
	return retval;
}
