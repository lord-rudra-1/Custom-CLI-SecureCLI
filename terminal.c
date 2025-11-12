#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "terminal.h"
#include "auth.h"

// Initialize terminal - clear screen and set up
void terminal_init(void) {
    // Check if stdout is a terminal
    if (isatty(STDOUT_FILENO)) {
        terminal_clear();
        terminal_print_banner();
    }
}

// Clear the terminal screen
void terminal_clear(void) {
    printf("\033[2J\033[H");  // Clear screen and move cursor to home
    fflush(stdout);
}

// Set terminal color
void terminal_set_color(const char *color) {
    printf("%s", color);
    fflush(stdout);
}

// Reset terminal color
void terminal_reset_color(void) {
    printf("%s", COLOR_RESET);
    fflush(stdout);
}

// Print QEMU-like banner
void terminal_print_banner(void) {
    terminal_set_color(COLOR_BRIGHT_CYAN);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║");
    terminal_set_color(COLOR_BRIGHT_GREEN);
    printf("          SystemCLI                                         ");
    terminal_set_color(COLOR_BRIGHT_CYAN);
    printf("║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    terminal_reset_color();
    printf("\n");
    
    terminal_set_color(COLOR_CYAN);
    printf("SecureSysCLI - Operating System Command Line Interface\n");
    terminal_reset_color();
    printf("Type 'help' for available commands or 'exit' to quit.\n");
    printf("───────────────────────────────────────────────────────────────\n\n");
}

// Print QEMU-like prompt
void terminal_print_prompt(void) {
    const char *username = (current_user && current_user->username[0])
                               ? current_user->username
                               : "user";

    terminal_set_color(COLOR_BRIGHT_GREEN);
    printf("SecureSysCLI");
    terminal_set_color(COLOR_WHITE);
    printf("@");
    terminal_set_color(COLOR_BRIGHT_CYAN);
    printf("%s", username);
    terminal_set_color(COLOR_WHITE);
    printf(":");
    terminal_set_color(COLOR_YELLOW);
    printf("~");
    terminal_set_color(COLOR_WHITE);
    printf("$ ");
    terminal_reset_color();
    fflush(stdout);
}

