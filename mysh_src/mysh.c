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

/*
 * Takes a char* as an argument and allocates a character
 * array with the text of the prompt.  The string must be
 * freed by the user.
 */
void getPrompt(char* prompt) {
    char cwd[FILENAME_MAX] = "";
    if (!getcwd(cwd, sizeof(cwd))) {
        strcpy(cwd, "?");
    }

    sprintf(prompt, "%s$ ", cwd);
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

/* count the spaces in the command */
int countWords(char* string) {
	int count = 0;
    while (*string) {
        if ( ' ' == *string)
            count++;
        string++;
    }
	return count;
}

/* remove all spaces from a cstring */
void replaceSpaces(char* string) {
	while (*string != '\0') {
		if (*string == '\n') {
			*string = ' ';
			++string;
		} else {
			++string;
		}
	}
}

/* trim whitepsace from string */
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

/*
 * retrieves a command from the user
 */
char* getCommand(char** command) {
    char* prompt = malloc(sizeof(char) * FILENAME_MAX);
	getPrompt(prompt);
    *command = readline(prompt);
    free(prompt);

    /* if non-empty add to history */
    if (command &&  *command)
        add_history(*command);

    return *command;
}

/*
 * Tokenize the command string
 */
void parseLine(char* line, char** result) {
    if (result) {
        size_t idx = 0;
        char* token = strtok(line, WHITESPACE);

        while (token) {
            *(result + idx++) = strdup(token);
            token = strtok(0, " ");
        }
        *(result + idx) = 0;          /* add terminating null */
    }
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
    /*TODO: waitpid(cpid1, NULL, 0);*/
    waitpid(cpid2, NULL, 0);

    /* close the extra file descriptors */
    close(pfd[0]);
    close(pfd[1]);

    /* return true */
    return 1;
}

int execRedirect(char** command, char* input, char* output) {
    /* file descriptors for input and output files */
    int infile, outfile;

    /* backup original stdin and stdout */
    int real_stdin = dup(STDIN);
    int real_stdout = dup(STDOUT);

    /* child PID */
    pid_t childPID;

    /* check and open each file if needed */
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
        dup2(real_stdin, STDIN);
        close(infile);
    }

    if (output) {
        dup2(real_stdout, STDOUT);
        close(outfile);
    }

    /* close the backups */
    close(real_stdin);
    close(real_stdout);

    return 1;
}

int main(int argc, char ** argv) {
    /* backup original stdin and stdout */
    int real_stdin  = dup(STDIN);
    int real_stdout = dup(STDOUT);

    /* set the working directory */
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
            parseLine(line, command1);

			char** command2 = malloc(sizeof(char)*(countWords(cursor) + 1));
            parseLine(cursor, command2);
            if (! execPipe(command1, command2))
                printf("Pipe failed\n");
            free(command1);
            free(command2);
            continue;
        }

        /* check for output redirection */
        if (cursor = strstr(line, ">")) {
            char* end = cursor;
            end += (*(cursor+1) == ' ') ? 2 : 1;
            sscanf(end, "%s", output);
            end += strlen(output);

            memmove(cursor, end, strlen(end)+1);
        }

        /* check for input redirection */
        if (cursor = strstr(line, "<")) {
            char* end = cursor;
            end += (*(cursor+1) == ' ') ? 2 : 1;
            sscanf(end, "%s", input);
            end += strlen(input);

            memmove(cursor, end, strlen(end)+1);
        }

        /* if either redirection exists */
        if (strlen(input) || strlen(output)) {
            char** command = malloc(sizeof(char) * countWords(line));
            parseLine(line, command);
            execRedirect(command, input, output);
            continue;
        }

        /* check for subshell */
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
int size = countWords(subcmd);
                parseLine(subcmd, command);
for( int i = 0; i < size; ++i)
	fprintf(stderr, "%s ", *command);
fprintf(stderr, "\n\n");
fflush(stderr);
exit(EXIT_SUCCESS);
                //execvp(*command, command);
            } else {
				close(pfd[1]);
			}

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
				parseLine(newCmd, command);
				execvp(*command, command);
			}

            continue;
        }

        pid_t childPID;
        /* attempt to fork the process */
        if ((childPID = fork()) == -1){
            perror("fork");
        } else if (childPID == 0) {   /* this is the child */
            char** command = malloc(sizeof(char) * countWords(line));
            parseLine(line, command);
            execvp(*command, command);

            /* if child returns then print out the error */
            perror("exec");
        } else {               /* this is the parent */
            /* TODO: do we need to customize handling of errors? */
            waitpid(childPID, NULL, 0);
        }

		/* close the backups */
		close(real_stdin);
		close(real_stdout);

		free(lineRead);

    }
}
