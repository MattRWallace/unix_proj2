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

#define UMASK      0600
#define STDIN      0
#define STDOUT     1
#define WHITESPACE "\x20\x09\x0a\x0b\x0c\x0d"
#define LARGE_BUF  4096

// prototypes

void nl2space(char*);
int updateWD();
char* getPrompt(char**);
char* getCommand(char**);
int countDelims(char*, char*);
void splitLine(char*, char**, const char);
void exec(char**, char**);
void processPipes(char*);
char* resolveSubshells(const char*);

/*
// count the spaces in the command
int countWords(char* string) {
	int count = 0;
    while (*string) {
        if ( ' ' == *string)
            count++;
        string++;
    }
	return count;
}

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

    // wait for processes to exit
    //TODO: waitpid(cpid1, NULL, 0);
    waitpid(cpid2, NULL, 0);

    // close the extra file descriptors
    close(pfd[0]);
    close(pfd[1]);

    // return true
    return 1;
}

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

/* ===================================================================================================*/

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
            *(result + idx++) = strdup(token);
            token = strtok(0, " ");
        }
        *(result + idx) = 0;          /* add terminating null */
    }
}

void exec(char** command, char** result) {
	// backup stdin and stdout
	int real_stdin  = dup(STDIN);
	int real_stdout = dup(STDOUT);

	// pipe to capture results
	int pfd[2];
	pid_t cpid;

	if (pipe(pfd) == -1) {
		perror("exec: pipe");
		return;
	}

	cpid = fork();
	if (cpid == -1) {
		perror("exec: fork");
		return;
	}

	if (cpid == 0) {
		dup2(pfd[1], 1);
		close(pfd[0]);
		close(pfd[1]);
		execvp(*command, command);
	}

	if (cpid > 0) {
		// wait for child and restore stdout
		waitpid(cpid, 0, 0);
		dup2(real_stdout, 1);

		// create the results buffer
		*result = malloc(sizeof(char) * (LARGE_BUF + 1));
		char test[2];


		int bytesRead = read(pfd[0], *result, LARGE_BUF);
		if (bytesRead == LARGE_BUF) {
			fprintf(stderr, "Pipe overflow\n");
			return;
		}
		(*result)[bytesRead] = '\0';

		nl2space(*result);
	}

	// close backup file descriptors
	close(real_stdin);
	close(real_stdout);
}

void processPipes(char* lineRead) {
	char** pipes = malloc(sizeof(char) * (countDelims(lineRead, "|") + 1));
	splitLine(lineRead, pipes, '|');

	// resolve all the subshells
	for (int i = 0; pipes && *(pipes + i); ++i) {
printf("%s ", *(pipes + i));
		int last = (*(pipes + i + 1)) ? 0 : 1;
		*(pipes + i) = resolveSubshells(*(pipes + i));

printf("%s ", *(pipes + i));

	}
	printf("\n");
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

		exec(parsed, &subshellDump);

		size_t tempSize = (originSize - subSize + strlen(subshellDump));
		char* temp = malloc(sizeof(char) * (tempSize + 1));
		sprintf(temp, "%s%s%s", newCommand, subshellDump, cursor);

		free(newCommand);
		newCommand = temp;
	}
}

int main(int argc, char** argv) {
	if (! updateWD()) {
		fprintf(stderr, "Error adding CWD to the path");
		return 127;
	}

	// handle a one-off execution
	if (argc > 1) {
		char* result = 0;
		exec(argv+1, &result);

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
	}



}


/* ====================================================================================*/

