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

void handler(int signum) {
	if (pid) {
		if (signum == 2)
			kill(pid, SIGINT);
		else if (signum == 20)
			kill(pid, SIGTSTP);
		printf("\n");
	}
	else 
		sig = 1;
}

int exec_prog(char* args[]) {

	int status;

	if ((pid=fork()) < 0) {
		perror ("Fork failed");
		exit(1);
	}
	if (!pid) {
		if (execvp(args[0], args) == -1) {
			printf("bad command\n");
			exit(1);
		}
	}
      	if (pid) {
		lastExit = waitpid (pid, &status, 0);
		return 0;
	}
	/*if ( WIFEXITED(status) ) {
        	lastExit = WEXITSTATUS(status);
    	}*/

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
	
	for (int i = 0; i < 4; i++) {
		args[i] = p;
		p = strtok(NULL, " ");
		if (i > 0 && (args[i-1] != NULL) && (strcmp(args[i-1], ">") == 0) && (args[i] != NULL)) {
			redirect = 1;
			filename = args[i];
			args[i-1] = NULL;
			args[i] = NULL;
		}
	}
	
	int pfd;
	int saved;

	if (redirect) {
		pfd = open("a.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		saved = dup(1);
		close(1);
		dup(pfd);
		close(pfd);
	}

	if (strcmp(args[0], "echo") == 0) {
		if (strcmp(args[1], "$?") == 0) {
			printf("%d\n", lastExit);
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
		exec_prog(args);
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
		pid = 0;
    	}
}
