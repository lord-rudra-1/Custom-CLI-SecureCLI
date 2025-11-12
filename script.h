#ifndef SCRIPT_H
#define SCRIPT_H

// Execute a .cli script file
// Returns 0 on success, -1 on error
int script_execute(const char *filename);

// Check if a file is a .cli script
int script_is_cli_file(const char *filename);

#endif



