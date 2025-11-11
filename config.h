#ifndef CONFIG_H
#define CONFIG_H

// Configuration variables
extern char default_color[10];
extern char startup_dir[256];
extern int show_banner;

// Load configuration from .securecli_config file
void load_config(void);

#endif

