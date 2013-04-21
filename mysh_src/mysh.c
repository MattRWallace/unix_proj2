#define _XOPEN_SOURCE      600

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include "strings.h"
#include "shell.h"

#define UMASK      0600
#define WHITESPACE "\x20\x09\x0a\x0b\x0c\x0d"
#define LARGE_BUF  4096

/*
int execRedirect(char** command, char* input, char* output) {
    // file descriptors for input and output files
    int infile, outfile;

    // backup original stdin and stdout
    int real_stdin = dup(STDIN);
    int real_stdout = dup(STDOUT);

    // child PID
    pid_t childPID;

    // check and open each file if needed
    if (strlen(input) != 0) {
        if ((infile = open(input, O_RDONLY, UMASK)) == -1) {
            perror(input);
            return -1;
        }
        dup2(infile, STDIN);
    }

    if (strlen(output) != 0) {
        if ((outfile = open(output, O_WRONLY | O_CREAT | O_TRUNC, UMASK)) == -1) {
            perror(output);
            printf("<%s>", output);
            return -1;
        }
        dup2(outfile, STDOUT);
    }

    // exec
    if ((childPID = fork()) == -1){
        perror("fork");
    } else if (childPID == 0) {   // this is the child

        execvp(*command, command);

        // if child returns then print out the error
        perror("exec");
    } else {               // this is the parent
        // TODO: do we need to customize handling of errors?
        waitpid(childPID, NULL, 0);
    }

    // redirect IO and close the files
    if (strlen(input) != 0) {
        dup2(real_stdin, STDIN);
        close(infile);
    }

    if (strlen(output) != 0) {
        dup2(real_stdout, STDOUT);
        close(outfile);
    }

    // close the backups
    close(real_stdin);
    close(real_stdout);

    return 1;
}
*/


int main(int argc, char** argv) {
	if (! updateWD()) {
		fprintf(stderr, "Error adding CWD to the path");
		return 127;
	}

	// flag to exit early
	int more = 1;

	while(more) {            // main terminal loop
		char* lineRead = { 0 };

		if (argc > 1) {
			glueStrings(&lineRead, argv, argc);
			more = 0;
		} else
			getCommand(&lineRead);
		processPipes(lineRead);


		free(lineRead);
		lineRead = 0;
	}
}
