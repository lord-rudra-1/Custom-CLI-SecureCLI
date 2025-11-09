#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <signal.h>


#define MAX_JOBS 100

typedef struct {
    pid_t pid;
    char  cmd[256];
    int   running;
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;

// ---------------- Command Implementations ----------------

// hello
void cmd_hello(int argc, char *argv[]) {
    printf("Hello from SecureSysCLI! 👋\n");
}

// help
void cmd_help(int argc, char *argv[]) {
    printf("Available commands:\n");
    printf("  hello                - Print greeting\n");
    printf("  help                 - Show this help message\n");
    printf("  list <dir>           - List files with permissions\n");
    printf("  create <filename>    - Create an empty file\n");
    printf("  copy <src> <dst>     - Copy a file\n");
    printf("  delete <file>        - Delete a file\n");
    printf("  run <program> [&]    - Run a program (append & for background)\n");
    printf("  pslist               - Show background jobs\n");
    printf("  fgproc <jobid>       - Bring a background job to foreground\n");
    printf("  bgproc <jobid>       - Resume a stopped job in background\n");
    printf("  killproc <pid>       - Kill a process by PID\n");
    printf("  exit / quit          - Exit the CLI\n");
}

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




// add and remove

void add_job(pid_t pid, const char *cmd) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].cmd, cmd, sizeof(jobs[job_count].cmd) - 1);
        jobs[job_count].cmd[sizeof(jobs[job_count].cmd) - 1] = '\0';
        jobs[job_count].running = 1;
        job_count++;
    } else {
        printf("Job table full, cannot add more processes\n");
    }
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

// Track jobs when run is background

void cmd_run(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: run <program> [&]\n");
        return;
    }

    int background = 0;
    if (strcmp(argv[argc - 1], "&") == 0) {
        background = 1;
        argv[argc - 1] = NULL;
        argc--;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        if (background) {
            printf("[bg] %d started: %s\n", pid, argv[1]);
            add_job(pid, argv[1]);
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}


// pslist (like jobs)

void cmd_pslist(int argc, char *argv[]) {
    (void)argc; (void)argv;
    for (int i = 0; i < job_count; i++) {
        if (kill(jobs[i].pid, 0) == 0) {
            printf("[%d] PID %d running %s\n", i, jobs[i].pid, jobs[i].cmd);
        } else {
            printf("[%d] PID %d (terminated)\n", i, jobs[i].pid);
        }

    }
}

// fgproc (like fg)

void cmd_fgproc(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: fgproc <jobid>\n");
        return;
    }

    int jid = atoi(argv[1]);
    if (jid < 0 || jid >= job_count || !jobs[jid].running) {
        printf("No such job: %d\n", jid);
        return;
    }

    pid_t pid = jobs[jid].pid;
    printf("Bringing job %d (PID %d) to foreground...\n", jid, pid);
    int status;
    waitpid(pid, &status, 0);
    jobs[jid].running = 0;   // mark as stopped/finished
    remove_job(pid);
}


// bgproc (like bg)

void cmd_bgproc(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: bgproc <program> [args...]\n");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        setsid();  // run independently
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        printf("[bg] %d started: %s\n", pid, argv[1]);
        add_job(pid, argv[1]);   // track in jobs[] array
    } else {
        perror("fork");
    }
}



// killproc (like kill)

void cmd_killproc(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: killproc <pid>\n");
        return;
    }

    pid_t pid = atoi(argv[1]);
    if (kill(pid, SIGKILL) == 0) {
        int status;
        waitpid(pid, &status, 0);  // reap the process to avoid zombie
        printf("Killed process %d\n", pid);

        // Remove from job table
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                // shift others down
                for (int j = i; j < job_count - 1; j++) {
                    jobs[j] = jobs[j + 1];
                }
                job_count--;
                break;
            }
        }
    } else {
        perror("kill");
    }
}



// ---------------- Command Dispatcher ----------------

struct Command {
    const char *name;
    void (*func)(int argc, char *argv[]);
};

struct Command commands[] = {
    {"hello", cmd_hello},
    {"help", cmd_help},
    {"list", cmd_list},
    {"create", cmd_create},
    {"copy", cmd_copy},
    {"delete", cmd_delete},
    {"run", cmd_run},
    {"pslist", cmd_pslist},
    {"fgproc", cmd_fgproc},
    {"bgproc", cmd_bgproc},
    {"killproc", cmd_killproc},
    {NULL, NULL}
};

// ---------------- Main REPL ----------------

int main() {
    char line[1024];

    printf("Welcome to SecureSysCLI! Type 'help' for commands.\n");

    while (1) {
        printf("SecureSysCLI> ");
        // Reads a line of text from a stream and store array line
        if (!fgets(line, sizeof(line), stdin)) break;

        // Remove newline
        line[strcspn(line, "\n")] = 0;

        // Exit commands
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            printf("Exiting SecureSysCLI...\n");
            break;
        }

        // Tokenize input
        char *argv[10];
        int argc = 0;
        char *token = strtok(line, " ");
        while (token && argc < 10) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }

        if (argc == 0) continue;

        // Match command
        int found = 0;
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                commands[i].func(argc, argv);
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Unknown command: %s\n", argv[0]);
        }
    }

    return 0;
}
