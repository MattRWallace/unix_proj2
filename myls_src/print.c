#include <sys/types.h>
#include <sys/stat.h>

#include <fts.h>
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
void printSCol(FINFO *fsi){
    struct stat *st;
    struct passwd *pw;
    struct group *grp;
    FTSENT *cur;

    printf("total %ld\n", fsi->total_blocksize);
    for(cur = fsi->list; cur != NULL; cur = cur->fts_link){
        if(cur->fts_number == 1) continue;
        st = cur->fts_statp;
        printf("%-10.10s  ", sperm(st->st_mode));
        printf("%*d ", fsi->max_hardlinks, (int)st->st_nlink);
        pw = getpwuid(st->st_uid);
        printf("%*s ", fsi->max_user_name, pw->pw_name);
        grp = getgrgid(st->st_gid);
        printf("%*s ", fsi->max_group_name, grp->gr_name);
        printf("%*ld ", fsi->max_file_size,(long int)st->st_size);
        printtime(st->st_mtime);
        printf("%s\n", cur->fts_name);
    }
}

/* Multi column print format for listing files not requiring stat information */
void printCol(FINFO *fsi){
    int i, numcols, numrows, row, col, colwidth, base, charCount, entries, max_name_len;
    FTSENT **parray;
    FTSENT *p;
    entries = fsi->entries;
    max_name_len = fsi->max_name_len;

    colwidth = max_name_len + 2;
    numcols  = (termwidth +1)/colwidth;
    colwidth = (termwidth +1)/numcols;
    numrows = entries / numcols;
    if(numrows % numcols || numrows == 0)
        numrows++;

    parray = (FTSENT **) malloc(entries * sizeof(FTSENT *));
    for(p = fsi->list, i = 0; p != NULL; p = p->fts_link)
        if(p->fts_number != 1)
            parray[i++] = p;

    for(row = 0; row < numrows; row++){
        for(base = row, col = 0;;){
            charCount =  printf("%-s", parray[base]->fts_name);
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
