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
char* getPrompt() {
	char cwd[FILENAME_MAX] = "";
	char* prompt = malloc(FILENAME_MAX);
	if (!getcwd(cwd, sizeof(cwd))) {
		/* TODO: handle error, store some indicator of unknown CWD? */
		strcpy(cwd, "");
	}

	sprintf(prompt, "%s$ ", cwd);
	return prompt;
}

/* adds the current working directory to the path so that we can find our
   custom executables */
int updateWD() {
	char cwd[FILENAME_MAX];
	const char* path = getenv("PATH");
	int size;
	char* newPath;

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("getcwd");
		return 0;
	}
	if (! path)
		return 0;

	size = sizeof(path) + sizeof(cwd) + 10;
	newPath = malloc(size);

	sprintf(newPath, "%s:%s", path, cwd);
	setenv("PATH", newPath, 1);
	free (newPath);

	return 1;
}

/* trim whitepsace from string */
char* trim(char* string) {
	char* end;

	while(isspace(*string))
		string++;

	if(*string == 0)
		return string;

	end = string + strlen(string) - 1;
	while (end > string && isspace(*end))
		end--;

	return string;
}

/*
 * retrieves a command from the user
 */
char* getCommand() {
	char* command = readline(getPrompt());

	/* if non-empty add to history */
	if (command &&  *command)
		add_history(command);

	return command;
}

/*
 * Tokenize the command string
 */
char** parseLine(char* line) {
	char** result = 0;
	char* temp    = line;
	size_t count  = 0;

	/* count the spaces in the command, will be >= to number of valid strings in the command */
	while (*temp) {
		if ( ' ' == *temp)
			count++;
		temp++;
	}

	count++;  /* add room for terminating NULL */

	result = malloc(sizeof(char*) * count);

	if (result) {
		size_t idx = 0;
		char* token = strtok(line, "\x20\x09\x0a\x0b\x0c\x0d");

		while (token) {
			*(result + idx++) = strdup(token);
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
	char** cursor;

	/* set the working directory */
	if (! updateWD()) {
		printf("Error adding CWD to the path");
		return 1;
	}

	while (1) {
		lineRead = getCommand();

		if (! lineRead) {
			/*TODO: handle command parse error */
			printf("Unknown command");
			continue;
		}

		/* trim any whitespace from front and end */
		lineRead = trim(lineRead);
		command  = parseLine(lineRead);

		/* command 'exit' exits the shell */
		if (strcmp("exit", lineRead) == 0)
			exit(EXIT_SUCCESS);

		/* attempt to fork the process */
		if ((childPID = fork()) == -1){
			perror("fork");
		} else if (childPID == 0) {   /* this is the child */
			/* first check for a pipe */
			cursor = command;
			while (cursor && *cursor && (strcmp(*cursor, "|") != 0))
				cursor++;

			if (*cursor) {
				cursor++;
				/* pipe! */
				printf("PIPE!\n");
				continue;
			}

			/* check for redirections */
			cursor = command;
			while (cursor && *cursor && *cursor[0] != '>')
				cursor++;

			if (*cursor) {
				/* one arg? (>dest) */

				/* remove ">dest" from array */


				/* two args? (> dest) */

				/* remove "> dest" from array */
			}


			cursor = command;
			while (cursor && *cursor && *cursor[0] != '<')
				cursor++;

			if (*cursor) {
				/* one arg? (<src) */

				/* remove "<src" from the array */

				/* two args? (< src) */

				/* remove "< src" from array */
			}

			execvp(*command, command);

			// if child returns then print out the error
			perror("exec");
		} else {               /* this is the parent */
			/* TODO: do we need to customize handling of errors? */
			waitpid(childPID, NULL, 0);
		}
	}
}
