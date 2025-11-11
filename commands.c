#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include "terminal.h"
#include "logger.h"
#include "auth.h"
#include "signals.h"

// ---------------------------------------------------------------------------
// 🔹 Input Sanitization
// ---------------------------------------------------------------------------

bool is_input_safe(const char *input) {
    const char *forbidden = ";|><`$";
    for (int i = 0; input[i]; i++) {
        if (strchr(forbidden, input[i])) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// 🔹 Command Implementations
// ---------------------------------------------------------------------------

// hello
void cmd_hello(int argc, char *argv[]) {
    (void)argc; (void)argv;
    printf("Hello from SecureSysCLI! 👋\n");
    log_command("hello");
}

// help
void cmd_help(int argc, char *argv[]) {
    (void)argc; (void)argv;
    printf("Available commands:\n");
    printf("  hello                - Print greeting\n");
    printf("  help                 - Show this help message\n");
    printf("  clear                - Clear the terminal screen\n");
    printf("  exec <program> [args]- Execute a system program securely\n");
    printf("  list <dir>           - List files with permissions\n");
    printf("  create <filename>    - Create an empty file\n");
    printf("  copy <src> <dst>     - Copy a file\n");
    printf("  delete <file>        - Delete a file (admin only)\n");
    printf("  run <program> [&]    - Run a program (background with &)\n");
    printf("  pslist               - Show background jobs\n");
    printf("  fgproc <jobid>       - Bring background job to foreground\n");
    printf("  bgproc <jobid>       - Resume stopped job in background\n");
    printf("  killproc <pid>       - Kill a process by PID (admin only)\n");
    printf("  whoami               - Show current user and role\n");
    printf("  exit / quit          - Exit the CLI\n");
    log_command("help");
}

// ---------------------------------------------------------------------------
// 🔹 exec <program> [args] — Secure fork/exec Implementation
// ---------------------------------------------------------------------------

void cmd_exec(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: exec <program> [args...]\n");
        printf("Example: exec ls -la\n");
        return;
    }

    // Sanitize all arguments
    for (int i = 1; i < argc; i++) {
        if (!is_input_safe(argv[i])) {
            printf("⚠️  Unsafe characters detected in argument: %s\n", argv[i]);
            return;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // Child process - restore default signal handlers
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        execvp(argv[1], &argv[1]);
        perror("execvp failed");
        _exit(1);
    } else {
        // Parent waits for child to finish
        // Set global variable so signal handler can forward SIGINT
        foreground_pid = pid;
        
        int status;
        waitpid(pid, &status, 0);
        
        // Clear foreground PID after process completes
        foreground_pid = 0;

        if (WIFEXITED(status)) {
            printf("Process exited with status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("\nProcess killed by signal: %d\n", WTERMSIG(status));
        } else {
            printf("Process ended abnormally.\n");
        }

        // Log the command string
        char full_cmd[512] = {0};
        for (int i = 1; i < argc; i++) {
            strcat(full_cmd, argv[i]);
            if (i < argc - 1) strcat(full_cmd, " ");
        }
        log_command(full_cmd);
    }
}

// ---------------------------------------------------------------------------
// clear - Clear the terminal screen
// ---------------------------------------------------------------------------

void cmd_clear(int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_clear();
    terminal_print_banner();
    log_command("clear");
}

// ---------------------------------------------------------------------------
// whoami - Show current user information
// ---------------------------------------------------------------------------

void cmd_whoami(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (current_user) {
        printf("You are %s (%s)\n", current_user->username, current_user->role);
    } else {
        printf("No user logged in.\n");
    }
    log_command("whoami");
}