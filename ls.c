#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>


typedef struct {
     char **array;
     size_t used;
     size_t size;
} Array;

void initArray(Array *a, size_t initialSize) {
     a->array = malloc(initialSize * sizeof(char*));
     a->used = 0;
     a->size = initialSize;
}

void insertArray(Array *a, char* element) {
     if (a->used == a->size) {
       a->size++;
       a->array = realloc(a->array, a->size * sizeof(char*));
     }
     a->array[a->used] = malloc(255);
     a->array[a->used++] = element;
}

void freeArray(Array *a) {
     free(a->array);
     a->array = NULL;
     a->used = a->size = 0;
}

int sort_string(const void *a, const void *b)
{
    char const *lhs = *(const char **)a;
    char const *rhs = *(const char **)b;
    while (*lhs || *rhs)
    {
        while (*lhs && (ispunct((unsigned char)*lhs) || isspace((unsigned char)*lhs))) {
            lhs++;
        }
        while (*rhs && (ispunct((unsigned char)*rhs) || isspace((unsigned char)*rhs))) {
            rhs++;
        }
        if (*lhs != *rhs) {
            return tolower((unsigned char)*lhs) - tolower((unsigned char)*rhs);
        }
        lhs++;
        rhs++;
    }
    return 0;
}

void mode_string(mode_t mode, char *str) {
    if (S_ISDIR(mode))       str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '-';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

void print_long(const char *dir, const char *name) {
    char fullpath[4096];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, name);

    struct stat st;
    if (lstat(fullpath, &st) < 0) {
        perror(name);
        return;
    }
    char modes[11];
    mode_string(st.st_mode, modes);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);
    const char *user = pw ? pw->pw_name : "?";
    const char *group = gr ? gr->gr_name : "?";

    char timebuf[64];

    struct tm *tm = localtime(&st.st_mtim.tv_sec);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm);

    printf(
        "%s %lu %s %s %ld %s %s\n",
        modes,
        (unsigned long)st.st_nlink,
        user,
        group,
        (long)st.st_size,
        timebuf,
        name
    );

}

int show_all = 0;
int long_format = 0;

int main (int argc, char *argv[]) {
    int opt;

    while((opt = getopt(argc, argv, "al")) != -1){
        switch (opt) {
            case 'a':
                show_all = 1;
                break;
            case 'l':
                long_format = 1;
                break;
            default:
                fprintf(stderr, "usage: %s [-al] [path]\n", argv[0]);
                return 1;
        }
    }

    const char *path = (optind < argc) ? argv[optind] : ".";

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    Array entry_list;
    initArray(&entry_list, 1);

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) { 
        insertArray(&entry_list, entry->d_name);
    }

    qsort(entry_list.array, entry_list.size, sizeof(entry_list.array[0]), sort_string);
    for(size_t i = 0; i < entry_list.used; ++i) {
        if (!show_all && entry_list.array[i][0] == '.') continue;
        if(long_format) {
            print_long(path, entry_list.array[i]);
        }
        else {
            printf("%s\n", entry_list.array[i]);
        }
    }

    freeArray(&entry_list);

    closedir(dir);
    return 0;
}
