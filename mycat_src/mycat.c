#define _XOPEN_SOURCE   500

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LARGE_BUFF 4096

int cat(int);

int main(int argc, char** argv) {

	if (argc == 1)
		cat(0);

	else {
		int fd;
		for (size_t i = 1; i < argc; ++i) {
			struct stat file;

			// get file information
			if (stat(argv[i], &file) == -1) {
				perror("mycat");
				break;
			}

			// print error and skip if a directory
			if ((file.st_mode & S_IFMT) == S_IFDIR) {
				fprintf(stderr, "mycat: %s is a directory\n", argv[i]);
				continue;
			}

			if ((fd = open(argv[i], O_RDONLY)) == -1) {
				fprintf(stderr, "Could not open file %s", argv[i]);
				break;
			}
			if (cat(fd) < 0) {
				fprintf(stderr, "Error reading file: %s\n", argv[i]);
			}
			close(fd);
		}
	}

	return 0;
}

int cat(int fd) {
	char buf[LARGE_BUFF];
	size_t bytesRead = 0;

	while((bytesRead = read(fd, buf, LARGE_BUFF - 1)) > 0) {
		if (write(1, buf, bytesRead) < 0)
			perror("stdout");
	}

	return bytesRead;
}
