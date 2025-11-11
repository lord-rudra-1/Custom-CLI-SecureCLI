#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "logger.h"

#define LOG_FILE "securecli.log"
#define MAX_LOG_LINE 1024

// Log a command to the command history log file
void log_command(const char *command) {
    if (!command) {
        return;
    }

    FILE *log = fopen(LOG_FILE, "a");
    if (!log) {
        // Silently fail if we can't open log file
        return;
    }

    // Get current timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Write log entry: timestamp | command
    fprintf(log, "[%s] %s\n", timestamp, command);
    fflush(log);  // Ensure it's written immediately
    fclose(log);
}

// Initialize the logger (optional, can be called at startup)
void logger_init(void) {
    // Create or truncate log file with header
    FILE *log = fopen(LOG_FILE, "w");
    if (log) {
        fprintf(log, "=== SecureSysCLI Command Log ===\n");
        fclose(log);
    }
}

