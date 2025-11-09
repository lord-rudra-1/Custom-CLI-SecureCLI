#ifndef PROCESS_MANAGEMENT_H
#define PROCESS_MANAGEMENT_H

// Process management command functions
void cmd_run(int argc, char *argv[]);
void cmd_pslist(int argc, char *argv[]);
void cmd_fgproc(int argc, char *argv[]);
void cmd_bgproc(int argc, char *argv[]);
void cmd_killproc(int argc, char *argv[]);

#endif

