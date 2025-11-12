#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include <termios.h>
#include "terminal.h"
#include "logger.h"
#include "auth.h"
#include "signals.h"
#include "crypto.h"
#include "remote.h"
#include "sandbox.h"
#include "acl.h"

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
    printf("  list [dir]           - List files with permissions (default: .)\n");
    printf("  create <filename>    - Create an empty file\n");
    printf("  copy <src> <dst>     - Copy a file\n");
    printf("  delete <file>        - Delete a file (admin only)\n");
    printf("  close                - Exit and close the terminal window\n");
    printf("  write <file> <text>  - Write text to a file\n");
    printf("  show <file>          - Display file contents\n");
    printf("  run <program> [&]    - Run a program (background with &)\n");
    printf("  pslist               - Show background jobs\n");
    printf("  fgproc <jobid>       - Bring background job to foreground\n");
    printf("  bgproc <jobid>       - Resume stopped job in background\n");
    printf("  killproc <pid>       - Kill a process by PID (admin only)\n");
    printf("  whoami               - Show current user and role\n");
    printf("  encrypt <in> <out>   - Encrypt a file with password\n");
    printf("  decrypt <in> <out>    - Decrypt a file with password\n");
    printf("  checksum <file>       - Compute SHA-256 checksum of a file\n");
    printf("  server <port>         - Start TLS server (admin only)\n");
    printf("  client <host> <port>  - Connect to TLS server\n");
    printf("  sandbox <cmd> [args]  - Run command in sandbox (admin only)\n");
    printf("  acl <add|list>        - Manage access control lists (admin only)\n");
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
// close - Exit SecureSysCLI and close terminal window if possible
// ---------------------------------------------------------------------------
void cmd_close(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    log_command("close");
    printf("Closing SecureSysCLI...\n");
    fflush(stdout);

    const char *launched = getenv("SECURECLI_LAUNCHED");
    if (launched && strcmp(launched, "1") == 0) {
        pid_t parent = getppid();
        if (parent > 1) {
            kill(parent, SIGTERM);
        }
    } else {
        printf("(Unable to close terminal automatically in this environment.)\n");
        fflush(stdout);
    }

    exit(0);
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
// Helper function to read password without echoing (for crypto commands)
// ---------------------------------------------------------------------------
static void read_crypto_password(char *password, size_t max_len) {
    struct termios old_term, new_term;
    
    // Get current terminal settings
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    
    // Disable echo
    new_term.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    
    // Read password
    if (fgets(password, max_len, stdin) == NULL) {
        password[0] = '\0';
    } else {
        // Remove newline if present
        size_t len = strlen(password);
        if (len > 0 && password[len - 1] == '\n') {
            password[len - 1] = '\0';
        }
    }
    
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    printf("\n");  // Print newline after password input
}

// ---------------------------------------------------------------------------
// encrypt <infile> <outfile> - Encrypt a file
// ---------------------------------------------------------------------------
void cmd_encrypt(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: encrypt <input> <output>\n");
        return;
    }

    char pass[128];
    printf("Enter password: ");
    fflush(stdout);
    read_crypto_password(pass, sizeof(pass));

    if (strlen(pass) == 0) {
        printf("Password cannot be empty\n");
        return;
    }

    if (encrypt_file(argv[1], argv[2], pass)) {
        printf("Encrypted %s -> %s\n", argv[1], argv[2]);
        log_command("encrypt");
    } else {
        printf("Encryption failed\n");
    }
    
    // Clear password from memory
    memset(pass, 0, sizeof(pass));
}

// ---------------------------------------------------------------------------
// decrypt <infile> <outfile> - Decrypt a file
// ---------------------------------------------------------------------------
void cmd_decrypt(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: decrypt <input> <output>\n");
        return;
    }

    char pass[128];
    printf("Enter password: ");
    fflush(stdout);
    read_crypto_password(pass, sizeof(pass));

    if (strlen(pass) == 0) {
        printf("Password cannot be empty\n");
        return;
    }

    if (decrypt_file(argv[1], argv[2], pass)) {
        printf("Decrypted %s -> %s\n", argv[1], argv[2]);
        log_command("decrypt");
    } else {
        printf("Decryption failed\n");
    }
    
    // Clear password from memory
    memset(pass, 0, sizeof(pass));
}

