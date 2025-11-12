#ifndef SANDBOX_H
#define SANDBOX_H

#include <stdbool.h>

// Execute a command in a sandboxed environment
// Uses Linux namespaces to restrict filesystem access
// Returns true on success, false on error
bool run_sandboxed(const char *command, char **argv, const char *sandbox_dir);

#endif // SANDBOX_H


