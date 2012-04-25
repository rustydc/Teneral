#ifndef __PROCESSES_H__
#define __PROCESSES_H__

#include <sys/time.h>

#include "messages.h"

typedef struct process_completion_s {
	int return_code;
	int by_signal;
	struct timeval end_time;
} process_completion_t;

typedef struct process_s {
        int pid;
        char **argv;
        int in;
        int out;
        int err;
	process_completion_t *completion;
} process_t;

process_t *execute_cmd(cmd_execute_t *cmd);
void delete_process(process_t *process);

void process_done(process_t *process, int return_code, int by_signal);

// redirect file in, redirect file out?
// pipelines?

#endif
