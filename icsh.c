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

static pid_t rpid;
static int sig = 0;
static int lastExit = 0;
static char* lastCommand = "";
int cjid = 0;
typedef struct Job {
	int jid;
	pid_t pid;
	char* status;
	char* command;
	int bg;
} Job;
Job jobs[100];

Job findjob(pid_t pid) {
	for (int i=0; i<100; i++) {
		if (jobs[i].pid == pid)
			return jobs[i];
	}
	// dummy output, should not get here
	return jobs[0];
}

int addjob(pid_t pid, char* command) {
        Job newjob;
        newjob.jid = ++cjid;
        newjob.pid = pid;
	newjob.status = "Running";
	newjob.command = command;
	newjob.bg = 0;
        for (int i=0; i<100; i++) {
                if (jobs[i].pid == 0) {
                        jobs[i] = newjob;
			break;
		}
        }
        return cjid;
}

int deletejob(pid_t pid) {
	for (int i=0; i<100; i++) {
		if (jobs[i].pid == pid) {
			jobs[i].pid = -1;
			return 0;
		}
	}
	return -1;
}

void printjob() {
	for (int i=0; i<100; i++) {
		if (jobs[i].pid != 0) {
			if (jobs[i].jid == cjid)
				printf("[%d]+  %s\t\t%s", jobs[i].jid, jobs[i].status, jobs[i].command);
			else 
				printf("[%d]-  %s\t\t%s", jobs[i].jid, jobs[i].status, jobs[i].command);
		}
	}
}

void sigchld_handler(int sig) {

    	sigset_t mask_all, prev_all;
    	sigfillset(&mask_all);

    	/*Reap zombie */
    	pid_t pid = waitpid(-1, NULL, WNOHANG);
    	if (pid > 0) {
        	sigprocmask(SIG_BLOCK, &mask_all, &prev_all);   //Block all signals
        	sigprocmask(SIG_SETMASK, &prev_all, NULL);	//Unblock signals
		Job job = findjob(pid);
		if (job.jid != 0 && pid != rpid) {
			printf("\n[%d]+  Done\t\t%s", job.jid, job.command);
			if (!job.bg)
				printf("icsh $ ");
			deletejob(pid);
		}
		//} else 
		//	printf("KILLED | STOPPED\n");
    	}
	//deletejob(rpid);
	//rpid = 0;

    return;
}

void handler(int signum) {
	//printf("%d", signum);
	if (rpid && (signum == 2 || signum == 20)) {
		//printf("%d", rpid);
		if (signum == 2) {
			kill(rpid, SIGINT);
			deletejob(rpid);
		}
		else {
			for (int i=0; i<100; i++) {
				if (rpid == jobs[i].pid)
					jobs[i].status = "Stopped";
			}
			kill(rpid, SIGSTOP);
		}
	}
	else if (signum == SIGCHLD) {
		sigchld_handler(signum);
		sig = 2;
		return;
	}
	else if (signum == SIGTTOU || signum == SIGTTIN) {
		return;
	}
	else if (signum == SIGCONT) {
		return;
	}
	else {
		sig = 1;
		return;
	}
	printf("\n");
}

int exec_prog(char* args[], int fg, char* command) {

	if (strcmp(args[0], "b") == 0)
		return 0;

	int status;
	int cpid;

	sigset_t mask_all, mask_one, prev_one;

    	sigfillset(&mask_all);
    	sigemptyset(&mask_one);
    	sigaddset(&mask_one, SIGCHLD);
	if (!fg)
		sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

	if ((cpid=fork()) < 0) {
		perror ("Fork failed");
		exit(1);
	}
	if (!cpid) {
		if (execvp(args[0], args) == -1) {
			printf("bad command\n");
			exit(1);
		}
	}
      	if (cpid) {
		int jid = addjob(cpid, command);
		if (fg) {
			rpid = cpid;
                        waitpid (cpid, &status, 0);
                }
		if (!fg) {
			sigprocmask(SIG_BLOCK, &mask_all, NULL);
        		setpgid(cpid, 0);
			printf("[%d] %d\n", jid, cpid);
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

void foreground(char* arg) {
	if (arg == NULL) {
		printf("Job not found\n");
		return;
	}
	int jid = atoi(arg+1);
	int found = 0;
	for (int i=0; i<100; i++) {
		if (jobs[i].jid == jid) {
			found = 1;
			rpid = jobs[i].pid;
			waitpid(jobs[i].pid, NULL, 0);
			//printf("fg %d",jobs[i].pid);
		}
	}
	if (!found)
		printf("Job not found\n");
}

void background(char* arg) {
        if (arg == NULL) {
                printf("Job not found\n");
                return;
        }
        int jid = atoi(arg+1);
        int found = 0;
        for (int i=0; i<100; i++) {
                if (jobs[i].jid == jid) {
                        found = 1;
			jobs[i].bg = 1;
			rpid = 0;
			kill(jobs[i].pid, SIGCONT);
                        //printf("bg %s",jobs[i].command);
                }
        }
        if (!found)
                printf("Job not found\n");
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
			exec_prog(args, fg, strdup(ori));
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
	else if (strcmp(args[0], "jobs") == 0) {
		printjob();
	}

	else if (strcmp(args[0], "fg") == 0) {
		foreground(args[1]);
	}

	else if (strcmp(args[0], "bg") == 0) {
		background(args[1]);
	}

	else {
		exec_prog(args, fg, strdup(ori));
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

	jobs[0].pid = -1;

    	while (1) {

		sigaction(SIGINT, &new_action, NULL);
		sigaction(SIGTSTP, &new_action, NULL);
		sigaction(SIGCHLD, &new_action, NULL);
		sigaction(SIGTTIN, &new_action, NULL);
                sigaction(SIGTTOU, &new_action, NULL);

		if (sig) {
			if (sig == 1)
				printf("\n");
			sig = 0;
			continue;
		}

		if (sig == 0)
			printf("icsh $ ");
		fgets(buffer, 255, stdin);


		if (sig) {
			if (sig == 1) 
				continue;
			if (sig == 2) {
				fgets(buffer, 255, stdin);
				sig = 0;
			}
		}

		if (strcmp(buffer, "\n") == 0) {
			continue;
		}

		if (strcmp(buffer, repeat) == 0) {
			parseCommand(buffer);
		}

		else if (sig == 0) {
			lastCommand = parseCommand(buffer);
		}

		if (waitpid(rpid, NULL, WNOHANG) < 0) {
			deletejob(rpid);
			rpid = 0;
		}
;
    	}
}
