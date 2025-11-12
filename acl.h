#ifndef ACL_H
#define ACL_H

#include <stdbool.h>

// Check if current user has permission to execute a command
// Returns true if allowed, false if denied
bool check_command_permission(const char *command);

// Load ACL rules from file
void load_acl_rules();

// Add ACL rule (admin only)
bool add_acl_rule(const char *username, const char *command, bool allowed);

#endif // ACL_H


