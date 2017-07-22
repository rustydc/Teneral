#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include "http.h"

#define WS_OP_CONT  0x0
#define WS_OP_TEXT  0x1
#define WS_OP_BIN   0x2
#define WS_OP_CLOSE 0x8
#define WS_OP_PING  0x9
#define WS_OP_PONG  0xA


typedef struct frame_s {
	char fin;
	char opcode;
	long int len;
	char *payload;
} frame_t;
	

int try_upgrade(request_t *req, connection_t *con);
char *unmask(char *mask, int length, char *data);
char *mask(char *mask, int length, char *data);

frame_t *new_frame(char fin, int len, int opcode, char *payload);
frame_t *read_frame(char *data, int length, int *used);
char *write_frame(frame_t *frame, int *length);

#endif
