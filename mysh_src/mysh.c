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

#define UMASK      0600
#define STDIN      0
#define STDOUT     1
#define WHITESPACE "\x20\x09\x0a\x0b\x0c\x0d"
#define LARGE_BUF  4096

// prototypes

char* trim(char*);
void nl2space(char*);
int updateWD();
char* getPrompt(char**);
char* getCommand(char**);
int countDelims(char*, char*);
void splitLine(char*, char**, const char);
int exec(char**, char**, char*);
int processPipes(char*);
char* resolveSubshells(const char*);

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


// trim whitepsace from string
char* trim(char* string) {
    // bail if NULL
    if (! string)
        return NULL;

    while(isspace(*string))
        string++;

    if(*string != 0) {
        char* end = string + strlen(string) - 1;
        while (end > string && isspace(*end))
            end--;
        *(end+1) = '\0';
    }

    return string;
}

/* remove all spaces from a cstring */
void nl2space(char* string) {
	while (*string != '\0') {
		if (*string == '\n') {
			*string = ' ';
			++string;
		} else {
			++string;
		}
	}
}

/* adds the current working directory to the path so that we can find our
   custom executables */
int updateWD() {
    char cwd[FILENAME_MAX];
    const char* path = getenv("PATH");
    int size;
    char* newPath;

    if (! getcwd(cwd, sizeof(cwd))) {
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
	newPath = 0;

    return 1;
}

/*
 * Takes a char* as an argument and allocates a character
 * array with the text of the prompt.  The string must be
 * freed by the user.
 */
char* getPrompt(char** prompt) {
	char cwd[FILENAME_MAX] = "";
	if (!getcwd(cwd, sizeof(cwd)))
		strcpy(cwd, "?");

	*prompt = malloc(sizeof(char) * (strlen(cwd) + 3));
	sprintf(*prompt, "%s$ ", cwd);

	return *prompt;
}

/*
 * retrieves a command from the user
 */
char* getCommand(char** command) {
    char* prompt;
    *command = readline(getPrompt(&prompt));
    free(prompt);
	prompt = 0;

    /* if non-empty add to history */
    if (command &&  *command)
        add_history(*command);

    return *command;
}

int countDelims(char* string, char* delim) {
	size_t count = 0;
	for (char* cursor = delim; *delim; ++delim) {
		while (*string) {
			if (*string == *cursor)
				count++;
			string++;
		}
	}
	return count;
}

/*
 * Tokenize the command string
 */
void splitLine(char* line, char** result, const char delim) {
	char delims[2] = { 0 };
	delims[0] = delim;

    if (result) {
        size_t idx = 0;
        char* token = strtok(line, delims);

        while (token) {
			result[idx++] = strdup(token);
            token = strtok(0, delims);
        }
		result[idx] = 0;              /* add terminating null */
    }
}

int exec(char** command, char** result, char* inputStr) {
	// backup stdin and stdout
	int real_stdin  = dup(STDIN);
	int real_stdout = dup(STDOUT);

	// pipe to capture results
	int pfd[2];
	int pfd2[2];
	int status[2];  // communication between child and parent
	int err;
	pid_t cpid;

	if (pipe(pfd) == -1) {
		perror("exec: pipe");
		return -1;
	}

	if (pipe(status) == -1) {
		perror("exec: pipe");
		return -1;
	}
	if (fcntl(status[1], F_SETFD, fcntl(status[1], F_GETFD) | FD_CLOEXEC)) {
		perror("fcntl");
		return -1;
	}

	cpid = fork();
	if (cpid == -1) {
		perror("exec: fork");
		return -1;
	}

	if (cpid == 0) {
		// If input provided, write to secondary pipe and connect to main pipe
		if (inputStr) {
			if (pipe(pfd2) == -1) {
				perror("exec: pipe2");
				return -1;
			}
			write(pfd2[1], inputStr, strlen(inputStr));
			dup2(pfd2[0], STDIN);
		} else {
			close(pfd2[0]);
		}
		close(pfd2[1]);
		dup2(pfd[1], STDOUT);
		close(pfd[0]);
		close(pfd[1]);
		close(status[0]);
		execvp(*command, command);

		// exec failed
		fprintf(stderr, "exec: failed to execute \"%s\"\n", *command);
		write(status[1], &errno, sizeof(int));
		dup2(real_stdin, STDIN);
		dup2(real_stdout, STDOUT);
		_exit(-1);
	}

	if (cpid > 0) {

		// check for a failure in the child
		close(status[1]);
		while (read(status[0], &err, sizeof(errno)) > 0) {
			return -1;
		}

		// wait for child and restore stdout
		int status = waitpid(cpid, 0, 0);
		dup2(real_stdout, 1);

		// create the results buffer
		*result = malloc(sizeof(char) * (LARGE_BUF + 1));
		char test[2];

		int bytesRead = read(pfd[0], *result, LARGE_BUF);
		if (bytesRead == LARGE_BUF) {
			fprintf(stderr, "Pipe overflow\n");
			return -1;
		}
		(*result)[bytesRead] = '\0';

		//nl2space(*result);
	}

	// close backup file descriptors
	close(real_stdin);
	close(real_stdout);
}

void getRedirections(char* command) {
	char *incur, *outcur;
	char input[FILENAME_MAX]  = "stdin";
	char output[FILENAME_MAX] = "stdout";

	// while some redirection still exists
	while ((incur = strstr(command, "<")) || (outcur = strstr(command, ">"))) {


	}
}

int processPipes(char* lineRead) {
	char** pipes = malloc(sizeof(char) * (countDelims(lineRead, "|") + 1));
	splitLine(lineRead, pipes, '|');

	char* last   = 0;
	int   status = 0;

	for (int i = 0; pipes && pipes[i]; ++i) {
		pipes[i]       = resolveSubshells(pipes[i]);
		// TODO: handle redirections here
		char** command = malloc(sizeof(char) * (countDelims(pipes[i], " ") + 1));
		splitLine(pipes[i], command, ' ');

		// check for an exit
		if (strcmp("exit", trim(pipes[i])) == 0)
			exit(EXIT_SUCCESS);

		if (i == 0) {              // first command
			status = exec(command, &last, 0);
		}
		else {                     // not first command
			char* temp = 0;
			status = exec(command, &temp, last);
			free(last);
			last = temp;
		}
		if (status == -1)
			break;
		if (! pipes[i+1])          // last command
			printf("%s\n", last);
	}
	free(last);
}

char* resolveSubshells(const char* line) {
	size_t originSize = strlen(line);
	char* newCommand = malloc(sizeof(char) * (originSize + 1));
	strcpy(newCommand, line);

	char* cursor;
	while(cursor = strstr(newCommand, "$(")) {
		char* subcmd = cursor + 2;
		*cursor = '\0';

		if(!(cursor = strstr(subcmd, ")"))) {
			fprintf(stderr, "error: non-terminated subshell");
			return 0;
		}

		cursor += 1;
		*(cursor - 1) = '\0';

		size_t subSize = strlen(subcmd);

		char** parsed = malloc(sizeof(char) * (countDelims(subcmd, WHITESPACE) + 1));
		splitLine(subcmd, parsed, ' ');
		char*  subshellDump = 0;

		exec(parsed, &subshellDump, 0);

		size_t tempSize = (originSize - subSize + strlen(subshellDump));
		char* temp = malloc(sizeof(char) * (tempSize + 1));
		sprintf(temp, "%s%s%s", newCommand, subshellDump, cursor);

		free(newCommand);
		newCommand = temp;
	}

	return newCommand;
}

int main(int argc, char** argv) {
	if (! updateWD()) {
		fprintf(stderr, "Error adding CWD to the path");
		return 127;
	}

	// handle a one-off execution
	if (argc > 1) {
		char* result = 0;
		exec(argv+1, &result, 0);

		if (result) {
			fprintf(stdout, "%s\n", result);
			exit(EXIT_SUCCESS);
		}
		else
			exit(EXIT_FAILURE);
	}

	while(1) {            // main terminal loop
		char* lineRead;
		getCommand(&lineRead);
		processPipes(lineRead);


		free(lineRead);
		lineRead = 0;
	}
}
