#define _BSD_SOURCE

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fts.h>
#include <pwd.h>
#include <grp.h>
#include <err.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <termios.h>
#include <math.h>
#include <string.h>

#include "myls.h"

static void (*print_func)(FINFO *);
static void traverse(char *argv[]);
static void display(FTSENT *list);
static int compare(const FTSENT **a, const FTSENT **b);

int termwidth = 80; /*Default terminal width*/
int options;
int level;

/*Flag*/
int column_flag; /*Column format flag*/

int main(int argc, char *argv[]){
    char dot[] = ".";
    char *curDir[] = {dot, NULL};
    struct winsize win;
    int opt;
    options = FTS_PHYSICAL;
    print_func = printCol;

    column_flag = 1;
    if(isatty(STDOUT_FILENO)){
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0 && win.ws_col > 0)
            termwidth = win.ws_col;
        column_flag = 1;
    }

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

    if(column_flag == 0) print_func = printSCol;
    if(argc)
        traverse(argv);
    else
        traverse(curDir);
    return 0;
}
static void traverse(char *argv[]){
    FTS *fp;
    FTSENT *list, *ptr, *cur;

    if((fp = fts_open(argv, options, compare)) == NULL)
       err(1, NULL);

    list = fts_children(fp, 0);
    level = 0;
    if(list->fts_info == FTS_F)
        display(list);

    if(column_flag == 1)
        options = FTS_NAMEONLY;
    else options = 0;

    level = 1;
    while((cur = fts_read(fp)) != NULL){
        if(cur->fts_name[0] == '.' && cur->fts_level != FTS_ROOTLEVEL)
            break;
        list = fts_children(fp,options);
        display(list);
        if(!(cur == NULL && errno != 0))
            (void)fts_set(fp, cur, FTS_SKIP);
        //if(cur->fts_info == FTS_ERR || cur->fts_info == FTS_DNR){
        if(errno)
            err(1,"");
    }



}

static void display(FTSENT *list){
    int entries, i, len, max_name_len, total_blocksize, max_file_size,
        max_hardlinks, max_user_name, max_group_name;
    struct stat *st;
    struct passwd *pw;
    struct group *grp;
    FINFO fi;
    FTS *fp;
    FTSENT *ptr;

    entries = 0;
    max_name_len = total_blocksize = max_hardlinks = max_file_size = max_user_name = max_group_name = 0;
    for(ptr = list, max_name_len =0; ptr != NULL; ptr = ptr->fts_link){
        if(ptr->fts_level != level) continue;
        if(ptr->fts_name[0] == '.') ptr->fts_number = 1;
        if(ptr->fts_name[0] != '.'){
            if(column_flag == 0){
                st = ptr->fts_statp;
                total_blocksize += st->st_blocks;
                if((len = ptr->fts_namelen) > max_name_len)
                    max_name_len = len;
                if((len = numLen((long int)st->st_nlink)) > max_hardlinks)
                    max_hardlinks = len;
                if((len = numLen((long int)st->st_size)) > max_file_size)
                    max_file_size = len;
                pw = getpwuid(st->st_uid);
                if((len = strlen(pw->pw_name)) > max_user_name)
                    max_user_name = len;
                grp = getgrgid(st->st_gid);
                if((len = strlen(grp->gr_name)) > max_group_name)
                    max_group_name = len;
                entries++;
            }
            else{
                if((len = ptr->fts_namelen) > max_name_len)
                    max_name_len = len;
            }
            entries++;
        }
    }
    if(entries == 0) return;
    fi.max_file_size = max_file_size;
    fi.max_hardlinks = max_hardlinks;
    fi.max_user_name = max_user_name;
    fi.max_group_name = max_group_name;
    fi.max_name_len = max_name_len;
    fi.total_blocksize = total_blocksize;
    fi.entries = entries;
    fi.list = list;
    print_func(&fi);
}

static int compare(const FTSENT **a, const FTSENT **b){
    return filter(*a, *b);
}





