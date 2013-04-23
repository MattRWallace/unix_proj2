#define _POSIX_C_SOURCE   200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

int main(int argc, char* argv[])
{
	FILE* fp;
	struct stat fileStat;
	int i;
	int c;

	if (argc == 1)
	{
		size_t size = 256;
		char* str = (char*) malloc(size);
		int count;
		while (true)
		{
			count = getline(&str, &size, stdin);
			if (count == -1)
				break;
			printf("%s", str);
		}
		free(str);
		return EXIT_SUCCESS;
	}

	for (i = 1; i < argc; i++)
	{
		if(stat(argv[i], &fileStat) < 0)
		{
			printf("mycat: %s: The file failed to open\n", argv[i]);
			continue;
		}

		if (S_ISDIR(fileStat.st_mode))
		{
			printf("mycat: %s: The file is a directory\n", argv[i]);
			continue;
		}

		fp = fopen(argv[i], "r");
		if (fp == NULL)
		{
			printf("mycat: %s: No such file or directory.\n", argv[i]);
			continue;
		}

		while (c = fgetc(fp))
		{
			if (c == EOF)
				break;
			printf("%c", c);
		}


		fclose(fp);
	}


	return EXIT_SUCCESS;
}
