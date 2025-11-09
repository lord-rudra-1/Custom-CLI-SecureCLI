#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_JOBS 100

typedef struct {
    pid_t pid;
    char  cmd[256];
    int   running;
} Job;

static Job jobs[MAX_JOBS];
static int job_count = 0;

// Internal job management functions
static void add_job(pid_t pid, const char *cmd) {
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

static void remove_job(pid_t pid) {
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

