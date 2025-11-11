#include <stdio.h>
#include <string.h>
#include "commands.h"
#include "file_management.h"
#include "process_management.h"
#include "terminal.h"

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
    {NULL, NULL}
};

// ---------------- Main REPL ----------------

int main() {
    char line[1024];

    // Initialize QEMU-like terminal
    terminal_init();

    while (1) {
        terminal_print_prompt();
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