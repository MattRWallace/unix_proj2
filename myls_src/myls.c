#define _BSD_SOURCE

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <dirent.h>
#include <err.h>
#include <unistd.h>
#include <termios.h>
#include <pwd.h>
#include <grp.h>
#include <locale.h>
#include <math.h>
#include <string.h>

#include "myls.h"


static void display(char *);

int termwidth = 80; /*Default terminal width*/

/*Flag*/
int column_flag; /*Column format flag*/

int main(int argc, char *argv[]){
    char *curDir = "./";
    struct winsize win;
    int opt;

    if(isatty(STDOUT_FILENO)){
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0 && win.ws_col > 0)
            termwidth = win.ws_col;
        column_flag = 1;
    }
    else column_flag = 0;

    while((opt = getopt(argc, argv, "l")) != -1){

        switch(opt){
            case 'l':
                column_flag = 0;
                break;
            case '?':
                usage();
                break;
            }
        }
    argc -= optind;
    argv += optind;
    if(argc)
        display(*argv);
    else
        display(curDir);
    return 0;
}

static void display(char *argv){
    int entries, i, len, max_name_len;
    char *path;
    struct dirent **ent;
    struct stat stat;

    lstat(argv, &stat);
    if(stat.st_mode & S_IFDIR){
        if((entries = scandir(argv, &ent, filter, alphasort)) == -1)
            err(EXIT_FAILURE, NULL);

        len = strlen(argv);
        if(argv[len] != '/'){
            path = (char *) malloc(len + 1);
            path[0] = '\0';
            for(i = 0; i < len; i++)
                path[i] = argv[i];
            path[len] = '/';
            len++;
        }
        else{
            path = argv;
        }
    }

    if(column_flag){
        for(i = 0; i < entries; ++i){
            if((len=strlen(ent[i]->d_name)) > max_name_len)
                max_name_len = len;
        }
        printCol(ent, entries, max_name_len);
    }
    else{
        FINFO fsi;
        int max, blocksize = 0;
        char* fullpath;
        struct stat *st;
        struct passwd *pw;
        struct group *grp;
        fsi.max_hardlinks = 0;
        fsi.max_file_size = 0;
        fsi.max_user_name = 0;
        fsi.max_group_name = 0;
        st = (struct stat*) malloc(entries * sizeof(struct stat));
        for(i = 0; i < entries; ++i){
            fullpath = (char *) malloc(len + strlen(ent[i]->d_name) + 1);
            fullpath[0] = '\0';
            strcat(fullpath, path);
            strcat(fullpath, ent[i]->d_name);
            lstat(fullpath, &st[i]);
            if((max = numLen((long int)st[i].st_nlink)) > fsi.max_hardlinks)
                fsi.max_hardlinks = max;
            if((max = numLen((long int)st[i].st_size)) > fsi.max_file_size)
                fsi.max_file_size = max;
            pw = getpwuid(st[i].st_uid);
            if((max = strlen(pw->pw_name)) > fsi.max_user_name)
                fsi.max_user_name = max;
            grp = getgrgid(st[i].st_gid);
            if((max = strlen(grp->gr_name)) > fsi.max_group_name)
                fsi.max_group_name = max;
            blocksize += st[i].st_blocks;
        }
        fsi.ent = ent;
        fsi.stat = st;
        fsi.total_blocksize = blocksize;
        printSCol(fsi, entries);
        free(st);
        free(path);
        free(fullpath);
    }
}






