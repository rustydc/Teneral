#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#define CMD_EXECUTE 0
#define CMD_SIGNAL 1

typedef struct cmd_s { int type; } cmd_t;

typedef struct cmd_execute_s {
	cmd_t command;
	char *command_str;
	char ***argv;    // Assume we pipe from argv[0] to argv[n]
	                 //  Each is a list of args.
	int request_id;  // Just a token to track which is which?
} cmd_execute_t;

char *json_pid_out(int pid, char *type, char *output, int length);
char *json_pid_status(int pid, int ret, int sig);
char *json_pid_new(int pid, char *command, int request_id);

cmd_t *parse_msg(char *msg);
void free_msg(cmd_t *msg);

#endif
