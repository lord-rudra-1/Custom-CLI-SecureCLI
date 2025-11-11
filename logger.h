#ifndef LOGGER_H
#define LOGGER_H

// Log a command to the command history log file
void log_command(const char *command);

// Initialize the logger (optional, can be called at startup)
void logger_init(void);

#endif

