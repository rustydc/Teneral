#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json/json.h>

// From 'libssl-dev'
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

#include "messages.h"



cmd_t *parse_msg(char *msg) {
	
	json_object *new_obj;
	new_obj = json_tokener_parse(msg);

	if (new_obj == NULL || json_object_is_type(new_obj, json_type_null)) {
		printf("Not a JSON object.\n");
		return NULL;
	}

	json_object *exec = json_object_object_get(new_obj, "execute");

	if (exec != NULL && json_object_get_type(exec) != json_type_null) {
		json_object *cmds = json_object_object_get(exec, "arguments");
		printf("args: '%s'\n", json_object_to_json_string(cmds));
		int len = json_object_array_length(cmds);
		int i, j;

		char ***argv = malloc(sizeof(char **) * (len+1));

		// For each arg group, which is an array:
		for (i = 0; i != len; i++) {
			json_object *args = json_object_array_get_idx(cmds, i);
			int len2 = json_object_array_length(args);
			argv[i] = malloc(sizeof(char *) * (len2+1));

			// For each arg in THIS array,
			for (j = 0; j != len2; j++) {
				json_object *arg = json_object_array_get_idx(args, j);
				printf("arg[%d][%d]: '%s'\n", i, j, json_object_to_json_string(arg));
				char *arg_str = (char *) json_object_get_string(arg);
				argv[i][j] = malloc(strlen(arg_str));
				strcpy(argv[i][j], arg_str);
			}
			argv[i][j] = NULL;
		}
		argv[i] = NULL;

		json_object *req_id_obj = json_object_object_get(exec, "requestId");
		int req_id = json_object_get_int(req_id_obj);

		json_object *cmd_obj = json_object_object_get(exec, "command");
		const char *cmd_str = json_object_get_string(cmd_obj);

		cmd_execute_t *retval = malloc(sizeof(cmd_execute_t));
		retval->command.type = CMD_EXECUTE;

		// TODO: retval->argv = argv;
		retval->request_id = req_id;
		retval->argv = argv;
		retval->command_str = strdup(cmd_str);
		
		// TODO:  Free the object.
		return (cmd_t *) retval;
	}


	// TODO: Signal message.
	json_object *signal = json_object_object_get(new_obj, "signal");
	if (signal != NULL && json_object_get_type(signal) != json_type_null) {
		printf("Signal.\n");
		printf("%s\n", json_object_to_json_string(signal));
	}


	return NULL;
}

char *json_pid_out(int pid, char *type, char *output, int length)
{
	json_object *msg = json_object_new_object();
	json_object *jpout = json_object_new_object();

	json_object *jpid = json_object_new_int(pid);
	json_object_object_add(jpout, "pid", jpid);

	BIO *b64 = BIO_new(BIO_f_base64());
	BIO *bmem = BIO_new(BIO_s_mem());
	BIO_push(b64, bmem);
	BIO_write(b64, output, length);
	if (BIO_flush(b64) != 1) perror("BIO_flush");
	BUF_MEM *bptr;
	BIO_get_mem_ptr(b64, &bptr);

	char *data_enc = malloc(bptr->length);
	memcpy(data_enc, bptr->data, bptr->length);
	data_enc[bptr->length - 1] = 0;

	json_object *jout = json_object_new_string_len(data_enc, bptr->length);
	json_object_object_add(jpout, "data", jout);

	json_object *jtype = json_object_new_string("stdout");
	json_object_object_add(jpout, "type", jtype);
	
	json_object_object_add(msg, "processOutput", jpout);

	return (char *) json_object_to_json_string(msg);
}

char *json_pid_status(int pid, int ret, int sig);
char *json_pid_new(int pid, char *command, int request_id);

