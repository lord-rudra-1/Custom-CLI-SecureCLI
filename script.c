#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "script.h"
#include "commands.h"
#include "file_management.h"
#include "process_management.h"
#include "plugin.h"
#include "logger.h"

#define MAX_LINE_LENGTH 1024
#define MAX_VARIABLES 100

// Simple variable storage for scripting
typedef struct {
    char name[64];
    char value[256];
} Variable;

static Variable variables[MAX_VARIABLES];
static int variable_count = 0;

// Set a variable
static void set_variable(const char *name, const char *value) {
    // Check if variable exists
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            strncpy(variables[i].value, value, sizeof(variables[i].value) - 1);
            variables[i].value[sizeof(variables[i].value) - 1] = '\0';
            return;
        }
    }

    // Add new variable
    if (variable_count < MAX_VARIABLES) {
        strncpy(variables[variable_count].name, name, sizeof(variables[0].name) - 1);
        variables[variable_count].name[sizeof(variables[0].name) - 1] = '\0';
        strncpy(variables[variable_count].value, value, sizeof(variables[0].value) - 1);
        variables[variable_count].value[sizeof(variables[0].value) - 1] = '\0';
        variable_count++;
    }
}

// Get a variable value
static const char *get_variable(const char *name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return NULL;
}

// Expand variables in a string (simple $VAR substitution)
static void expand_variables(char *line, size_t size) {
    char result[MAX_LINE_LENGTH * 2] = {0};
    char *src = line;
    char *dst = result;

    while (*src && (size_t)(dst - result) < sizeof(result) - 1) {
        if (*src == '$' && (src[1] == '_' || (src[1] >= 'a' && src[1] <= 'z') || 
                           (src[1] >= 'A' && src[1] <= 'Z'))) {
            // Extract variable name
            char var_name[64] = {0};
            src++; // Skip $
            int i = 0;
            while ((*src == '_' || (*src >= 'a' && *src <= 'z') || 
                   (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9')) &&
                   (size_t)i < sizeof(var_name) - 1) {
                var_name[i++] = *src++;
            }

            // Get variable value
            const char *value = get_variable(var_name);
            if (value) {
                strncpy(dst, value, sizeof(result) - (dst - result) - 1);
                dst += strlen(value);
            } else {
                // Variable not found, keep $VAR
                *dst++ = '$';
                strncpy(dst, var_name, sizeof(result) - (dst - result) - 1);
                dst += strlen(var_name);
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    // Copy result back if it fits
    if (strlen(result) < size) {
        strncpy(line, result, size - 1);
        line[size - 1] = '\0';
    }
}

// Execute a command (similar to main.c's command dispatcher)
static void execute_command_line(const char *line) {
    if (!line || strlen(line) == 0) return;

    // Log the command
    log_command(line);

    // Tokenize
    char line_copy[MAX_LINE_LENGTH];
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    char *argv[64];
    int argc = 0;
    char *token = strtok(line_copy, " \t");
    while (token && argc < 63) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    argv[argc] = NULL;

    if (argc == 0) return;

    // Handle special scripting commands
    if (strcmp(argv[0], "set") == 0) {
        if (argc >= 3) {
            set_variable(argv[1], argv[2]);
        }
        return;
    }

    if (strcmp(argv[0], "echo") == 0) {
        for (int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
        return;
    }

    // Check for built-in commands
    struct {
        const char *name;
        void (*func)(int argc, char *argv[]);
    } builtin_commands[] = {
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

    int found = 0;
    for (int i = 0; builtin_commands[i].name != NULL; i++) {
        if (strcmp(argv[0], builtin_commands[i].name) == 0) {
            builtin_commands[i].func(argc, argv);
            found = 1;
            break;
        }
    }

    // Check plugins
    if (!found) {
        Plugin *plugin = plugin_get(argv[0]);
        if (plugin && plugin->func) {
            plugin->func(argc, argv);
            found = 1;
        }
    }

    if (!found) {
        printf("Unknown command: %s\n", argv[0]);
    }
}

int script_is_cli_file(const char *filename) {
    if (!filename) return 0;
    const char *ext = strrchr(filename, '.');
    return (ext && strcmp(ext, ".cli") == 0);
}

int script_execute(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open script file: %s\n", filename);
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    printf("Executing script: %s\n", filename);

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        // Remove newline
        line[strcspn(line, "\n")] = 0;

        // Skip empty lines and comments
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (*trimmed == '\0' || *trimmed == '#') continue;

        // Expand variables
        expand_variables(trimmed, sizeof(line));

        // Execute the command
        execute_command_line(trimmed);
    }

    fclose(file);
    printf("Script execution completed.\n");
    return 0;
}

