#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"

// hello
void cmd_hello(int argc, char *argv[]) {
    (void)argc; (void)argv;
    printf("Hello from SecureSysCLI! 👋\n");
}

// help
void cmd_help(int argc, char *argv[]) {
    (void)argc; (void)argv;
    printf("Available commands:\n");
    printf("  hello                - Print greeting\n");
    printf("  help                 - Show this help message\n");
    printf("  clear                - Clear the terminal screen\n");
    printf("  exec <command>       - Execute a shell command\n");
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

// exec <command> - Execute a shell command
void cmd_exec(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: exec <command> [args...]\n");
        printf("Example: exec ls -la\n");
        printf("Example: exec echo Hello World\n");
        return;
    }

    // Build command string from arguments
    char command[1024] = {0};
    size_t pos = 0;
    size_t max_len = sizeof(command) - 1;
    
    for (int i = 1; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (pos + len + 1 < max_len) {
            if (i > 1) {
                command[pos++] = ' ';
            }
            strcpy(command + pos, argv[i]);
            pos += len;
        } else {
            printf("Error: Command too long\n");
            return;
        }
    }

    // Execute the command
    printf("Executing: %s\n", command);
    int status = system(command);
    
    if (status == -1) {
        perror("system");
    } else {
        printf("Command exited with status: %d\n", status);
    }
}

// clear - Clear the terminal screen
void cmd_clear(int argc, char *argv[]) {
    (void)argc; (void)argv;
    terminal_clear();
    terminal_print_banner();
}

