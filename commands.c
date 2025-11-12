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
#include "dashboard.h"
#include "script.h"
#include "plugin.h"
#include <ncurses.h>
#include <unistd.h>

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
    printf("  dashboard            - Launch ncurses dashboard\n");
    printf("  source <script.cli>  - Execute a .cli script file\n");
    printf("  plugins [cmd]        - List/manage plugins\n");
    printf("  exit / quit          - Exit the CLI\n");
    
    // Show loaded plugins
    int count;
    Plugin *plugins = plugin_get_all(&count);
    if (count > 0) {
        printf("\nLoaded plugins:\n");
        for (int i = 0; i < count; i++) {
            printf("  %s - %s\n", plugins[i].name, plugins[i].description);
        }
    }
    
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

// ---------------------------------------------------------------------------
// dashboard - Launch ncurses dashboard
// ---------------------------------------------------------------------------

void cmd_dashboard(int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    if (dashboard_init() != 0) {
        printf("Failed to initialize dashboard. Is ncurses installed?\n");
        return;
    }

    // Set non-blocking input with timeout
    timeout(1000); // 1000ms (1 second) timeout
    
    int ch;
    while (1) {
        dashboard_update();
        ch = getch();
        
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'r' || ch == 'R' || ch == KEY_RESIZE) {
            dashboard_update();
        } else if (ch == ERR) {
            // Timeout - continue loop to update display
            continue;
        }
    }

    dashboard_cleanup();
    printf("Dashboard closed.\n");
    log_command("dashboard");
}

// ---------------------------------------------------------------------------
// source - Execute a .cli script file
// ---------------------------------------------------------------------------

void cmd_source(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: source <script.cli>\n");
        printf("Example: source setup.cli\n");
        return;
    }

    if (script_execute(argv[1]) != 0) {
        printf("Failed to execute script: %s\n", argv[1]);
    }
    log_command("source");
}

// ---------------------------------------------------------------------------
// plugins - List loaded plugins or manage plugins
// ---------------------------------------------------------------------------

void cmd_plugins(int argc, char *argv[]) {
    if (argc == 1) {
        // List all plugins
        int count;
        Plugin *plugins = plugin_get_all(&count);
        if (count == 0) {
            printf("No plugins loaded.\n");
        } else {
            printf("Loaded plugins (%d):\n", count);
            for (int i = 0; i < count; i++) {
                printf("  %s - %s\n", plugins[i].name, plugins[i].description);
            }
        }
    } else if (argc == 2 && strcmp(argv[1], "reload") == 0) {
        // Reload all plugins
        plugin_cleanup();
        plugin_load_all();
        printf("Plugins reloaded.\n");
    } else if (argc == 3 && strcmp(argv[1], "load") == 0) {
        // Load a specific plugin
        if (plugin_load(argv[2]) == 0) {
            printf("Plugin loaded successfully.\n");
        } else {
            printf("Failed to load plugin: %s\n", argv[2]);
        }
    } else if (argc == 3 && strcmp(argv[1], "unload") == 0) {
        // Unload a plugin
        plugin_unload(argv[2]);
    } else {
        printf("Usage: plugins [reload|load <file>|unload <name>]\n");
    }
    log_command("plugins");
}