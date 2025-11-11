#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

// Default configuration values
char default_color[10] = "green";
char startup_dir[256] = ".";
int show_banner = 1;

// Load configuration from .securecli_config file
void load_config(void) {
    FILE *f = fopen(".securecli_config", "r");
    if (!f) {
        // Config file doesn't exist, use defaults
        return;
    }

    char key[64], value[256];
    while (fscanf(f, "%63s = %255s", key, value) == 2) {
        if (strcmp(key, "color") == 0) {
            strncpy(default_color, value, sizeof(default_color) - 1);
            default_color[sizeof(default_color) - 1] = '\0';
        } else if (strcmp(key, "startup_dir") == 0) {
            strncpy(startup_dir, value, sizeof(startup_dir) - 1);
            startup_dir[sizeof(startup_dir) - 1] = '\0';
        } else if (strcmp(key, "show_banner") == 0) {
            show_banner = atoi(value);
        }
    }
    fclose(f);
}

