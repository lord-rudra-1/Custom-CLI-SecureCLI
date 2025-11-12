#include "acl.h"
#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACL_RULES 100
#define ACL_FILE "acl.db"

typedef struct {
    char username[50];
    char command[50];
    bool allowed;
} ACLRule;

static ACLRule acl_rules[MAX_ACL_RULES];
static int acl_count = 0;

// Load ACL rules from file
void load_acl_rules() {
    FILE *f = fopen(ACL_FILE, "r");
    if (!f) {
        // No ACL file - create default rules
        // By default, admin can do everything, user has restricted access
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f) && acl_count < MAX_ACL_RULES) {
        char username[50], command[50], allow_str[10];
        if (sscanf(line, "%49s %49s %9s", username, command, allow_str) == 3) {
            strncpy(acl_rules[acl_count].username, username, sizeof(acl_rules[acl_count].username) - 1);
            strncpy(acl_rules[acl_count].command, command, sizeof(acl_rules[acl_count].command) - 1);
            acl_rules[acl_count].allowed = (strcmp(allow_str, "allow") == 0);
            acl_rules[acl_count].username[sizeof(acl_rules[acl_count].username) - 1] = '\0';
            acl_rules[acl_count].command[sizeof(acl_rules[acl_count].command) - 1] = '\0';
            acl_count++;
        }
    }

    fclose(f);
}

// Check if current user has permission to execute a command
bool check_command_permission(const char *command) {
    if (!current_user) {
        return false; // No user logged in
    }

    // Admin can do everything
    if (is_admin()) {
        return true;
    }

    // Check ACL rules
    for (int i = 0; i < acl_count; i++) {
        if (strcmp(acl_rules[i].username, current_user->username) == 0) {
            // Check if command matches (exact or wildcard)
            if (strcmp(acl_rules[i].command, "*") == 0) {
                // Wildcard rule
                return acl_rules[i].allowed;
            } else if (strcmp(acl_rules[i].command, command) == 0) {
                // Exact match
                return acl_rules[i].allowed;
            }
        }
    }

    // Default: deny if no rule found (secure by default)
    // For user role, allow basic commands by default
    const char *allowed_commands[] = {
        "hello", "help", "clear", "list", "whoami",
        "encrypt", "decrypt", "checksum", "create", "copy",
        NULL
    };

    for (int i = 0; allowed_commands[i]; i++) {
        if (strcmp(allowed_commands[i], command) == 0) {
            return true;
        }
    }

    // Restricted commands for non-admin users
    const char *restricted_commands[] = {
        "delete", "killproc", "exec", "run",
        NULL
    };

    for (int i = 0; restricted_commands[i]; i++) {
        if (strcmp(restricted_commands[i], command) == 0) {
            return false;
        }
    }

    // Unknown command - deny by default
    return false;
}

// Add ACL rule (admin only)
bool add_acl_rule(const char *username, const char *command, bool allowed) {
    if (!is_admin()) {
        return false;
    }

    if (acl_count >= MAX_ACL_RULES) {
        return false;
    }

    // Check if rule already exists
    for (int i = 0; i < acl_count; i++) {
        if (strcmp(acl_rules[i].username, username) == 0 &&
            strcmp(acl_rules[i].command, command) == 0) {
            // Update existing rule
            acl_rules[i].allowed = allowed;
            return true;
        }
    }

    // Add new rule
    strncpy(acl_rules[acl_count].username, username, sizeof(acl_rules[acl_count].username) - 1);
    strncpy(acl_rules[acl_count].command, command, sizeof(acl_rules[acl_count].command) - 1);
    acl_rules[acl_count].allowed = allowed;
    acl_rules[acl_count].username[sizeof(acl_rules[acl_count].username) - 1] = '\0';
    acl_rules[acl_count].command[sizeof(acl_rules[acl_count].command) - 1] = '\0';
    acl_count++;

    // Save to file
    FILE *f = fopen(ACL_FILE, "w");
    if (!f) {
        return false;
    }

    for (int i = 0; i < acl_count; i++) {
        fprintf(f, "%s %s %s\n",
                acl_rules[i].username,
                acl_rules[i].command,
                acl_rules[i].allowed ? "allow" : "deny");
    }

    fclose(f);
    return true;
}


