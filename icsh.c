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


#define MAX_CMD_BUFFER 255

static int pid;
static int sig = 0;
static int lastExit = 0;
static char* lastCommand = "";

void handlerPID(int signum) {
	if (pid) {
		if (signum == 2)
			kill(pid, SIGINT);
		else if (signum == 20)
			kill(pid, SIGSTOP);
		printf("\n");
	}
}

void handler(int signum) {
	sig = 1;
	return;
}

int exec_prog(char* args[]) {

	struct sigaction new_action;
        sigemptyset(&new_action.sa_mask);
        new_action.sa_handler = handlerPID;
        new_action.sa_flags = 0;
        sigaction(SIGINT, &new_action, NULL);
	sigaction(SIGTSTP, &new_action, NULL);
	
	int status;

	if ((pid=fork()) < 0) {
		perror ("Fork failed");
		exit(1);
	}
	if (!pid) {
		execvp(args[0], args);
	}

      	if (pid) {
		waitpid (pid, &status, 0);
		return 0;
	}
	if ( WIFEXITED(status) ) {
        	lastExit = WEXITSTATUS(status);
    	}
	
	pid = 0;
	return 1;
}

char* parseCommand(char* input) {
	char* ori = strdup(input);
	input[strlen(input)-1] = '\0';

	char* p = strtok(input, " ");
	char* arg[4];
	
	for (int i=0; i<4; i++) {
		arg[i] = p;
		p = strtok(NULL, " ");
	}	

	if (strcmp(arg[0], "echo") == 0) {
		if (strcmp(arg[1], "$?") == 0) {
			printf("%d\n", lastExit);
		}
		else {
			printf("%s",ori+5);
		}
		lastExit = 0;
	}

	else if (strcmp(arg[0], "exit") == 0) {
		char exitCode = atoi(arg[1]);
		printf("bye\n");
		lastExit = 0;
		exit(exitCode);
	}

	else if (strcmp(arg[0], "!!") == 0) {
		if (strcmp(lastCommand, "") != 0) {
			printf("%s", lastCommand);
			lastCommand = parseCommand(lastCommand);
		}
	}

	else {
		int bufferLen = 128;
		char *comm = (char*)malloc(bufferLen * sizeof(char));
		sprintf(comm, "which %s > /dev/null 2>&1", arg[0]);
		if (system(comm)) {
			printf("bad command\n");
		}
		else {
			exec_prog(arg);
		}
		free(comm);
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

    	while (1) {

		sigaction(SIGINT, &new_action, NULL);
		sigaction(SIGTSTP, &new_action, NULL);

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
    	}
}

