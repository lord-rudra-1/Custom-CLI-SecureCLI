#ifndef PLUGIN_H
#define PLUGIN_H

// Plugin function signature
// All plugin commands must follow this signature
typedef void (*plugin_command_func)(int argc, char *argv[]);

// Plugin structure
typedef struct {
    char *name;                    // Command name
    void *handle;                  // dlopen handle
    plugin_command_func func;      // Function pointer
    char *description;            // Help description
} Plugin;

// Initialize plugin system
void plugin_init(void);

// Load all plugins from /plugins directory
void plugin_load_all(void);

// Load a specific plugin from a .so file
int plugin_load(const char *path);

// Unload a plugin
void plugin_unload(const char *name);

// Unload all plugins
void plugin_cleanup(void);

// Get plugin by name
Plugin *plugin_get(const char *name);

// Get all loaded plugins (for help command)
// Returns pointer to the plugins array
Plugin *plugin_get_all(int *count);

#endif

