#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include "commands.h"
#include "file_management.h"
#include "process_management.h"
#include "terminal.h"
#include "auth.h"
#include "logger.h"
#include "config.h"
#include "signals.h"
#include "acl.h"

// Global flag to track if we're in the main loop (not running a foreground process)
static volatile sig_atomic_t in_main_loop = 1;
// Global variable to track foreground child process (used by signal handler)
volatile pid_t foreground_pid = 0;

// ---------------- Autocomplete Setup ----------------

// List of available commands
static char *command_list[] = {
    "hello", "help", "clear", "exec", "list", "create", "copy", "delete",
    "run", "pslist", "fgproc", "bgproc", "killproc", "whoami",
    "encrypt", "decrypt", "checksum",
    "server", "client", "sandbox", "acl",
    "exit", "quit", NULL
};

// Command generator for readline completion
static char *command_generator(const char *text, int state) {
    static int idx, len;
    if (!state) {
        idx = 0;
        len = strlen(text);
    }
    
    while (command_list[idx]) {
        const char *name = command_list[idx++];
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }
    return NULL;
}

// Completion function for readline
static char **securecli_completion(const char *text, int start, int end) {
    (void)end;
    
    // If start == 0, we're completing the first token (command name)
    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    } else {
        // For other tokens, use filename completion
        return rl_completion_matches(text, rl_filename_completion_function);
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
    {"encrypt", cmd_encrypt},
    {"decrypt", cmd_decrypt},
    {"checksum", cmd_checksum},
    {"server", cmd_server},
    {"client", cmd_client},
    {"sandbox", cmd_sandbox},
    {"acl", cmd_acl},
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

    // Load configuration
    load_config();
    
    // Change to startup directory if configured
    if (chdir(startup_dir) != 0) {
        // If startup_dir doesn't exist, stay in current directory
        chdir(".");
    }

    load_users();
    load_acl_rules();
    printf("🔒 Welcome to SecureSysCLI\n");
    printf("Note: Use 'exit' to quit. Ctrl+C will kill foreground processes.\n");

    while (!login()) {
        printf("Try again.\n");
    }

    // Initialize QEMU-like terminal (only if show_banner is enabled)
    if (show_banner) {
        terminal_init();
    }

    // Initialize readline history system
    read_history(".securecli_history");
    using_history();
    
    // Set up autocomplete
    rl_attempted_completion_function = securecli_completion;

    char *input_line;
    while (1) {
        in_main_loop = 1;  // We're in the main loop waiting for input
        
        // Use readline for input (with history and autocomplete)
        input_line = readline("SecureSysCLI@qemu:~$ ");
        
        if (!input_line) {
            // Ctrl+D or EOF - exit gracefully
            printf("\nExiting SecureSysCLI...\n");
            break;
        }
        
        in_main_loop = 0;  // We're executing a command

        // Add non-empty lines to history
        if (*input_line) {
            add_history(input_line);
        }

        // Log the command before execution
        log_command(input_line);

        // Exit commands
        if (strcmp(input_line, "exit") == 0 || strcmp(input_line, "quit") == 0) {
            printf("Exiting SecureSysCLI...\n");
            free(input_line);
            break;
        }

        // Tokenize input
        char *argv[10];
        int argc = 0;
        char *line_copy = strdup(input_line);  // strtok modifies the string
        char *token = strtok(line_copy, " ");
        while (token && argc < 10) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }

        if (argc == 0) {
            free(input_line);
            free(line_copy);
            continue;
        }

        // Match command
        int found = 0;
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                // Check ACL permission
                if (!check_command_permission(argv[0])) {
                    printf("🚫  Permission denied: you don't have permission to execute '%s'\n", argv[0]);
                    log_command("UNAUTHORIZED command attempt");
                    found = 1;
                    break;
                }
                commands[i].func(argc, argv);
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Unknown command: %s\n", argv[0]);
        }

        free(input_line);
        free(line_copy);
    }

    // Save history before exiting
    write_history(".securecli_history");
    
    return 0;
}