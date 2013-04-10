#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * Prints the command prompt
 */
void printPrompt() {
	char cwd[FILENAME_MAX] = "";
	if (!getcwd(cwd, sizeof(cwd))) {
		// TODO: handle error, store some indicator of unknown CWD?
		strcpy(cwd, "");
	}

	printf("%s$ ", cwd);
}

/*
 * retrieves a command from the user
 */
char* getCommand() {
	char* command = readline("");

	// if non-empty add to history
	if (command &&  *command)
		add_history(command);

	return command;
}

/*
 * Tokenize the command string and handle any redirections
 */
char** parseLine(char* line) {
	char** result = 0;
	char* temp    = line;
	size_t count  = 0;

	// count the spaces in the command, will be >= to number of valid strings in the command
	while (*temp) {
		if ( ' ' == *temp)
			count++;
		temp++;
	}

	count++;  /* add room for terminating NULL */

	result = malloc(sizeof(char*) * count);

	if (result) {
		size_t idx = 0;
		char* token = strtok(line, " ");

		while (token) {
			if (*token) {            /* token is not an empty string */
				// TODO: need to handle redirections here.
				*(result + idx++) = strdup(token);
			}
			token = strtok(0, " ");
		}
		*(result + idx) = 0;          /* add terminating null */
	}

	return result;
}

int main(int argc, char ** argv) {
	pid_t childPID;
	char* lineRead;
	char** command;

	while (1) {
		printPrompt();

		lineRead = getCommand();
		command  = parseLine(lineRead);

		if (! command) {
			//TODO: handle command parse error
			continue;
		}

		if (strcmp("exit", command[0]) == 0)
			exit(EXIT_SUCCESS);

		childPID = fork();
		if (childPID == 0) {   /* this is the child */
			execvp(*command, command);
		} else {               /* this is the parent */
			// TODO: do we need to customize handling of errors?
			waitpid(childPID, NULL, 0);
		}
	}
}
