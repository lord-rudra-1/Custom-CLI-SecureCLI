#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// list <dir>
void cmd_list(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: list <dir>\n");
        return;
    }

    DIR *d;
    struct dirent *entry;
    struct stat fileStat;

    d = opendir(argv[1]);
    if (d == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(d)) != NULL) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", argv[1], entry->d_name);

        if (stat(path, &fileStat) == 0) {
            printf("%c%c%c%c%c%c%c%c%c ",
                   (fileStat.st_mode & S_IRUSR) ? 'r' : '-',
                   (fileStat.st_mode & S_IWUSR) ? 'w' : '-',
                   (fileStat.st_mode & S_IXUSR) ? 'x' : '-',
                   (fileStat.st_mode & S_IRGRP) ? 'r' : '-',
                   (fileStat.st_mode & S_IWGRP) ? 'w' : '-',
                   (fileStat.st_mode & S_IXGRP) ? 'x' : '-',
                   (fileStat.st_mode & S_IROTH) ? 'r' : '-',
                   (fileStat.st_mode & S_IWOTH) ? 'w' : '-',
                   (fileStat.st_mode & S_IXOTH) ? 'x' : '-');
            printf("%s\n", entry->d_name);
        }
    }

    closedir(d);
}

// copy <src> <dst>
void cmd_copy(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: copy <src> <dst>\n");
        return;
    }

    FILE *fsrc = fopen(argv[1], "rb");
    if (!fsrc) {
        perror("fopen src");
        return;
    }

    FILE *fdst = fopen(argv[2], "wb");
    if (!fdst) {
        perror("fopen dst");
        fclose(fsrc);
        return;
    }

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), fsrc)) > 0) {
        fwrite(buffer, 1, bytes, fdst);
    }

    fclose(fsrc);
    fclose(fdst);

    printf("Copied %s -> %s\n", argv[1], argv[2]);
}

// create file
void cmd_create(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: create <filename>\n");
        return;
    }

    FILE *f = fopen(argv[1], "w");  // Open for writing (creates or truncates)
    if (!f) {
        perror("fopen");
        return;
    }
    fclose(f);
    printf("Created file: %s\n", argv[1]);
}

// delete <file>
void cmd_delete(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: delete <file>\n");
        return;
    }

    if (unlink(argv[1]) == 0) {
        printf("Deleted: %s\n", argv[1]);
    } else {
        perror("unlink");
    }
}

