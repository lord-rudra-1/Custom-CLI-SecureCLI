#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "auth.h"
#include "logger.h"

// list <dir>
void cmd_list(int argc, char *argv[]) {
    const char *target = (argc < 2) ? "." : argv[1];

    DIR *d;
    struct dirent *entry;
    struct stat fileStat;

    d = opendir(target);
    if (d == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(d)) != NULL) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", target, entry->d_name);

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

    // 🔒 check permission
    if (!is_admin()) {
        printf("🚫  Permission denied: only admin can delete files.\n");
        log_command("UNAUTHORIZED delete attempt");
        return;
    }

    if (unlink(argv[1]) == 0) {
        printf("File deleted: %s\n", argv[1]);
        log_command("delete file");
    } else {
        perror("unlink failed");
    }
}

// write <file> <content>
void cmd_write(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: write <file> <content>\n");
        return;
    }

    FILE *f = fopen(argv[1], "w");
    if (!f) {
        perror("fopen");
        return;
    }

    for (int i = 2; i < argc; i++) {
        if (i > 2) {
            fputc(' ', f);
        }
        fputs(argv[i], f);
    }
    fputc('\n', f);

    fclose(f);
    printf("Wrote to file: %s\n", argv[1]);
    log_command("write file");
}

// show <file>
void cmd_show(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: show <file>\n");
        return;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), f)) {
        fputs(buffer, stdout);
    }

    if (ferror(f)) {
        perror("read error");
    }

    fclose(f);
    log_command("show file");
}