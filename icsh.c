/* ICCS227: Project 1: icsh
 * Name: Niraporn Kovitaya
 * StudentID: 6281507
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>


#define MAX_CMD_BUFFER 255

static pid_t pid;
static int sig = 0;
static int lastExit = 0;
static char* lastCommand = "";
int keeprunning = 1;
int cjid = 0;
int jobs[100];

void sigchld_handler(int sig) 
{
    	sigset_t mask_all, prev_all;
    	sigfillset(&mask_all);

    	/*Reap zombie */
    	pid_t pid;
    	if((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        	sigprocmask(SIG_BLOCK, &mask_all, &prev_all);   //Block all signals
        	//deletejob(jobs, pid);                           //Delete job from jobs array
        	sigprocmask(SIG_SETMASK, &prev_all, NULL);	//Unblock signals
		//printf("ffffffff");
    	}

    return;
}

void handler(int signum) {
	printf("%d", signum);
	if (pid && (signum == 2 || signum == 20)) {
		if (signum == 2) 
			kill(pid, SIGINT);
		else if (signum == 20) 
			kill(pid, SIGTSTP);
	}
	else if (signum == SIGCHLD) {
		sigchld_handler(sig);
		sig = 1;
		return;
	}
	else if (signum == SIGTTOU || signum == SIGTTIN) {
		return;
	}
	else {
		sig = 1;
		return;
	}
	printf("\n");
}

int addjob(pid_t pid) {
	struct Job {
		int jid;
		int pid;
	} newjob;
	newjob.jid = ++cjid;
	newjob.pid = pid;
	return cjid;
}

int exec_prog(char* args[], int fg) {

	int status;
	int cpid;
	pid_t ppid;

	sigset_t mask_all, mask_one, prev_one;

    	sigfillset(&mask_all);
    	sigemptyset(&mask_one);
    	sigaddset(&mask_one, SIGCHLD);
	sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

	if ((cpid=fork()) < 0) {
		perror ("Fork failed");
		exit(1);
	}
	if (!cpid) {
		if (!fg) {
			sigprocmask(SIG_BLOCK, &mask_all, NULL);
		}
		if (execvp(args[0], args) == -1) {
			printf("bad command\n");
			exit(1);
		}
	}
      	if (cpid) {
		int jid = addjob(cpid);
		if (fg) {
                        waitpid (cpid, &status, 0);
			pid = cpid;
                }
		if (!fg) {
			printf("[%d]\t", jid);
			for (int i=0; i<4; i++) {
				if (args[i] == NULL)
					break;
				printf("%s ", args[i]);
			}
			printf("\n");
		}
	}

	if (!fg)
		sigprocmask(SIG_SETMASK, &prev_one, NULL);

	if ( WIFEXITED(status) ) {
                lastExit = WEXITSTATUS(status);
        }


	fflush(stdout);
	
	return 1;
}

char* parseCommand(char* input) {

	char* ori = strdup(input);
	input[strlen(input)-1] = '\0';

	char* p = strtok(input, " ");
	char* args[4];
	int redirect = 0;
	char* filename;
	int fg = 1;
	int write = 0;
	FILE* fptr;
	
	for (int i = 0; i < 4; i++) {
		args[i] = p;
		p = strtok(NULL, " ");
		if (write && args[i] != NULL) {
			fprintf(fptr, "%s ", args[i]);
		}
		if (i > 0 && args[i] != NULL && strcmp(args[i], "&") == 0) {
			fg = 0;
			args[i] = NULL;
		}
		if (i > 0  && (args[i-1] != NULL) && (strcmp(args[i-1], "<") == 0) && (args[i] != NULL)) {
				filename = args[0];
				fptr = fopen(filename, "w");
				write = 1;
				char* temp = args[i];
				args[i-1] = NULL;
				i = 0;
				args[i] = temp; 
				fprintf(fptr, "%s ", args[i]);
				i++;
		}		
		if (i > 0 && (args[i-1] != NULL) && (strcmp(args[i-1], ">") == 0) && (args[i] != NULL)) {
			redirect = 1;
			filename = args[i];
			args[i-1] = NULL;
			args[i] = NULL;
		}
	}

	if (write) {
		fprintf(fptr, "\n"); 
		fclose(fptr);
	}
	
	int pfd;
	int saved;

	if (redirect) {
		pfd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		saved = dup(1);
		close(1);
		dup(pfd);
		close(pfd);
	}

	if (strcmp(args[0], "echo") == 0) {
		if (strcmp(args[1], "$?") == 0) {
			printf("%d\n", lastExit);
		}
		else if (redirect || fg == 0) {
			exec_prog(args, fg);
		}
		else {
			printf("%s",ori+5);
		}
		lastExit = 0;
	}

	else if (strcmp(args[0], "exit") == 0) {
		char exitCode = atoi(args[1]);
		printf("bye\n");
		lastExit = 0;
		exit(exitCode);
	}

	else if (strcmp(args[0], "!!") == 0) {
		if (strcmp(lastCommand, "") != 0) {
			printf("%s", lastCommand);
			lastCommand = parseCommand(lastCommand);
		}
	}

	else {
		exec_prog(args, fg);
	}

	if (redirect) {
		fflush(stdout);
		// restore stdout back
		dup2(saved, 1);
		close(saved);
	}

	return ori;
}

void scriptMode(char* filepath) {
    	FILE * fp;
    	char * line = NULL;
    	size_t len = 0;
    	ssize_t read;

    	fp = fopen(filepath, "r");
    	if (fp == NULL)
        	printf("file not found\n");

    	while ((read = getline(&line, &len, fp)) != -1) {
        	lastCommand = parseCommand(line);
    	}

    	fclose(fp);
    	if (line)
        	free(line);
}

int main(int argc, char **argv) {

	struct sigaction new_action;
        sigemptyset(&new_action.sa_mask);
        new_action.sa_handler = handler;
        new_action.sa_flags = 0;

	char buffer[MAX_CMD_BUFFER];
	char* repeat = "!!\n";

	if (argc > 1) {
		scriptMode(argv[1]);	
	}

    	while (keeprunning) {

		sigaction(SIGINT, &new_action, NULL);
		sigaction(SIGTSTP, &new_action, NULL);
		sigaction(SIGCHLD, &new_action, NULL);
		sigaction(SIGTTIN, &new_action, NULL);
                sigaction(SIGTTOU, &new_action, NULL);

        	printf("icsh $ ");
		fgets(buffer, 255, stdin);

		if (sig == 1) {
			sig = 0;
			printf("\n"); 
			continue;
		}

		if (strcmp(buffer, "\n") == 0) {
                	continue;
		}

		if (strcmp(buffer, repeat) == 0) {
			parseCommand(buffer);
		}

		else {
			lastCommand = parseCommand(buffer);
		}

		pid = 0;
    	}
}
