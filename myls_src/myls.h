typedef struct {
    FTSENT *list;
    long int total_blocksize;
    int max_file_size;
    int max_hardlinks;
    int max_user_name;
    int max_group_name;
    int max_name_len;
    int entries;
} FINFO;

extern int termwidth;

int filter(const FTSENT *a, const FTSENT *b);
int numLen(long int x);
char const *sperm(int mode);

void usage(void);
void printCol(FINFO *fsi);
void printSCol(FINFO *fsi);
