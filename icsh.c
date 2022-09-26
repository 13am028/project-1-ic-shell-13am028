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


#define MAX_CMD_BUFFER 255


int exec_prog(char* args[]) {
	int pid;
	if ((pid=fork()) < 0) {
		perror ("Fork failed");
		exit(1);
	}
	if (!pid) {
		execvp(args[0], args);
	}

      	if (pid) {
		waitpid (pid, NULL, 0);
		return 0;
	}
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
		printf("%s",ori+5);
	}

	else if (strcmp(arg[0], "exit") == 0) {
		char exitCode = atoi(arg[1]);
		printf("bye\n");
		exit(exitCode);
	}

	else {
		if (exec_prog(arg) != 0) {
		       printf("bad command\n");
		}	       
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
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        parseCommand(line);
    }

    fclose(fp);
    if (line)
        free(line);
}

int main(int argc, char **argv) {
    char buffer[MAX_CMD_BUFFER];
    char* lastCommand;
    char* repeat = "!!\n";
    if (argc > 1) {
    	scriptMode(argv[1]);
    }
    while (1) {
        printf("icsh $ ");
	fgets(buffer, 255, stdin);
        if (strcmp(buffer, repeat) == 0) {
		printf("%s", lastCommand);
                parseCommand(strdup(lastCommand));
        } 
	else {
		lastCommand = parseCommand(buffer);
	}
    }
}

