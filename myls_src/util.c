#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#include "myls.h"

void usage(void){
    (void)fprintf(stderr, "usage: ls [-l] [file ...]\n");
    exit(1);
}

int filter(const struct dirent *d){
    if(*(d->d_name) == '.')
        return 0;
    else return 1;
}

int numLen(long int x){

    int i = 0;

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
    if(x >= 10){
        i++;
        x /= 10;
    }
    if(x >= 1){
        i++;
    }
    return i;
}

char const * sperm(int mode) {
    static char local_buff[17] = {0};
    int i = 0;
    if((mode & S_IFDIR) == S_IFDIR) local_buff[i] = 'd';
    else local_buff[i] = '-';
    i++;
    /* user permissions */
    if ((mode & S_IRUSR) == S_IRUSR) local_buff[i] = 'r';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IWUSR) == S_IWUSR) local_buff[i] = 'w';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IXUSR) == S_IXUSR) local_buff[i] = 'x';
    else local_buff[i] = '-';
    i++;
    /* group permissions */
    if ((mode & S_IRGRP) == S_IRGRP) local_buff[i] = 'r';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IWGRP) == S_IWGRP) local_buff[i] = 'w';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IXGRP) == S_IXGRP) local_buff[i] = 'x';
    else local_buff[i] = '-';
    i++;
    /* other permissions */
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
