#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <math.h>
#include <time.h>

#include "myls.h"

static void printtime(time_t time);

/* Single column print format */
void printSCol(FINFO fsi, int entries){
    int row;
    struct passwd *pw;
    struct group *grp;
    
    printf("total %ld\n", fsi.total_blocksize);
    for(row = 0; row < entries; row++){
        printf("%-10.10s  ", sperm(fsi.stat[row].st_mode));
        printf("%*d ", fsi.max_hardlinks, fsi.stat[row].st_nlink);
        pw = getpwuid(fsi.stat[row].st_uid);
        printf("%*s  ", fsi.max_user_name, pw->pw_name);
        grp = getgrgid(fsi.stat[row].st_gid);
        printf("%*s  ", fsi.max_group_name, grp->gr_name);
        printf("%*ld ", fsi.max_file_size,(long int)fsi.stat[row].st_size);
        printtime(fsi.stat[row].st_mtime);
        printf("%s\n", fsi.ent[row]->d_name);
    }
}

/* Multi column print format for listing files not requiring stat information */
void printCol(struct dirent **ent, const int entries, const int max_name_len){
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
        (void)putchar('\n');
    }
}

static void printtime(time_t mtime)
{
	int i;
	char *longstring;
    
	longstring = ctime(&mtime);
    for (i = 4; i < 11; ++i)
		(void)putchar(longstring[i]);
    
#define	SIXMONTHS	((365 / 2) * (24 * 60 * 60))
	if (mtime + SIXMONTHS > time(NULL))
		for (i = 11; i < 16; ++i)
			(void)putchar(longstring[i]);
	else {
		(void)putchar(' ');
		for (i = 20; i < 24; ++i)
			(void)putchar(longstring[i]);
	}
	(void)putchar(' ');
}