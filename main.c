#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "commands.h"
#include "file_management.h"
#include "process_management.h"
#include "terminal.h"
#include "auth.h"
#include "logger.h"

// Global flag to track if we're in the main loop (not running a foreground process)
static volatile sig_atomic_t in_main_loop = 1;
// Global variable to track foreground child process (used by signal handler)
volatile pid_t foreground_pid = 0;

// ---------------- Command Dispatcher ----------------

struct Command {
    const char *name;
    void (*func)(int argc, char *argv[]);
};

struct Command commands[] = {
    {"hello", cmd_hello},
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"exec", cmd_exec},
    {"list", cmd_list},
    {"create", cmd_create},
    {"copy", cmd_copy},
    {"delete", cmd_delete},
    {"run", cmd_run},
    {"pslist", cmd_pslist},
    {"fgproc", cmd_fgproc},
    {"bgproc", cmd_bgproc},
    {"killproc", cmd_killproc},
    {"whoami", cmd_whoami},
    {NULL, NULL}
};

// Signal handler for SIGINT (Ctrl+C)
static void sigint_handler(int sig) {
    (void)sig;
    if (in_main_loop || foreground_pid == 0) {
        // In main loop or no foreground process - just print a newline
        printf("\n");
        fflush(stdout);
    } else {
        // Forward SIGINT to the foreground child process
        kill(foreground_pid, SIGINT);
    }
}

// ---------------- Main REPL ----------------

int main() {
    // Set up signal handler to ignore Ctrl+C in main loop
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    load_users();
    printf("Welcome to SecureSysCLI\n");
    printf("Note: Use 'exit' to quit. Ctrl+C will kill foreground processes.\n");

    while (!login()) {
        // login() already prints error message, just prompt to try again
        printf("Please try again.\n");
    }

    char line[1024];

    // Initialize QEMU-like terminal
    terminal_init();

    while (1) {
        in_main_loop = 1;  // We're in the main loop waiting for input
        terminal_print_prompt();
        // Reads a line of text from a stream and store array line
        if (!fgets(line, sizeof(line), stdin)) {
            // EOF or error - exit gracefully
            printf("\nExiting SecureSysCLI...\n");
            break;
        }
        in_main_loop = 0;  // We're executing a command

        // Remove newline
        line[strcspn(line, "\n")] = 0;

        // Exit commands
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            printf("Exiting SecureSysCLI...\n");
            log_command("exit");
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

        // Log the command before execution
        log_command(line);

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