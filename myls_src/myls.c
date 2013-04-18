#include<tzfile.h>
#include<fts.h>
#include<err.h>



#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <getopt.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <pwd.h>
#include <grp.h>
#include <locale.h>
#include <time.h>
#include <math.h>
#include <string.h>

struct fsInfo{
     int total_size;
     long total_blocksize;
     int max_file_size;
     int max_hardlinks;
     int max_user_name;
     int max_group_name;
 };
int numLen(long int y){

    int i = 0;
    long int x;
    x = y;

    if(x >= 100000000){
        i += 8;
        x /= 100000000;
    }
    if(x >= 1000000){
        i += 6;
        x /= 1000000;
    }
    if(x >= 10000){
        i += 4;
        x /= 10000;
    }
    if(x >= 1000){
        i += 3;
        x /= 1000;
    }
    if(x >= 100){
        i += 2;
        x /= 100;
    }
    if(x >= 1){
        i++;;
    }
    return i;
}
char const * sperm(mode_t mode);
static void printtime(time_t ftime)
{
	int i;
	char *longstring;

	longstring = ctime(&ftime);
	for (i = 4; i < 11; ++i)
		(void)putchar(longstring[i]);

#define	SIXMONTHS	((365 / 2) * (24 * 60 * 60))
	if (ftime + SIXMONTHS > time(NULL))
		for (i = 11; i < 16; ++i)
			(void)putchar(longstring[i]);
	else {
		(void)putchar(' ');
		for (i = 20; i < 24; ++i)
			(void)putchar(longstring[i]);
	}
	(void)putchar(' ');
}
int filter(const struct dirent *d){
    if(*(d->d_name) == '.')
        return 0;
    else return 1;

}
void display(char *);
void printSCol(struct dirent **, struct stat *, struct fsInfo fsi, int entries);
void printCol(struct dirent **, int, int);

long blocksize;
int termwidth = 80; /*Default terminal width*/
/*Flags*/
int column_flag; /*Column format flag*/

int main(int argc, char *argv[]){
    char *curDir = "./";
    struct winsize win;
    int opt;
    extern int optind;

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


void display(char *argv){
    struct dirent **ent;
    int entries, i;

    entries = scandir(argv, &ent, filter, alphasort);

    if(column_flag){
        int len, max_name_len = 0;
        for(i = 0; i < entries; ++i){

            if((len=strlen(ent[i]->d_name)) > max_name_len)
                max_name_len = len;
        }
        printCol(ent, entries, max_name_len);
    }
    else{
        int max;
        int buf[4];
        struct stat *st;
        struct fsInfo fsi;
        struct passwd *pw;
        struct group *grp;
        (void)getbsize(buf, &blocksize);
        st = (struct stat*)malloc(entries * sizeof(struct stat));
        fsi.max_file_size = 0;
        fsi.max_hardlinks = 0;
        fsi.max_user_name = 0;
        fsi.max_group_name = 0;
        fsi.total_size = 0;
        fsi.total_blocksize = blocksize;
        for(i = 0; i < entries; ++i){
            lstat(ent[i]->d_name, &st[i]);
            fsi.total_size += st[i].st_size;
            pw = getpwuid(st[i].st_uid);
            grp = getgrgid(st[i].st_gid);
            if((max = st[i].st_nlink) > fsi.max_hardlinks)
                fsi.max_hardlinks = max;
            if((max = numLen((long int)st[i].st_size)) > fsi.max_file_size)
                fsi.max_file_size = max;
            if((max = strlen(pw->pw_name)) > fsi.max_user_name)
                fsi.max_user_name = max;
            if((max = strlen(grp->gr_name)) > fsi.max_group_name)
                fsi.max_group_name = max;
        }
        printSCol(ent, st, fsi, entries);
        free(st);
    }
}

void printSCol(struct dirent **ent, struct stat *st, struct fsInfo fsi, int entries){
    int row;
    struct passwd *pw;
    struct group *grp;

    printf("total %ld\n", fsi.total_blocksize);
    printf("total %d\n", fsi.total_size);
    for(row = 0; row < entries; row++){
        printf("%-10.10s ", sperm(st[row].st_mode));
        printf("%*d ", fsi.max_hardlinks, st[row].st_nlink);
        pw = getpwuid(st[row].st_uid);
        printf("%*s  ", fsi.max_user_name, pw->pw_name);
        grp = getgrgid(st[row].st_gid);
        printf("%*s  ", fsi.max_group_name, grp->gr_name);
        printf("%*ld ", fsi.max_file_size,(long int)st[row].st_size);
        printtime(st[row].st_mtime);
        printf("%s\n", ent[row]->d_name);
    }
}

void printCol(struct dirent **ent, int entries, int max_name_len){
    int numcols, numrows, row, col, colwidth, base, charCount;

    colwidth = (int)(log((double)max_name_len) / log(2));
    colwidth = 8 * (colwidth - 1);
    numcols = (termwidth + colwidth) / colwidth;
    numrows = entries / numcols;
    if(numrows % numcols || numrows == 0)
        numrows++;

    for(row = 0; row < numrows; row++){
        for(base = row, col = 0;;){
            charCount =  printf("%s", ent[base]->d_name);
            if((base += numrows) >= entries) break;
            if(++col == numcols) break;
            while(charCount++ < colwidth) (void)putchar(' ');

        }
        putchar('\n');
    }

}

char const * sperm(mode_t mode) {
    static char local_buff[17] = {0};
    int i = 0;
    if((mode & S_IFDIR) == S_IFDIR) local_buff[i] = 'd';
    else local_buff[i] = '-';
    i++;
    // user permissions
    if ((mode & S_IRUSR) == S_IRUSR) local_buff[i] = 'r';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IWUSR) == S_IWUSR) local_buff[i] = 'w';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IXUSR) == S_IXUSR) local_buff[i] = 'x';
    else local_buff[i] = '-';
    i++;
    // group permissions
    if ((mode & S_IRGRP) == S_IRGRP) local_buff[i] = 'r';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IWGRP) == S_IWGRP) local_buff[i] = 'w';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IXGRP) == S_IXGRP) local_buff[i] = 'x';
    else local_buff[i] = '-';
    i++;
    // other permissions
    if ((mode & S_IROTH) == S_IROTH) local_buff[i] = 'r';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IWOTH) == S_IWOTH) local_buff[i] = 'w';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IXOTH) == S_IXOTH) local_buff[i] = 'x';
    else local_buff[i] = '-';
    return local_buff;
}






