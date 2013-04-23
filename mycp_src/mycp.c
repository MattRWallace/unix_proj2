/*
Author: Trevor Mendenhall

Date: 4/12/13

Description: This program acomplishes the actions of the cp shell command.  The program expects two arguments in the form './mycp.exe a_file b_file' it will then copy the contense of the text file "a_file" to the file "b_file".  The program will create b_file if it has not yet been created.  The program will overwrite the contense of b_file if user promts to.  The command also will copy directories recursivly given the form './mycp.exe -R a_dir b_dir.

Return: The program will fail if a_file does not have read premission.  Also, if b_file does not have write premission, or an in appropriate number of arguments passed.

*/
#define _BSD_SOURCE

#include<dirent.h>
#include<sys/stat.h>
#include<stdio.h>
#include<errno.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<assert.h>
#include <err.h>



int cpdir(DIR * a, char * Apath, char * Bpath){

	int flag = 0;

	struct dirent* dp;

	FILE * fin;

	FILE * fout;

	char Apath2[256];

        char Bpath2[256];

	while((dp = readdir(a)) != NULL){

		strcpy(Apath2, Apath);

	        strcpy(Bpath2, Bpath);

		if(strcmp(".", dp->d_name) == 0 || strcmp("..", dp->d_name) == 0){

			continue;

		}


		if(dp->d_type == DT_DIR){

			strcat(Apath, dp->d_name);

			strcat(Bpath, dp->d_name);

			mkdir(Bpath, S_IRWXG | S_IRWXO | S_IRWXU);

			DIR * temp = opendir(Apath);

			strcat(Apath, "/");

			strcat(Bpath, "/");

			cpdir(temp, Apath, Bpath);

			strcpy(Apath, Apath2);

			strcpy(Bpath, Bpath2);

			flag = 1;
		}

		if(flag == 1){continue;}

		strcat(Apath2, dp->d_name);

                strcat(Bpath2, dp->d_name);

		int fin = open(Apath2, O_RDONLY);

		assert(fin >= 0);

		int fout = open(Bpath2, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); //permissions may be wrong

		assert(fout >= 0);

		char buf[8192];

		while (1) {

   			 ssize_t result = read(fin, &buf[0], sizeof(buf));

    			 if (!result) break;

    			 assert(result > 0);

   			 assert(write(fout, &buf[0], result) == result);

		}

		close(fin);

		close(fout);

	}

	return 0;

}

void usage() {
	printf("Usage mycp [-R] SOURCE DESTINATION\n");
	printf("  -R          copy directories recursively\n");
}


int main(int argc, char *argv[]){

	if(argc == 1){
		usage();
		/*
		printf("cp must be called in form 'mycp.exe a_file b_file'\n");
		printf("or form 'mycp.exe -R a_dir b_dir\n");
		*/

		return -1;
	}

	if(argc == 2){

		usage();

		/*
		printf("cp missing file argument, call in form 'mycp.exe a_file b_file'\n");
		printf("or form 'mycp.exe -R a_dir b_dir\n");
		*/

		return -1;
	}

	if(argc == 3){

		FILE * fp1;

		FILE * fp2;

		char overwrite[1]; //expects only one entry

		int myFlag = 0;

		char no = 'n';

		FILE * file = fopen(argv[2], "r");

		if (file != NULL){

			fclose(file);

			 myFlag = 1;
    		}
		else{ myFlag = 0; }


		if(myFlag){

			printf("mycp: overwrite '%s'?", argv[2]);
			//printf("overwrite '%s', y or n:\n", argv[2]);

			scanf("%c", overwrite);

			if(no == overwrite[0]){return 0;}

		}
		if(fp1 = fopen(argv[1], "r")){

			if(fp2 = fopen(argv[2], "w+")){

				char line[1100];

				while(fgets(line, sizeof(line), fp1) != 0){

					fputs(line, fp2);
				}

				fclose(fp1);

				fclose(fp2);

				return 0;

			}
			else{

				err(-1, "cannot open %s for reading", argv[2]);

				/*
				printf("'%s' exists and does not allow write privledge\n", argv[2]);

				return -1;
				*/

			}

		}
		else if(EACCES){
			err(-1, "cannot open %s for reading", argv[2]);

			/*
			printf("'%s' does not have read privledge\n", argv[1]);

			return -1;
			*/

		}
		else{
			err(-1, "error");

			/*
			printf("'%s'_file does not exist\n", argv[1]);

			return -1;
			*/
		}
	}

	if(argc == 4){

		char option[] = "-R";

		if(strcmp(option, argv[1]) == 0){

			char Apath[256];

			char Bpath[256];

			getcwd(Apath, 255);

			getcwd(Bpath, 255);

			strcat(Apath, "/");

			strcat(Bpath, "/");

			strcat(Apath, argv[2]);

			strcat (Bpath, argv[3]);

			strcat(Apath, "/");

			strcat(Bpath, "/");

			DIR * dp1;

			DIR * dp2;

			if(dp1 = opendir(argv[2])){

			    if(dp2 = opendir(argv[3])){

					int cp = cpdir(dp1, Apath, Bpath);

					if(cp != 0){ fputs ("cpdir failed, directories were opened & not copied", stderr); return -1;}

					return 0;
				}

				 if(ENOENT){    //dirb doesnt exist

					mkdir(argv[3], S_IRWXG | S_IRWXO | S_IRWXU);

					dp2 = opendir(argv[3]);

					int cp2 = cpdir(dp1, Apath, Bpath);

					if(cp2 != 0){ fputs ("cpdir failed", stderr); return -1;}

					return 0;
				}


				if(EACCES){
					err(-1, "error");

					/*
					printf("%s exists and does not have apropriate privledges\n", argv[3]);

					return -1;
					*/


				}
			}

			else{

				err(-1, "error");

				/*
				fputs ("a_directory could not be opened", stderr);

				return -1;
				*/

			}

		}
		else{

			printf("mycp: invalid option -- %s", argv[1]);
			//printf("mycp does not accenpt %s as an option\n", argv[1]);

			return -1;
		}
	}

	if(argc > 4){

		usage();
		//printf("mycp does not accenpt %d arguments\n", argc);

		return -1;
	}

	return 0;
}
