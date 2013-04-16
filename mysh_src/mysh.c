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

#define UMASK      0777
#define STDIN      0
#define STDOUT     1
#define WHITESPACE "\x20\x09\x0a\x0b\x0c\x0d"

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
		char* token = strtok(line, WHITESPACE);

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

/* TODO: add error checking to this function */
int execRedirect(char** command, char* input, char* output) {
	/* file descriptors for input and output files */
	int infile, outfile;

	/* backup original stdin and stdout */
	int real_stdin = dup(0);
	int real_stdout = dup(1);

	/* child PID */
	pid_t childPID;


	/* check and open each file if needed */
	if (input) {
		infile = open(input, O_RDONLY, UMASK);
		dup2(infile, 0);
	}

	if (output) {
		outfile = open(output, O_CREAT | O_WRONLY | O_TRUNC, UMASK);
		dup2(outfile, 1);
	}

	/* exec */
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

	/* redirect IO and close the files */
	if (input) {
		dup2(real_stdin, 0);
		close(infile);
	}

	if (output) {
		dup2(real_stdout, 1);
		close(outfile);
	}

	/* close the backups */
	close(real_stdin);
	close(real_stdout);

	return 1;
}

int main(int argc, char ** argv) {
	/* set the working directory */
	if (! updateWD()) {
		printf("Error adding CWD to the path");
		return 1;
	}



	while (1) {
		char* lineRead;
		char* cursor;
		char  input[FILENAME_MAX]  = { 0 };
		char  output[FILENAME_MAX] = { 0 };

		// get the line
		lineRead = trim(getCommand());

		if (! lineRead)   // blank command, do nothing
			continue;

		// command 'exit' exits the shell
		if (strcmp("exit", lineRead) == 0)
			exit(EXIT_SUCCESS);

		// first check for a pipe

		cursor = strstr(lineRead, "|");

		if (cursor) {
			cursor++;
			*(cursor - 1) = 0;
			if (! execPipe(parseLine(lineRead), parseLine(cursor)))
				printf("Pipe failed\n");
			continue;
		}

		/* check for redirections */
		cursor = strstr(lineRead, ">");
		if (cursor) {
			char* end = cursor;
			end += (*(cursor+1) == ' ') ? 2 : 1;
			sscanf(end, "%s", output);
			end += strlen(output);

			memmove(cursor, end, strlen(end)+1);
		}

		cursor = strstr(lineRead, "<");
		if (cursor) {
			char* end = cursor;
			end += (*(cursor+1) == ' ') ? 2 : 1;
			sscanf(end, "%s", input);
			end += strlen(input);

			memmove(cursor, end, strlen(end)+1);
		}

		if (strlen(input) || strlen(output)) {
			execRedirect(parseLine(lineRead), input, output);
			continue;
		}

		pid_t childPID;
		/* attempt to fork the process */
		if ((childPID = fork()) == -1){
			perror("fork");
		} else if (childPID == 0) {   /* this is the child */
			char** command = parseLine(lineRead);
			execvp(*command, command);

			/* if child returns then print out the error */
			perror("exec");
		} else {               /* this is the parent */
			/* TODO: do we need to customize handling of errors? */
			waitpid(childPID, NULL, 0);
		}
	}
}
