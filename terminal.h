#ifndef TERMINAL_H
#define TERMINAL_H

// Terminal control functions
void terminal_init(void);
void terminal_clear(void);
void terminal_set_color(const char *color);
void terminal_reset_color(void);
void terminal_print_banner(void);
void terminal_print_prompt(void);

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_BLACK   "\033[30m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BRIGHT_GREEN "\033[92m"
#define COLOR_BRIGHT_CYAN  "\033[96m"

#endif