/*
int oldmain(int argc, char ** argv) {
    // backup original stdin and stdout
    int real_stdin  = dup(STDIN);
    int real_stdout = dup(STDOUT);

    // set the working directory
    if (! updateWD()) {
        printf("Error adding CWD to the path");
        return 1;
    }

    while (1) {
        char* lineRead;
        char* line;
        char* cursor;
        char  input[FILENAME_MAX]  = { 0 };
        char  output[FILENAME_MAX] = { 0 };

        // get the line
        line = trim(getCommand(&lineRead));

        if (! line)   // blank command, do nothing
            continue;

        // command 'exit' exits the shell
        if (strcmp("exit", line) == 0) {
            free(lineRead);
            exit(EXIT_SUCCESS);
        }

        // first check for a pipe
        if (cursor = strstr(line, "|")) {
            cursor++;
            *(cursor - 1) = 0;


			char** command1 = malloc(sizeof(char)*(countWords(line) + 1));
            splitLine(line, command1, WHITESPACE);

			char** command2 = malloc(sizeof(char)*(countWords(cursor) + 1));
            splitLine(cursor, command2, WHITESPACE);
            if (! execPipe(command1, command2))
                printf("Pipe failed\n");
            free(command1);
            free(command2);
            continue;
        }

        // check for output redirection
        if (cursor = strstr(line, ">")) {
            char* end = cursor;
            end += (*(cursor+1) == ' ') ? 2 : 1;
            sscanf(end, "%s", output);
            end += strlen(output);

            memmove(cursor, end, strlen(end)+1);
        }

        // check for input redirection
        if (cursor = strstr(line, "<")) {
            char* end = cursor;
            end += (*(cursor+1) == ' ') ? 2 : 1;
            sscanf(end, "%s", input);
            end += strlen(input);

            memmove(cursor, end, strlen(end)+1);
        }

        // if either redirection exists
        if (strlen(input) || strlen(output)) {
            char** command = malloc(sizeof(char) * countWords(line));
            splitLine(line, command, WHITESPACE);
            execRedirect(command, input, output);
            continue;
        }

        // check for subshell
        if (cursor = strstr(line, "$(")) {
            char* subcmd = cursor+2;
            *cursor = '\0';

            cursor = strstr(subcmd, ")");
            if (! cursor) {
                fprintf(stderr, "error: no termination of subshell\n");
                continue;
            }

            cursor += 1;
            *(cursor - 1) = '\0';

            int pfd[2];
            pid_t cpid;

            if(pipe(pfd) == -1) {
                perror("pipe (subshell)");
                continue;
            }

            cpid = fork();
            if (cpid == -1) {
                perror("fork (subshell)");
                continue;
            }

			if (cpid == 0) {    // child process
                dup2(pfd[1], 1);
                close(pfd[0]);
                close(pfd[1]);
                char** command = malloc(sizeof(char) * countWords(subcmd));
                splitLine(subcmd, command);
                execvp(*command, command);
            } else {
				close(pfd[1]);
				waitpid(cpid, NULL, 0);
			}

			dup2(real_stdout, 1);

			char* subshellDump = malloc(sizeof(char) * LARGE_BUF);
			char  test[2];

			read(pfd[0], subshellDump, LARGE_BUF - 1);
			if (read(pfd[0], test, 1) > 0) {
				fprintf(stderr, "subshell overflow\n");
				continue;
			}
			replaceSpaces(subshellDump);

			cpid = fork();
            if (cpid == -1) {
                perror("fork (subshell)");
                continue;
            }

			if (cpid == 0) {

				int size = strlen(line) + strlen(subshellDump) + strlen(cursor) + 3;
				char* newCmd = malloc(sizeof(char) * size);
				sprintf(newCmd, "%s %s %s", line, subshellDump, cursor);

				char** command = malloc(sizeof(char) * countWords(newCmd));
				splitLine(newCmd, command);
				execvp(*command, command);
            } else {
				waitpid(cpid, NULL, 0);
			}

            continue;
        }

        pid_t childPID;
        // attempt to fork the process
        if ((childPID = fork()) == -1){
            perror("fork");
        } else if (childPID == 0) {   // this is the child
            char** command = malloc(sizeof(char) * countWords(line));
            splitLine(line, command, WHITESPACE);
            execvp(*command, command);

            // if child returns then print out the error
            perror("exec");
        } else {               // this is the parent
            // TODO: do we need to customize handling of errors?
            waitpid(childPID, NULL, 0);
        }

		// close the backups
		close(real_stdin);
		close(real_stdout);

		free(lineRead);

    }
}

*/
