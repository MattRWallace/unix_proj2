#define STDIN              0
#define STDOUT             1
#define STDERR             2
#define UMASK              0600
#define LARGE_BUF          4096

int updateWD();
char* getPrompt(char**);
char* getCommand(char**);
int exec(char*, char**, char*);
void getRedirections(char*, char**, char**, char**);
int processPipes(char*);
char* resolveSubshells(const char*);

typedef enum {
	IN, OUT, ERR
} redirect;

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

int exec(char* comm, char** result, char* inputStr) {
	int returnFlag = 0;  // hacky solution to pipe after an output redirect
	                     // but not print results after and output

	// backup stdin and stdout
	int real_stdin  = dup(STDIN);
	int real_stdout = dup(STDOUT);
	int real_stderr = dup(STDERR);

	// get reirections for the command
	char* input=0, *output=0, *error = 0;
	getRedirections(comm, &input, &output, &error);

	char** command = malloc(sizeof(char) * (countDelims(comm, " ") + 1));
	splitLine(comm, command, ' ');

	// pipe to capture results
	int pfd[2];
	int pfd2[2];
	int status;
	pid_t cpid;

	if (pipe(pfd) == -1) {
		perror("exec: pipe");
		return -1;
	}

	cpid = fork();
	if (cpid == -1) {
		perror("exec: fork");
		return -1;
	}

	if (cpid == 0) {

		int inputFd = 0, outputFd = 0, errorFd = 0;

		// handle stderr
		if (error) {
			if ((errorFd = open(error, O_RDWR | O_CREAT, UMASK)) == -1) {
				perror("exec: stderr redirect");
			}
			dup2(errorFd, STDERR);
		}

		// Handle input
		if (inputStr || input) {
			if (pipe(pfd2) == -1) {
				perror("exec: pipe2");
				return -1;
			}

			// Incoming pipe, write to secondary pipe and connect to main pipe
			if (inputStr) {
				write(pfd2[1], inputStr, strlen(inputStr));    // TODO: add error checking to write
				dup2(pfd2[0], STDIN);
			}

			if (input) {
				if ((inputFd = open(input, O_RDONLY) == -1)) {
					perror("exec: input redirect");
				}
				// if incoming pipe, then tag on to the end of it
				if (inputStr) {
					int numRead = 0;
					char character[2];
					while ((numRead = read(inputFd, character, 1)) > 0)    //TODO: add error checking to read/write
						write(pfd2[1], character, 1);
				}
				// otherwise, just hook to stdin
				else
					dup2(inputFd, STDIN);
			}

			// close the input pipe
			close(pfd2[0]);
			close(pfd2[1]);

		} /*else {
			close(pfd[0]);
		}*/

		// free redirect strings if they were allocated
		if (input)
			free(input);
		if (error)
			free(error);

		dup2(pfd[1], STDOUT);
		close(pfd[0]);
		close(pfd[1]);

		execvp(*command, command);

		// exec failed
		perror("perror");
		fprintf(stderr, "\nexec: failed to execute \"%s\"\n", comm);

		// cleanup
		dup2(real_stdin, STDIN);
		dup2(real_stdout, STDOUT);
		dup2(real_stderr, STDERR);

		// return -1, _exit used instead of exit to ensure child does no
		// cleanup that should be done by the parent
		_exit(-1);
	}

	if (cpid > 0) {
		// close the write end of the pipe
		close(pfd[1]);

		// wait for child and return in child failed
		waitpid(cpid, &status, 0);
		if (WIFEXITED(status) &&  WEXITSTATUS(status) != 0) {
			return -1;
		}

		// restore stdout
		dup2(real_stdout, 1);

		// create the results buffer
		*result = malloc(sizeof(char) * (LARGE_BUF + 1));

		int bytesRead = read(pfd[0], *result, LARGE_BUF);
		if (bytesRead == LARGE_BUF) {
			fprintf(stderr, "Pipe overflow\n");
			return -1;
		}
		(*result)[bytesRead] = '\0';

		if (output) {
			int fd;
			if ((fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, UMASK)) == -1) {
				perror("exec: stdout redirect");
				fd = STDOUT;
			}
			write(fd, *result, strlen(*result));        // TODO: error checking

			free(output);
			returnFlag = 1;
		}

		//nl2space(*result);

		return returnFlag;
	}

	// close backup file descriptors
	close(real_stdin);
	close(real_stdout);
	close(real_stderr);
}

void getRedirections(char* command, char **input, char **output, char **error) {
	redirect selected;
	char *incur, *outcur, *errcur;
	char target[FILENAME_MAX];

	*input = 0;
	*output = 0;
	*error = 0;

	// while some redirection still exists
	incur  = strstr(command, "<");
	outcur = strstr(command, ">");
	errcur = strstr(command, "2>");

	while (incur || outcur|| errcur) {
		// find the closest to the start
		char* cursor = (void *)-1;

		if (incur != 0)
			cursor = incur;
		if (outcur != 0 && outcur < cursor)
			cursor = outcur;
		if (errcur != 0 && errcur < cursor)
			cursor = errcur;

		// set the redirect type
		if (cursor == incur)
			selected = IN;
		else if (cursor == outcur)
			selected = OUT;
		else
			selected = ERR;

		// mark start of delete
		char* mark1 = cursor;

		// grab the value
		cursor += (selected == ERR) ? 2 : 1;
		cursor += (*cursor == ' ') ? 1 : 0;
		sscanf(cursor, "%s", target);

		// store the value
		if (selected == IN) {
			if (*input)
				free(*input);
			*input = strdup(target);
		}
		else if (selected == OUT) {
			if (*output)
				free(*output);
			*output = strdup(target);
		}
		else if (selected == ERR) {
			if (*error)
				free(*error);
			*error = strdup(target);
		}

		cursor += strlen(target);
		memcpy(mark1, cursor, strlen(cursor)+1);

		incur  = strstr(command, "<");
		outcur = strstr(command, ">");
		errcur = strstr(command, "2>");
	}
}

int processPipes(char* lineRead) {
	char** pipes = malloc(sizeof(char) * (countDelims(lineRead, "|") + 1));
	splitLine(lineRead, pipes, '|');

	char* last   = 0;
	int   status = 0;

	for (int i = 0; pipes && pipes[i]; ++i) {
		pipes[i]       = resolveSubshells(pipes[i]);

		// check for an exit
		if (strcmp("exit", trim(pipes[i])) == 0)
			exit(EXIT_SUCCESS);

		if (i == 0) {              // first command
			status = exec(pipes[i], &last, 0);
		}
		else {                     // not first command
			char* temp = 0;
			status = exec(pipes[i], &temp, last);
			free(last);
			last = temp;
		}
		if (status == -1) {
			fprintf(stderr, "command not found: %s\n", pipes[i]);
			break;
		}
		if (! pipes[i+1] && status == 0)          // last command
			printf("%s", last);
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

		char*  subshellDump = 0;
		exec(subcmd, &subshellDump, 0);

		size_t tempSize = (originSize - subSize + strlen(subshellDump));
		char* temp = malloc(sizeof(char) * (tempSize + 1));
		sprintf(temp, "%s%s%s", newCommand, subshellDump, cursor);

		free(newCommand);
		newCommand = temp;
	}

	return newCommand;
}
