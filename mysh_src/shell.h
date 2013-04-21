#define STDIN              0
#define STDOUT             1
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
	// backup stdin and stdout
	int real_stdin  = dup(STDIN);
	int real_stdout = dup(STDOUT);

	// get reirections for the command
	char* input=0, *output=0, *error = 0;
	getRedirections(comm, &input, &output, &error);
fprintf(stderr, "\tinput: %s\n\toutput: %s\n\terror: %s\n", input, output, error);

	// TODO: perform the actual redirections, the three strings must be freed also

	char** command = malloc(sizeof(char) * (countDelims(comm, " ") + 1));
	splitLine(comm, command, ' ');


	// pipe to capture results
	int pfd[2];
	int pfd2[2];
	int status;
	int err;
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
		//close(status[0]);
		execvp(*command, command);

		// exec failed, cleanup
		fprintf(stderr, "exec: failed to execute \"%s\"\n", *command);
		dup2(real_stdin, STDIN);
		dup2(real_stdout, STDOUT);

		// return -1, _exit used instead of exit to ensure child does no
		// cleanup that should be done by the parent
		_exit(-1);
	}

	if (cpid > 0) {
		// wait for child and restore stdout
		waitpid(cpid, &status, 0);
		dup2(real_stdout, 1);

		// return if child failed
		if (status != 0)
			return -1;

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
