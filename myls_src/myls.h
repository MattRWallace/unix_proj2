
extern int termwidth;

typedef struct {
    struct dirent **ent;
    struct stat *stat;
    char **f_name;
    long int total_blocksize;
    int max_file_size;
    int max_hardlinks;
    int max_user_name;
    int max_group_name;
} FINFO;


int filter(const struct dirent *d);
int numLen(long int x);
char const *sperm(int mode);

void usage(void);
void printCol(struct dirent **, const int, const int);
void printSCol(FINFO fsi, int entries);
