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
