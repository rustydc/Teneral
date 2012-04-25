#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "processes.h"
#include "tnrld.h"

extern tnrld_t *tnrld;

// TODO:  Should probably set up the read watcher before the fork.
process_t *execute_prog(char **argv, int in, int out, int err) //cmd_execute_t *cmd)
{
	int pid = fork();

	if (pid == 0) {
		// Child.
		close(tnrld->listen_fd);
		printf("Calling execvp: '%s'...\n", argv[0]);

		// Install default pipe signal handler.
		signal(SIGPIPE, SIG_DFL);

		if ( dup2(in, 0) == -1) {
			perror("dup");
		}
		if ( dup2(out, 1) == -1) {
			perror("dup");
		}
		if ( dup2(err, 2) == -1) {
			perror("dup");
		}

		execvp(argv[0], argv);
		perror("execvp");
		exit(1);
	} else {
		close(in);
		close(out);
		process_t *p = malloc(sizeof(process_t));
		p->pid = pid;
		p->in = in;
		p->out = out;
		p->err = err;
		p->argv = argv;
		return p;
	}

	return NULL;
}

process_t *execute_cmd(cmd_execute_t *cmd)
{
	int in[2];
	int out[2];
	int err[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, in)) {
		perror("socketpair");
		exit(1);
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, out)) {
		perror("socketpair");
		exit(1);
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, err)) {
		perror("socketpair");
		exit(1);
	}
 
	// loop over the commands, executing each one.
	int i, p_in, p_out;
	int pfds[2];
	process_t *last_proc;
	for (i = 0; cmd->argv[i] != NULL; i++) {
		// If it's the first command, use the new socket.
		if (i == 0) {
			p_in = in[0];
		} else {
			p_in = pfds[0];
		}
		
		// If it's the last, use the new out socket.
		if (cmd->argv[i+1] == NULL) {
			p_out = out[0];
		} else {
			if (pipe(pfds) == -1) {
				perror("pipe");
				exit(1);
			}
			p_out = pfds[1];
		}

		last_proc = execute_prog(cmd->argv[i], p_in, p_out, err[0]);
		// TODO:  Save or free the non-last ones.
	}

	// Return the in/out/err of the whole pipeline.
	last_proc->in = in[1];
	last_proc->out = out[1];
	last_proc->err = err[1];

	return last_proc;
}

