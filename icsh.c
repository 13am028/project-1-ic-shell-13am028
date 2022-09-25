/* ICCS227: Project 1: icsh
 * Name: Niraporn Kovitaya
 * StudentID: 6281507
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"


#define MAX_CMD_BUFFER 255

char* parseCommand(char* input) {
	char* copy = strdup(input);
	input[strlen(input)-1] = '\0';

	int i = 0;
	char* p = strtok(input, " ");
	char* arg[3];
	
	while (p != NULL) {
		arg[i++] = p;
		p = strtok(NULL, " ");
	}	

	if (strcmp(arg[0], "echo") == 0) {
		printf("%s",copy+5);
	}

	if (strcmp(arg[0], "exit") == 0) {
		char exitCode = atoi(arg[1]);
		printf("bye\n");
		exit(exitCode);
	}

	return copy;
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
    if (argc > 0) {
    	scriptMode(argv[1]);
    }
    while (1) {
        printf("icsh $ ");
	fgets(buffer, 255, stdin);
        if (strcmp(buffer, repeat) == 0) {
		printf("%s", lastCommand);
                parseCommand(lastCommand);
        } 
	else {
		lastCommand = parseCommand(buffer);
	}
    }
}

