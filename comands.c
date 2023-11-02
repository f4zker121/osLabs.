#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <getopt.h>

#define MAX_PATH_LENGTH 256
#define TIME_STRING_LENGTH 20

#define COLOR_RESET "\033[0m"
#define COLOR_BLUE "\033[94m"
#define COLOR_GREEN "\033[92m"
#define COLOR_CYAN "\033[96m"
#define COLOR_RED "\033[91m"
#define COLOR_MAGENTA "\033[95m"
#define COLOR_DEFAULT "\033[39m"

void print_file_info(struct dirent *entry, const char *path, int use_color) {
    struct stat file_stat;
    if (stat(path, &file_stat) < 0) {
        perror("stat");
        return;
    }

    char mode[] = "----------";
    if (S_ISDIR(file_stat.st_mode)) {
        mode[0] = 'd';
        if (use_color) printf("%s", COLOR_BLUE);
    } else if (file_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        if (use_color) printf("%s", COLOR_GREEN);
    } else if (S_ISLNK(file_stat.st_mode)) {
        if (use_color) printf("%s", COLOR_CYAN);
    } else if (S_ISREG(file_stat.st_mode) && (file_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ) {
        if (use_color) printf("%s", COLOR_RED);
    } else if (S_ISBLK(file_stat.st_mode) || S_ISCHR(file_stat.st_mode)) {
        if (use_color) printf("%s", COLOR_MAGENTA);
    } else if (use_color) {
        printf("%s", COLOR_DEFAULT);
    }

    if (file_stat.st_mode & S_IRUSR) mode[1] = 'r';
    if (file_stat.st_mode & S_IWUSR) mode[2] = 'w';
    if (file_stat.st_mode & S_IXUSR) mode[3] = 'x';
    if (file_stat.st_mode & S_IRGRP) mode[4] = 'r';
    if (file_stat.st_mode & S_IWGRP) mode[5] = 'w';
    if (file_stat.st_mode & S_IXGRP) mode[6] = 'x';
    if (file_stat.st_mode & S_IROTH) mode[7] = 'r';
    if (file_stat.st_mode & S_IWOTH) mode[8] = 'w';
    if (file_stat.st_mode & S_IXOTH) mode[9] = 'x';

    struct passwd *pw = getpwuid(file_stat.st_uid);
    struct group *gr = getgrgid(file_stat.st_gid);

    char time_str[TIME_STRING_LENGTH];
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", localtime(&file_stat.st_mtime));

    printf("%s %2lu %s %s %8ld %s %s", mode, file_stat.st_nlink, pw->pw_name, gr->gr_name, (long)file_stat.st_size, time_str, entry->d_name);

    if (use_color) printf(COLOR_RESET);
    printf("\n");
}

int main(int argc, char **argv) {
    const char *dir_path = "."; // Путь по умолчанию
    int show_hidden = 0;
    int print_long_format = 0;
    int use_color = isatty(STDOUT_FILENO);

    int opt;
    while ((opt = getopt(argc, argv, "al")) != -1) {
        switch (opt) {
            case 'a':
                show_hidden = 1;
                break;
            case 'l':
                print_long_format = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-al] [directory_path]\n", argv[0]);
                return 1;
        }
    }


    if (optind < argc) {
        dir_path = argv[optind];
    }

    char *real_path = realpath(dir_path, NULL);
    if (real_path == NULL) {
        perror("realpath");
        return 1;
    }

    
    if (strcmp(real_path, "/") == 0) {
        fprintf(stderr, "Error: Invalid path goes beyond root directory.\n");
        free(real_path);
        return 1;
    }

    DIR *directory = opendir(real_path);
    if (!directory) {
        perror("opendir");
        free(real_path);
        return 1;
    }

    struct dirent *entry;

    while ((entry = readdir(directory)) != NULL) {
        if (!show_hidden && (entry->d_name[0] == '.' && (strlen(entry->d_name) == 1 || (entry->d_name[1] == '.' && strlen(entry->d_name) == 2)))) {
            continue;
        }

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", real_path, entry->d_name);
        if (print_long_format) {
            print_file_info(entry, full_path, use_color);
        } else {
            struct stat file_stat;
            if (stat(full_path, &file_stat) < 0) {
                perror("stat");
                return -1;
            }

            if (S_ISDIR(file_stat.st_mode)) {
                if (use_color) printf("\033[34m%s\033[0m\n", entry->d_name);
            } else if (file_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                if (use_color) printf("\033[92m%s\033[0m\n", entry->d_name);
            } else if (S_ISLNK(file_stat.st_mode)) {
                if (use_color) printf("\033[96m%s\033[0m\n", entry->d_name);
            } else if (S_ISREG(file_stat.st_mode) && (file_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
                if (use_color) printf("\033[91m%s\033[0m\n", entry->d_name);
            } else if (S_ISBLK(file_stat.st_mode) || S_ISCHR(file_stat.st_mode)) {
                if (use_color) printf("\033[95m%s\033[0m\n", entry->d_name);
            } else if (use_color) {
                printf("%s\n", entry->d_name);
            }
        }
    }

    if (!print_long_format) {
        printf("\n");
    }

    free(real_path);
    closedir(directory);
    return 0;
}