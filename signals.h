#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>
#include <signal.h>

// Global variable to track foreground child process (defined in main.c)
extern volatile pid_t foreground_pid;

#endif

