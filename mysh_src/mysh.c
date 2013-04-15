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

int execPipe(char** command1, char** command2) {
	int pfd[2];
	pid_t cpid1, cpid2;

	if (pipe(pfd) == -1) {
		perror("pipe");
		return -1;
	}


	cpid1 = fork();
	if (cpid1 == -1) {
		perror("fork");
		return -1;
	}

	if (cpid1 == 0) {
		dup2(pfd[1], 1);
		close(pfd[0]);
		close(pfd[1]);
		execvp(*command1, command1);
	} else {
		close(pfd[1]);
	}

	cpid2 = fork();
	if (cpid2 == -1) {
		perror("fork");
		return -1;
	}

	if (cpid2 == 0) {
		dup2(pfd[0], 0);
		close(pfd[0]);
		close(pfd[1]);
		execvp(*command2, command2);
	}

	/* wait for processes to exit */
	/*waitpid(cpid1, NULL, 0);*/
	waitpid(cpid2, NULL, 0);

	/* close the extra file descriptors */
	close(pfd[0]);
	close(pfd[1]);

	/* return true */
	return 1;
}

int main(int argc, char ** argv) {
	pid_t childPID;
	char*  lineRead;
	char** command;
	char** cursor;
	char*  input;
	char*  output;
	int infile, outfile;

	/* set the working directory */
	if (! updateWD()) {
		printf("Error adding CWD to the path");
		return 1;
	}

	while (1) {
		infile = outfile = 0;

		lineRead = trim(getCommand());
		command  = parseLine(lineRead);

		if (! lineRead) {
			/*TODO: handle command parse error */
			printf("Unknown command");
			continue;
		}

		/* command 'exit' exits the shell */
		if (strcmp("exit", lineRead) == 0)
			exit(EXIT_SUCCESS);

		/* first check for a pipe */
		cursor = command;
		while (cursor && *cursor && (strcmp(*cursor, "|") != 0))
			cursor++;

		if (*cursor) {
			cursor++;
			*(cursor-1) = 0;
			if (! execPipe(command, cursor)) {
				printf("Pipe failed\n");
			}
			continue;
		}



		/* check for redirections */
		cursor = command;
		while (cursor && *cursor && *cursor[0] != '>')
			cursor++;

		if (*cursor) {
			/* ">dest" format */
			if (strcmp(*cursor, ">") != 0) {
				output = (*cursor) + 1;
				printf("output to %s\n", output);

				/* TODO: remove ">dest" from array */
			}
			/*  "< dest" format */
			else {
				output = *(cursor + 1);
				printf("output to %s\n", output);
				/* TODO: remove "> dest" from array */
			}

			/* TODO: verify these are right flags */
			outfile = open(output, O_CREAT | O_WRONLY | O_TRUNC, 0777);
			dup2(outfile, 1);
		}


		cursor = command;
		while (cursor && *cursor && *cursor[0] != '<')
			cursor++;

		if (*cursor) {
			/* "<dest" format */
			if (strcmp(*cursor, "<") != 0) {
				input = (*cursor) + 1;
				printf("input to %s\n", input);

				/* TODO: remove "<src" from array */
			}
			/*  "< src" format */
			else {
				input = *(cursor + 1);
				printf("intput to %s\n", input);
				/* TODO: remove "< src" from array */
			}

			/* TODO: verify these are right flags */
			infile = open(output, O_RDONLY, 0777);
			dup2(infile, 0);
		}



		/* attempt to fork the process */
		if ((childPID = fork()) == -1){
			perror("fork");
		} else if (childPID == 0) {   /* this is the child */


			execvp(*command, command);

			/* if child returns then print out the error */
			perror("exec");
		} else {               /* this is the parent */
			/* TODO: do we need to customize handling of errors? */
			waitpid(childPID, NULL, 0);
		}

		/* reset the standard IO */
		if (infile)
			dup2(infile, 0);
		if (outfile)
			dup2(outfile, 1);
	}
}
