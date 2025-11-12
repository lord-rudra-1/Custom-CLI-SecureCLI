#define _GNU_SOURCE
#include "sandbox.h"
#include "logger.h"
#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <linux/sched.h>
#include <errno.h>

#define STACK_SIZE (1024 * 1024) // 1MB stack

// Sandbox directory (default: /tmp/securecli_sandbox)
#define DEFAULT_SANDBOX_DIR "/tmp/securecli_sandbox"

// Child process function for sandbox
static int sandbox_child(void *arg) {
    char **args = (char **)arg;
    const char *sandbox_dir = args[0];
    
    // Change root to sandbox directory (chroot)
    if (chroot(sandbox_dir) != 0) {
        perror("chroot failed");
        return 1;
    }

    // Change to root directory inside chroot
    if (chdir("/") != 0) {
        perror("chdir failed");
        return 1;
    }

    // Drop privileges (set UID/GID to nobody if possible)
    // Note: This requires appropriate capabilities
    // For demo, we'll skip this but it's recommended for production

    // Execute the command
    execvp(args[1], &args[1]);
    perror("execvp failed");
    return 1;
}

// Create sandbox directory structure
static bool setup_sandbox_dir(const char *sandbox_dir) {
    // Create sandbox directory
    if (mkdir(sandbox_dir, 0755) != 0 && errno != EEXIST) {
        perror("mkdir sandbox");
        return false;
    }

    // Create minimal directory structure
    char path[512];
    
    // Create /bin, /usr/bin, /lib, /lib64 for basic commands
    snprintf(path, sizeof(path), "%s/bin", sandbox_dir);
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/usr/bin", sandbox_dir);
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/lib", sandbox_dir);
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/lib64", sandbox_dir);
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/tmp", sandbox_dir);
    mkdir(path, 1777); // Sticky bit for /tmp
    
    snprintf(path, sizeof(path), "%s/dev", sandbox_dir);
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/proc", sandbox_dir);
    mkdir(path, 0755);

    return true;
}

// Run command in sandbox
bool run_sandboxed(const char *command, char **argv, const char *sandbox_dir) {
    if (!sandbox_dir) {
        sandbox_dir = DEFAULT_SANDBOX_DIR;
    }

    // Setup sandbox directory
    if (!setup_sandbox_dir(sandbox_dir)) {
        return false;
    }

    // Allocate stack for clone
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc stack");
        return false;
    }

    // Prepare arguments for child
    // args[0] = sandbox_dir, args[1] = command, rest = argv
    char **child_args = malloc(sizeof(char *) * 20);
    if (!child_args) {
        free(stack);
        return false;
    }
    
    child_args[0] = (char *)sandbox_dir;
    child_args[1] = (char *)command;
    int i = 2;
    if (argv) {
        for (int j = 0; argv[j] && i < 19; j++) {
            child_args[i++] = argv[j];
        }
    }
    child_args[i] = NULL;

    // Clone with new namespace
    // CLONE_NEWNS = new mount namespace
    // CLONE_NEWPID = new PID namespace (optional, requires more setup)
    // CLONE_NEWNET = new network namespace (optional)
    // Note: clone() requires appropriate capabilities (CAP_SYS_ADMIN) or root
    pid_t pid = clone(sandbox_child, stack + STACK_SIZE,
                      CLONE_NEWNS | SIGCHLD,
                      (void *)child_args);

    if (pid < 0) {
        perror("clone failed");
        // Fallback: try without namespace (just chroot)
        // This requires root or appropriate capabilities
        free(child_args);
        free(stack);
        return false;
    }

    // Wait for child
    int status;
    waitpid(pid, &status, 0);

    free(child_args);
    free(stack);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) == 0;
    }

    return false;
}

// Alternative simpler sandbox using chroot only (requires root)
static bool run_chroot_sandbox(const char *command, char **argv, const char *sandbox_dir) {
    if (!sandbox_dir) {
        sandbox_dir = DEFAULT_SANDBOX_DIR;
    }

    if (!setup_sandbox_dir(sandbox_dir)) {
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        // Child: chroot and execute
        if (chroot(sandbox_dir) != 0) {
            perror("chroot");
            _exit(1);
        }

        if (chdir("/") != 0) {
            perror("chdir");
            _exit(1);
        }

        execvp(command, argv);
        perror("execvp");
        _exit(1);
    } else {
        // Parent: wait for child
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