// ---------------------------------------------------------------------------
// checksum <file> - Compute SHA-256 checksum
// ---------------------------------------------------------------------------
void cmd_checksum(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: checksum <file>\n");
        return;
    }

    char hex[65];
    if (sha256_file_hex(argv[1], hex)) {
        printf("%s  %s\n", hex, argv[1]);
        log_command("checksum");
    } else {
        printf("Checksum failed\n");
    }
}

// ---------------------------------------------------------------------------
// server <port> - Start TLS server
// ---------------------------------------------------------------------------
void cmd_server(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: server <port>\n");
        printf("Example: server 8443\n");
        return;
    }

    if (!is_admin()) {
        printf("🚫  Permission denied: only admin can start server.\n");
        log_command("UNAUTHORIZED server attempt");
        return;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        printf("Invalid port number. Must be between 1 and 65535.\n");
        return;
    }

    printf("Starting TLS server on port %d...\n", port);
    log_command("server start");
    
    // This will run indefinitely
    start_tls_server(port);
}

// ---------------------------------------------------------------------------
// client <hostname> <port> - Connect to TLS server
// ---------------------------------------------------------------------------
void cmd_client(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: client <hostname> <port>\n");
        printf("Example: client localhost 8443\n");
        return;
    }

    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        printf("Invalid port number. Must be between 1 and 65535.\n");
        return;
    }

    log_command("client connect");
    
    if (!connect_tls_client(argv[1], port)) {
        printf("Failed to connect to server\n");
    }
}

// ---------------------------------------------------------------------------
// sandbox <command> [args...] - Run command in sandbox
// ---------------------------------------------------------------------------
void cmd_sandbox(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: sandbox <command> [args...]\n");
        printf("Example: sandbox ls -la\n");
        return;
    }

    if (!is_admin()) {
        printf("🚫  Permission denied: only admin can use sandbox.\n");
        log_command("UNAUTHORIZED sandbox attempt");
        return;
    }

    log_command("sandbox");

    // Prepare arguments
    char **cmd_argv = &argv[1];
    
    if (run_sandboxed(argv[1], cmd_argv, NULL)) {
        printf("Command executed successfully in sandbox\n");
    } else {
        printf("Sandbox execution failed (may require root privileges)\n");
    }
}

// ---------------------------------------------------------------------------
// acl <add|list> [username] [command] [allow|deny] - Manage ACL rules
// ---------------------------------------------------------------------------
void cmd_acl(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: acl <add|list> [username] [command] [allow|deny]\n");
        printf("  acl list                    - List all ACL rules\n");
        printf("  acl add user delete allow   - Allow user to use delete command\n");
        printf("  acl add user * deny         - Deny all commands for user\n");
        return;
    }

    if (strcmp(argv[1], "list") == 0) {
        // List ACL rules (simplified - would need to read from file)
        printf("ACL rules loaded from acl.db\n");
        printf("Use 'acl add' to add new rules (admin only)\n");
        log_command("acl list");
    } else if (strcmp(argv[1], "add") == 0) {
        if (!is_admin()) {
            printf("🚫  Permission denied: only admin can modify ACL.\n");
            log_command("UNAUTHORIZED acl add attempt");
            return;
        }

        if (argc < 5) {
            printf("Usage: acl add <username> <command> <allow|deny>\n");
            return;
        }

        bool allowed = (strcmp(argv[4], "allow") == 0);
        
        if (add_acl_rule(argv[2], argv[3], allowed)) {
            printf("ACL rule added: %s %s %s\n", argv[2], argv[3], argv[4]);
            log_command("acl add");
        } else {
            printf("Failed to add ACL rule\n");
        }
    } else {
        printf("Unknown ACL command: %s\n", argv[1]);
    }
}